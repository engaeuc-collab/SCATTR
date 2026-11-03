#pragma once
#include "PluginProcessor.h"

namespace Scat
{
    // dusk-meadow palette: indigo sky, plum clouds, magenta blooms, sun gold, grass greens
    inline const juce::Colour bgTop     { 0xff171a3b };
    inline const juce::Colour bgMid     { 0xff33224a };
    inline const juce::Colour bgBot     { 0xff0f1a15 };
    inline const juce::Colour soil      { 0xff130f20 };
    inline const juce::Colour moss      { 0xff382850 };
    inline const juce::Colour ink       { 0xfff6e8de };
    inline const juce::Colour dim       { 0xffb3a2c2 };
    inline const juce::Colour line      { 0xff4d3f6b };
    inline const juce::Colour bark      { 0xff86566e };
    inline const juce::Colour acc       { 0xffee62c6 };
    inline const juce::Colour accDark   { 0xff7b3a9e };
    inline const juce::Colour amber     { 0xffffb347 };
    inline const juce::Colour grass     { 0xff7bbf5e };
    inline const juce::Colour grassDark { 0xff2f6b42 };

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
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Scat::serif (14.f); }
    juce::Font getLabelFont (juce::Label&) override { return Scat::serif (13.f); }
};

class ScattrEditor : public juce::AudioProcessorEditor,
                     public juce::FileDragAndDropTarget,
                     private juce::Timer
{
public:
    explicit ScattrEditor (ScattrProcessor&);
    ~ScattrEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void drawVine (juce::Graphics&);
    void applyScale (float newScale);
    juce::Rectangle<float> waveRect() const { return { 18.f, 72.f, (float) getWidth() - 36.f, 170.f }; }

    static constexpr int listY = 268, rowH = 18, maxRows = 6;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    ScattrProcessor& proc;
    ScattrLNF lnf;
    std::array<Knob, 18> knobs;
    juce::TextButton loadBtn { "PLANT" }, clearBtn { "CLEAR" };
    juce::TextButton typeBtn { "LP" };
    juce::TextButton scaleBtn { "100%" };
    juce::ToggleButton droneBtn { "DRIFT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> droneAttach;
    juce::ToggleButton tuneBtn { "TUNE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tuneAttach;
    std::unique_ptr<juce::FileChooser> chooser;

    struct Tick { float pos, y, alpha; };
    std::vector<Tick> ticks;
    float lastGrain = -1.f, progress = 0.f, anim = 0.f;
    int shown = -1, listOff = 0;
    bool playing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScattrEditor)
};
