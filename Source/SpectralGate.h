#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------------------------
// Spectral gate: every FFT bin gets its own gate.
//
//  * sqrt-Hann analysis + synthesis windows, 4x overlap (75 %), so the overlap-add sums back to unity
//  * both channels go through ONE complex FFT (left = real part, right = imaginary part)
//  * the two channels are fully linked: each bin's level is the louder of the two, and the same gain is
//    applied to both, so the stereo image never wobbles
//  * a bin is "open" when its level is above the threshold curve (soft knee). Closed bins are pushed down
//    by `range` dB instead of being cut dead, so the gate can be anything from a gentle hush to a hard mute
//  * per-bin hold keeps a bin open for a moment after it falls quiet (stops flutter on decaying tails)
//  * per-bin attack (opening) / release (closing), smoothed in dB once per hop
//  * "smooth" lets every open bin spread its openness to its neighbours along a sloping skirt (linear in dB).
//    Peaks stay fully open, but the edges of open regions turn soft instead of leaving hard spectral holes
//    around each partial, which is what makes a bin-by-bin gate sound hollow and watery
//  * the dry path is delayed by the same latency, so the mix control never combs
//
// Latency is exactly the window size (see getLatency()). Everything is pre-allocated for the largest
// window, so changing the window size on the audio thread never allocates.
// ---------------------------------------------------------------------------------------------
class SpectralGate
{
public:
    static constexpr int minOrder = 9, maxOrder = 12;            // 512 .. 4096
    static constexpr int maxN = 1 << maxOrder;
    static constexpr int overlap = 4;
    static constexpr int numBands = 64;                           // analyzer bands (log spaced)
    static constexpr float fMinHz = 20.f, fMaxHz = 20000.f;       // analyzer range

    struct Params
    {
        float mix = 1.f;            // 0..1
        float outGainDb = 0.f;
        float thrDb = -60.f;        // threshold at the centre frequency; 0 dB = a full-scale sine in one bin
        float centerHz = 1000.f;
        float slope = -3.f;         // dB per octave around the centre
        float curve = 0.f;          // dB per octave^2
        float rangeDb = -40.f;      // how far a closed bin is pushed down (<= 0)
        float holdMs = 30.f;        // a bin stays open this long after it drops below the threshold
        float kneeDb = 6.f;         // width of the soft transition around the threshold
        float smooth = 0.25f;       // 0..1: how far open bins spread their openness to the neighbouring bins
        float attackMs = 5.f;       // how fast a bin opens
        float releaseMs = 100.f;    // how fast a bin closes
        int order = 11;             // window = 2^order
    };

    // The threshold at a frequency (shared with the UI so the curve it draws is the real one)
    static float thresholdDb (const Params& p, float hz)
    {
        const float o = std::log2 (std::max (hz, 1.f) / std::max (p.centerHz, 1.f));
        return p.thrDb + p.slope * o + p.curve * o * o;
    }

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
        for (auto* v : { &gDb, &gApp, &lvDb, &outDb, &holdLeft }) v->assign ((size_t) maxN / 2 + 1, 0.f);
        heldOpen.assign ((size_t) maxN / 2 + 1, 1.f);
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
        const float outLin = std::pow (10.f, p.outGainDb / 20.f);
        const float mixT = std::min (1.f, std::max (0.f, p.mix));

        const bool running = on || en > 0.f;
        if (running && ! wasRunning)
        {
            std::fill (accL.begin(), accL.begin() + N, 0.f);
            std::fill (accR.begin(), accR.begin() + N, 0.f);
            resetGates();
            counter = 0;
            mixSm = mixT;
            gainSm = outLin;
        }
        wasRunning = running;

        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = stereo ? R[i] : 0.f;

            const float dl = inL[(size_t) inPos], dr = inR[(size_t) inPos];   // the input from exactly N samples ago
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
                processFrame (p, stereo, mixSm);
            }

            mixSm += (mixT - mixSm) * 0.002f;
            gainSm += (outLin - gainSm) * 0.002f;
            en = on ? std::min (1.f, en + enUp) : std::max (0.f, en - enDown);

            const float cl = (dl * (1.f - mixSm) + wl * mixSm) * gainSm;
            const float cr = (dr * (1.f - mixSm) + wr * mixSm) * gainSm;
            L[i] = xl + (cl - xl) * en;
            if (stereo) R[i] = xr + (cr - xr) * en;
        }
    }

    // For the analyzer (written by the audio thread once per hop, read by the UI). Levels in dB, 0 dB = full-scale sine.
    std::atomic<float> dispIn[numBands], dispOut[numBands];
    std::atomic<unsigned> frames { 0 };

private:
    void resetGates()
    {
        std::fill (gDb.begin(), gDb.end(), 0.f);          // every gate starts fully open
        std::fill (heldOpen.begin(), heldOpen.end(), 1.f);
        std::fill (holdLeft.begin(), holdLeft.end(), 0.f);
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

    // 0 = fully closed, 1 = fully open. `over` is how far the level sits above the threshold (dB).
    // Across the knee it follows a smoothstep, so the gate eases open instead of snapping.
    static float openAmount (float over, float knee)
    {
        if (knee < 0.01f) return over >= 0.f ? 1.f : 0.f;
        const float x = (over + 0.5f * knee) / knee;
        if (x <= 0.f) return 0.f;
        if (x >= 1.f) return 1.f;
        return x * x * (3.f - 2.f * x);
    }

    void processFrame (const Params& p, bool stereo, float mixNow)
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
        const float aC = std::exp (-hopSec / (0.001f * std::max (0.1f, p.attackMs)));    // opening
        const float rC = std::exp (-hopSec / (0.001f * std::max (0.1f, p.releaseMs)));   // closing
        const float range = std::min (0.f, p.rangeDb);
        const float holdSec = 0.001f * std::max (0.f, p.holdMs);
        const float kn = p.kneeDb;
        const float mix = std::min (1.f, std::max (0.f, mixNow));

        // pass 1: measure every bin and move its gate (in dB) towards open or closed
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

            // (the DC bin is gated like the rest, using the first bin's threshold: left open it would leak
            //  about 1/N of any noise's power straight through, which puts a floor under the range)
            const float o = openAmount (lv - thresholdDb (p, (float) std::max (k, 1) * binHz), kn);
            float& held = heldOpen[(size_t) k];
            float& hl   = holdLeft[(size_t) k];
            if (o >= held)       { held = o; hl = holdSec; }   // louder than before: reopen and restart the hold
            else if (hl > 0.f)   hl -= hopSec;                   // quieter, but still holding
            else                 held = o;                       // hold is over: follow the level down
            const float open = held;

            const float target = range * (1.f - open);   // 0 dB when open, `range` when closed
            float& g = gDb[(size_t) k];
            g += (target - g) * (1.f - (target > g ? aC : rC));       // rising = opening = attack
        }

        // optional skirts: every bin's gain can lift its neighbours, falling by `fall` dB per bin of distance, so
        // it reaches the closed level after `rad` bins. Two linear scans (up, then down the spectrum) give the
        // exact "max over neighbours" without a search. It works on a copy: the stored gains stay un-spread.
        const int rad = (int) std::lround (std::min (1.f, std::max (0.f, p.smooth)) * 16.f * (float) N / 2048.f);
        std::copy (gDb.begin(), gDb.begin() + half + 1, gApp.begin());
        if (rad > 0 && range < -0.01f)
        {
            const float fall = -range / (float) (rad + 1);
            for (int k = 1; k <= half; ++k)    gApp[(size_t) k] = std::max (gApp[(size_t) k], gApp[(size_t) k - 1] - fall);
            for (int k = half - 1; k >= 0; --k) gApp[(size_t) k] = std::max (gApp[(size_t) k], gApp[(size_t) k + 1] - fall);
        }

        // pass 2: apply the gains
        for (int k = 0; k <= half; ++k)
        {
            const int k2 = (N - k) & (N - 1);
            const float gl = std::pow (10.f, gApp[(size_t) k] / 20.f);
            re[(size_t) k] *= gl;  im[(size_t) k] *= gl;
            if (k > 0 && k < half) { re[(size_t) k2] *= gl; im[(size_t) k2] *= gl; }

            // what the listener ends up with in this bin: dry and processed blended, then the output gain
            outDb[(size_t) k] = lvDb[(size_t) k] + 20.f * std::log10 ((1.f - mix) + mix * gl + 1.0e-9f) + p.outGainDb;
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
    float norm = 1.f, en = 0.f, mixSm = 1.f, gainSm = 1.f;
    bool wasRunning = false;

    std::vector<float> twC, twS, inL, inR, accL, accR, re, im, win;
    std::vector<float> gDb, gApp, lvDb, outDb;     // per-bin gate gain (dB), the skirted copy that is applied, input level, output level
    std::vector<float> heldOpen, holdLeft;          // per-bin hold state: how open it is held, seconds of hold left
    std::vector<int> rev;

    int bandLo[numBands] {}, bandHi[numBands] {}, bandI0[numBands] {};
    float bandFr[numBands] {};
    bool bandNarrow[numBands] {};
};
