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

    struct Band
    {
        bool  on = false;
        int   type = Bell;
        float freq = 1000.f, gainDb = 0.f, q = 1.f;
        int   slope = 0;   // only used by the cuts

        bool operator== (const Band& o) const
        {
            return on == o.on && type == o.type && freq == o.freq && gainDb == o.gainDb && q == o.q && slope == o.slope;
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
                    if (b[i].on && ! s.band.on) s.reset();   // a band waking up starts from silence
                    s.band = b[i];
                    const int oldN = s.sec.n;
                    s.sec = designBand (s.band, sr);
                    if (s.sec.n != oldN) s.reset();          // a different slope: the old section states mean nothing
                    s.coeffsValid = true;
                }
                if (s.band.on) anyOn = true;
            }
        }

        bool active() const { return anyOn; }

        void process (float* L, float* R, int n)
        {
            if (! anyOn) return;
            const int numCh = R != nullptr ? 2 : 1;
            for (auto& s : slots)
            {
                if (! s.band.on) continue;
                const int ns = s.sec.n;
                for (int ch = 0; ch < numCh; ++ch)
                {
                    float* x = ch == 0 ? L : R;
                    auto& z = s.z[ch];
                    for (int i = 0; i < n; ++i)
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

    private:
        struct Slot
        {
            Band band;
            Sections sec;
            double z[2][maxSections][2] {};
            bool coeffsValid = false;
            void reset() { for (auto& ch : z) for (auto& sec : ch) sec[0] = sec[1] = 0.0; }
        };

        double sr = 44100.0;
        bool anyOn = false;
        Slot slots[numBands];
    };
}
