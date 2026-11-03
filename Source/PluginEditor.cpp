#include "PluginEditor.h"
#include "ArtData.h"   // embedded theme backdrops (Backgrounds/ folder)

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

void drawFlower (juce::Graphics& g, float x, float y, float r, juce::Colour petal, juce::Colour centre)
{
    g.setColour (petal);
    for (int k = 0; k < 5; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::twoPi / 5.f;
        g.fillEllipse (x + std::sin (a) * r * 0.6f - r * 0.4f, y - std::cos (a) * r * 0.6f - r * 0.4f, r * 0.8f, r * 0.8f);
    }
    g.setColour (centre);
    g.fillEllipse (x - r * 0.25f, y - r * 0.25f, r * 0.5f, r * 0.5f);
}

const char* const kNoteNames[11] = { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2", "1/1" };

void drawMaple (juce::Graphics& g, float x, float y, float rot, float r, juce::Colour c)
{
    juce::Path p;
    for (int i = 0; i < 10; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::pi / 5.f;
        const float rr = (i % 2 == 0) ? r : r * 0.45f;
        const float px = std::sin (a) * rr, py = -std::cos (a) * rr;
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    p.closeSubPath();
    const auto t = juce::AffineTransform::rotation (rot).translated (x, y);
    g.setColour (c);
    g.fillPath (p.createPathWithRoundedCorners (1.5f), t);

    juce::Path stem;
    stem.startNewSubPath (0.f, 0.f);
    stem.lineTo (0.f, r * 0.95f);
    g.setColour (c.darker (0.5f));
    g.strokePath (stem, juce::PathStrokeType (1.f), t);
}

void drawSunflower (juce::Graphics& g, float x, float y, float r, juce::Colour petal, juce::Colour centre)
{
    g.setColour (petal);
    for (int k = 0; k < 12; ++k)
    {
        const auto t = juce::AffineTransform::rotation ((float) k * juce::MathConstants<float>::twoPi / 12.f).translated (x, y);
        juce::Path p;
        p.addEllipse (-r * 0.17f, -r, r * 0.34f, r * 0.62f);
        g.fillPath (p, t);
    }
    g.setColour (centre);
    g.fillEllipse (x - r * 0.4f, y - r * 0.4f, r * 0.8f, r * 0.8f);
    g.setColour (centre.brighter (0.5f).withAlpha (0.6f));
    g.fillEllipse (x - r * 0.15f, y - r * 0.15f, r * 0.3f, r * 0.3f);
}

void drawSnowflake (juce::Graphics& g, float x, float y, float r, juce::Colour c, float alpha)
{
    g.setColour (c.withAlpha (alpha));
    for (int k = 0; k < 6; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::pi / 3.f;
        g.drawLine (x, y, x + std::sin (a) * r, y - std::cos (a) * r, 1.f);
        const float mx = x + std::sin (a) * r * 0.6f, my = y - std::cos (a) * r * 0.6f;
        for (int s = -1; s <= 1; s += 2)
        {
            const float b = a + (float) s * 0.7f;
            g.drawLine (mx, my, mx + std::sin (b) * r * 0.35f, my - std::cos (b) * r * 0.35f, 1.f);
        }
    }
}

void drawBlossom (juce::Graphics& g, float x, float y, float r, juce::Colour petal, juce::Colour centre, juce::Colour vein)
{
    for (int k = 0; k < 5; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::twoPi / 5.f;
        const float px = x + std::sin (a) * r * 0.62f, py = y - std::cos (a) * r * 0.62f;
        g.setColour (petal);
        g.fillEllipse (px - r * 0.42f, py - r * 0.42f, r * 0.84f, r * 0.84f);
        g.setColour (vein.withAlpha (0.55f));
        g.drawLine (x + std::sin (a) * r * 0.15f, y - std::cos (a) * r * 0.15f,
                    x + std::sin (a) * r * 0.5f,  y - std::cos (a) * r * 0.5f, 0.8f);
    }
    g.setColour (centre);
    g.fillEllipse (x - r * 0.2f, y - r * 0.2f, r * 0.4f, r * 0.4f);
}

// A raised, glossy key. Lit keys glow in the accent colour; everything is drawn inside the button's own
// bounds (hence the 2 px margin) so the glow never gets clipped.
void drawKeyBody (juce::Graphics& g, juce::Rectangle<float> bounds, bool selected, bool over, bool down, const Scat::Palette& pal)
{
    const auto r = bounds.reduced (2.f);
    const float cr = 8.f;

    if (selected)
    {
        g.setColour (pal.acc.withAlpha (0.10f));
        g.fillRoundedRectangle (r.expanded (2.f), cr + 2.f);
        g.setColour (pal.acc.withAlpha (0.16f));
        g.fillRoundedRectangle (r.expanded (1.f), cr + 1.f);
    }
    else if (! down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (r.translated (0.f, 1.5f), cr);
    }

    const auto top = selected ? pal.accDark.brighter (0.35f) : down ? pal.soil : over ? pal.moss.brighter (0.3f) : pal.moss.brighter (0.1f);
    const auto bot = selected ? pal.accDark.darker (0.35f)   : down ? pal.moss : over ? pal.moss                 : pal.soil;
    g.setGradientFill (juce::ColourGradient (top, 0.f, r.getY(), bot, 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, cr);

    // top gloss
    const auto gl = r.reduced (1.5f, 1.f).withHeight (r.getHeight() * 0.48f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (down ? 0.04f : 0.22f), 0.f, gl.getY(),
                                             juce::Colours::white.withAlpha (0.f), 0.f, gl.getBottom(), false));
    g.fillRoundedRectangle (gl, cr - 1.f);

    g.setColour (selected ? pal.acc.withAlpha (0.95f) : over ? pal.acc.withAlpha (0.8f) : pal.bark);
    g.drawRoundedRectangle (r, cr, 1.2f);
}
}

// ---------------------------------------------------------------- look and feel

ScattrLNF::ScattrLNF()
{
    setPalette (Scat::themes()[0].pal);
}

void ScattrLNF::setPalette (const Scat::Palette& p)
{
    pal = p;
    setColour (juce::TextButton::textColourOffId, pal.ink);
    setColour (juce::TextButton::textColourOnId, pal.ink);
    setColour (juce::Slider::textBoxTextColourId, pal.ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, pal.dim);
}

void ScattrLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float a0, float a1, juce::Slider& sl)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (7.f);
    const auto c = r.getCentre();
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const float a = a0 + pos * (a1 - a0);
    const bool hot = sl.isMouseOverOrDragging();
    const auto white = juce::Colours::white, black = juce::Colours::black;

    // soft drop shadow
    for (int i = 3; i >= 1; --i)
    {
        const float e = (float) i * 2.f;
        g.setColour (black.withAlpha (0.13f));
        g.fillEllipse (c.x - rad - e, c.y - rad - e + 4.f, (rad + e) * 2.f, (rad + e) * 2.f);
    }

    // wooden stump: domed, lit from the top-left
    {
        juce::ColourGradient body (pal.bark.brighter (0.45f), c.x - rad * 0.45f, c.y - rad * 0.55f,
                                   pal.soil.darker (0.6f),    c.x + rad * 0.55f, c.y + rad * 0.75f, true);
        body.addColour (0.55, pal.soil.brighter (0.1f));
        g.setGradientFill (body);
        g.fillEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f);

        g.setGradientFill (juce::ColourGradient (white.withAlpha (hot ? 0.55f : 0.38f), c.x, c.y - rad,
                                                 black.withAlpha (0.45f), c.x, c.y + rad, false));
        g.drawEllipse (c.x - rad + 0.8f, c.y - rad + 0.8f, rad * 2.f - 1.6f, rad * 2.f - 1.6f, 1.6f);
    }

    // sunken centre cap (the opposite light direction makes it read as a dish)
    {
        const float ic = rad * 0.62f;
        g.setGradientFill (juce::ColourGradient (pal.soil.darker (0.5f), c.x, c.y - ic,
                                                 pal.moss.brighter (0.15f), c.x, c.y + ic, false));
        g.fillEllipse (c.x - ic, c.y - ic, ic * 2.f, ic * 2.f);
        g.setGradientFill (juce::ColourGradient (black.withAlpha (0.5f), c.x, c.y - ic,
                                                 white.withAlpha (0.18f), c.x, c.y + ic, false));
        g.drawEllipse (c.x - ic, c.y - ic, ic * 2.f, ic * 2.f, 1.2f);
    }

    // growth arc with a glow
    const float ar = rad + 4.f;
    juce::Path track, val;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a1, true);
    val.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a, true);
    g.setColour (black.withAlpha (0.35f));
    g.strokePath (track, juce::PathStrokeType (4.5f));
    g.setColour (pal.line);
    g.strokePath (track, juce::PathStrokeType (2.5f));

    if (pos > 0.001f)
    {
        auto rounded = [] (float t) { return juce::PathStrokeType (t, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };
        g.setColour (pal.acc.withAlpha (hot ? 0.20f : 0.14f));
        g.strokePath (val, rounded (7.f));
        g.setColour (pal.acc.withAlpha (0.30f));
        g.strokePath (val, rounded (5.f));
        g.setColour (pal.acc);
        g.strokePath (val, rounded (3.f));
        g.setColour (pal.acc.brighter (0.6f).withAlpha (0.7f));
        g.strokePath (val, rounded (1.1f));
    }

    // glowing bead at the end of the arc
    {
        const float bx = c.x + std::sin (a) * ar, by = c.y - std::cos (a) * ar;
        g.setGradientFill (juce::ColourGradient (pal.acc.brighter (0.6f).withAlpha (hot ? 0.95f : 0.75f), bx, by,
                                                 pal.acc.withAlpha (0.f), bx + 7.f, by, true));
        g.fillEllipse (bx - 7.f, by - 7.f, 14.f, 14.f);
        g.setColour (white.withAlpha (0.9f));
        g.fillEllipse (bx - 1.8f, by - 1.8f, 3.6f, 3.6f);
    }

    // leaf pointer (with its own little shadow)
    const auto t = juce::AffineTransform::translation (0.f, -rad * 0.2f)
                       .followedBy (juce::AffineTransform::rotation (a))
                       .followedBy (juce::AffineTransform::translation (c.x, c.y));
    const auto leaf = Scat::leafPath (rad * 0.75f, rad * 0.26f);
    g.setColour (black.withAlpha (0.35f));
    g.fillPath (leaf, t.translated (1.2f, 2.f));
    g.setGradientFill (juce::ColourGradient (pal.amber.brighter (0.3f), c.x, c.y - rad,
                                             pal.amber.darker (0.25f), c.x, c.y + rad * 0.3f, false));
    g.fillPath (leaf, t);
    juce::Path rib;
    rib.startNewSubPath (0.f, 0.f);
    rib.lineTo (0.f, -rad * 0.75f * 0.85f);
    g.setColour (pal.amber.darker (0.6f).withAlpha (0.7f));
    g.strokePath (rib, juce::PathStrokeType (1.f), t);
}

void ScattrLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    // tab buttons are toggles; ordinary buttons never are
    drawKeyBody (g, b.getLocalBounds().toFloat(), b.getToggleState(), over, down, pal);
}

void ScattrLNF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    const bool on = b.getToggleState();
    drawKeyBody (g, b.getLocalBounds().toFloat(), on, over, down, pal);

    g.setFont (serif (14.f));
    if (on)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, 1), juce::Justification::centred);
    }
    g.setColour (on ? pal.ink : pal.dim);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

// ---------------------------------------------------------------- panel

ScattrPanel::ScattrPanel (ScattrProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    setSize (baseW, baseH);

    struct Def { const char* id; const char* text; int dp; };
    // dp = digits shown under the knob (whole numbers for big ranges, 0.00 style for the small ones)
    static const Def defs[21] = {
        { "size", "seed size", 0 }, { "dens", "rain", 0 }, { "pitch", "pitch", 2 },
        { "wind", "wind", 2 }, { "spray", "scatter", 2 }, { "hold", "bloom", 2 },
        { "gap", "rest", 2 }, { "amp", "sun", 2 },
        { "cutoff", "cutoff", 0 }, { "reso", "reso", 2 }, { "drive", "drive", 2 },
        { "dnote", "time", -1 }, { "dfb", "feedback", 2 }, { "dmix", "echo mix", 2 },
        { "rsize", "size", 2 }, { "rdamp", "damp", 2 }, { "rmix", "reverb mix", 2 },
        { "mtune", "master tune", 0 }, { "cycle", "cycle", 2 }, { "rand", "random", 2 },
        { "fmix", "filter mix", 2 }
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (k.slider);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, defs[i].id, k.slider);

        // the attachment installs the host's long-winded text; replace it with a short fixed-decimals readout
        const int dp = defs[i].dp;
        if (dp < 0)   // delay time: shows note values (1/4, 1/8, ...)
        {
            k.slider.textFromValueFunction = [] (double v) { return juce::String (kNoteNames[juce::jlimit (0, 10, juce::roundToInt (v))]); };
            k.slider.valueFromTextFunction = [] (const juce::String& t)
            {
                for (int j = 0; j < 11; ++j)
                    if (t.trim().equalsIgnoreCase (kNoteNames[j])) return (double) j;
                return 4.0;
            };
        }
        else
        {
            k.slider.textFromValueFunction = [dp] (double v) { return juce::String (v, dp); };
            k.slider.valueFromTextFunction = [] (const juce::String& t) { return t.retainCharacters ("-0123456789.").getDoubleValue(); };
        }
        k.slider.updateText();

        k.label.setText (defs[i].text, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (serif (14.f, true));
        addAndMakeVisible (k.label);
    }

    // tabs
    static const char* tabNames[2] = { "GRAINS", "FX" };
    for (int i = 0; i < 2; ++i)
    {
        auto& b = tabBtn[(size_t) i];
        b.setButtonText (tabNames[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1001);
        b.onClick = [this, i] { if (tabBtn[(size_t) i].getToggleState()) setTab (i); };
        addAndMakeVisible (b);
    }
    tabBtn[0].setToggleState (true, juce::dontSendNotification);

    addAndMakeVisible (typeBtn);
    typeBtn.onClick = [this]
    {
        auto* par = proc.apvts.getParameter ("ftype");
        const int cur = (int) proc.apvts.getRawParameterValue ("ftype")->load();
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) ((cur + 1) % 3)));
        par->endChangeGesture();
    };

    addAndMakeVisible (spaceBtn);
    spaceBtn.onClick = [this]
    {
        auto* par = proc.apvts.getParameter ("rtype");
        const int cur = (int) proc.apvts.getRawParameterValue ("rtype")->load();
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) ((cur + 1) % 4)));
        par->endChangeGesture();
    };

    for (auto* b : { &fonBtn, &donBtn, &ronBtn }) addAndMakeVisible (*b);
    fonAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "fon", fonBtn);
    donAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "don", donBtn);
    ronAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "ron", ronBtn);

    addAndMakeVisible (scaleBtn);
    scaleBtn.setButtonText (juce::String (juce::roundToInt (proc.editorScale.load() * 100.f)) + "%");
    scaleBtn.onClick = [this]
    {
        static const float options[] = { 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
        const float cur = proc.editorScale.load();

        juce::PopupMenu menu;
        for (int i = 0; i < 5; ++i)
            menu.addItem (i + 1, juce::String (juce::roundToInt (options[i] * 100.f)) + "%",
                          true, std::abs (options[i] - cur) < 0.01f);

        juce::Component::SafePointer<ScattrPanel> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&scaleBtn),
                            [safe] (int result)
                            {
                                if (safe == nullptr || result <= 0) return;
                                const float s = options[result - 1];
                                safe->scaleBtn.setButtonText (juce::String (juce::roundToInt (s * 100.f)) + "%");
                                if (safe->onScaleChosen) safe->onScaleChosen (s);
                            });
    };

    addAndMakeVisible (themeBtn);
    themeBtn.onClick = [this]
    {
        juce::PopupMenu menu;
        const int cur = proc.themeIdx.load();
        for (int i = 0; i < (int) Scat::themes().size(); ++i)
            menu.addItem (i + 1, Scat::themes()[(size_t) i].name, true, i == cur);

        juce::Component::SafePointer<ScattrPanel> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&themeBtn),
                            [safe] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->applyTheme (result - 1);
                            });
    };

    addAndMakeVisible (loadBtn);
    addAndMakeVisible (clearBtn);
    addAndMakeVisible (droneBtn);
    droneAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "drone", droneBtn);
    addAndMakeVisible (tuneBtn);
    tuneAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "tune", tuneBtn);

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
    clearBtn.onClick = [this] { proc.clearSamples(); shown = -1; pinned = -1; listOff = 0; repaint(); };

    applyTheme (proc.themeIdx.load());
    setTab (0);
    startTimerHz (30);
}

ScattrPanel::~ScattrPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ScattrPanel::applyTheme (int idx)
{
    idx = juce::jlimit (0, (int) Scat::themes().size() - 1, idx);
    curTheme = idx;
    proc.themeIdx = idx;
    pal = Scat::themes()[(size_t) idx].pal;
    lnf.setPalette (pal);
    buildBackdrop (idx);
    themeBtn.setButtonText (Scat::themes()[(size_t) idx].name);
    sendLookAndFeelChange();   // refreshes the knob readouts' colours
    repaint();
}

void ScattrPanel::setTab (int t)
{
    tab = t;
    const bool g = (t == 0);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const bool vis = (onGrainsTab (i) == g);
        knobs[i].slider.setVisible (vis);
        knobs[i].label.setVisible (vis);
    }
    typeBtn.setVisible (! g);
    spaceBtn.setVisible (! g);
    for (auto* b : { &fonBtn, &donBtn, &ronBtn }) b->setVisible (! g);
    loadBtn.setVisible (g);
    clearBtn.setVisible (g);
    droneBtn.setVisible (g);
    tuneBtn.setVisible (g);
    tabBtn[(size_t) t].setToggleState (true, juce::dontSendNotification);
    repaint();
}

void ScattrPanel::resized()
{
    // header
    tabBtn[0].setBounds (360, 22, 100, 34);
    tabBtn[1].setBounds (466, 22, 100, 34);
    themeBtn.setBounds (594, 26, 80, 26);
    scaleBtn.setBounds (682, 26, 80, 26);

    // sample panel buttons
    const auto sp = sampleRect();
    clearBtn.setBounds ((int) sp.getRight() - 6 - 66, (int) sp.getY() + 6, 66, 22);
    loadBtn.setBounds (clearBtn.getX() - 4 - 66, (int) sp.getY() + 6, 66, 22);

    auto place = [this] (size_t idx, juce::Rectangle<float> panel, int n, int k, int sliderH)
    {
        const int sw = (int) ((panel.getWidth() - 24.f) / (float) n);
        const int x = (int) panel.getX() + 12 + k * sw, y = (int) panel.getY();
        knobs[idx].label.setBounds (x, y + 22, sw, 16);
        knobs[idx].slider.setBounds (x, y + 38, sw, sliderH);
    };

    // grains tab: two roomy rows
    {
        static const size_t row1[6] = { 0, 1, 2, 3, 4, 17 };      // seed size, rain, pitch, wind, scatter, master tune
        static const size_t row2[5] = { 5, 6, 18, 19, 7 };        // bloom, rest, cycle, random, sun
        for (int k = 0; k < 6; ++k) place (row1[k], grainPanel(), 6, k, 88);
        for (int k = 0; k < 5; ++k) place (row2[k], motionPanel(), 6, k, 88);

        const auto mp = motionPanel();
        const int sw = (int) ((mp.getWidth() - 24.f) / 6.f);
        const int bx = (int) mp.getX() + 12 + 5 * sw + (sw - 80) / 2;
        droneBtn.setBounds (bx, (int) mp.getY() + 40, 80, 28);
        tuneBtn.setBounds  (bx, (int) mp.getY() + 78, 80, 28);
    }

    // fx tab: one row per effect, shared 5-column grid
    {
        static const size_t filterKnobs[4] = { 8, 9, 10, 20 };   // cutoff, reso, drive, filter mix
        for (int k = 0; k < 4; ++k) place (filterKnobs[k], fxPanel (0), 5, k, 94);
        for (int k = 0; k < 3; ++k) place ((size_t) (11 + k), fxPanel (1), 5, k, 94);   // time, feedback, echo mix
        for (int k = 0; k < 3; ++k) place ((size_t) (14 + k), fxPanel (2), 5, k, 94);   // size, damp, reverb mix

        const int sw = (int) ((fxPanel (0).getWidth() - 24.f) / 5.f);
        auto colX = [&] (int col, int w) { return (int) fxPanel (0).getX() + 12 + col * sw + (sw - w) / 2; };
        typeBtn.setBounds  (colX (4, 100), (int) fxPanel (0).getY() + 64, 100, 28);
        spaceBtn.setBounds (colX (4, 100), (int) fxPanel (2).getY() + 64, 100, 28);

        // bypass buttons in each panel's top-right corner
        fonBtn.setBounds ((int) fxPanel (0).getRight() - 12 - 56, (int) fxPanel (0).getY() + 7, 56, 24);
        donBtn.setBounds ((int) fxPanel (1).getRight() - 12 - 56, (int) fxPanel (1).getY() + 7, 56, 24);
        ronBtn.setBounds ((int) fxPanel (2).getRight() - 12 - 56, (int) fxPanel (2).getY() + 7, 56, 24);
    }
}

void ScattrPanel::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files) proc.addSampleFile (juce::File (f));
    repaint();
}

int ScattrPanel::hitHandle (float x) const
{
    if (tab != 0 || shown < 0 || shown >= (int) proc.samples.size()) return 0;
    const auto& s = *proc.samples[(size_t) shown];
    const auto in = waveInner();
    const float ds = std::abs (x - (in.getX() + s.startN.load() * in.getWidth()));
    const float de = std::abs (x - (in.getX() + s.endN.load() * in.getWidth()));
    if (juce::jmin (ds, de) > 12.f) return 0;
    if (ds == de) return x < in.getX() + s.startN.load() * in.getWidth() ? 1 : 2;
    return ds < de ? 1 : 2;
}

void ScattrPanel::mouseDown (const juce::MouseEvent& e)
{
    if (tab != 0) return;

    if (waveRect().contains (e.position))
    {
        dragHandle = hitHandle (e.position.x);
        if (dragHandle != 0) pinned = shown;   // grabbing a bar locks the display onto that sample
        repaint();
        return;
    }

    const auto sp = sampleRect();
    if (sp.contains (e.position) && e.position.y >= sp.getY() + (float) listTop)
    {
        const int row = (int) ((e.position.y - (sp.getY() + (float) listTop)) / (float) rowH);
        const int idx = listOff + row;
        if (row < maxRows && idx >= 0 && idx < (int) proc.samples.size())
        {
            if (e.position.x < sp.getX() + 32.f)   // leaf icon: dormant / awake
            {
                auto& en = proc.samples[(size_t) idx]->enabled;
                en = ! en.load();
            }
            else                                   // name: pick this sample for trimming (click again to follow playback)
                pinned = (pinned == idx ? -1 : idx);
            repaint();
        }
    }
}

void ScattrPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (dragHandle == 0 || shown < 0 || shown >= (int) proc.samples.size()) return;
    auto& s = *proc.samples[(size_t) shown];
    const auto in = waveInner();
    const float n = juce::jlimit (0.f, 1.f, (e.position.x - in.getX()) / in.getWidth());
    const float gap = s.minGapN();
    if (dragHandle == 1) s.startN = juce::jlimit (0.f, s.endN.load() - gap, n);
    else                 s.endN   = juce::jlimit (s.startN.load() + gap, 1.f, n);
    repaint();
}

void ScattrPanel::mouseUp (const juce::MouseEvent&)
{
    dragHandle = 0;
    repaint();
}

void ScattrPanel::mouseMove (const juce::MouseEvent& e)
{
    const bool over = tab == 0 && waveRect().contains (e.position) && hitHandle (e.position.x) != 0;
    setMouseCursor (over ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void ScattrPanel::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (tab == 0 && waveRect().contains (e.position) && shown >= 0 && shown < (int) proc.samples.size())
    {
        auto& s = *proc.samples[(size_t) shown];
        s.startN = 0.f;
        s.endN = 1.f;
        repaint();
    }
}

void ScattrPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (tab != 0 || ! sampleRect().contains (e.position)) return;
    const int maxOff = juce::jmax (0, (int) proc.samples.size() - maxRows);
    listOff = juce::jlimit (0, maxOff, listOff + (w.deltaY > 0 ? -1 : 1));
    repaint();
}

void ScattrPanel::timerCallback()
{
    anim += 0.033f;

    static const char* ft[3] = { "LP", "HP", "BP" };
    const int fi = juce::jlimit (0, 2, (int) proc.apvts.getRawParameterValue ("ftype")->load());
    if (typeBtn.getButtonText() != ft[fi]) typeBtn.setButtonText (ft[fi]);

    static const char* rt[4] = { "ROOM", "PLATE", "HALL", "CATHEDRAL" };
    const int ri = juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("rtype")->load());
    if (spaceBtn.getButtonText() != rt[ri]) spaceBtn.setButtonText (rt[ri]);

    // bypassed effects look dimmed
    const float aF = fonBtn.getToggleState() ? 1.f : 0.4f;
    const float aD = donBtn.getToggleState() ? 1.f : 0.4f;
    const float aR = ronBtn.getToggleState() ? 1.f : 0.4f;
    for (size_t idx : { (size_t) 8, (size_t) 9, (size_t) 10, (size_t) 20 }) { knobs[idx].slider.setAlpha (aF); knobs[idx].label.setAlpha (aF); }
    for (size_t idx : { (size_t) 11, (size_t) 12, (size_t) 13 })            { knobs[idx].slider.setAlpha (aD); knobs[idx].label.setAlpha (aD); }
    for (size_t idx : { (size_t) 14, (size_t) 15, (size_t) 16 })            { knobs[idx].slider.setAlpha (aR); knobs[idx].label.setAlpha (aR); }
    typeBtn.setAlpha (aF);
    spaceBtn.setAlpha (aR);

    const int c = proc.uiSample.load();
    playIdx = c;
    if (pinned >= (int) proc.samples.size()) pinned = -1;
    if (pinned >= 0)    shown = pinned;
    else if (c >= 0)    shown = c;
    if (shown >= (int) proc.samples.size()) shown = -1;
    if (shown < 0 && ! proc.samples.empty()) shown = (int) proc.samples.size() - 1;

    playing  = proc.uiPlaying.load();
    shownPlaying = playing && playIdx == shown;
    progress = proc.uiProgress.load();

    const float gp = proc.uiGrain.load();
    if (shownPlaying && gp != lastGrain)
    {
        lastGrain = gp;
        ticks.push_back ({ gp, juce::Random::getSystemRandom().nextFloat(), 1.f });
    }
    for (auto& t : ticks) t.alpha -= 0.04f;
    ticks.erase (std::remove_if (ticks.begin(), ticks.end(), [] (const Tick& t) { return t.alpha <= 0.f; }), ticks.end());

    repaint();
}

// ---------------------------------------------------------------- vines (one per theme)

void ScattrPanel::drawVine (juce::Graphics& g)
{
    const float W = (float) baseW, H = (float) baseH;
    const float y0 = H - 16.f, top = H - 44.f;
    auto yAt = [&] (float x) { return y0 + 3.f * std::sin (x * 0.02f + anim * 0.4f); };
    auto sway = [&] (float x) { return 0.12f * std::sin (anim * 0.8f + x * 0.05f); };
    const float pi = juce::MathConstants<float>::pi;

    // main stem
    const bool woody = (curTheme >= 1);   // autumn, winter, spring: bare branch
    juce::Path stem;
    stem.startNewSubPath (0.f, yAt (0.f));
    for (float x = 0.f; x <= W; x += 6.f) stem.lineTo (x, yAt (x));
    g.setColour (curTheme == 2 ? pal.bark.brighter (0.3f) : woody ? pal.bark : pal.grassDark);
    g.strokePath (stem, juce::PathStrokeType (woody ? 2.4f : 1.8f));

    int i = 0;
    switch (curTheme)
    {
        default:
        case 0:   // summer: broad leaves and sunflowers
            for (float x = 26.f; x < W - 20.f; x += 46.f, ++i)
            {
                const float sw = sway (x);
                const float ang = ((i % 2 == 0) ? 1.0f : pi - 1.0f) + sw;
                drawLeaf (g, x, yAt (x), ang, 20.f + (float) (i % 3) * 2.f,
                          (i % 2 == 0 ? pal.grass : pal.grassDark.brighter (0.25f)).withAlpha (0.9f));

                if (i % 3 == 1)
                {
                    const float fx = x + 23.f, fy = yAt (fx);
                    const float tx = fx + sw * 16.f, ty = fy - 17.f;
                    g.setColour (pal.grassDark);
                    g.drawLine (fx, fy, tx, ty, 1.8f);
                    drawSunflower (g, tx, ty, 7.5f, pal.amber, pal.accDark.darker (0.7f));
                }
            }
            break;

        case 1:   // autumn: maple leaves on a bare branch, a few drifting down
        {
            const juce::Colour cols[4] = { pal.acc, pal.amber, pal.accDark.brighter (0.5f), pal.grass };
            for (float x = 24.f; x < W - 16.f; x += 38.f, ++i)
            {
                const float dir = (i % 2 == 0) ? 1.f : -1.f;
                const float sy = yAt (x);
                g.setColour (pal.bark);
                g.drawLine (x, sy, x + 9.f * dir, sy - 9.f, 1.5f);
                drawMaple (g, x + 10.f * dir, sy - 11.f, 0.6f * dir + sway (x), 8.5f + (float) (i % 3), cols[i % 4].withAlpha (0.92f));
            }
            for (int k = 0; k < 4; ++k)
            {
                const float t = std::fmod (anim * 0.05f + (float) k * 0.25f, 1.f);
                const float fx = W * (0.14f + 0.24f * (float) k) + 24.f * std::sin (anim * 0.7f + (float) k * 2.f);
                drawMaple (g, fx, top + t * (y0 - top - 4.f), anim * 1.2f + (float) k, 5.5f, cols[k % 4].withAlpha (0.85f * (1.f - t * 0.6f)));
            }
            break;
        }

        case 2:   // winter: frosted twig, snow caps, icicles and drifting flakes
            for (float x = 20.f; x < W - 16.f; x += 36.f, ++i)
            {
                const float sy = yAt (x);
                const float dir = (i % 2 == 0) ? 1.f : -1.f;
                g.setColour (pal.bark.brighter (0.3f));
                g.drawLine (x, sy, x + 10.f * dir, sy - 11.f, 1.4f);
                g.drawLine (x + 10.f * dir, sy - 11.f, x + 15.f * dir, sy - 15.f, 1.1f);
                if (i % 2 == 0)   // snow cap
                {
                    g.setColour (pal.ink.withAlpha (0.9f));
                    g.fillEllipse (x - 7.f, sy - 5.f, 14.f, 6.f);
                }
                if (i % 3 == 1)   // icicle
                {
                    juce::Path ic;
                    ic.addTriangle (x - 2.5f, sy + 1.f, x + 2.5f, sy + 1.f, x, sy + 8.f + (float) (i % 3) * 2.f);
                    g.setColour (pal.amber.withAlpha (0.85f));
                    g.fillPath (ic);
                }
            }
            for (int k = 0; k < 7; ++k)
            {
                const float t = std::fmod (anim * 0.06f + (float) k * 0.143f, 1.f);
                const float fx = W * (0.07f + 0.14f * (float) k) + 16.f * std::sin (anim * 0.9f + (float) k * 1.7f);
                drawSnowflake (g, fx, top + t * (y0 - top - 3.f), 3.5f + (float) (k % 3), pal.ink, 0.85f * (1.f - t * 0.5f));
            }
            break;

        case 3:   // spring: cherry-blossom branch with falling petals
            for (float x = 22.f; x < W - 16.f; x += 52.f, ++i)
            {
                const float sy = yAt (x);
                const float dir = (i % 2 == 0) ? 1.f : -1.f;
                const float bx = x + 11.f * dir, by = sy - 12.f;
                g.setColour (pal.bark);
                g.drawLine (x, sy, bx, by, 1.5f);
                drawBlossom (g, bx, by - 2.f, 6.5f, pal.acc.brighter (0.45f), pal.amber, pal.accDark);
                drawBlossom (g, bx - 9.f * dir, by + 5.f, 4.5f, pal.acc.brighter (0.6f), pal.amber, pal.accDark);
                drawLeaf (g, x + 20.f * dir, yAt (x + 20.f * dir), (dir > 0 ? 1.0f : -1.0f) + sway (x), 11.f, pal.grass.withAlpha (0.85f));
                g.setColour (pal.acc);
                g.fillEllipse (x + 28.f * dir - 2.f, yAt (x) - 7.f, 4.f, 5.f);   // bud
            }
            for (int k = 0; k < 6; ++k)
            {
                const float t = std::fmod (anim * 0.05f + (float) k * 0.167f, 1.f);
                const float fx = W * (0.08f + 0.16f * (float) k) + 20.f * std::sin (anim * 0.8f + (float) k * 2.3f);
                const float fy = top + t * (y0 - top - 3.f);
                juce::Path p;
                p.addEllipse (-2.5f, -1.8f, 5.f, 3.6f);
                g.setColour (pal.acc.brighter (0.5f).withAlpha (0.9f * (1.f - t * 0.6f)));
                g.fillPath (p, juce::AffineTransform::rotation (anim * 1.5f + (float) k).translated (fx, fy));
            }
            break;
    }
}

// ---------------------------------------------------------------- painting

// Loads this theme's picture, "cover"-fits it, bakes in the shading (dark top/bottom, vignette) and makes a
// small blurred copy that the glass panels draw over. Done once per theme change, not per frame.
void ScattrPanel::buildBackdrop (int idx)
{
    bgImg = {};
    blurImg = {};

    const auto& th = Scat::themes()[(size_t) idx];
    int size = 0;
    const char* data = ArtData::getNamedResource (th.image, size);
    const juce::Image src = (data != nullptr && size > 0) ? juce::ImageFileFormat::loadFrom (data, (size_t) size) : juce::Image();
    if (! src.isValid()) return;   // paint() falls back to the plain sky gradient

    const int W = baseW * 2, H = baseH * 2;   // 2x so it stays crisp at the larger zoom levels
    juce::Image img (juce::Image::ARGB, W, H, true);
    {
        juce::Graphics g (img);
        g.fillAll (th.pal.bgMid);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

        const float k = juce::jmax ((float) W / (float) src.getWidth(), (float) H / (float) src.getHeight());
        const int dw = juce::roundToInt ((float) src.getWidth() * k) + 1, dh = juce::roundToInt ((float) src.getHeight() * k) + 1;
        g.drawImage (src, (W - dw) / 2, (H - dh) / 2, dw, dh, 0, 0, src.getWidth(), src.getHeight());

        // keep text readable: tint the sky, hold the middle lighter, sink the footer
        juce::ColourGradient scrim (th.pal.bgTop.withAlpha (0.50f), 0.f, 0.f, th.pal.bgBot.withAlpha (0.82f), 0.f, (float) H, false);
        scrim.addColour (0.28, th.pal.bgMid.withAlpha (0.18f));
        scrim.addColour (0.62, th.pal.bgMid.withAlpha (0.30f));
        g.setGradientFill (scrim);
        g.fillAll();

        // vignette
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.45f,
                                                 juce::Colours::black.withAlpha (0.5f), 0.f, 0.f, true));
        g.fillAll();
    }

    // frosted copy: shrink, blur, and let bilinear scaling smooth it back up
    const juce::Image small = img.rescaled (W / 6, H / 6, juce::Graphics::highResamplingQuality);
    juce::Image blurred = small.createCopy();
    juce::ImageConvolutionKernel kernel (13);
    kernel.createGaussianBlur (4.5f);
    kernel.applyToImage (blurred, small, blurred.getBounds());

    bgImg = img;
    blurImg = blurred;
}

void ScattrPanel::paintBackdrop (juce::Graphics& g)
{
    if (bgImg.isValid())
    {
        g.drawImage (bgImg, 0, 0, baseW, baseH, 0, 0, bgImg.getWidth(), bgImg.getHeight());
    }
    else
    {
        juce::ColourGradient sky (pal.bgTop, 0.f, 0.f, pal.bgBot, 0.f, (float) baseH, false);
        sky.addColour (0.30, pal.bgMid);
        g.setGradientFill (sky);
        g.fillAll();
    }

    // a soft light bloom that breathes very slowly
    const float gx = (float) baseW * 0.82f, br = 0.10f + 0.035f * std::sin (anim * 0.6f);
    g.setGradientFill (juce::ColourGradient (pal.amber.withAlpha (br), gx, 40.f, pal.amber.withAlpha (0.f), gx, 340.f, true));
    g.fillRect (0, 0, baseW, 340);
}

// A frosted-glass card: accent glow and shadow behind, blurred picture + dark tint inside, bright rim on top.
void ScattrPanel::drawGlass (juce::Graphics& g, juce::Rectangle<float> r, float radius, float tint, bool inset)
{
    const auto white = juce::Colours::white, black = juce::Colours::black;

    for (int i = 4; i >= 1; --i)   // coloured glow
    {
        const float e = (float) i * 2.5f;
        g.setColour (pal.acc.withAlpha (0.022f));
        g.fillRoundedRectangle (r.expanded (e), radius + e);
    }
    for (int i = 4; i >= 1; --i)   // drop shadow
    {
        const float e = (float) i * 1.6f;
        g.setColour (black.withAlpha (0.10f));
        g.fillRoundedRectangle (r.expanded (e).translated (0.f, 3.f), radius + e);
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, radius);
        g.reduceClipRegion (clip);

        if (blurImg.isValid())
            g.drawImage (blurImg, 0, 0, baseW, baseH, 0, 0, blurImg.getWidth(), blurImg.getHeight());
        else
        {
            g.setColour (pal.bgMid);
            g.fillRect (r);
        }

        g.setColour (pal.soil.withAlpha (tint));
        g.fillRect (r);

        if (inset)   // recessed: shadow falling from the top edge
        {
            g.setGradientFill (juce::ColourGradient (black.withAlpha (0.45f), 0.f, r.getY(),
                                                     black.withAlpha (0.f), 0.f, r.getY() + 16.f, false));
            g.fillRect (r.withHeight (16.f));
        }
        else         // raised: sheen on the upper half
        {
            g.setGradientFill (juce::ColourGradient (white.withAlpha (0.13f), 0.f, r.getY(),
                                                     white.withAlpha (0.f), 0.f, r.getY() + r.getHeight() * 0.55f, false));
            g.fillRect (r);
        }

        g.setGradientFill (juce::ColourGradient (black.withAlpha (0.f), 0.f, r.getBottom() - r.getHeight() * 0.4f,
                                                 black.withAlpha (0.22f), 0.f, r.getBottom(), false));
        g.fillRect (r);
    }

    g.setGradientFill (juce::ColourGradient (white.withAlpha (inset ? 0.10f : 0.34f), 0.f, r.getY(),
                                             pal.acc.withAlpha (0.16f), 0.f, r.getBottom(), false));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.2f);

    if (! inset)   // rim light along the top edge
    {
        g.setColour (white.withAlpha (0.20f));
        g.drawLine (r.getX() + radius, r.getY() + 1.6f, r.getRight() - radius, r.getY() + 1.6f, 1.f);
    }
}

void ScattrPanel::drawGroup (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    drawGlass (g, r, 12.f, 0.52f);
    g.setFont (serif (13.f, true));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawText (title, (int) r.getX() + 15, (int) r.getY() + 5, 300, 16, juce::Justification::centredLeft);
    g.setColour (pal.dim);
    g.drawText (title, (int) r.getX() + 14, (int) r.getY() + 4, 300, 16, juce::Justification::centredLeft);
}

void ScattrPanel::paintWave (juce::Graphics& g)
{
    // forest floor / waveform
    auto wr = waveRect();
    drawGlass (g, wr, 10.f, 0.74f, true);

    if (proc.samples.empty())
    {
        g.setColour (pal.dim);
        g.setFont (serif (16.f, true));
        g.drawText ("plant some samples: drop audio files here", wr, juce::Justification::centred);
    }
    else if (shown >= 0 && shown < (int) proc.samples.size())
    {
        const auto& pk = proc.samples[(size_t) shown]->peaks;
        const int n = (int) pk.size();
        const auto inner = wr.reduced (10.f, 0.f);
        const float cy = wr.getCentreY(), hh = wr.getHeight() * 0.44f;

        const float al = shownPlaying ? 1.f : pinned >= 0 ? 0.75f : 0.45f;
        for (int pass = 0; pass < 2; ++pass)   // pass 0 = soft glow, pass 1 = the waveform itself
        {
            if (pass == 0)
                g.setColour (pal.acc.withAlpha (al * 0.22f));
            else
                g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (al), 0.f, wr.getY(),
                                                         pal.accDark.withAlpha (al), 0.f, wr.getBottom(), false));
            const float grow = pass == 0 ? 3.f : 0.f;
            for (int i = 0; i < n; ++i)
            {
                const float x = inner.getX() + inner.getWidth() * (float) i / (float) n;
                g.fillRect (x, cy - pk[(size_t) i].second * hh - grow, inner.getWidth() / (float) n + 0.5f,
                            juce::jmax (1.5f, (pk[(size_t) i].second - pk[(size_t) i].first) * hh) + grow * 2.f);
            }
        }

        {
            // trim bars: darken everything outside the playable region, then draw the two handles
            const auto& sm = *proc.samples[(size_t) shown];
            const float xs = inner.getX() + sm.startN.load() * inner.getWidth();
            const float xe = inner.getX() + sm.endN.load() * inner.getWidth();
            g.setColour (pal.soil.withAlpha (0.72f));
            g.fillRect (inner.getX(), wr.getY() + 1.f, xs - inner.getX(), wr.getHeight() - 2.f);
            g.fillRect (xe, wr.getY() + 1.f, inner.getRight() - xe, wr.getHeight() - 2.f);

            for (int h = 1; h <= 2; ++h)
            {
                const float x = h == 1 ? xs : xe, dir = h == 1 ? 1.f : -1.f;
                const bool hot = dragHandle == h;
                g.setColour (hot ? pal.amber : pal.grass);
                g.drawLine (x, wr.getY() + 4.f, x, wr.getBottom() - 4.f, hot ? 2.5f : 1.8f);
                juce::Path tri;
                tri.addTriangle (x, cy - 7.f, x, cy + 7.f, x + dir * 9.f, cy);
                g.fillPath (tri);
                g.fillRoundedRectangle (x - 4.f, wr.getY() + 3.f, 8.f, 5.f, 2.f);
                g.fillRoundedRectangle (x - 4.f, wr.getBottom() - 8.f, 8.f, 5.f, 2.f);
            }
        }

        if (shownPlaying)
        {
            // fireflies for each new grain
            for (auto& t : ticks)
            {
                const float fx = inner.getX() + t.pos * inner.getWidth();
                const float fy = wr.getY() + 14.f + t.y * (wr.getHeight() - 28.f)
                                 + 3.f * std::sin (anim * 3.f + t.pos * 20.f);
                const float r = 4.f + 8.f * t.alpha;
                g.setGradientFill (juce::ColourGradient (pal.amber.withAlpha (0.9f * t.alpha), fx, fy,
                                                         pal.amber.withAlpha (0.f), fx + r, fy, true));
                g.fillEllipse (fx - r, fy - r, r * 2.f, r * 2.f);
                g.setColour (juce::Colours::white.withAlpha (t.alpha));
                g.fillEllipse (fx - 1.5f, fy - 1.5f, 3.f, 3.f);
            }

            const auto& sp = *proc.samples[(size_t) shown];
            const float rs = sp.startN.load(), re = sp.endN.load();
            const float px = inner.getX() + (rs + progress * (re - rs)) * inner.getWidth();
            g.setColour (pal.amber.withAlpha (0.8f));
            g.drawLine (px, wr.getY() + 6.f, px, wr.getBottom() - 6.f, 1.2f);
        }
    }

    // status line under the waveform
    g.setFont (serif (14.f, true));
    g.setColour (pal.dim);
    const int count = (int) proc.samples.size();
    juce::String st = count == 0 ? "the grove is empty"
                    : pinned >= 0 && pinned < count
                          ? "trimming: " + proc.samples[(size_t) pinned]->name + "  (click name to release)"
                    : playing && shown >= 0 ? "blooming: " + proc.samples[(size_t) shown]->name
                                            : "resting...";
    g.drawText (st, (int) wr.getX() + 4, (int) wr.getBottom() + 4, (int) wr.getWidth() - 8, 18, juce::Justification::centredLeft, true);
}

void ScattrPanel::paintSamples (juce::Graphics& g)
{
    const auto sp = sampleRect();
    drawGlass (g, sp, 10.f, 0.60f);

    g.setColour (pal.dim);
    g.setFont (serif (13.f, true));
    g.drawText ("sample grove", (int) sp.getX() + 12, (int) sp.getY() + 6, 96, 22, juce::Justification::centredLeft);

    g.setFont (serif (14.f));
    const int count = (int) proc.samples.size();
    const float rowX = sp.getX() + 6.f, rowW = sp.getWidth() - 12.f - (count > maxRows ? 6.f : 0.f);

    for (int r = 0; r < maxRows && listOff + r < count; ++r)
    {
        const int i = listOff + r;
        const auto& s = *proc.samples[(size_t) i];
        const bool on = s.enabled.load();
        const auto row = juce::Rectangle<float> (rowX, sp.getY() + (float) listTop + (float) (r * rowH), rowW, (float) rowH);

        const bool isCur = playing && i == playIdx;
        if (i == pinned)
        {
            g.setColour (pal.amber.withAlpha (0.18f));
            g.fillRoundedRectangle (row, 6.f);
            g.setColour (pal.amber.withAlpha (0.6f));
            g.drawRoundedRectangle (row.reduced (0.5f), 6.f, 1.f);
        }
        if (isCur)
        {
            g.setColour (pal.moss);
            g.fillRoundedRectangle (row, 6.f);
        }
        drawLeaf (g, row.getX() + 12.f, row.getCentreY() + 5.f, 0.9f, 11.f,
                  on ? (isCur ? pal.grass : pal.grassDark.brighter (0.2f)) : pal.bark.withAlpha (0.6f));

        const auto textArea = row.toNearestInt().withTrimmedLeft (26).withTrimmedRight (38);
        g.setColour (isCur ? pal.ink : on ? pal.dim.brighter (0.3f) : pal.dim.withAlpha (0.5f));
        g.drawText (s.name + (on ? "" : "  (dormant)"), textArea, juce::Justification::centredLeft, true);

        const juce::String key = s.rootNote >= 0.f
            ? juce::MidiMessage::getMidiNoteName (juce::roundToInt (s.rootNote), true, true, 5) : juce::String ("--");
        g.setColour (pal.dim.withAlpha (on ? 0.9f : 0.5f));
        g.drawText (key, row.toNearestInt().reduced (6, 0), juce::Justification::centredRight);
    }

    if (count > maxRows)   // scroll thumb
    {
        const float th = (float) (maxRows * rowH);
        const float ty = sp.getY() + (float) listTop;
        g.setColour (pal.bark.withAlpha (0.3f));
        g.fillRoundedRectangle (sp.getRight() - 9.f, ty, 3.f, th, 1.5f);
        g.setColour (pal.acc.withAlpha (0.8f));
        g.fillRoundedRectangle (sp.getRight() - 9.f, ty + th * (float) listOff / (float) count,
                                3.f, th * (float) maxRows / (float) count, 1.5f);
    }
}

void ScattrPanel::paintGrainsTab (juce::Graphics& g)
{
    paintWave (g);
    paintSamples (g);
    drawGroup (g, grainPanel(),  "grove: grains");
    drawGroup (g, motionPanel(), "bloom: motion");
}

void ScattrPanel::paintFxTab (juce::Graphics& g)
{
    drawGroup (g, fxPanel (0), "canopy: filter");
    drawGroup (g, fxPanel (1), "canyon: delay");
    drawGroup (g, fxPanel (2), "cavern: reverb");

    const int sw = (int) ((fxPanel (0).getWidth() - 24.f) / 5.f);
    g.setColour (pal.dim);
    g.setFont (serif (14.f, true));
    g.drawText ("mode",  (int) fxPanel (0).getX() + 12 + 4 * sw, (int) fxPanel (0).getY() + 40, sw, 16, juce::Justification::centred);
    g.drawText ("space", (int) fxPanel (2).getX() + 12 + 4 * sw, (int) fxPanel (2).getY() + 40, sw, 16, juce::Justification::centred);
}

void ScattrPanel::paint (juce::Graphics& g)
{
    paintBackdrop (g);

    // title (glowing) + little sprout
    g.setFont (serif (38.f));
    g.setColour (pal.acc.withAlpha (0.13f));
    for (int dx = -2; dx <= 2; dx += 2)
        for (int dy = -2; dy <= 2; dy += 2)
            if (dx != 0 || dy != 0)
                g.drawText ("S C A T T R", 18 + dx, 14 + dy, 300, 50, juce::Justification::centredLeft);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("S C A T T R", 19, 16, 300, 50, juce::Justification::centredLeft);
    g.setColour (pal.ink);
    g.drawText ("S C A T T R", 18, 14, 300, 50, juce::Justification::centredLeft);
    const float sw = 0.08f * std::sin (anim * 0.9f);
    drawLeaf (g, 318.f, 52.f, 0.7f + sw, 20.f, pal.grassDark.brighter (0.2f));
    drawLeaf (g, 318.f, 52.f, -0.5f + sw, 16.f, pal.grass.darker (0.3f));
    drawLeaf (g, 318.f, 52.f, 0.1f + sw, 26.f, pal.grass);
    {
        const float fx = 318.f + std::sin (0.1f + sw) * 28.f, fy = 52.f - 28.f;
        g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (0.55f), fx, fy, pal.acc.withAlpha (0.f), fx + 16.f, fy, true));
        g.fillEllipse (fx - 16.f, fy - 16.f, 32.f, 32.f);
        drawFlower (g, fx, fy, 5.5f, pal.acc, pal.amber);
    }

    if (tab == 0) paintGrainsTab (g);
    else          paintFxTab (g);

    drawVine (g);   // lives in its own footer strip, clear of every control
}

// ---------------------------------------------------------------- plugin window

ScattrEditor::ScattrEditor (ScattrProcessor& p) : juce::AudioProcessorEditor (&p), proc (p), panel (p)
{
    setResizable (false, false);
    addAndMakeVisible (panel);
    panel.onScaleChosen = [this] (float s) { applyScale (s); };
    applyScale (proc.editorScale.load());   // restore the zoom the user last picked
}

void ScattrEditor::applyScale (float s)
{
    s = juce::jlimit (0.5f, 2.0f, s);
    proc.editorScale = s;
    setSize (juce::roundToInt ((float) ScattrPanel::baseW * s), juce::roundToInt ((float) ScattrPanel::baseH * s));
    resized();   // setSize does nothing if the size is unchanged, but the transform must still be (re)applied
}

void ScattrEditor::resized()
{
    panel.setBounds (0, 0, ScattrPanel::baseW, ScattrPanel::baseH);
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) ScattrPanel::baseW));
}
