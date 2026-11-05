#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------------------------
// Spectral gate: "only allow frequency components above or below a particular cut-off amplitude".
//
// Every FFT bin is measured against the cutoff amplitude and counts as STRONG (at or above it) or WEAK.
// The cutoff is RELATIVE: it is "dB below the loudest bin of the moment", so the gate behaves the same
// whatever level the signal arrives at (turn the input up or down and the same partials survive).
//   * BALANCE decides how much of each side is let through. The side being held back is lowered in dB,
//     linearly with the knob, down to -80 dB at the ends:
//        0.0  only the weak bins pass   (the loud parts are removed, what is left is the quiet "air")
//        0.5  everything passes         (the gate is neutral)
//        1.0  only the strong bins pass (a classic gate: everything under the cutoff is removed)
//     e.g. 0.7 holds the weak bins back by 32 dB, 0.3 holds the strong bins back by 32 dB.
//   * TILT slants the cutoff across the spectrum (0.5 = flat), pivoting at 1 kHz. Only while enabled.
//
// What makes the gating accurate rather than merely "bin by bin":
//   * 8x overlap with sqrt-Hann windows: the overlap-add stays clean even though every frame is modified
//   * HYSTERESIS: a bin that is open must fall 2 dB below the cutoff to close, so material sitting right on
//     the cutoff does not flutter
//   * HOLD / ATTACK / RELEASE act on how "strong" each bin is, in both directions, so the gate behaves the
//     same whether it is keeping the loud side or the quiet side (attack = a bin turning strong,
//     release = a bin turning weak)
//   * MAIN-LOBE PROTECTION: a strong partial smears over about +-2 bins in the analysis. Those bins are
//     treated as part of it, so the partial is gated as one piece instead of being chopped at its edges
//   * SMOOTH spreads each strong bin's influence outward along a sloping skirt, which removes the hollow,
//     watery sound hard spectral holes make
//
// Engine: both channels go through ONE complex FFT (left = real part, right = imaginary part), fully linked:
// each bin's level is the louder of the two and the same gain is applied to both, so the stereo image never
// wobbles. Latency is exactly the window size (getLatency()). Everything is pre-allocated for the largest
// window, so changing the window size on the audio thread never allocates.
// ---------------------------------------------------------------------------------------------
class SpectralGate
{
public:
    static constexpr int minOrder = 7, maxOrder = 13;            // 128 .. 8192
    static constexpr int maxN = 1 << maxOrder;
    static constexpr int overlap = 8;
    static constexpr int numBands = 64;                           // analyzer bands (log spaced)
    static constexpr float fMinHz = 20.f, fMaxHz = 20000.f;       // analyzer range
    static constexpr float tiltPivotHz = 1000.f;                  // the cutoff keeps its set value here, whatever the tilt
    static constexpr float maxTiltDbPerOct = 9.f;                 // tilt 0 / 1 = -9 / +9 dB per octave
    static constexpr float hysteresisDb = 2.f;
    static constexpr int   lobeBins = 2;                          // main-lobe protection radius
    static constexpr float holdBackDb = 80.f;                     // how far the held-back side drops at the end of the balance knob
    static constexpr float refFloorDb = -72.f;                    // the "loudest bin" is never taken to be quieter than this (near-silence is not boosted into the gate)
    static constexpr float refFallDbPerSec = 40.f;                // how fast the remembered loudest level falls when the sound gets quieter

    struct Params
    {
        float cutoffDb = -24.f;     // cutoff amplitude in dB BELOW the loudest bin (0 = only the loudest bin is strong)
        float balance = 0.7f;       // 0..1: 0 = only weak bins pass, 0.5 = all pass, 1 = only strong bins pass
        bool  tiltOn = false;
        float tilt = 0.5f;          // 0..1, 0.5 = flat
        float attackMs = 5.f;       // how fast a bin turns strong
        float holdMs = 25.f;        // a bin stays strong this long after it drops below the cutoff
        float releaseMs = 90.f;     // how fast a bin turns weak
        float smooth = 0.3f;        // 0..1: how far a strong bin's influence spreads to its neighbours
        int   order = 11;           // window = 2^order (7 .. 13 = 128 .. 8192)
    };

    // The cutoff at a frequency, in dB relative to the loudest bin (the UI draws dispRefDb + this)
    static float thresholdDb (const Params& p, float hz)
    {
        float t = p.cutoffDb;
        if (p.tiltOn)
            t += (p.tilt - 0.5f) * 2.f * maxTiltDbPerOct * std::log2 (std::max (hz, 1.f) / tiltPivotHz);
        return t;
    }

    // Linear gain of the weak bins / the strong bins for a balance setting
    // (the held-back side falls linearly in dB: 0 dB at the middle of the knob, -holdBackDb at the end)
    static float weakGain (const Params& p)   { return std::pow (10.f, -holdBackDb * clamp01 (2.f * p.balance - 1.f) / 20.f); }
    static float strongGain (const Params& p) { return std::pow (10.f, -holdBackDb * clamp01 (1.f - 2.f * p.balance) / 20.f); }

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
        for (auto* v : { &lvDb, &outDb, &holdLeft, &strongPrev, &o0, &o1, &oS }) v->assign ((size_t) maxN / 2 + 1, 0.f);
        for (auto& a : dispIn)  a.store (-200.f);
        for (auto& a : dispOut) a.store (-200.f);
        dispRefDb.store (refFloorDb);
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
            resetGates();
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
    std::atomic<float> dispRefDb { -72.f };   // the loudest bin of the moment (dB): the 0 point of the cutoff
    std::atomic<unsigned> frames { 0 };

private:
    static float clamp01 (float v) { return std::min (1.f, std::max (0.f, v)); }

    void resetGates()
    {
        std::fill (holdLeft.begin(), holdLeft.end(), 0.f);
        std::fill (strongPrev.begin(), strongPrev.end(), 0.f);
        primed = false;   // the first frame sets every bin's smoothed state straight to its target
        refDb = refFloorDb;
    }

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
        resetGates();
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
        const float hopSec = (float) hop / (float) sr;
        const float aC = std::exp (-hopSec / (0.001f * std::max (0.1f, p.attackMs)));    // a bin turning strong
        const float rC = std::exp (-hopSec / (0.001f * std::max (0.1f, p.releaseMs)));   // a bin turning weak
        const float holdSec = 0.001f * std::max (0.f, p.holdMs);
        const float gWeak = weakGain (p), gStrong = strongGain (p);
        const float dbWeak = 20.f * std::log10 (std::max (gWeak, 1.0e-4f)), dbStrong = 20.f * std::log10 (std::max (gStrong, 1.0e-4f));   // -80 dB floor

        // 1a) measure every bin, and find the loudest one (the DC bin is left out: a DC offset must not become the reference)
        float frameMax = -200.f;
        for (int k = 0; k <= half; ++k)
        {
            const int k2 = (N - k) & (N - 1);
            const float zr = re[(size_t) k], zi = im[(size_t) k];
            const float cr = re[(size_t) k2], ci = -im[(size_t) k2];          // conj (Z[N-k])
            const float lr = 0.5f * (zr + cr), li = 0.5f * (zi + ci);          // left channel's bin
            const float rr = 0.5f * (zi - ci), ri = -0.5f * (zr - cr);         // right channel's bin
            const float mag = std::sqrt (std::max (lr * lr + li * li, rr * rr + ri * ri)) * norm;
            const float lv = 20.f * std::log10 (mag + 1.0e-9f);
            lvDb[(size_t) k] = lv;
            if (k > 0) frameMax = std::max (frameMax, lv);
        }

        // the reference level: jumps up at once to a louder frame, falls slowly when the sound gets quieter
        refDb = std::max (std::max (frameMax, refFloorDb), refDb - refFallDbPerSec * hopSec);
        dispRefDb.store (refDb, std::memory_order_relaxed);

        // 1b) decide strong / weak against the cutoff (with hysteresis and hold)
        for (int k = 0; k <= half; ++k)
        {
            const float lv = lvDb[(size_t) k];

            // (the DC bin is judged with the first bin's cutoff)
            const float thr = refDb + thresholdDb (p, (float) std::max (k, 1) * binHz);
            bool strong = lv >= (strongPrev[(size_t) k] > 0.5f ? thr - hysteresisDb : thr);

            if (strong)                       holdLeft[(size_t) k] = holdSec;
            else if (holdLeft[(size_t) k] > 0.f) { holdLeft[(size_t) k] -= hopSec; strong = true; }

            strongPrev[(size_t) k] = strong ? 1.f : 0.f;
            o0[(size_t) k] = strong ? 1.f : 0.f;
        }

        // 2) main-lobe protection: a strong bin takes its +-lobeBins neighbours with it
        for (int k = 0; k <= half; ++k)
        {
            float m = 0.f;
            for (int j = std::max (0, k - lobeBins); j <= std::min (half, k + lobeBins); ++j) m = std::max (m, o0[(size_t) j]);
            o1[(size_t) k] = m;
        }

        // 3) skirts: influence falls off linearly beyond the lobe (two scans give the exact max-with-falloff)
        const int rad = (int) std::lround (clamp01 (p.smooth) * 12.f * (float) N / 2048.f);
        if (rad > 0)
        {
            const float fall = 1.f / (float) (rad + 1);
            for (int k = 1; k <= half; ++k)      o1[(size_t) k] = std::max (o1[(size_t) k], o1[(size_t) k - 1] - fall);
            for (int k = half - 1; k >= 0; --k)  o1[(size_t) k] = std::max (o1[(size_t) k], o1[(size_t) k + 1] - fall);
        }

        // 4) smooth "strongness" in time (attack when it rises, release when it falls), then turn it into gain
        if (! primed)
        {
            for (int k = 0; k <= half; ++k) oS[(size_t) k] = o1[(size_t) k];
            primed = true;
        }

        for (int k = 0; k <= half; ++k)
        {
            float& s = oS[(size_t) k];
            const float t = o1[(size_t) k];
            s += (t - s) * (1.f - (t > s ? aC : rC));

            const float gDb = dbWeak + (dbStrong - dbWeak) * s;
            const float gl = std::pow (10.f, gDb / 20.f);

            const int k2 = (N - k) & (N - 1);
            re[(size_t) k] *= gl;  im[(size_t) k] *= gl;
            if (k > 0 && k < half) { re[(size_t) k2] *= gl; im[(size_t) k2] *= gl; }

            outDb[(size_t) k] = lvDb[(size_t) k] + gDb;
        }

        fft (true);

        const float sc = (2.f / (float) overlap) / (float) N;   // 1/N for the inverse FFT, 2/overlap for the sum of the squared windows
        for (int j = 0; j < N; ++j)
        {
            const int idx = (accPos + j) & mask;
            accL[(size_t) idx] += re[(size_t) j] * win[(size_t) j] * sc;
            accR[(size_t) idx] += im[(size_t) j] * win[(size_t) j] * sc;
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
    int order = -1, N = 2048, hop = 256;
    int inPos = 0, accPos = 0, counter = 0;
    float norm = 1.f, en = 0.f;
    bool wasRunning = false, primed = false;
    float refDb = -72.f;   // remembered loudest-bin level (dB)

    std::vector<float> twC, twS, inL, inR, accL, accR, re, im, win, lvDb, outDb;
    std::vector<float> holdLeft, strongPrev, o0, o1, oS;   // per-bin: hold timer, last decision, raw / spread / smoothed "strongness"
    std::vector<int> rev;

    int bandLo[numBands] {}, bandHi[numBands] {}, bandI0[numBands] {};
    float bandFr[numBands] {};
    bool bandNarrow[numBands] {};
};
