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
    // (The second softening pass: 0.55 / 0.58 / 0.55 before; the skies are also a touch lighter now,
    //  so the window feels airy rather than heavy.)
    inline constexpr float calmBg = 0.40f, calmAccent = 0.46f, calmText = 0.38f;

    inline juce::Colour calm (juce::uint32 argb, float sat, float bright = 1.f)
    {
        const juce::Colour c (argb);
        return juce::Colour::fromHSV (c.getHue(), juce::jlimit (0.f, 1.f, c.getSaturation() * sat),
                                      juce::jlimit (0.f, 1.f, c.getBrightness() * bright), 1.f);
    }

    inline Palette makePal (juce::uint32 bgTop, juce::uint32 bgMid, juce::uint32 bgBot, juce::uint32 soil,
                            juce::uint32 moss, juce::uint32 ink, juce::uint32 dim, juce::uint32 line,
                            juce::uint32 bark, juce::uint32 acc, juce::uint32 accDark, juce::uint32 amber,
                            juce::uint32 grass, juce::uint32 grassDark, juce::uint32 glow)
    {
        return { calm (bgTop, calmBg, 1.12f), calm (bgMid, calmBg, 1.12f), calm (bgBot, calmBg, 1.06f), calm (soil, calmBg),
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

    // The same palette with the colour taken out: every role keeps its brightness, so the layout still reads
    inline Palette grayPal (const Palette& p)
    {
        auto g = [] (juce::Colour c) { return juce::Colour::greyLevel (c.getPerceivedBrightness()).withAlpha (c.getAlpha()); };
        return { g (p.bgTop), g (p.bgMid), g (p.bgBot), g (p.soil), g (p.moss), g (p.ink), g (p.dim), g (p.line),
                 g (p.bark), g (p.acc), g (p.accDark), g (p.amber), g (p.grass), g (p.grassDark), g (p.glow) };
    }

    struct Theme { const char* name; Palette pal; };

    // sky top/mid/bottom, panel, hover, text, dim text, track, border, accent, accent dark, highlight, leaf, leaf dark, light
    // "light" is the colour of the glow that spills out of cards, the sun / aurora in the sky and the floating motes.
    inline const std::array<Theme, 10>& themes()
    {
        static const std::array<Theme, 10> t {{
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
                       0xff437f5e, 0xff79a572, 0xffff8cc6, 0xffb8467f, 0xffffe58a, 0xff9be86a, 0xff3b8f4a, 0xffffc2e2) },
            { "meadow",   // daisies under a high blue sky
              makePal (0xff2c6a80, 0xff5d9f78, 0xff16342a, 0xff14302a, 0xff3d7058, 0xfffffbea, 0xffc6dcc2,
                       0xff4f8a6b, 0xff7f9a62, 0xfff2c14e, 0xffb9831a, 0xffffe9a0, 0xffa8de7e, 0xff4a9a52, 0xffffeab0) },
            { "tide",     // sunlight sinking through shallow sea
              makePal (0xff0b3a58, 0xff1c7488, 0xff071f33, 0xff092838, 0xff1f5a70, 0xffeafcff, 0xffa4d2de,
                       0xff2d7a90, 0xffa89a78, 0xff62e2d2, 0xff1b8f98, 0xffffdcc4, 0xff6fd6a0, 0xff2a8a74, 0xffa4f2ff) },
            { "dune",     // dusk over warm sand
              makePal (0xff55395c, 0xffc47a4e, 0xff2a1710, 0xff2b1b15, 0xff6b3f2c, 0xfffff1de, 0xffe2bc9e,
                       0xff9a5a3c, 0xffb58250, 0xffff9d5e, 0xffb5471f, 0xffffd48e, 0xffa0c47c, 0xff62905c, 0xffffb680) },
            { "moonlit",  // fireflies in an indigo wood
              makePal (0xff0b1236, 0xff202f62, 0xff070b18, 0xff0c1326, 0xff273b66, 0xffeff1ff, 0xffaab6da,
                       0xff3b4e88, 0xff55639a, 0xffb4ee7c, 0xff4e8f2c, 0xfffff0b4, 0xff7bd6b0, 0xff2e7868, 0xffd6dcff) },
            { "rain",     // mist and ferns after a downpour
              makePal (0xff263d3e, 0xff437066, 0xff0e1a18, 0xff102220, 0xff32564c, 0xffeef8f1, 0xffaac8be,
                       0xff40706a, 0xff6c7c64, 0xff82e4bc, 0xff2f8f6c, 0xffd2eaa8, 0xff80d08c, 0xff2c7c4a, 0xffc0eae2) },
            { "lavender", // a violet field at dusk
              makePal (0xff3b2d66, 0xff7d5da4, 0xff1a1230, 0xff1e1734, 0xff4e3d7c, 0xfffaf4ff, 0xffd2c2e8,
                       0xff6c54a2, 0xff8c72b4, 0xffc99dff, 0xff7c46c4, 0xffffe2f2, 0xffa2d48c, 0xff4e8c5a, 0xffe8caff) }
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

    // The logo face: a heavy, bookish serif for the lowercase "nature" wordmark (first one installed wins).
    inline juce::Font logoFont (float h)
    {
        static const juce::String name = []
        {
            const auto all = juce::Font::findAllTypefaceNames();
            for (auto* c : { "Georgia", "Palatino Linotype", "Book Antiqua", "Cambria", "Palatino", "Times New Roman" })
                if (all.contains (c)) return juce::String (c);
            return juce::Font::getDefaultSerifFontName();
        }();
        return juce::Font (juce::FontOptions().withName (name).withStyle ("Bold").withHeight (h).withKerningFactor (0.01f));
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
    void setStyle (int themeIndex) { style = themeIndex; }   // which little decoration / pointer the knobs wear
    Scat::Palette pal;
    int style = 0;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Scat::serif (14.f); }
    juce::Font getLabelFont (juce::Label&) override { return Scat::serif (13.f); }
};

class SpectralOverlay;
class EqDisplay;

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
    bool keyPressed (const juce::KeyPress&) override;   // Ctrl+Z / Ctrl+Y undo and redo for the equalizer (FX tab)

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    friend class SpectralOverlay;
    friend class EqDisplay;

    void timerCallback() override;
    void openSpectral();             // snapshot + blur everything, then raise the spectral gate window
    void applyTheme (int idx, bool animate = true, bool force = false);
    bool animOn() const { return ! proc.setNoAnim.load(); }   // false when the user has switched animations off
    void showSampleMenu (int idx);   // right-click menu on a sample: mute / remove
    void setTab (int t);
    void restoreFocus();        // take the keyboard back after a native dialog / drop (see the .cpp)
    void restoreFocusSoon();

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
    // FX tab: the equalizer across the top, then two rows of effect cards whose widths follow their knob counts,
    // so every knob sits on the same spacing and nothing is left empty.
    //   row 1: phaser (5 knobs) | chorus (3)        row 2: delay (3) | reverb (3)
    juce::Rectangle<float> eqPanel() const { return { 18.f, 76.f, 744.f, 172.f }; }

    // Settings tab: one glass card holding four rows (a title and a note on the left, the control on the right)
    juce::Rectangle<float> settingsPanel() const { return { 18.f, 76.f, 744.f, 438.f }; }
    juce::Rectangle<float> settingsRow (int i) const   // five rows: master, tips, animations, themes, startup size
    {
        const auto sp = settingsPanel();
        return { sp.getX() + 16.f, sp.getY() + 34.f + 80.f * (float) i, sp.getWidth() - 32.f, 76.f };
    }
    juce::Rectangle<float> fxPanel (int k) const   // 0 phaser, 1 chorus, 2 delay, 3 reverb
    {
        constexpr float W = 744.f, pad = 24.f, gap = 8.f;
        if (k < 2)
        {
            const float u = (W - 2.f * pad - gap) / 8.f;
            const float w0 = 5.f * u + pad, w1 = 3.f * u + pad;
            return k == 0 ? juce::Rectangle<float> (18.f, 256.f, w0, 124.f)
                          : juce::Rectangle<float> (18.f + w0 + gap, 256.f, w1, 124.f);
        }
        const float w = (W - gap) / 2.f;
        return { 18.f + (float) (k - 2) * (w + gap), 388.f, w, 126.f };
    }
    juce::Rectangle<float> waveInner() const  { return waveRect().reduced (10.f, 0.f); }
    juce::Rectangle<float> eqRect() const     { return { 506.f, 20.f, 88.f, 42.f }; }   // top right, beside the theme / zoom buttons
    float titleX() const;     // the title and its sprout are centred as one group in the header
    float sproutX() const;
    int hitHandle (float x) const;   // 0 = none, 1 = start bar, 2 = end bar (for the shown sample)

    static constexpr int rowH = 20, maxRows = 5, listTop = 34;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    // knobs 0..7, 17..19 and 21 live on the grains tab; 11..16 and 22..29 (delay, reverb, phaser, chorus) on the FX tab.
    // 3 was the reverse knob and 8, 9, 10 and 20 the filter's: those are gone, but the slots stay so every other knob keeps its index.
    static bool retiredKnob (size_t i) { return i == 3 || i == 8 || i == 9 || i == 10 || i == 20; }
    static bool onGrainsTab (size_t knobIndex) { return knobIndex <= 7 || (knobIndex >= 17 && knobIndex <= 19) || knobIndex == 21; }

    ScattrProcessor& proc;
    ScattrLNF lnf;
    Scat::Palette pal;                     // what is drawn right now (blends fromPal -> targetPal during a transition)
    Scat::Palette fromPal, targetPal;
    float themeT = 1.f;                    // 0..1 progress of the theme transition (1 = settled)
    juce::Image prevCache;                 // the old theme's static layer, faded out during the transition
    static constexpr float themeFadeSec = 0.7f;
    int curTheme = 0, tab = 0;
    std::array<Knob, 30> knobs;
    std::array<juce::TextButton, 3> tabBtn;   // grains, fx, settings
    juce::TextButton loadBtn { "PLANT" }, clearBtn { "CLEAR" };
    juce::TextButton spaceBtn { "HALL" };
    juce::ToggleButton donBtn { "ON" }, ronBtn { "ON" }, ponBtn { "ON" }, conBtn { "ON" };   // per-effect bypass
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> donAttach, ronAttach, ponAttach, conAttach;
    std::unique_ptr<EqDisplay> eqView;               // the FX tab's equalizer (spectrum, curve and draggable bands)
    juce::TextButton scaleBtn { "100%" };
    std::array<juce::ToggleButton, 3> setBtn;       // settings: disable tips, disable animations, disable themes
    juce::TextButton startBtn { "LAST USED" };      // settings: the size the window opens at
    juce::TextButton themeBtn { "summer" };
    juce::ToggleButton droneBtn { "DRIFT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> droneAttach;
    juce::ToggleButton tuneBtn { "TUNE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tuneAttach;
    juce::ToggleButton specBtn { "SPECTRAL GATE" };   // lit while the gate is on; pressing it opens the pop-out window
    bool specRightDown = false;                       // the press on specBtn was a right-click (toggles the gate instead of opening the window)
    std::unique_ptr<SpectralOverlay> spectral;        // the pop-out window (hidden until the button is pressed)
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
    float playGlow = 0.f;   // 0..1: the soft light under the waveform while a sample sounds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScattrPanel)
};

// ---------------------------------------------------------------------------------------------
// The FX tab's equalizer. A glass plot with the live spectrum behind a combined response curve, and up to
// 24 draggable bands:
//   drag a node        moves its frequency and gain          scroll over a node   changes its width (Q)
//   double-click       adds a band here / removes the one clicked
//   right-click        on a node: its shape (bell, shelves, tilt, cuts, notch, band pass) and, for cuts, its slope
//                      on empty space: add a band here, or clear them all
// The curve is computed from the very same filter maths the audio runs (EqCore).
// ---------------------------------------------------------------------------------------------
class EqDisplay : public juce::Component
{
public:
    EqDisplay (ScattrPanel&, ScattrProcessor&);

    void update();   // ~30x a second while the FX tab is showing: refreshes the spectrum

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    juce::ToggleButton preBtn { "PRE" };   // also draw the spectrum going INTO the equalizer

    bool undo();   // step back through band edits (Ctrl+Z); false when there is nothing to undo
    bool redo();   // and forward again (Ctrl+Y / Ctrl+Shift+Z)

private:
    struct BandP { juce::RangedAudioParameter *on = nullptr, *type = nullptr, *freq = nullptr, *gain = nullptr, *q = nullptr, *slope = nullptr,
                   *dyn = nullptr, *thresh = nullptr, *range = nullptr, *speed = nullptr; };

    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().withTrimmedLeft (12.f).withTrimmedRight (36.f).withTrimmedTop (28.f).withTrimmedBottom (22.f); }
    float xOfHz (float hz) const;
    float hzOfX (float x) const;
    float yOfDb (float db) const;
    float dbOfY (float y) const;
    EqCore::Band readBand (int i) const;
    juce::Point<float> nodePos (int i) const;
    int  hitBand (juce::Point<float> p) const;
    void setNorm (juce::RangedAudioParameter* p, float plainValue);
    void beginGesture (int band);
    void beginDrag (juce::RangedAudioParameter* a, juce::RangedAudioParameter* b = nullptr);
    void endGesture();
    int  hitDynHandle (juce::Point<float> p, int band) const;   // 1 = the range handle, 2 = the threshold handle, 0 = neither
    float yOfThreshold (const EqCore::Band& b) const;
    static float analyzerTilt (float hz) { return 4.5f * std::log2 (hz / 1000.f); }   // the analyzer's own tilt, so the threshold lines up with the spectrum
    void addBandAt (juce::Point<float> p);
    void removeBand (int i);
    void showTypeMenu (int i);
    void showPlotMenu (juce::Point<float> p);

    // undo history: a snapshot of all 24 bands is taken before each edit and pushed if the edit changed anything
    struct Snap
    {
        std::array<EqCore::Band, EqCore::numBands> b;
        bool operator== (const Snap& o) const { return b == o.b; }
    };
    Snap snapshot() const;
    void applySnapshot (const Snap&);
    void record (const Snap& before);   // push `before` as an undo step if the bands are no longer the same
    struct Scope   // records one undo step covering everything done while it is alive
    {
        explicit Scope (EqDisplay& e) : d (e), before (e.snapshot()) {}
        ~Scope() { d.record (before); }
        EqDisplay& d;
        Snap before;
    };
    std::vector<Snap> undoStack, redoStack;
    Snap dragBefore;
    int lastWheelBand = -1;
    float slopeWheelAcc = 0.f;   // shift + scroll: collects wheel movement until it adds up to one slope step
    double lastWheelMs = 0.0;
    static juce::Colour bandColour (int i);
    void computeSpectrum (const std::atomic<float>* ring, std::vector<float>& out, std::vector<float>& smoothed, bool stale);

    ScattrPanel& panel;
    ScattrProcessor& proc;
    std::array<BandP, EqCore::numBands> bp;

    juce::dsp::FFT fft { 12 };   // 4096 points
    juce::dsp::WindowingFunction<float> window { 4096, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, 8192> fftData {};
    std::vector<float> specPost, specPre, rawPost, rawPre;   // one value per plot column, in dB (smoothed / raw)
    int lastTapW = -1, staleTicks = 0;

    int hover = -1, selected = -1;
    bool dragging = false;
    int dragKind = 0;   // what is being dragged: 0 a band's node, 1 its dynamic range handle, 2 its threshold handle
    juce::RangedAudioParameter *dragA = nullptr, *dragB = nullptr;
    juce::Point<float> hoverPt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqDisplay)
};

// ---------------------------------------------------------------------------------------------
// The spectral gate window. It covers the whole panel (so nothing behind it can be touched), paints a
// blurred, dimmed snapshot of the plugin as its backdrop, and floats one glass card on top.
// ---------------------------------------------------------------------------------------------
class SpectralOverlay : public juce::Component,
                        private juce::Timer
{
public:
    SpectralOverlay (ScattrPanel&, ScattrProcessor&);
    ~SpectralOverlay() override;

    void open (const juce::Image& snapshot);   // snapshot = the panel as it looks right now
    void close();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}   // never scroll the panel behind
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void buildLayers();                     // renders the backdrop and the card once per opening
    void paintCardStatic (juce::Graphics&); // card, header, group panels, analyzer shell and grid
    void paintAnalyzerShell (juce::Graphics&);   // glass, grid, labels, legend (static)
    void paintAnalyzerLive (juce::Graphics&);
    void paintKnobGlows (juce::Graphics&);
    SpectralGate::Params readParams() const;
    juce::Rectangle<float> plotRect() const { return analyzer().reduced (12.f, 12.f).withTrimmedBottom (34.f).withTrimmedLeft (18.f); }

    juce::Rectangle<float> card() const      { return { 14.f, 40.f, 752.f, 490.f }; }
    juce::Rectangle<float> group (int row) const   // "gate", "response" and "tilt & window"
    {
        static const float y[3] = { 98.f, 254.f, 418.f }, h[3] = { 148.f, 156.f, 86.f };
        return { 26.f, y[row], 400.f, h[row] };
    }
    juce::Rectangle<float> analyzer() const  { return { 438.f, 98.f, 316.f, 406.f }; }

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    static constexpr int numKnobs = 7;   // cutoff, weak/strong balance, tilt | attack, hold, release, smooth

    ScattrPanel& panel;
    ScattrProcessor& proc;

    std::array<Knob, (size_t) numKnobs> knobs;
    juce::ToggleButton onBtn { "ON" }, tiltBtn { "ENABLE TILT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttach, tiltAttach;
    juce::TextButton winBtn { "2048" }, doneBtn { "DONE" };   // winBtn cycles the FFT size

    juce::Image sharp, backdrop, cardImg;   // live snapshot (only used while fading in), then two cached layers
    static constexpr float layerScale = 2.f;
    float progress = 0.f;      // 0..1 open animation
    bool closing = false;
    float anim = 0.f;
    double lastMs = 0.0;

    std::array<float, (size_t) SpectralGate::numBands> shIn {}, shOut {};
    unsigned lastFrames = 0;
    int stale = 0;
    bool live = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralOverlay)
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
