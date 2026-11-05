#pragma once
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------------------------
// Parametric equalizer: eight bands, each a bell, shelf, cut or notch (RBJ biquads, 12 dB / octave).
//
// The design and response functions are shared with the UI, so the curve drawn in the editor is computed
// from exactly the same coefficients the audio runs.
// ---------------------------------------------------------------------------------------------
namespace EqCore
{
    constexpr int numBands = 8;
    constexpr float minHz = 20.f, maxHz = 20000.f;
    constexpr float maxGainDb = 18.f;

    enum Type { Bell = 0, LowShelf, HighShelf, LowCut, HighCut, Notch, numTypes };

    inline const char* typeName (int t)
    {
        static const char* n[numTypes] = { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch" };
        return n[std::min (numTypes - 1, std::max (0, t))];
    }

    // does the band's gain mean anything for this shape? (cuts and notches only use frequency and Q)
    inline bool usesGain (int t) { return t == Bell || t == LowShelf || t == HighShelf; }

    struct Band
    {
        bool  on = false;
        int   type = Bell;
        float freq = 1000.f, gainDb = 0.f, q = 1.f;

        bool operator== (const Band& o) const
        {
            return on == o.on && type == o.type && freq == o.freq && gainDb == o.gainDb && q == o.q;
        }
    };

    struct Coeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };   // a0 normalised to 1

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
        }

        Coeffs c;
        c.b0 = b0 / a0;  c.b1 = b1 / a0;  c.b2 = b2 / a0;
        c.a1 = a1 / a0;  c.a2 = a2 / a0;
        return c;
    }

    // magnitude of one filter at a frequency, in dB
    inline double magDb (const Coeffs& c, double hz, double sr)
    {
        const double w = 2.0 * 3.14159265358979323846 * hz / sr;
        const double c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = c.b0 + c.b1 * c1 + c.b2 * c2, ni = -(c.b1 * s1 + c.b2 * s2);
        const double dr = 1.0 + c.a1 * c1 + c.a2 * c2,  di = -(c.a1 * s1 + c.a2 * s2);
        return 10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1.0e-30);
    }

    inline double bandDb (const Band& b, double hz, double sr)
    {
        return b.on ? magDb (design (b.type, b.freq, b.gainDb, b.q, sr), hz, sr) : 0.0;
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
                    s.c = design (s.band.type, s.band.freq, s.band.gainDb, s.band.q, sr);
                    s.coeffsValid = true;
                }
                if (s.band.on) anyOn = true;
            }
        }

        bool active() const { return anyOn; }

        void process (float* L, float* R, int n)
        {
            if (! anyOn) return;
            for (auto& s : slots)
            {
                if (! s.band.on) continue;
                const auto& c = s.c;
                for (int ch = 0; ch < (R != nullptr ? 2 : 1); ++ch)
                {
                    float* x = ch == 0 ? L : R;
                    double z1 = s.z[ch][0], z2 = s.z[ch][1];
                    for (int i = 0; i < n; ++i)
                    {
                        const double in = x[i];
                        const double out = c.b0 * in + z1;
                        z1 = c.b1 * in - c.a1 * out + z2;
                        z2 = c.b2 * in - c.a2 * out;
                        x[i] = (float) out;
                    }
                    s.z[ch][0] = z1;  s.z[ch][1] = z2;
                }
            }
        }

    private:
        struct Slot
        {
            Band band;
            Coeffs c;
            double z[2][2] {};
            bool coeffsValid = false;
            void reset() { z[0][0] = z[0][1] = z[1][0] = z[1][1] = 0.0; }
        };

        double sr = 44100.0;
        bool anyOn = false;
        Slot slots[numBands];
    };
}
