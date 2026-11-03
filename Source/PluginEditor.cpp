#include "PluginEditor.h"

using namespace Scat;

namespace
{
void drawLeaf (juce::Graphics& g, float x, float y, float angle, float size, juce::Colour c)
{
    const auto t = juce::AffineTransform::rotation (angle).translated (x, y);
    g.setColour (c);
    g.fillPath (leafPath (size, size * 0.38f), t);

    juce::Path rib;
    rib.startNewSubPath (0.f, 0.f);
    rib.lineTo (0.f, -size * 0.85f);
    g.setColour (c.darker (0.6f));
    g.strokePath (rib, juce::PathStrokeType (1.f), t);
}

void drawFlower (juce::Graphics& g, float x, float y, float r, juce::Colour petal)
{
    g.setColour (petal);
    for (int k = 0; k < 5; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::twoPi / 5.f;
        g.fillEllipse (x + std::sin (a) * r * 0.6f - r * 0.4f, y - std::cos (a) * r * 0.6f - r * 0.4f, r * 0.8f, r * 0.8f);
    }
    g.setColour (amber);
    g.fillEllipse (x - r * 0.25f, y - r * 0.25f, r * 0.5f, r * 0.5f);
}
}

// ---------------------------------------------------------------- look and feel

ScattrLNF::ScattrLNF()
{
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, dim);
}

void ScattrLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float a0, float a1, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (7.f);
    const auto c = r.getCentre();
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const float a = a0 + pos * (a1 - a0);

    // wooden stump
    g.setColour (soil);
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f);
    g.setColour (bark);
    g.drawEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f, 2.f);
    g.setColour (bark.withAlpha (0.35f));
    g.drawEllipse (c.x - rad * 0.55f, c.y - rad * 0.55f, rad * 1.1f, rad * 1.1f, 1.f);

    // growth arc
    const float ar = rad + 4.f;
    juce::Path track, val;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a1, true);
    val.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a, true);
    g.setColour (line);
    g.strokePath (track, juce::PathStrokeType (2.5f));
    g.setColour (acc);
    g.strokePath (val, juce::PathStrokeType (3.f));

    // leaf pointer
    const auto t = juce::AffineTransform::translation (0.f, -rad * 0.2f)
                       .followedBy (juce::AffineTransform::rotation (a))
                       .followedBy (juce::AffineTransform::translation (c.x, c.y));
    g.setColour (amber);
    g.fillPath (leafPath (rad * 0.75f, rad * 0.26f), t);
}

void ScattrLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? moss.brighter (0.2f) : over ? moss : soil);
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (over ? acc : bark);
    g.drawRoundedRectangle (r, 8.f, 1.3f);
}

void ScattrLNF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    if (b.getToggleState())
    {
        g.setColour (accDark);
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (ink);
    }
    else
    {
        g.setColour (soil);
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (bark);
        g.drawRoundedRectangle (r, 8.f, 1.3f);
        g.setColour (dim);
    }
    g.setFont (serif (14.f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

// ---------------------------------------------------------------- editor

ScattrEditor::ScattrEditor (ScattrProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setSize (780, 650);

    struct Def { const char* id; const char* text; const char* suffix; int dp; };
    static const Def defs[17] = {
        { "size", "seed size", " ms", 0 }, { "dens", "rain", " /s", 0 }, { "pitch", "pitch", " st", 1 },
        { "pjit", "wind", " st", 1 }, { "spray", "scatter", "", 2 }, { "hold", "bloom", " s", 1 },
        { "gap", "rest", " s", 2 }, { "amp", "sun", "", 2 },
        { "cutoff", "cutoff", " Hz", 0 }, { "reso", "reso", "", 2 }, { "drive", "drive", "", 2 },
        { "dtime", "time", " ms", 0 }, { "dfb", "feedback", "", 2 }, { "dmix", "echo mix", "", 2 },
        { "rsize", "size", "", 2 }, { "rdamp", "damp", "", 2 }, { "rmix", "reverb mix", "", 2 }
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
        k.slider.setNumDecimalPlacesToDisplay (defs[i].dp);
        k.slider.setTextValueSuffix (defs[i].suffix);
        addAndMakeVisible (k.slider);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, defs[i].id, k.slider);

        k.label.setText (defs[i].text, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (serif (14.f, true));
        addAndMakeVisible (k.label);
    }

    addAndMakeVisible (typeBtn);
    typeBtn.onClick = [this]
    {
        auto* par = proc.apvts.getParameter ("ftype");
        const int cur = (int) proc.apvts.getRawParameterValue ("ftype")->load();
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) ((cur + 1) % 3)));
        par->endChangeGesture();
    };

    addAndMakeVisible (loadBtn);
    addAndMakeVisible (clearBtn);
    addAndMakeVisible (droneBtn);
    droneAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "drone", droneBtn);

    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Select samples", juce::File(),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectMultipleItems,
                              [this] (const juce::FileChooser& fc)
                              {
                                  for (auto& f : fc.getResults()) proc.addSampleFile (f);
                                  repaint();
                              });
    };
    clearBtn.onClick = [this] { proc.clearSamples(); shown = -1; listOff = 0; repaint(); };

    startTimerHz (30);
}

ScattrEditor::~ScattrEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ScattrEditor::resized()
{
    const int w = getWidth();
    droneBtn.setBounds (w - 18 - 80, 26, 80, 26);
    clearBtn.setBounds (w - 18 - 80 - 8 - 80, 26, 80, 26);
    loadBtn.setBounds  (w - 18 - 80 - 8 - 80 - 8 - 80, 26, 80, 26);

    const int cw1 = (w - 36) / 8, cw2 = (w - 36) / 10;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (i < 8)
        {
            const int x = 18 + (int) i * cw1;
            knobs[i].label.setBounds (x, 414, cw1, 18);
            knobs[i].slider.setBounds (x, 430, cw1, 82);
        }
        else
        {
            const int j = (int) i - 8, col = j < 3 ? j : j + 1;
            const int x = 18 + col * cw2;
            knobs[i].label.setBounds (x, 534, cw2, 18);
            knobs[i].slider.setBounds (x, 550, cw2, 84);
        }
    }
    typeBtn.setBounds (18 + 3 * cw2 + 10, 550 + 28, cw2 - 20, 26);
}

void ScattrEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files) proc.addSampleFile (juce::File (f));
    repaint();
}

void ScattrEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.y >= listY && e.y < listY + rowH * maxRows)
    {
        const int idx = listOff + (e.y - listY) / rowH;
        if (idx >= 0 && idx < (int) proc.samples.size())
        {
            auto& en = proc.samples[(size_t) idx]->enabled;
            en = ! en.load();
            repaint();
        }
    }
}

void ScattrEditor::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int maxOff = juce::jmax (0, (int) proc.samples.size() - maxRows);
    listOff = juce::jlimit (0, maxOff, listOff + (w.deltaY > 0 ? -1 : 1));
    repaint();
}

void ScattrEditor::timerCallback()
{
    anim += 0.033f;

    static const char* ft[3] = { "LP", "HP", "BP" };
    const int fi = juce::jlimit (0, 2, (int) proc.apvts.getRawParameterValue ("ftype")->load());
    if (typeBtn.getButtonText() != ft[fi]) typeBtn.setButtonText (ft[fi]);

    const int c = proc.uiSample.load();
    if (c >= 0) shown = c;
    if (shown >= (int) proc.samples.size()) shown = -1;
    if (shown < 0 && ! proc.samples.empty()) shown = (int) proc.samples.size() - 1;

    playing  = proc.uiPlaying.load();
    progress = proc.uiProgress.load();

    const float gp = proc.uiGrain.load();
    if (playing && gp != lastGrain)
    {
        lastGrain = gp;
        ticks.push_back ({ gp, juce::Random::getSystemRandom().nextFloat(), 1.f });
    }
    for (auto& t : ticks) t.alpha -= 0.04f;
    ticks.erase (std::remove_if (ticks.begin(), ticks.end(), [] (const Tick& t) { return t.alpha <= 0.f; }), ticks.end());

    repaint();
}

void ScattrEditor::drawVine (juce::Graphics& g)
{
    const float y0 = 386.f;
    auto yAt = [&] (float x) { return y0 + 4.f * std::sin (x * 0.02f + anim * 0.4f); };

    juce::Path stem;
    stem.startNewSubPath (18.f, yAt (18.f));
    for (float x = 18.f; x <= (float) getWidth() - 18.f; x += 6.f) stem.lineTo (x, yAt (x));
    g.setColour (grassDark);
    g.strokePath (stem, juce::PathStrokeType (1.8f));

    int i = 0;
    for (float x = 40.f; x < (float) getWidth() - 30.f; x += 44.f, ++i)
    {
        const float sway = 0.12f * std::sin (anim * 0.8f + x * 0.05f);
        const bool up = (i % 2 == 0);
        const float ang = (up ? 0.9f : juce::MathConstants<float>::pi - 0.9f) + sway;
        const auto col = (i % 3 == 0 ? grass.darker (0.2f) : grassDark.brighter (0.15f)).withAlpha (0.85f);
        drawLeaf (g, x, yAt (x), ang, 16.f + (float) (i % 3) * 3.f, col);

        if (i % 3 == 1)   // a little wildflower on a stalk
        {
            const float fx = x + 22.f, fy = yAt (fx);
            const float tx = fx + sway * 30.f, ty = fy - 16.f;
            g.setColour (grassDark);
            g.drawLine (fx, fy, tx, ty, 1.3f);
            drawFlower (g, tx, ty, 5.f, (i % 2 == 0 ? acc : acc.brighter (0.25f)));
        }
    }
}

void ScattrEditor::paint (juce::Graphics& g)
{
    {
        juce::ColourGradient sky (bgTop, 0.f, 0.f, bgBot, 0.f, (float) getHeight(), false);
        sky.addColour (0.30, bgMid);
        g.setGradientFill (sky);
        g.fillAll();

        const float gx = (float) getWidth() * 0.82f;
        g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.12f), gx, 40.f, amber.withAlpha (0.f), gx, 320.f, true));
        g.fillRect (0, 0, getWidth(), 330);
    }

    // title + little sprout
    g.setColour (ink);
    g.setFont (serif (38.f));
    g.drawText ("S C A T T R", 18, 14, 300, 50, juce::Justification::centredLeft);
    const float sw = 0.08f * std::sin (anim * 0.9f);
    drawLeaf (g, 318.f, 52.f, 0.7f + sw, 20.f, grassDark.brighter (0.2f));
    drawLeaf (g, 318.f, 52.f, -0.5f + sw, 16.f, grass.darker (0.3f));
    drawLeaf (g, 318.f, 52.f, 0.1f + sw, 26.f, grass);
    drawFlower (g, 318.f + std::sin (0.1f + sw) * 28.f, 52.f - 28.f, 5.5f, acc);

    // forest floor / waveform
    auto wr = waveRect();
    g.setColour (soil);
    g.fillRoundedRectangle (wr, 10.f);
    g.setColour (bark);
    g.drawRoundedRectangle (wr, 10.f, 1.5f);

    if (proc.samples.empty())
    {
        g.setColour (dim);
        g.setFont (serif (16.f, true));
        g.drawText ("plant some samples: drop audio files here", wr, juce::Justification::centred);
    }
    else if (shown >= 0 && shown < (int) proc.samples.size())
    {
        const auto& pk = proc.samples[(size_t) shown]->peaks;
        const int n = (int) pk.size();
        const auto inner = wr.reduced (10.f, 0.f);
        const float cy = wr.getCentreY(), hh = wr.getHeight() * 0.44f;

        const float al = playing ? 1.f : 0.45f;
        g.setGradientFill (juce::ColourGradient (acc.withAlpha (al), 0.f, wr.getY(),
                                                 accDark.withAlpha (al), 0.f, wr.getBottom(), false));
        for (int i = 0; i < n; ++i)
        {
            const float x = inner.getX() + inner.getWidth() * (float) i / (float) n;
            g.fillRect (x, cy - pk[(size_t) i].second * hh, inner.getWidth() / (float) n + 0.5f,
                        juce::jmax (1.5f, (pk[(size_t) i].second - pk[(size_t) i].first) * hh));
        }

        if (playing)
        {
            // fireflies for each new grain
            for (auto& t : ticks)
            {
                const float fx = inner.getX() + t.pos * inner.getWidth();
                const float fy = wr.getY() + 14.f + t.y * (wr.getHeight() - 28.f)
                                 + 3.f * std::sin (anim * 3.f + t.pos * 20.f);
                const float r = 4.f + 8.f * t.alpha;
                g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.9f * t.alpha), fx, fy,
                                                         amber.withAlpha (0.f), fx + r, fy, true));
                g.fillEllipse (fx - r, fy - r, r * 2.f, r * 2.f);
                g.setColour (juce::Colours::white.withAlpha (t.alpha));
                g.fillEllipse (fx - 1.5f, fy - 1.5f, 3.f, 3.f);
            }

            const float px = inner.getX() + progress * inner.getWidth();
            g.setColour (amber.withAlpha (0.8f));
            g.drawLine (px, wr.getY() + 6.f, px, wr.getBottom() - 6.f, 1.2f);
        }
    }

    // status
    g.setFont (serif (14.f, true));
    g.setColour (dim);
    juce::String st = proc.samples.empty() ? "the grove is empty"
                    : playing && shown >= 0 ? "blooming: " + proc.samples[(size_t) shown]->name
                                            : "resting...";
    g.drawText (st, 18, 246, getWidth() - 36, 18, juce::Justification::centredLeft);

    // sample list
    g.setFont (serif (14.f));
    const int count = (int) proc.samples.size();
    for (int r = 0; r < maxRows && listOff + r < count; ++r)
    {
        const int i = listOff + r;
        const auto& s = *proc.samples[(size_t) i];
        const bool on = s.enabled.load();
        const auto row = juce::Rectangle<int> (18, listY + r * rowH, getWidth() - 36, rowH);

        const bool isCur = playing && i == shown;
        if (isCur)
        {
            g.setColour (moss);
            g.fillRoundedRectangle (row.toFloat(), 6.f);
        }
        drawLeaf (g, (float) row.getX() + 12.f, (float) row.getCentreY() + 5.f, 0.9f, 11.f,
                  on ? (isCur ? grass : grassDark.brighter (0.2f)) : bark.withAlpha (0.6f));

        g.setColour (isCur ? ink : on ? dim.brighter (0.3f) : dim.withAlpha (0.5f));
        g.drawText (s.name + (on ? "" : "  (dormant)"), row.withTrimmedLeft (28).withTrimmedRight (60),
                    juce::Justification::centredLeft, true);
        g.drawText (juce::String (s.length / s.sampleRate, 1) + " s", row.reduced (8, 0),
                    juce::Justification::centredRight);
    }

    drawVine (g);

    // section headings
    auto head = [&g] (float x, float y, float w, const juce::String& t)
    {
        g.setColour (dim);
        g.setFont (serif (13.f, true));
        g.drawText (t, (int) x, (int) y, (int) w, 16, juce::Justification::centredLeft);
    };
    const float cw2 = ((float) getWidth() - 36.f) / 10.f;
    head (18.f, 396.f, 300.f, "grove: grains");
    head (18.f, 516.f, cw2 * 4.f, "canopy: filter");
    head (18.f + cw2 * 4.f, 516.f, cw2 * 3.f, "canyon: delay");
    head (18.f + cw2 * 7.f, 516.f, cw2 * 3.f, "cavern: reverb");
}
