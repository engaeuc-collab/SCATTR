#pragma once
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------------------------
// Parametric equalizer: 24 bands, each a bell, shelf, cut, notch, band-pass or tilt (RBJ biquads).
// Cuts can be steepened from 12 dB / octave up to 96 dB / octave (cascaded Butterworth sections).
//
// The design and response functions are shared with the UI, so the curve drawn in the editor is computed
// from exactly the same coefficients the audio runs.
// ---------------------------------------------------------------------------------------------
namespace EqCore
{
    constexpr int numBands = 24;
    constexpr float minHz = 20.f, maxHz = 20000.f;
    constexpr float maxGainDb = 18.f;
    constexpr int maxSections = 8;   // the steepest cut (96 dB / oct) is eight biquads

    // New shapes are only ever appended, so projects saved with the older, shorter list keep their meaning.
    enum Type { Bell = 0, LowShelf, HighShelf, LowCut, HighCut, Notch, BandPass, TiltShelf, numTypes };

    inline const char* typeName (int t)
    {
        static const char* n[numTypes] = { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch", "Band pass", "Tilt shelf" };
        return n[std::min (numTypes - 1, std::max (0, t))];
    }

    // does the band's gain mean anything for this shape? (cuts, notches and band-pass only use frequency and Q)
    inline bool usesGain (int t) { return t == Bell || t == LowShelf || t == HighShelf || t == TiltShelf; }
    inline bool usesQ    (int t) { return t != TiltShelf; }
    inline bool isCut    (int t) { return t == LowCut || t == HighCut; }

    // cut steepness
    constexpr int numSlopes = 6;
    inline int slopeSections (int s)
    {
        static const int n[numSlopes] = { 1, 2, 3, 4, 6, 8 };
        return n[std::min (numSlopes - 1, std::max (0, s))];
    }
    inline const char* slopeName (int s)
    {
        static const char* n[numSlopes] = { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct" };
        return n[std::min (numSlopes - 1, std::max (0, s))];
    }

    // dynamic bands: how fast the band's level detector reacts
    constexpr int numSpeeds = 3;
    inline const char* speedName (int s)
    {
        static const char* n[numSpeeds] = { "Fast", "Medium", "Slow" };
        return n[std::min (numSpeeds - 1, std::max (0, s))];
    }
    inline void speedTimes (int s, double& attackMs, double& releaseMs)
    {
        static const double a[numSpeeds] = { 2.0, 10.0, 40.0 }, r[numSpeeds] = { 50.0, 150.0, 500.0 };
        s = std::min (numSpeeds - 1, std::max (0, s));
        attackMs = a[s];  releaseMs = r[s];
    }

    struct Band
    {
        bool  on = false;
        int   type = Bell;
        float freq = 1000.f, gainDb = 0.f, q = 1.f;
        int   slope = 0;   // only used by the cuts

        // Dynamic: the band listens to the signal around its own frequency. Once that passes `thresh` (dB), the band's gain
        // moves from `gainDb` towards `gainDb + range`. (A bell at 0 dB with range -8 is a de-esser; a bell at -6 dB with
        // range +6 opens up as the music gets louder.) Only bands that have a gain can be dynamic.
        bool  dyn = false;
        float thresh = -30.f, range = -6.f;
        int   speed = 1;

        bool operator== (const Band& o) const
        {
            return on == o.on && type == o.type && freq == o.freq && gainDb == o.gainDb && q == o.q && slope == o.slope
                && dyn == o.dyn && thresh == o.thresh && range == o.range && speed == o.speed;
        }
    };

    struct Coeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };   // a0 normalised to 1

    // one biquad section
    inline Coeffs design (int type, double f, double gainDb, double q, double sr)
    {
        const double pi = 3.14159265358979323846;
        f = std::min (std::max (f, 10.0), 0.49 * sr);
        q = std::max (q, 0.05);

        const double w0 = 2.0 * pi * f / sr, cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * q);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double sA = 2.0 * std::sqrt (A) * alpha;

        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
        switch (type)
        {
            default:
            case Bell:
                b0 = 1 + alpha * A;  b1 = -2 * cw;  b2 = 1 - alpha * A;
                a0 = 1 + alpha / A;  a1 = -2 * cw;  a2 = 1 - alpha / A;
                break;
            case LowShelf:
                b0 =      A * ((A + 1) - (A - 1) * cw + sA);
                b1 =  2 * A * ((A - 1) - (A + 1) * cw);
                b2 =      A * ((A + 1) - (A - 1) * cw - sA);
                a0 =           (A + 1) + (A - 1) * cw + sA;
                a1 =     -2 * ((A - 1) + (A + 1) * cw);
                a2 =           (A + 1) + (A - 1) * cw - sA;
                break;
            case HighShelf:
                b0 =      A * ((A + 1) + (A - 1) * cw + sA);
                b1 = -2 * A * ((A - 1) + (A + 1) * cw);
                b2 =      A * ((A + 1) + (A - 1) * cw - sA);
                a0 =           (A + 1) - (A - 1) * cw + sA;
                a1 =      2 * ((A - 1) - (A + 1) * cw);
                a2 =           (A + 1) - (A - 1) * cw - sA;
                break;
            case LowCut:   // high-pass
                b0 = (1 + cw) * 0.5;  b1 = -(1 + cw);  b2 = (1 + cw) * 0.5;
                a0 = 1 + alpha;       a1 = -2 * cw;    a2 = 1 - alpha;
                break;
            case HighCut:  // low-pass
                b0 = (1 - cw) * 0.5;  b1 = 1 - cw;     b2 = (1 - cw) * 0.5;
                a0 = 1 + alpha;       a1 = -2 * cw;    a2 = 1 - alpha;
                break;
            case Notch:
                b0 = 1;               b1 = -2 * cw;    b2 = 1;
                a0 = 1 + alpha;       a1 = -2 * cw;    a2 = 1 - alpha;
                break;
            case BandPass: // constant 0 dB peak gain
                b0 = alpha;           b1 = 0;          b2 = -alpha;
                a0 = 1 + alpha;       a1 = -2 * cw;    a2 = 1 - alpha;
                break;
        }

        Coeffs c;
        c.b0 = b0 / a0;  c.b1 = b1 / a0;  c.b2 = b2 / a0;
        c.a1 = a1 / a0;  c.a2 = a2 / a0;
        return c;
    }

    // a whole band: usually one biquad, a few for steep cuts, two for a tilt
    struct Sections { int n = 0; Coeffs c[maxSections]; };

    inline Sections designBand (const Band& b, double sr)
    {
        const double pi = 3.14159265358979323846;
        Sections s;

        if (b.type == TiltShelf)
        {
            // lows go one way and highs the other, meeting at the band's frequency (up = brighter)
            s.c[0] = design (LowShelf,  b.freq, -0.5 * b.gainDb, 0.5, sr);
            s.c[1] = design (HighShelf, b.freq,  0.5 * b.gainDb, 0.5, sr);
            s.n = 2;
        }
        else if (isCut (b.type))
        {
            // Butterworth: section k has Q = 1 / (2 cos((2k-1) pi / 2N)). Q on the band adds a resonant peak, which goes on
            // the sharpest section (the last one). With one section that is simply Q itself.
            const int n = slopeSections (b.slope), order = 2 * n;
            const double res = std::max ((double) b.q, 0.05) / 0.70710678118654752;
            for (int i = 0; i < n; ++i)
            {
                double qi = 1.0 / (2.0 * std::cos ((2.0 * i + 1.0) * pi / (2.0 * order)));
                if (i == n - 1) qi *= res;
                s.c[i] = design (b.type, b.freq, 0.0, qi, sr);
            }
            s.n = n;
        }
        else
        {
            s.c[0] = design (b.type, b.freq, b.gainDb, b.q, sr);
            s.n = 1;
        }
        return s;
    }

    // the filter a dynamic band listens through: its own region of the spectrum
    inline Coeffs designDetector (const Band& b, double sr)
    {
        switch (b.type)
        {
            case LowShelf:  return design (HighCut,  b.freq, 0.0, 0.70710678, sr);   // a low shelf listens to what is below it
            case HighShelf: return design (LowCut,   b.freq, 0.0, 0.70710678, sr);
            case TiltShelf: return design (BandPass, b.freq, 0.0, 0.5, sr);
            default:        return design (BandPass, b.freq, 0.0, std::max (0.4, (double) b.q), sr);
        }
    }

    // magnitude of one biquad at a frequency, in dB
    inline double magDb (const Coeffs& c, double hz, double sr)
    {
        const double w = 2.0 * 3.14159265358979323846 * hz / sr;
        const double c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = c.b0 + c.b1 * c1 + c.b2 * c2, ni = -(c.b1 * s1 + c.b2 * s2);
        const double dr = 1.0 + c.a1 * c1 + c.a2 * c2,  di = -(c.a1 * s1 + c.a2 * s2);
        return 10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1.0e-30);
    }

    inline double sectionsDb (const Sections& s, double hz, double sr)
    {
        double db = 0.0;
        for (int i = 0; i < s.n; ++i) db += magDb (s.c[i], hz, sr);
        return db;
    }

    inline double bandDb (const Band& b, double hz, double sr)
    {
        return b.on ? sectionsDb (designBand (b, sr), hz, sr) : 0.0;
    }

    // The stereo processor. Both channels share the coefficients; state is kept in doubles because a
    // 12 dB/oct filter at 20 Hz is touchy in single precision.
    class Processor
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            for (auto& s : slots) { s.coeffsValid = false; s.reset(); }
        }

        // call once per block with the current settings; coefficients are only redesigned when a band changed
        void setBands (const Band* b)
        {
            anyOn = false;
            for (int i = 0; i < numBands; ++i)
            {
                auto& s = slots[(size_t) i];
                if (! s.coeffsValid || ! (s.band == b[i]))
                {
                    const bool wasOn = s.band.on;
                    if (b[i].on && ! wasOn) s.reset();       // a band waking up starts from silence
                    s.band = b[i];
                    const int oldN = s.sec.n;
                    s.sec = designBand (s.band, sr);
                    if (s.sec.n != oldN) s.reset();          // a different slope: the old section states mean nothing
                    s.coeffsValid = true;
                    s.lastG = s.band.gainDb;

                    const bool dynNow = s.band.on && s.band.dyn && usesGain (s.band.type);
                    if (dynNow != s.dynActive || (s.band.on && ! wasOn)) { s.env = 0.0; s.dz[0] = s.dz[1] = 0.0; s.amt = 0.f; }
                    s.dynActive = dynNow;
                    if (dynNow)
                    {
                        s.det = designDetector (s.band, sr);
                        double a, r;  speedTimes (s.band.speed, a, r);
                        s.atkC = 1.0 - std::exp (-1.0 / (0.001 * a * sr));
                        s.relC = 1.0 - std::exp (-1.0 / (0.001 * r * sr));
                    }
                    else s.amt = 0.f;
                }
                if (s.band.on) anyOn = true;
            }
        }

        bool active() const { return anyOn; }

        // 0..1: how far a dynamic band has moved towards its range right now (the UI draws the curve with it)
        float dynAmount (int i) const { return slots[(size_t) i].dynActive ? slots[(size_t) i].amt : 0.f; }

        void process (float* L, float* R, int n)
        {
            if (! anyOn) return;
            const int numCh = R != nullptr ? 2 : 1;

            // Work in short chunks: at the start of each one the dynamic bands listen to the (still untouched) input and
            // nudge their coefficients, then every band filters the chunk.
            constexpr int chunk = 32;
            for (int start = 0; start < n; start += chunk)
            {
                const int len = std::min (chunk, n - start);
                updateDynamics (L + start, R != nullptr ? R + start : nullptr, len);

                for (auto& s : slots)
                {
                    if (! s.band.on) continue;
                    const int ns = s.sec.n;
                    for (int ch = 0; ch < numCh; ++ch)
                    {
                        float* x = (ch == 0 ? L : R) + start;
                        auto& z = s.z[ch];
                        for (int i = 0; i < len; ++i)
                        {
                            double v = x[i];
                            for (int k = 0; k < ns; ++k)   // through every section in double precision
                            {
                                const auto& c = s.sec.c[k];
                                const double out = c.b0 * v + z[k][0];
                                z[k][0] = c.b1 * v - c.a1 * out + z[k][1];
                                z[k][1] = c.b2 * v - c.a2 * out;
                                v = out;
                            }
                            x[i] = (float) v;
                        }
                    }
                }
            }
        }

    private:
        struct Slot
        {
            Band band;
            Sections sec;
            double z[2][maxSections][2] {};
            bool coeffsValid = false;

            // dynamic bands
            bool dynActive = false;
            Coeffs det;                  // the detector filter
            double dz[2] {};             // its state
            double env = 0.0;            // the level follower
            double atkC = 0.01, relC = 0.001;
            float amt = 0.f, lastG = 0.f;

            void reset() { for (auto& ch : z) for (auto& sec : ch) sec[0] = sec[1] = 0.0; }
        };

        void updateDynamics (const float* l, const float* r, int len)
        {
            for (auto& s : slots)
            {
                if (! s.dynActive) { s.amt = 0.f; continue; }

                double z1 = s.dz[0], z2 = s.dz[1], env = s.env;
                const auto& c = s.det;
                for (int i = 0; i < len; ++i)
                {
                    const double in = r != nullptr ? 0.5 * ((double) l[i] + (double) r[i]) : (double) l[i];
                    const double out = c.b0 * in + z1;
                    z1 = c.b1 * in - c.a1 * out + z2;
                    z2 = c.b2 * in - c.a2 * out;
                    const double a = std::fabs (out);
                    env += (a > env ? s.atkC : s.relC) * (a - env);
                }
                s.dz[0] = z1;  s.dz[1] = z2;
                s.env = env < 1.0e-12 ? 0.0 : env;

                // above the threshold the band moves towards its range, over a soft 8 dB knee
                const double levelDb = 20.0 * std::log10 (s.env + 1.0e-9);
                const double x = std::min (1.0, std::max (0.0, (levelDb - ((double) s.band.thresh - 4.0)) / 8.0));
                const float target = (float) (x * x * (3.0 - 2.0 * x));
                s.amt += (target - s.amt) * 0.4f;

                const float g = s.band.gainDb + s.band.range * s.amt;
                if (std::fabs (g - s.lastG) > 0.02f)
                {
                    Band eb = s.band;
                    eb.gainDb = g;
                    s.sec = designBand (eb, sr);
                    s.lastG = g;
                }
            }
        }

        double sr = 44100.0;
        bool anyOn = false;
        Slot slots[numBands];
    };
}
