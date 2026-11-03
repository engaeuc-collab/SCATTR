#pragma once
#include "PluginProcessor.h"

namespace Scat
{
    struct Palette
    {
        juce::Colour bgTop, bgMid, bgBot, soil, moss, ink, dim, line, bark, acc, accDark, amber, grass, grassDark;
    };

    inline Palette makePal (juce::uint32 bgTop, juce::uint32 bgMid, juce::uint32 bgBot, juce::uint32 soil,
                            juce::uint32 moss, juce::uint32 ink, juce::uint32 dim, juce::uint32 line,
                            juce::uint32 bark, juce::uint32 acc, juce::uint32 accDark, juce::uint32 amber,
                            juce::uint32 grass, juce::uint32 grassDark)
    {
        return { juce::Colour (bgTop), juce::Colour (bgMid), juce::Colour (bgBot), juce::Colour (soil),
                 juce::Colour (moss), juce::Colour (ink), juce::Colour (dim), juce::Colour (line),
                 juce::Colour (bark), juce::Colour (acc), juce::Colour (accDark), juce::Colour (amber),
                 juce::Colour (grass), juce::Colour (grassDark) };
    }

    struct Theme { const char* name; Palette pal; };

    // sky top/mid/bottom, panel, hover, text, dim text, track, border, accent, accent dark, highlight, leaf, leaf dark
    inline const std::array<Theme, 4>& themes()
    {
        static const std::array<Theme, 4> t {{
            { "summer",
              makePal (0xff0f3b57, 0xff1b6a7a, 0xff0f2a1f, 0xff0b2a35, 0xff1f5663, 0xfffff6e0, 0xff9ccbd0,
                       0xff2f7482, 0xff3f8a8f, 0xffff7a45, 0xffc2410c, 0xffffd23f, 0xff8fd14f, 0xff2e8540) },
            { "autumn",
              makePal (0xff2b1710, 0xff4a2314, 0xff1a120c, 0xff1c110b, 0xff4a3020, 0xfff8e6cf, 0xffc9a98a,
                       0xff6b4630, 0xff9a5b34, 0xffe8641c, 0xff8f2d0f, 0xffffb02e, 0xffd08a2a, 0xff7d3a1a) },
            { "winter",
              makePal (0xff0b1d33, 0xff1c3a5e, 0xff0b141c, 0xff0a1624, 0xff23405f, 0xffeaf6ff, 0xff9fb9d0,
                       0xff345676, 0xff5f87ad, 0xff6fd3ff, 0xff2a6fb0, 0xffd8f1ff, 0xff9fe0d6, 0xff2d6b78) },
            { "spring",
              makePal (0xff1b3a2b, 0xff2f5e4a, 0xff10211a, 0xff0f2219, 0xff2f5a43, 0xfff3fff0, 0xffa9cdb4,
                       0xff3f7a58, 0xff6f9a6a, 0xffff8fc7, 0xffb8467f, 0xffffe27a, 0xff8fe06a, 0xff3a8a48) }
        }};
        return t;
    }

    inline juce::Font serif (float h, bool italic = false)
    {
        juce::Font f (juce::FontOptions().withName (juce::Font::getDefaultSerifFontName()).withHeight (h));
        if (italic) f.setItalic (true);
        return f;
    }

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
    void applyTheme (int idx);
    void showSampleMenu (int idx);   // right-click menu on a sample: mute / remove
    void setTab (int t);

    void paintGrainsTab (juce::Graphics&);
    void paintFxTab (juce::Graphics&);
    void paintWave (juce::Graphics&);
    void paintSamples (juce::Graphics&);
    void paintBackdrop (juce::Graphics&);
    void fillSky (juce::Graphics&);
    void paintIdleDrift (juce::Graphics&, juce::Rectangle<float> area);   // themed floaters while nothing is playing
    void drawGlass (juce::Graphics&, juce::Rectangle<float>, float radius, float tint, bool inset = false);
    void drawGroup (juce::Graphics&, juce::Rectangle<float>, const juce::String&);
    void drawVine (juce::Graphics&);

    // layout (design units)
    juce::Rectangle<float> waveRect() const   { return { 18.f, 76.f, 480.f, 142.f }; }
    juce::Rectangle<float> sampleRect() const { return { 510.f, 76.f, 252.f, 142.f }; }
    juce::Rectangle<float> grainPanel() const  { return { 18.f, 246.f, 744.f, 130.f }; }
    juce::Rectangle<float> motionPanel() const { return { 18.f, 384.f, 744.f, 130.f }; }
    juce::Rectangle<float> fxPanel (int k) const { return { 18.f, 80.f + (float) k * 144.f, 744.f, 136.f }; }
    juce::Rectangle<float> waveInner() const  { return waveRect().reduced (10.f, 0.f); }
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
    Scat::Palette pal;
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
