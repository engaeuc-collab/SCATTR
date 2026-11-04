#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------------------------
// Spectral gate: "only allow frequency components above or below a particular cut-off amplitude".
//
// The signal is split into FFT bins. Every bin is measured against the cutoff amplitude:
//   * a bin at or above the cutoff is STRONG, a bin below it is WEAK
//   * the BALANCE control decides how much of each side is let through:
//        0.0  only the weak bins pass   (the loud parts are removed, what is left is the quiet "air")
//        0.5  everything passes         (the gate is neutral)
//        1.0  only the strong bins pass (a classic gate: everything under the cutoff is removed)
//     in between, the side that is being pushed back fades out linearly
//   * TILT slants the cutoff across the spectrum (0.5 = flat). Towards 1 the cutoff rises with frequency, so
//     the highs are gated harder; towards 0 it falls with frequency, so the lows are. It only acts while
//     tilt is enabled.
//
// The decision is made fresh for every bin of every frame: no attack, release or hold, exactly like the
// gate it is modelled on. That is what gives it its characteristic broken-up, "spectral" sound.
//
// Engine: sqrt-Hann analysis + synthesis windows with 4x overlap (so the overlap-add sums back to unity),
// both channels through ONE complex FFT (left = real part, right = imaginary part). The two channels are
// fully linked: each bin's level is the louder of the two and the same gain is applied to both, so the
// stereo image never wobbles. Latency is exactly the window size (see getLatency()). Everything is
// pre-allocated for the largest window, so changing the window size on the audio thread never allocates.
// ---------------------------------------------------------------------------------------------
class SpectralGate
{
public:
    static constexpr int minOrder = 9, maxOrder = 12;            // 512 .. 4096
    static constexpr int maxN = 1 << maxOrder;
    static constexpr int overlap = 4;
    static constexpr int numBands = 64;                           // analyzer bands (log spaced)
    static constexpr float fMinHz = 20.f, fMaxHz = 20000.f;       // analyzer range
    static constexpr float tiltPivotHz = 1000.f;                  // the cutoff keeps its set value here, whatever the tilt
    static constexpr float maxTiltDbPerOct = 9.f;                 // tilt 0 / 1 = -9 / +9 dB per octave

    struct Params
    {
        float cutoffDb = -44.4f;    // cutoff amplitude; 0 dB = a full-scale sine in one bin
        float balance = 0.7f;       // 0..1: 0 = only weak bins pass, 0.5 = all pass, 1 = only strong bins pass
        bool  tiltOn = false;
        float tilt = 0.5f;          // 0..1, 0.5 = flat
        int   order = 11;           // window = 2^order
    };

    // The cutoff at a frequency (shared with the UI so the curve it draws is the real one)
    static float thresholdDb (const Params& p, float hz)
    {
        float t = p.cutoffDb;
        if (p.tiltOn)
            t += (p.tilt - 0.5f) * 2.f * maxTiltDbPerOct * std::log2 (std::max (hz, 1.f) / tiltPivotHz);
        return t;
    }

    // Linear gain of the weak bins / the strong bins for a balance setting
    static float weakGain (const Params& p)   { return std::min (1.f, 2.f * (1.f - clamp01 (p.balance))); }
    static float strongGain (const Params& p) { return std::min (1.f, 2.f * clamp01 (p.balance)); }

    SpectralGate()
    {
        const float pi = 3.14159265358979f;
        twC.resize ((size_t) maxN / 2);
        twS.resize ((size_t) maxN / 2);
        for (int i = 0; i < maxN / 2; ++i)
        {
            twC[(size_t) i] = std::cos (2.f * pi * (float) i / (float) maxN);
            twS[(size_t) i] = std::sin (2.f * pi * (float) i / (float) maxN);
        }
        for (auto* v : { &inL, &inR, &accL, &accR, &re, &im, &win }) v->assign ((size_t) maxN, 0.f);
        rev.assign ((size_t) maxN, 0);
        for (auto* v : { &lvDb, &outDb }) v->assign ((size_t) maxN / 2 + 1, 0.f);
        for (auto& a : dispIn)  a.store (-200.f);
        for (auto& a : dispOut) a.store (-200.f);
        setOrder (11);
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int o = order;
        order = -1;           // force a rebuild
        setOrder (o);
        en = 0.f;
        wasRunning = false;
    }

    int getLatency() const { return N; }
    int getOrder() const { return order; }

    // Processes in place. R may be null (mono). `on` fades the whole effect in and out.
    void process (float* L, float* R, int n, const Params& p, bool on)
    {
        if (p.order != order) setOrder (p.order);

        const bool stereo = R != nullptr;
        const int mask = N - 1;
        const float enUp = 1.f / (float) N, enDown = 1.f / std::max (16.f, 0.02f * (float) sr);

        const bool running = on || en > 0.f;
        if (running && ! wasRunning)
        {
            std::fill (accL.begin(), accL.begin() + N, 0.f);
            std::fill (accR.begin(), accR.begin() + N, 0.f);
            counter = 0;
        }
        wasRunning = running;

        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = stereo ? R[i] : 0.f;

            inL[(size_t) inPos] = xl;
            inR[(size_t) inPos] = xr;
            inPos = (inPos + 1) & mask;

            if (! running) continue;

            const float wl = accL[(size_t) accPos], wr = accR[(size_t) accPos];
            accL[(size_t) accPos] = 0.f;
            accR[(size_t) accPos] = 0.f;
            accPos = (accPos + 1) & mask;

            if (++counter >= hop)
            {
                counter = 0;
                processFrame (p, stereo);
            }

            en = on ? std::min (1.f, en + enUp) : std::max (0.f, en - enDown);

            L[i] = xl + (wl - xl) * en;
            if (stereo) R[i] = xr + (wr - xr) * en;
        }
    }

    // For the analyzer (written by the audio thread once per hop, read by the UI). Levels in dB, 0 dB = full-scale sine.
    std::atomic<float> dispIn[numBands], dispOut[numBands];
    std::atomic<unsigned> frames { 0 };

private:
    static float clamp01 (float v) { return std::min (1.f, std::max (0.f, v)); }

    void setOrder (int o)
    {
        o = std::min (maxOrder, std::max (minOrder, o));
        if (o == order) return;
        order = o;
        N = 1 << order;
        hop = N / overlap;

        const float pi = 3.14159265358979f;
        double sum = 0.0;
        for (int j = 0; j < N; ++j)
        {
            const float h = 0.5f - 0.5f * std::cos (2.f * pi * (float) j / (float) N);   // periodic Hann
            win[(size_t) j] = std::sqrt (h);
            sum += win[(size_t) j];
        }
        norm = 2.f / (float) sum;   // a full-scale sine in one bin reads 1.0

        for (int i = 0; i < N; ++i)   // bit reversal table
        {
            int r = 0;
            for (int b = 0; b < order; ++b) if (i & (1 << b)) r |= 1 << (order - 1 - b);
            rev[(size_t) i] = r;
        }

        for (auto* v : { &inL, &inR, &accL, &accR }) std::fill (v->begin(), v->end(), 0.f);
        inPos = accPos = counter = 0;

        // analyzer band -> bin map
        const float binHz = (float) sr / (float) N;
        const float top = std::min (fMaxHz, 0.49f * (float) sr);
        for (int b = 0; b < numBands; ++b)
        {
            const float f0 = fMinHz * std::pow (top / fMinHz, (float) b / (float) numBands);
            const float f1 = fMinHz * std::pow (top / fMinHz, (float) (b + 1) / (float) numBands);
            bandLo[b] = (int) std::ceil (f0 / binHz);
            bandHi[b] = (int) std::floor (f1 / binHz);
            const float pc = std::sqrt (f0 * f1) / binHz;
            bandI0[b] = std::min (N / 2 - 1, std::max (1, (int) pc));
            bandFr[b] = std::min (1.f, std::max (0.f, pc - (float) bandI0[b]));
            bandNarrow[b] = bandHi[b] < bandLo[b];
            bandLo[b] = std::max (1, std::min (N / 2, bandLo[b]));
            bandHi[b] = std::max (bandLo[b], std::min (N / 2, bandHi[b]));
        }
    }

    void fft (bool inverse)
    {
        float* r = re.data();
        float* m = im.data();
        for (int i = 0; i < N; ++i)
        {
            const int j = rev[(size_t) i];
            if (j > i) { std::swap (r[i], r[j]); std::swap (m[i], m[j]); }
        }
        const float sgn = inverse ? 1.f : -1.f;
        for (int len = 2; len <= N; len <<= 1)
        {
            const int half = len >> 1, step = maxN / len;
            for (int i = 0; i < N; i += len)
                for (int j = 0; j < half; ++j)
                {
                    const float wr = twC[(size_t) (j * step)], wi = sgn * twS[(size_t) (j * step)];
                    const int a = i + j, b = a + half;
                    const float vr = r[b] * wr - m[b] * wi, vi = r[b] * wi + m[b] * wr;
                    r[b] = r[a] - vr;  m[b] = m[a] - vi;
                    r[a] += vr;        m[a] += vi;
                }
        }
    }

    void processFrame (const Params& p, bool stereo)
    {
        const int mask = N - 1;
        for (int j = 0; j < N; ++j)
        {
            const int idx = (inPos + j) & mask;   // inPos is the oldest sample
            re[(size_t) j] = inL[(size_t) idx] * win[(size_t) j];
            im[(size_t) j] = stereo ? inR[(size_t) idx] * win[(size_t) j] : 0.f;
        }
        fft (false);

        const int half = N / 2;
        const float binHz = (float) sr / (float) N;
        const float gWeak = weakGain (p), gStrong = strongGain (p);
        const float dbWeak = 20.f * std::log10 (gWeak + 1.0e-9f), dbStrong = 20.f * std::log10 (gStrong + 1.0e-9f);

        for (int k = 0; k <= half; ++k)
        {
            const int k2 = (N - k) & (N - 1);
            const float zr = re[(size_t) k], zi = im[(size_t) k];
            const float cr = re[(size_t) k2], ci = -im[(size_t) k2];          // conj (Z[N-k])
            const float lr = 0.5f * (zr + cr), li = 0.5f * (zi + ci);          // left channel's bin
            const float rr = 0.5f * (zi - ci), ri = -0.5f * (zr - cr);         // right channel's bin
            const float mag = std::sqrt (std::max (lr * lr + li * li, rr * rr + ri * ri)) * norm;
            const float lv = 20.f * std::log10 (mag + 1.0e-9f);

            // strong or weak? (the DC bin is judged with the first bin's cutoff)
            const bool strong = lv >= thresholdDb (p, (float) std::max (k, 1) * binHz);
            const float gl = strong ? gStrong : gWeak;

            re[(size_t) k] *= gl;  im[(size_t) k] *= gl;
            if (k > 0 && k < half) { re[(size_t) k2] *= gl; im[(size_t) k2] *= gl; }

            lvDb[(size_t) k]  = lv;
            outDb[(size_t) k] = lv + (strong ? dbStrong : dbWeak);
        }

        fft (true);

        const float s = 0.5f / (float) N;   // 1/N for the inverse FFT, 1/2 for the 4x overlap of squared windows
        for (int j = 0; j < N; ++j)
        {
            const int idx = (accPos + j) & mask;
            accL[(size_t) idx] += re[(size_t) j] * win[(size_t) j] * s;
            accR[(size_t) idx] += im[(size_t) j] * win[(size_t) j] * s;
        }

        // analyzer
        for (int b = 0; b < numBands; ++b)
        {
            float a, o;
            if (bandNarrow[b])
            {
                const int i0 = bandI0[b];
                a = lvDb[(size_t) i0] + (lvDb[(size_t) i0 + 1] - lvDb[(size_t) i0]) * bandFr[b];
                o = outDb[(size_t) i0] + (outDb[(size_t) i0 + 1] - outDb[(size_t) i0]) * bandFr[b];
            }
            else
            {
                a = o = -200.f;
                for (int i = bandLo[b]; i <= bandHi[b]; ++i)
                {
                    a = std::max (a, lvDb[(size_t) i]);
                    o = std::max (o, outDb[(size_t) i]);
                }
            }
            dispIn[b].store (a, std::memory_order_relaxed);
            dispOut[b].store (o, std::memory_order_relaxed);
        }
        frames.fetch_add (1, std::memory_order_release);
    }

    double sr = 44100.0;
    int order = -1, N = 2048, hop = 512;
    int inPos = 0, accPos = 0, counter = 0;
    float norm = 1.f, en = 0.f;
    bool wasRunning = false;

    std::vector<float> twC, twS, inL, inR, accL, accR, re, im, win, lvDb, outDb;
    std::vector<int> rev;

    int bandLo[numBands] {}, bandHi[numBands] {}, bandI0[numBands] {};
    float bandFr[numBands] {};
    bool bandNarrow[numBands] {};
};
