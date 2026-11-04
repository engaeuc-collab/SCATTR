#pragma once
#include "PluginProcessor.h"

namespace Scat
{
    struct Palette
    {
        juce::Colour bgTop, bgMid, bgBot, soil, moss, ink, dim, line, bark, acc, accDark, amber, grass, grassDark, glow;
    };

    // Global "calmness" of the look: 1 = the old, bold colours, lower = softer and more muted.
    // Saturation is scaled per colour role so backgrounds go quietest and accents stay readable.
    inline constexpr float calmBg = 0.55f, calmAccent = 0.58f, calmText = 0.55f;

    inline juce::Colour calm (juce::uint32 argb, float sat, float bright = 1.f)
    {
        const juce::Colour c (argb);
        return juce::Colour::fromHSV (c.getHue(), c.getSaturation() * sat, c.getBrightness() * bright, 1.f);
    }

    inline Palette makePal (juce::uint32 bgTop, juce::uint32 bgMid, juce::uint32 bgBot, juce::uint32 soil,
                            juce::uint32 moss, juce::uint32 ink, juce::uint32 dim, juce::uint32 line,
                            juce::uint32 bark, juce::uint32 acc, juce::uint32 accDark, juce::uint32 amber,
                            juce::uint32 grass, juce::uint32 grassDark, juce::uint32 glow)
    {
        return { calm (bgTop, calmBg), calm (bgMid, calmBg), calm (bgBot, calmBg), calm (soil, calmBg),
                 calm (moss, calmBg), calm (ink, calmText), calm (dim, calmText), calm (line, calmBg),
                 calm (bark, calmBg), calm (acc, calmAccent, 0.94f), calm (accDark, calmAccent, 0.94f), calm (amber, calmAccent, 0.94f),
                 calm (grass, calmAccent, 0.94f), calm (grassDark, calmAccent), calm (glow, calmAccent, 0.94f) };
    }

    // Blend two palettes (used for the theme transition)
    inline Palette lerpPal (const Palette& a, const Palette& b, float t)
    {
        auto m = [t] (juce::Colour x, juce::Colour y) { return x.interpolatedWith (y, t); };
        return { m (a.bgTop, b.bgTop), m (a.bgMid, b.bgMid), m (a.bgBot, b.bgBot), m (a.soil, b.soil),
                 m (a.moss, b.moss), m (a.ink, b.ink), m (a.dim, b.dim), m (a.line, b.line),
                 m (a.bark, b.bark), m (a.acc, b.acc), m (a.accDark, b.accDark), m (a.amber, b.amber),
                 m (a.grass, b.grass), m (a.grassDark, b.grassDark), m (a.glow, b.glow) };
    }

    struct Theme { const char* name; Palette pal; };

    // sky top/mid/bottom, panel, hover, text, dim text, track, border, accent, accent dark, highlight, leaf, leaf dark, light
    // "light" is the colour of the glow that spills out of cards, the sun / aurora in the sky and the floating motes.
    inline const std::array<Theme, 4>& themes()
    {
        static const std::array<Theme, 4> t {{
            { "summer",   // lagoon at golden hour
              makePal (0xff0b3a5a, 0xff16807f, 0xff0b2b22, 0xff08292f, 0xff1b5d66, 0xfffff4dc, 0xff9fd4cf,
                       0xff2a7a84, 0xff3f9a97, 0xffff8a3d, 0xffc2410c, 0xffffd34d, 0xff9be15d, 0xff2f9a4a, 0xffffc46b) },
            { "autumn",   // plum dusk over embers
              makePal (0xff2b1233, 0xff6b2a2f, 0xff170d0b, 0xff1d0f10, 0xff4d2a2b, 0xfffbe9d3, 0xffd4a99a,
                       0xff7a3d36, 0xffa8573a, 0xffff6b35, 0xff9a2a14, 0xffffb84a, 0xffe0a032, 0xff8a3f1c, 0xffff9a52) },
            { "winter",   // aurora night
              makePal (0xff070f2b, 0xff1b3f73, 0xff08121c, 0xff0a1526, 0xff213f63, 0xffeef8ff, 0xffa4bcd6,
                       0xff32577c, 0xff6a8fb8, 0xff6fd8ff, 0xff2a6fb0, 0xffe6f6ff, 0xff9fe8dc, 0xff2d6e7e, 0xff7affd4) },
            { "spring",   // blossom morning
              makePal (0xff1e4a3c, 0xff3f7f5c, 0xff0f241b, 0xff0e241b, 0xff2f6049, 0xfff6fff2, 0xffb0d6ba,
                       0xff437f5e, 0xff79a572, 0xffff8cc6, 0xffb8467f, 0xffffe58a, 0xff9be86a, 0xff3b8f4a, 0xffffc2e2) }
        }};
        return t;
    }

    inline juce::Font serif (float h, bool italic = false)
    {
        juce::Font f (juce::FontOptions().withName (juce::Font::getDefaultSerifFontName()).withHeight (h));
        if (italic) f.setItalic (true);
        return f;
    }

    // Title face: a light, rounded sans (first one installed wins), with a little extra letter spacing so the
    // name reads soft and airy. Falls back to the system sans in its light style.
    inline juce::Font titleFont (float h)
    {
        static const juce::String name = []
        {
            const auto all = juce::Font::findAllTypefaceNames();
            for (auto* c : { "Quicksand", "Nunito", "Varela Round", "Segoe UI", "Avenir Next", "Helvetica Neue" })
                if (all.contains (c)) return juce::String (c);
            return juce::Font::getDefaultSansSerifFontName();
        }();
        return juce::Font (juce::FontOptions().withName (name).withStyle ("Light").withHeight (h).withKerningFactor (0.14f));
    }

    // Space kept around a rotary knob inside its slider bounds (the arc glow has to fit in it)
    inline constexpr float knobPad = 9.f;

    // A leaf pointing up from the origin
    inline juce::Path leafPath (float len, float wid)
    {
        juce::Path p;
        p.startNewSubPath (0.f, 0.f);
        p.quadraticTo (wid * 1.6f, -len * 0.5f, 0.f, -len);
        p.quadraticTo (-wid * 1.6f, -len * 0.5f, 0.f, 0.f);
        p.closeSubPath();
        return p;
    }
}

struct ScattrLNF : public juce::LookAndFeel_V4
{
    ScattrLNF();
    void setPalette (const Scat::Palette&);
    Scat::Palette pal;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Scat::serif (14.f); }
    juce::Font getLabelFont (juce::Label&) override { return Scat::serif (13.f); }
};

// ---------------------------------------------------------------------------------------------
// The whole UI lives in this panel, drawn at a fixed 780 x 570 design size.
// The editor window scales it with a transform and resizes itself to match, so the zoom level is
// owned by us (and survives closing / reopening the window) instead of being fought over with the host.
// ---------------------------------------------------------------------------------------------
class ScattrPanel : public juce::Component,
                    public juce::FileDragAndDropTarget,
                    private juce::Timer
{
public:
    static constexpr int baseW = 780, baseH = 570;

    explicit ScattrPanel (ScattrProcessor&);
    ~ScattrPanel() override;

    std::function<void (float)> onScaleChosen;   // editor resizes itself when the user picks a zoom

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void applyTheme (int idx, bool animate = true);
    void showSampleMenu (int idx);   // right-click menu on a sample: mute / remove
    void setTab (int t);

    // static layer: sky, atmosphere and card shells. Drawn once per theme / tab into `cache`, then blitted every frame.
    void rebuildCache();
    void paintStatic (juce::Graphics&);
    void fillSky (juce::Graphics&);
    void paintAtmosphere (juce::Graphics&);
    void paintTitleGlow (juce::Graphics&);
    void drawGlass (juce::Graphics&, juce::Rectangle<float>, float radius, float tint, bool inset = false);
    void drawGroup (juce::Graphics&, juce::Rectangle<float>);
    void drawGroupTitle (juce::Graphics&, juce::Rectangle<float>, const juce::String&);

    // live layer
    void paintMotes (juce::Graphics&);       // slow themed light particles drifting over the whole window
    void paintGlints (juce::Graphics&);      // a highlight that sweeps along the top edge of each card
    void paintLabels (juce::Graphics&);
    void paintEq (juce::Graphics&);
    void paintWave (juce::Graphics&);
    void paintSamples (juce::Graphics&);
    void paintIdleDrift (juce::Graphics&, juce::Rectangle<float> area);   // themed floaters while nothing is playing
    void paintKnobGlows (juce::Graphics&);   // soft bloom behind every knob; grows with its value
    void paintToggleGlows (juce::Graphics&); // glow behind lit on/off buttons (animated)
    void paintTip (juce::Graphics&);
    void drawVine (juce::Graphics&);

    void updateEq();
    int hoveredControl() const;              // index into the tip table, or -1

    // layout (design units)
    juce::Rectangle<float> waveRect() const   { return { 18.f, 76.f, 480.f, 142.f }; }
    juce::Rectangle<float> sampleRect() const { return { 510.f, 76.f, 252.f, 142.f }; }
    juce::Rectangle<float> grainPanel() const  { return { 18.f, 246.f, 744.f, 130.f }; }
    juce::Rectangle<float> motionPanel() const { return { 18.f, 384.f, 744.f, 130.f }; }
    juce::Rectangle<float> fxPanel (int k) const { return { 18.f, 80.f + (float) k * 144.f, 744.f, 136.f }; }
    juce::Rectangle<float> waveInner() const  { return waveRect().reduced (10.f, 0.f); }
    juce::Rectangle<float> eqRect() const     { return { 186.f, 20.f, 94.f, 42.f }; }   // sits right beside the title
    int hitHandle (float x) const;   // 0 = none, 1 = start bar, 2 = end bar (for the shown sample)

    static constexpr int rowH = 20, maxRows = 5, listTop = 34;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    static bool onGrainsTab (size_t knobIndex) { return knobIndex <= 7 || (knobIndex >= 17 && knobIndex <= 19) || knobIndex == 21; }

    ScattrProcessor& proc;
    ScattrLNF lnf;
    Scat::Palette pal;                     // what is drawn right now (blends fromPal -> targetPal during a transition)
    Scat::Palette fromPal, targetPal;
    float themeT = 1.f;                    // 0..1 progress of the theme transition (1 = settled)
    juce::Image prevCache;                 // the old theme's static layer, faded out during the transition
    static constexpr float themeFadeSec = 0.7f;
    int curTheme = 0, tab = 0;
    std::array<Knob, 22> knobs;
    std::array<juce::TextButton, 2> tabBtn;
    juce::TextButton loadBtn { "PLANT" }, clearBtn { "CLEAR" };
    juce::TextButton typeBtn { "LP" };
    juce::TextButton spaceBtn { "HALL" };
    juce::ToggleButton fonBtn { "ON" }, donBtn { "ON" }, ronBtn { "ON" };   // per-effect bypass
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fonAttach, donAttach, ronAttach;
    juce::TextButton scaleBtn { "100%" };
    juce::TextButton themeBtn { "summer" };
    juce::ToggleButton droneBtn { "DRIFT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> droneAttach;
    juce::ToggleButton tuneBtn { "TUNE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tuneAttach;
    std::unique_ptr<juce::FileChooser> chooser;

    // cached static layer (2x design size so it stays sharp at 200 %)
    juce::Image cache;
    int cacheTheme = -1, cacheTab = -1;
    static constexpr float cacheScale = 2.f;

    // header equalizer
    static constexpr int eqBands = 14, eqN = 2048;
    juce::dsp::FFT fft { 11 };   // 2^11 = eqN
    juce::dsp::WindowingFunction<float> window { (size_t) eqN, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, (size_t) eqN * 2> fftData {};
    std::array<float, (size_t) eqBands> eqLevel {}, eqPeak {};
    float eqRef = -32.f;
    int lastScopeW = -1, scopeStale = 0;

    // hover tips: a note appears after the pointer rests on a control for a moment
    int hoverId = -1, tipId = -1;
    double hoverSince = 0.0, tipDelayMs = 500.0, lastTickMs = 0.0;
    float tipAlpha = 0.f;

    struct Tick { float pos, y, alpha; };
    std::vector<Tick> ticks;
    float lastGrain = -1.f, progress = 0.f, anim = 0.f, idleFade = 1.f;
    int shown = -1, listOff = 0;
    int pinned = -1;        // sample chosen for trimming (-1 = waveform follows playback)
    int playIdx = -1;       // sample the engine is playing right now
    int dragHandle = 0;     // 0 none, 1 start bar, 2 end bar
    bool playing = false, shownPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScattrPanel)
};

// Thin plugin window: just hosts the panel at the saved zoom.
class ScattrEditor : public juce::AudioProcessorEditor
{
public:
    explicit ScattrEditor (ScattrProcessor&);
    void resized() override;

private:
    void applyScale (float s);

    ScattrProcessor& proc;
    ScattrPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScattrEditor)
};
