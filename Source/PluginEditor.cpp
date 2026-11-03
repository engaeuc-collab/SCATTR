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
    static const Def defs[19] = {
        { "size", "seed size", " ms", 0 }, { "dens", "rain", " /s", 0 }, { "pitch", "pitch", " st", 1 },
        { "pjit", "wind", " st", 1 }, { "spray", "scatter", "", 2 }, { "hold", "bloom", " s", 1 },
        { "gap", "rest", " s", 2 }, { "amp", "sun", "", 2 },
        { "cutoff", "cutoff", " Hz", 0 }, { "reso", "reso", "", 2 }, { "drive", "drive", "", 2 },
        { "dtime", "time", " ms", 0 }, { "dfb", "feedback", "", 2 }, { "dmix", "echo mix", "", 2 },
        { "rsize", "size", "", 2 }, { "rdamp", "damp", "", 2 }, { "rmix", "reverb mix", "", 2 },
        { "mtune", "master tune", " ct", 0 }, { "cycle", "cycle", " s", 2 }
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

    addAndMakeVisible (scaleBtn);
    scaleBtn.onClick = [this]
    {
        static const float options[] = { 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
        const float cur = proc.editorScale.load();

        juce::PopupMenu menu;
        for (int i = 0; i < 5; ++i)
            menu.addItem (i + 1, juce::String (juce::roundToInt (options[i] * 100.f)) + "%",
                          true, std::abs (options[i] - cur) < 0.01f);

        juce::Component::SafePointer<ScattrEditor> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&scaleBtn),
                            [safe] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->applyScale (options[result - 1]);
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

    applyScale (proc.editorScale.load());
    startTimerHz (30);
}

ScattrEditor::~ScattrEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ScattrEditor::applyScale (float s)
{
    proc.editorScale = s;
    scaleBtn.setButtonText (juce::String (juce::roundToInt (s * 100.f)) + "%");
    setScaleFactor (s);
}

void ScattrEditor::resized()
{
    const int w = getWidth();
    droneBtn.setBounds (w - 18 - 80, 26, 80, 26);
    clearBtn.setBounds (w - 18 - 80 - 8 - 80, 26, 80, 26);
    loadBtn.setBounds  (w - 18 - 80 - 8 - 80 - 8 - 80, 26, 80, 26);
    tuneBtn.setBounds (w - 18 - 80, 244, 80, 22);
    scaleBtn.setBounds (w - 18 - 80 - 8 - 80 - 8 - 80 - 8 - 80, 26, 80, 26);

    const int cw1 = (w - 36) / 10, cw2 = (w - 36) / 10;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (i < 8 || i == 17 || i == 18)   // grain row (key cycle, then master tune, sit at the end of it)
        {
            const int col = i < 8 ? (int) i : (i == 18 ? 8 : 9);
            const int x = 18 + col * cw1;
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

int ScattrEditor::hitHandle (float x) const
{
    if (shown < 0 || shown >= (int) proc.samples.size()) return 0;
    const auto& s = *proc.samples[(size_t) shown];
    const auto in = waveInner();
    const float ds = std::abs (x - (in.getX() + s.startN.load() * in.getWidth()));
    const float de = std::abs (x - (in.getX() + s.endN.load() * in.getWidth()));
    if (juce::jmin (ds, de) > 12.f) return 0;
    if (ds == de) return x < in.getX() + s.startN.load() * in.getWidth() ? 1 : 2;
    return ds < de ? 1 : 2;
}

void ScattrEditor::mouseDown (const juce::MouseEvent& e)
{
    if (waveRect().contains (e.position))
    {
        dragHandle = hitHandle (e.position.x);
        if (dragHandle != 0) pinned = shown;   // grabbing a bar locks the display onto that sample
        repaint();
        return;
    }

    if (e.y >= listY && e.y < listY + rowH * maxRows)
    {
        const int idx = listOff + (e.y - listY) / rowH;
        if (idx >= 0 && idx < (int) proc.samples.size())
        {
            if (e.x < 46)   // leaf icon: dormant / awake
            {
                auto& en = proc.samples[(size_t) idx]->enabled;
                en = ! en.load();
            }
            else            // name: pick this sample for trimming (click again to follow playback)
                pinned = (pinned == idx ? -1 : idx);
            repaint();
        }
    }
}

void ScattrEditor::mouseDrag (const juce::MouseEvent& e)
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

void ScattrEditor::mouseUp (const juce::MouseEvent&)
{
    dragHandle = 0;
    repaint();
}

void ScattrEditor::mouseMove (const juce::MouseEvent& e)
{
    const bool over = waveRect().contains (e.position) && hitHandle (e.position.x) != 0;
    setMouseCursor (over ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void ScattrEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (waveRect().contains (e.position) && shown >= 0 && shown < (int) proc.samples.size())
    {
        auto& s = *proc.samples[(size_t) shown];
        s.startN = 0.f;
        s.endN = 1.f;
        repaint();
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

        const float al = shownPlaying ? 1.f : pinned >= 0 ? 0.75f : 0.45f;
        g.setGradientFill (juce::ColourGradient (acc.withAlpha (al), 0.f, wr.getY(),
                                                 accDark.withAlpha (al), 0.f, wr.getBottom(), false));
        for (int i = 0; i < n; ++i)
        {
            const float x = inner.getX() + inner.getWidth() * (float) i / (float) n;
            g.fillRect (x, cy - pk[(size_t) i].second * hh, inner.getWidth() / (float) n + 0.5f,
                        juce::jmax (1.5f, (pk[(size_t) i].second - pk[(size_t) i].first) * hh));
        }

        {
            // trim bars: dim everything outside the playable region, then draw the two handles
            const auto& sm = *proc.samples[(size_t) shown];
            const float xs = inner.getX() + sm.startN.load() * inner.getWidth();
            const float xe = inner.getX() + sm.endN.load() * inner.getWidth();
            g.setColour (soil.withAlpha (0.72f));
            g.fillRect (inner.getX(), wr.getY() + 1.f, xs - inner.getX(), wr.getHeight() - 2.f);
            g.fillRect (xe, wr.getY() + 1.f, inner.getRight() - xe, wr.getHeight() - 2.f);

            for (int h = 1; h <= 2; ++h)
            {
                const float x = h == 1 ? xs : xe, dir = h == 1 ? 1.f : -1.f;
                const bool hot = dragHandle == h;
                g.setColour (hot ? amber : grass);
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
                g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.9f * t.alpha), fx, fy,
                                                         amber.withAlpha (0.f), fx + r, fy, true));
                g.fillEllipse (fx - r, fy - r, r * 2.f, r * 2.f);
                g.setColour (juce::Colours::white.withAlpha (t.alpha));
                g.fillEllipse (fx - 1.5f, fy - 1.5f, 3.f, 3.f);
            }

            const auto& sp = *proc.samples[(size_t) shown];
            const float rs = sp.startN.load(), re = sp.endN.load();
            const float px = inner.getX() + (rs + progress * (re - rs)) * inner.getWidth();
            g.setColour (amber.withAlpha (0.8f));
            g.drawLine (px, wr.getY() + 6.f, px, wr.getBottom() - 6.f, 1.2f);
        }
    }

    // status
    g.setFont (serif (14.f, true));
    g.setColour (dim);
    juce::String st = proc.samples.empty() ? "the grove is empty"
                    : pinned >= 0 && pinned < (int) proc.samples.size()
                          ? "trimming: " + proc.samples[(size_t) pinned]->name + "   (click its name again to follow playback)"
                    : playing && shown >= 0 ? "blooming: " + proc.samples[(size_t) shown]->name
                                            : "resting...";
    g.drawText (st, 18, 246, getWidth() - 36 - 90, 18, juce::Justification::centredLeft);

    // sample list
    g.setFont (serif (14.f));
    const int count = (int) proc.samples.size();
    for (int r = 0; r < maxRows && listOff + r < count; ++r)
    {
        const int i = listOff + r;
        const auto& s = *proc.samples[(size_t) i];
        const bool on = s.enabled.load();
        const auto row = juce::Rectangle<int> (18, listY + r * rowH, getWidth() - 36, rowH);

        const bool isCur = playing && i == playIdx;
        if (i == pinned)
        {
            g.setColour (amber.withAlpha (0.18f));
            g.fillRoundedRectangle (row.toFloat(), 6.f);
            g.setColour (amber.withAlpha (0.6f));
            g.drawRoundedRectangle (row.toFloat().reduced (0.5f), 6.f, 1.f);
        }
        if (isCur)
        {
            g.setColour (moss);
            g.fillRoundedRectangle (row.toFloat(), 6.f);
        }
        drawLeaf (g, (float) row.getX() + 12.f, (float) row.getCentreY() + 5.f, 0.9f, 11.f,
                  on ? (isCur ? grass : grassDark.brighter (0.2f)) : bark.withAlpha (0.6f));

        g.setColour (isCur ? ink : on ? dim.brighter (0.3f) : dim.withAlpha (0.5f));
        g.drawText (s.name + (on ? "" : "  (dormant)"), row.withTrimmedLeft (28).withTrimmedRight (110),
                    juce::Justification::centredLeft, true);
        const juce::String key = s.rootNote >= 0.f
            ? juce::MidiMessage::getMidiNoteName (juce::roundToInt (s.rootNote), true, true, 5) : juce::String ("--");
        g.drawText (key + "   " + juce::String (s.length / s.sampleRate, 1) + " s", row.reduced (8, 0),
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
