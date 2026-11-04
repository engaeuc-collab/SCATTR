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

// A five-lobed maple leaf with veins and a stem. `r` is the leaf's half-height; the stem points down
// before rotation. (The old version was a plain five-point star.)
void drawMaple (juce::Graphics& g, float x, float y, float rot, float r, juce::Colour c)
{
    // right half of the outline (y up, tip at the top); the left half is mirrored
    static const float half[][2] = {
        { 0.00f,  1.00f }, { 0.11f,  0.80f }, { 0.21f,  0.84f }, { 0.19f,  0.60f }, { 0.30f,  0.64f }, { 0.27f,  0.44f },
        { 0.52f,  0.58f }, { 0.58f,  0.80f }, { 0.70f,  0.66f }, { 0.92f,  0.70f }, { 0.78f,  0.42f }, { 0.86f,  0.30f },
        { 0.66f,  0.20f }, { 0.98f, -0.02f }, { 0.62f, -0.10f }, { 0.50f, -0.30f }, { 0.14f, -0.22f }, { 0.00f, -0.34f }
    };
    constexpr int n = (int) (sizeof (half) / sizeof (half[0]));

    juce::Path p;
    p.startNewSubPath (0.f, -r);
    for (int i = 1; i < n; ++i)       p.lineTo (half[i][0] * r, -half[i][1] * r);
    for (int i = n - 2; i >= 1; --i)  p.lineTo (-half[i][0] * r, -half[i][1] * r);
    p.closeSubPath();

    const auto t = juce::AffineTransform::rotation (rot).translated (x, y);
    g.setColour (c);
    g.fillPath (p.createPathWithRoundedCorners (juce::jmax (0.4f, r * 0.07f)), t);

    const auto vein = c.darker (0.55f);
    juce::Path v;
    if (r >= 7.f)   // veins only when the leaf is big enough to show them
    {
        const float tips[3][2] = { { 0.f, 0.85f }, { 0.64f, 0.44f }, { 0.64f, -0.02f } };
        for (auto& tp : tips)
        {
            v.startNewSubPath (0.f, r * 0.28f);
            v.lineTo (tp[0] * r, -tp[1] * r);
            if (tp[0] > 0.f) { v.startNewSubPath (0.f, r * 0.28f); v.lineTo (-tp[0] * r, -tp[1] * r); }
        }
    }
    v.startNewSubPath (0.f, r * 0.34f);   // stem
    v.lineTo (0.f, r * 0.95f);
    g.setColour (vein);
    g.strokePath (v, juce::PathStrokeType (r >= 7.f ? 0.9f : 1.1f), t);
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

inline float smooth01 (float t)
{
    t = juce::jlimit (0.f, 1.f, t);
    return t * t * (3.f - 2.f * t);
}

// Width of the "Nature" title at its drawn size, so the title and the sprout beside it can be centred as one group.
float titleWidth()
{
    static const float w = []
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (Scat::titleFont (50.f), "Nature", 0.f, 0.f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    }();
    return w;
}
constexpr float sproutGap = 24.f, sproutHalf = 15.f;   // title -> sprout stem, and how far the sprout reaches right of its stem

// How "on" a toggle looks, 0..1. The panel glides this for the on/off buttons (the "lit" property, 0.2 s);
// every other button simply follows its toggle state.
inline float buttonLit (const juce::Button& b)
{
    const auto& props = b.getProperties();
    if (props.contains ("lit")) return (float) (double) props["lit"];
    return b.getToggleState() ? 1.f : 0.f;
}

// Hover notes. The index order matches ScattrPanel::hoveredControl(): the 22 knobs (same order as the
// `knobs` array), then DRIFT, TUNE, filter mode, reverb space and the three on/off buttons, then the spectral knob.
struct Tip { const char* name; const char* text; };
const Tip kTips[30] = {
    { "seed size",     "Length of each grain. Short is gritty, long is smooth and recognisable." },
    { "rain",          "How many grains fall per second. More makes a thicker, denser cloud." },
    { "speed",         "How fast the playhead travels through each sample. 1x is normal; pitch stays put." },
    { "wind",          "Mixes in a looping wind texture underneath the grains." },
    { "scatter",       "How far grains stray from the playhead. Low is focused, high is wide." },
    { "bloom",         "Seconds each sample plays before the next one, when no key is held." },
    { "rest",          "Silence between samples, in seconds, when no key is held." },
    { "sun",           "Overall volume of the grains." },
    { "cutoff",        "The frequency where the filter starts to cut." },
    { "reso",          "Boosts the sound around the cutoff for a sharper, ringing tone." },
    { "drive",         "Adds warm saturation after the filter." },
    { "time",          "Echo delay time as a note length, synced to your host's tempo." },
    { "feedback",      "How many times the echo repeats." },
    { "echo mix",      "Level of the echoes against the dry sound." },
    { "size",          "How big the space is. Larger gives a longer tail." },
    { "damp",          "Absorbs highs in the tail. More damping sounds darker and softer." },
    { "reverb mix",    "Level of the reverb against the dry sound." },
    { "master tune",   "Fine tuning for everything, in cents." },
    { "cycle",         "(retired) Held keys now play each sample all the way through." },
    { "random",        "Chance that a grain is borrowed from a different sample." },
    { "filter mix",    "Blends between the dry sound and the filtered sound." },
    { "pan spread",    "Scatters grains randomly left and right. Zero keeps them centred." },
    { "drift",         "On: plays by itself. Off: only sounds while a key is held." },
    { "tune",          "On: samples retune to the key you play. Off: keys just transpose them." },
    { "filter mode",   "Click to cycle between low-pass, high-pass and band-pass." },
    { "reverb space",  "Click to cycle between room, plate, hall and cathedral." },
    { "filter",        "Turns the filter on or off." },
    { "echo",          "Turns the echo on or off." },
    { "reverb",        "Turns the reverb on or off." },
    { "spectral",      "Opens the spectral compressor, which evens out each frequency on its own." }
};

// A raised, glossy key. `lit` (0..1) is how switched-on it looks: the body, glow and rim fade between the two
// states, and while it is mid-switch the key swells slightly and flashes. Everything is drawn inside the
// button's own bounds (hence the 2 px margin) so nothing gets clipped; the wider glow lives in the panel.
void drawKeyBody (juce::Graphics& g, juce::Rectangle<float> bounds, float lit, bool over, bool down, const Scat::Palette& pal)
{
    const float e = smooth01 (lit);
    const float flip = (lit > 0.001f && lit < 0.999f) ? std::sin (juce::MathConstants<float>::pi * lit) : 0.f;
    const auto r = bounds.reduced (2.f - flip);
    const float cr = 8.f;

    if (e > 0.01f)
    {
        g.setColour (pal.acc.withAlpha (0.06f * e));
        g.fillRoundedRectangle (r.expanded (2.f), cr + 2.f);
        g.setColour (pal.acc.withAlpha (0.09f * e));
        g.fillRoundedRectangle (r.expanded (1.f), cr + 1.f);
    }
    if (e < 0.99f && ! down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f * (1.f - e)));
        g.fillRoundedRectangle (r.translated (0.f, 1.5f), cr);
    }

    const auto topOff = down ? pal.soil : over ? pal.moss.brighter (0.3f) : pal.moss.brighter (0.1f);
    const auto botOff = down ? pal.moss : over ? pal.moss : pal.soil;
    const auto top = topOff.interpolatedWith (pal.accDark.brighter (0.35f), e);
    const auto bot = botOff.interpolatedWith (pal.accDark.darker (0.35f), e);
    g.setGradientFill (juce::ColourGradient (top, 0.f, r.getY(), bot, 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, cr);

    // top gloss
    const auto gl = r.reduced (1.5f, 1.f).withHeight (r.getHeight() * 0.48f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (down ? 0.03f : 0.12f), 0.f, gl.getY(),
                                             juce::Colours::white.withAlpha (0.f), 0.f, gl.getBottom(), false));
    g.fillRoundedRectangle (gl, cr - 1.f);

    if (flip > 0.f)   // the flash of switching
    {
        g.setColour (pal.glow.brighter (0.4f).withAlpha (0.20f * flip));
        g.fillRoundedRectangle (r, cr);
    }

    const auto rimOff = over ? pal.acc.withAlpha (0.8f) : pal.bark;
    g.setColour (rimOff.interpolatedWith (pal.acc.withAlpha (0.95f), e));
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
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (Scat::knobPad);
    const auto c = r.getCentre();
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const float a = a0 + pos * (a1 - a0);
    const bool hot = sl.isMouseOverOrDragging();
    const auto white = juce::Colours::white, black = juce::Colours::black;
    // every knob breathes slightly out of step with its neighbours
    const float pulse = 0.5f + 0.5f * std::sin ((float) (juce::Time::getMillisecondCounterHiRes() * 0.001) * 2.4f + c.x * 0.05f);

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

        g.setGradientFill (juce::ColourGradient (white.withAlpha (hot ? 0.36f : 0.24f), c.x, c.y - rad,
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

    // growth arc: a soft bloom underneath, then a core that warms from deep to bright along its length
    // (the wide bloom behind the whole knob is painted by the panel, where it has room to spread)
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

        const float breathe = 0.85f + 0.15f * pulse;
        for (int i = 4; i >= 1; --i)   // bloom: wide and faint -> narrow and strong
        {
            g.setColour (pal.acc.withAlpha ((hot ? 0.034f : 0.022f) * breathe * (5.f - (float) i)));
            g.strokePath (val, rounded (3.f + (float) i * 1.4f));
        }

        const int segs = juce::jmax (2, (int) (pos * 28.f));
        for (int k = 0; k < segs; ++k)
        {
            const float t0 = (float) k / (float) segs, t1 = (float) (k + 1) / (float) segs;
            juce::Path seg;
            seg.addCentredArc (c.x, c.y, ar, ar, 0.f, a0 + (a - a0) * t0, a0 + (a - a0) * t1, true);
            g.setColour (pal.accDark.brighter (0.25f).interpolatedWith (pal.acc.brighter (0.3f), t1));
            g.strokePath (seg, rounded (3.f));
        }
        g.setColour (pal.acc.brighter (0.5f).withAlpha (0.35f));
        g.strokePath (val, rounded (1.1f));
    }

    // bright bead at the end of the arc (its big halo is painted by the panel)
    {
        const float bx = c.x + std::sin (a) * ar, by = c.y - std::cos (a) * ar;
        g.setGradientFill (juce::ColourGradient (pal.glow.brighter (0.4f).withAlpha (hot ? 1.f : 0.85f), bx, by,
                                                 pal.glow.withAlpha (0.f), bx + 5.f, by, true));
        g.fillEllipse (bx - 5.f, by - 5.f, 10.f, 10.f);
        g.setColour (white.withAlpha (0.95f));
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
    drawKeyBody (g, b.getLocalBounds().toFloat(), buttonLit (b), over, down, pal);
}

void ScattrLNF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    const float lit = buttonLit (b), e = smooth01 (lit);
    drawKeyBody (g, b.getLocalBounds().toFloat(), lit, over, down, pal);

    g.setFont (serif (14.f));
    if (e > 0.01f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f * e));
        g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, 1), juce::Justification::centred);
    }
    g.setColour (pal.dim.interpolatedWith (pal.ink, e));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

// ---------------------------------------------------------------- panel

ScattrPanel::ScattrPanel (ScattrProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    setSize (baseW, baseH);

    struct Def { const char* id; const char* text; int dp; };
    // dp = digits shown under the knob (whole numbers for big ranges, 0.00 style for the small ones)
    static const Def defs[22] = {
        { "size", "seed size", 0 }, { "dens", "rain", 0 }, { "speed", "speed", 2 },
        { "wind", "wind", 2 }, { "spray", "scatter", 2 }, { "hold", "bloom", 2 },
        { "gap", "rest", 2 }, { "amp", "sun", 2 },
        { "cutoff", "cutoff", 0 }, { "reso", "reso", 2 }, { "drive", "drive", 2 },
        { "dnote", "time", -1 }, { "dfb", "feedback", 2 }, { "dmix", "echo mix", 2 },
        { "rsize", "size", 2 }, { "rdamp", "damp", 2 }, { "rmix", "reverb mix", 2 },
        { "mtune", "master tune", 0 }, { "cycle", "cycle", 2 }, { "rand", "random", 2 },
        { "fmix", "filter mix", 2 }, { "width", "pan spread", 2 }
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
            const bool isSpeed = juce::String (defs[i].id) == "speed";
            k.slider.textFromValueFunction = [dp, isSpeed] (double v) { return juce::String (v, dp) + (isSpeed ? "x" : ""); };
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

    addAndMakeVisible (specKnob);
    specKnob.onOpen = [this] { openSpectral(); };
    specLabel.setText ("spectral", juce::dontSendNotification);
    specLabel.setJustificationType (juce::Justification::centred);
    specLabel.setFont (serif (14.f, true));
    addAndMakeVisible (specLabel);

    // the pop-out window goes in last so it sits above everything else
    spectral = std::make_unique<SpectralOverlay> (*this, proc);
    addChildComponent (*spectral);
    spectral->setBounds (getLocalBounds());

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

    applyTheme (proc.themeIdx.load(), false);
    setTab (0);
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

ScattrPanel::~ScattrPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ScattrPanel::applyTheme (int idx, bool animate)
{
    idx = juce::jlimit (0, (int) Scat::themes().size() - 1, idx);
    if (animate && idx == curTheme && themeT >= 1.f) return;

    // What is on screen right now becomes the "from" side. If we're interrupting an unfinished
    // transition, bake the current blend into one image so the fade carries on from what the user sees.
    if (animate && ! cache.isNull())
    {
        if (themeT < 1.f && prevCache.isValid())
        {
            juce::Image mix (juce::Image::RGB, cache.getWidth(), cache.getHeight(), true);
            juce::Graphics mg (mix);
            mg.drawImageAt (prevCache, 0, 0);
            mg.setOpacity (smooth01 (themeT));
            mg.drawImageAt (cache, 0, 0);
            prevCache = mix;
        }
        else
            prevCache = cache;
    }
    else
        prevCache = {};

    fromPal = pal;
    curTheme = idx;
    proc.themeIdx = idx;
    targetPal = Scat::themes()[(size_t) idx].pal;
    themeT = animate ? 0.f : 1.f;
    if (! animate) pal = targetPal;

    cacheTheme = -1;   // the cached sky and cards are rebuilt with the new palette (see rebuildCache)
    lnf.setPalette (pal);
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
        const bool vis = (onGrainsTab (i) == g) && i != 18;   // 18 = "cycle": retired, samples now play all the way through
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
    specKnob.setVisible (g);
    specLabel.setVisible (g);
    tabBtn[(size_t) t].setToggleState (true, juce::dontSendNotification);
    repaint();
}

void ScattrPanel::resized()
{
    // header
    tabBtn[0].setBounds (18, 22, 88, 34);
    tabBtn[1].setBounds (112, 22, 88, 34);
    themeBtn.setBounds (600, 26, 76, 26);
    scaleBtn.setBounds (682, 26, 80, 26);

    if (spectral != nullptr) spectral->setBounds (getLocalBounds());

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
        static const size_t row1[7] = { 0, 1, 2, 3, 4, 21, 17 };  // seed size, rain, speed, wind, scatter, pan spread, master tune
        static const size_t row2[4] = { 5, 6, 19, 7 };             // bloom, rest, random, sun
        for (int k = 0; k < 7; ++k) place (row1[k], grainPanel(), 7, k, 88);
        for (int k = 0; k < 4; ++k) place (row2[k], motionPanel(), 7, k, 88);

        const auto mp = motionPanel();
        const int sw = (int) ((mp.getWidth() - 24.f) / 7.f);
        const int bx = (int) mp.getX() + 12 + 4 * sw + (sw - 80) / 2;
        droneBtn.setBounds (bx, (int) mp.getY() + 40, 80, 28);
        tuneBtn.setBounds  (bx, (int) mp.getY() + 78, 80, 28);

        // the spectral compressor knob sits in the next free column of the motion row
        const int sx = (int) mp.getX() + 12 + 5 * sw;
        specLabel.setBounds (sx, (int) mp.getY() + 22, sw, 16);
        specKnob.setBounds  (sx, (int) mp.getY() + 38, sw, 88);
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
    if (spectral != nullptr && spectral->isVisible()) return;
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

// "Nature" and its sprout sit together in the middle of the header
float ScattrPanel::titleX() const  { return (float) baseW * 0.5f - (titleWidth() + sproutGap + sproutHalf) * 0.5f; }
float ScattrPanel::sproutX() const { return titleX() + titleWidth() + sproutGap; }

bool ScattrPanel::isInterestedInFileDrag (const juce::StringArray&)
{
    return spectral == nullptr || ! spectral->isVisible();   // nothing behind the spectral window may be dropped on
}

void ScattrPanel::openSpectral()
{
    if (spectral == nullptr || spectral->isVisible()) return;

    // Hover notes belong to the controls that are about to be covered
    hoverId = tipId = -1;
    tipAlpha = 0.f;

    // Grab the panel as it looks right now (before the window is shown), at the current zoom so it stays sharp
    const float s = juce::jmax (1.f, proc.editorScale.load());
    spectral->open (createComponentSnapshot (getLocalBounds(), true, s));
}

void ScattrPanel::showSampleMenu (int idx)
{
    const auto target = proc.samples[(size_t) idx];   // keep the sample's identity: the list can change while the menu is open

    juce::PopupMenu menu;
    menu.addItem (1, target->enabled.load() ? "Mute" : "Unmute");
    menu.addItem (2, "Remove");

    juce::Component::SafePointer<ScattrPanel> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safe, target] (int result)
    {
        if (safe == nullptr || result <= 0) return;
        auto& p = *safe;

        int i = -1;
        for (size_t k = 0; k < p.proc.samples.size(); ++k)
            if (p.proc.samples[k] == target) { i = (int) k; break; }
        if (i < 0) return;

        if (result == 1)
            target->enabled = ! target->enabled.load();
        else
        {
            p.proc.removeSample (i);
            if (p.pinned == i)     p.pinned = -1;
            else if (p.pinned > i) --p.pinned;
            p.shown = -1;
            p.listOff = juce::jlimit (0, juce::jmax (0, (int) p.proc.samples.size() - maxRows), p.listOff);
        }
        p.repaint();
    });
}

void ScattrPanel::mouseDown (const juce::MouseEvent& e)
{
    if (tab != 0) return;

    if (e.mods.isPopupMenu())   // right-click: only does something on a sample row
    {
        const auto lp = sampleRect();
        if (lp.contains (e.position) && e.position.y >= lp.getY() + (float) listTop)
        {
            const int row = (int) ((e.position.y - (lp.getY() + (float) listTop)) / (float) rowH);
            const int idx = listOff + row;
            if (row < maxRows && idx >= 0 && idx < (int) proc.samples.size())
                showSampleMenu (idx);
        }
        return;
    }

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

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit (0.f, 0.1f, (float) ((nowMs - lastTickMs) * 0.001));
    lastTickMs = nowMs;

    // theme transition: the live colours glide to the new palette while the cached backdrop crossfades
    if (themeT < 1.f)
    {
        themeT = juce::jmin (1.f, themeT + dt / themeFadeSec);
        pal = Scat::lerpPal (fromPal, targetPal, smooth01 (themeT));
        lnf.setPalette (pal);
        sendLookAndFeelChange();
        if (themeT >= 1.f) prevCache = {};
    }

    // on/off buttons glide between their two looks in 0.2 s
    for (auto* b : { &droneBtn, &tuneBtn, &fonBtn, &donBtn, &ronBtn })
    {
        const float target = b->getToggleState() ? 1.f : 0.f;
        float v = buttonLit (*b);
        const float step = dt / 0.2f;
        v = v < target ? juce::jmin (target, v + step) : juce::jmax (target, v - step);
        b->getProperties().set ("lit", v);
    }

    // hover tips: wait ~0.5 s on a control (0.1 s when moving between controls while a tip is already up)
    {
        const int hov = juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown() ? -1 : hoveredControl();
        if (hov != hoverId)
        {
            hoverId = hov;
            hoverSince = nowMs;
            tipDelayMs = tipAlpha > 0.4f ? 100.0 : 500.0;
        }
        const bool show = hoverId >= 0 && nowMs - hoverSince >= tipDelayMs;
        if (show) tipId = hoverId;
        tipAlpha = juce::jlimit (0.f, 1.f, tipAlpha + (show ? 1.f : -1.f) * dt / 0.12f);
    }

    specKnob.setValue (proc.apvts.getRawParameterValue ("son")->load() > 0.5f ? 1.0 : 0.0, juce::dontSendNotification);

    updateEq();

    static const char* ft[3] = { "LP", "HP", "BP" };
    const int fi = juce::jlimit (0, 2, (int) proc.apvts.getRawParameterValue ("ftype")->load());
    if (typeBtn.getButtonText() != ft[fi]) typeBtn.setButtonText (ft[fi]);

    static const char* rt[4] = { "ROOM", "PLATE", "HALL", "CATHEDRAL" };
    const int ri = juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("rtype")->load());
    if (spaceBtn.getButtonText() != rt[ri]) spaceBtn.setButtonText (rt[ri]);

    // bypassed effects look dimmed
    const float aF = 0.4f + 0.6f * smooth01 (buttonLit (fonBtn));
    const float aD = 0.4f + 0.6f * smooth01 (buttonLit (donBtn));
    const float aR = 0.4f + 0.6f * smooth01 (buttonLit (ronBtn));
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
    idleFade += ((shownPlaying ? 0.f : 1.f) - idleFade) * 0.2f;
    progress = proc.uiProgress.load();

    const float gp = proc.uiGrain.load();
    if (shownPlaying && gp != lastGrain)
    {
        lastGrain = gp;
        ticks.push_back ({ gp, juce::Random::getSystemRandom().nextFloat(), 1.f });
    }
    for (auto& t : ticks) t.alpha -= 0.07f;
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
                drawMaple (g, fx, top + t * (y0 - top - 4.f), anim * 1.2f + (float) k, 6.5f, cols[k % 4].withAlpha (0.85f * (1.f - t * 0.6f)));
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

void ScattrPanel::fillSky (juce::Graphics& g)
{
    juce::ColourGradient sky (pal.bgTop, 0.f, 0.f, pal.bgBot, 0.f, (float) baseH, false);
    sky.addColour (0.30, pal.bgMid);
    g.setGradientFill (sky);
    g.fillRect (0, 0, baseW, baseH);
}

// Everything behind the cards that never changes between frames: light from the upper right, a counter-light
// low on the left, each theme's weather (sun shafts / aurora / embers / soft morning light), a vignette and
// the mist the vine grows out of. Cached, so it can afford to be smooth.
void ScattrPanel::paintAtmosphere (juce::Graphics& g)
{
    const float W = (float) baseW, H = (float) baseH;
    const auto black = juce::Colours::black;

    {
        juce::ColourGradient sun (pal.glow.withAlpha (0.22f), W * 0.80f, -30.f, pal.glow.withAlpha (0.f), W * 0.80f, 450.f, true);
        sun.addColour (0.35, pal.glow.withAlpha (0.08f));
        g.setGradientFill (sun);
        g.fillRect (0.f, 0.f, W, H);
    }
    {
        juce::ColourGradient low (pal.acc.withAlpha (0.10f), W * 0.06f, H * 0.96f, pal.acc.withAlpha (0.f), W * 0.06f, H * 0.96f - 380.f, true);
        low.addColour (0.4, pal.acc.withAlpha (0.035f));
        g.setGradientFill (low);
        g.fillRect (0.f, 0.f, W, H);
    }

    auto shaft = [&] (float x0, float w, float lean, float alpha, juce::Colour c)
    {
        juce::Path p;
        p.startNewSubPath (x0, -10.f);
        p.lineTo (x0 + w, -10.f);
        p.lineTo (x0 + w + lean + w * 0.8f, H);
        p.lineTo (x0 + lean, H);
        p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (c.withAlpha (alpha * 0.65f), 0.f, 0.f, c.withAlpha (0.f), 0.f, H * 0.85f, false));
        g.fillPath (p);
    };

    switch (curTheme)
    {
        default:
        case 0:   // summer: warm sun shafts through the canopy
            shaft (W * 0.50f,  30.f, -200.f, 0.05f, pal.glow);
            shaft (W * 0.62f,  70.f, -240.f, 0.10f, pal.glow);
            shaft (W * 0.72f,  40.f, -260.f, 0.08f, pal.glow);
            shaft (W * 0.82f, 100.f, -300.f, 0.07f, pal.glow);
            break;

        case 1:   // autumn: low, reddish light and a glow along the horizon
            shaft (W * 0.58f,  60.f, -230.f, 0.07f, pal.acc);
            shaft (W * 0.76f,  90.f, -280.f, 0.06f, pal.glow);
            {
                juce::ColourGradient hz (pal.glow.withAlpha (0.12f), W * 0.5f, H + 10.f, pal.glow.withAlpha (0.f), W * 0.5f, H - 300.f, true);
                g.setGradientFill (hz);
                g.fillRect (0.f, 0.f, W, H);
            }
            break;

        case 2:   // winter: aurora curtains
            for (int k = 0; k < 3; ++k)
            {
                const juce::Colour c = (k == 1) ? pal.acc : pal.glow;
                const float y0 = 26.f + 36.f * (float) k;
                juce::Path ribbon;
                bool first = true;
                for (float x = -10.f; x <= W + 10.f; x += 10.f)
                {
                    const float y = y0 + 26.f * std::sin (x * 0.0085f + (float) k * 1.9f) + 12.f * std::sin (x * 0.021f + (float) k);
                    if (first) { ribbon.startNewSubPath (x, y); first = false; }
                    else         ribbon.lineTo (x, y);
                }
                for (float x = W + 10.f; x >= -10.f; x -= 10.f)
                    ribbon.lineTo (x, y0 + 150.f + 40.f * std::sin (x * 0.006f + (float) k * 1.1f + 1.f));
                ribbon.closeSubPath();

                juce::ColourGradient gr (c.withAlpha (0.f), 0.f, y0 - 30.f, c.withAlpha (0.f), 0.f, y0 + 200.f, false);
                gr.addColour (0.18, c.withAlpha (0.14f));
                gr.addColour (0.55, c.withAlpha (0.045f));
                g.setGradientFill (gr);
                g.fillPath (ribbon);
            }
            break;

        case 3:   // spring: wide, soft morning light
            shaft (W * 0.46f, 110.f, -220.f, 0.05f, pal.glow);
            shaft (W * 0.64f,  70.f, -250.f, 0.07f, pal.glow);
            shaft (W * 0.80f, 130.f, -300.f, 0.06f, pal.amber);
            break;
    }

    {   // vignette
        juce::ColourGradient vg (black.withAlpha (0.f), W * 0.5f, H * 0.46f, black.withAlpha (0.40f), 0.f, 0.f, true);
        vg.addColour (0.55, black.withAlpha (0.f));
        vg.addColour (0.80, black.withAlpha (0.22f));
        g.setGradientFill (vg);
        g.fillRect (0.f, 0.f, W, H);
    }
    {   // mist along the bottom, lit from within
        g.setGradientFill (juce::ColourGradient (black.withAlpha (0.f), 0.f, H - 130.f, black.withAlpha (0.42f), 0.f, H, false));
        g.fillRect (0.f, H - 130.f, W, 130.f);
        juce::ColourGradient gl (pal.glow.withAlpha (0.06f), W * 0.5f, H, pal.glow.withAlpha (0.f), W * 0.5f, H - 150.f, true);
        g.setGradientFill (gl);
        g.fillRect (0.f, H - 150.f, W, 150.f);
    }
}

// Soft light behind the title letters (the letters themselves are drawn live, on top)
void ScattrPanel::paintTitleGlow (juce::Graphics& g)
{
    const float twoPi = juce::MathConstants<float>::twoPi;
    g.setFont (titleFont (50.f));
    for (int layer = 0; layer < 3; ++layer)
    {
        const float rr = 6.f - 2.f * (float) layer;   // 6, 4, 2 px
        g.setColour (pal.glow.withAlpha (0.03f + 0.02f * (float) layer));
        for (int k = 0; k < 12; ++k)
        {
            const float ang = (float) k * twoPi / 12.f;
            g.drawText ("Nature", juce::roundToInt (titleX() + std::cos (ang) * rr), juce::roundToInt (14.f + std::sin (ang) * rr),
                        300, 50, juce::Justification::centredLeft);
        }
    }
}

// Slow themed floaters inside the waveform window while nothing is playing: leaves and fireflies (summer),
// maple leaves (autumn), snowflakes (winter), blossom petals (spring). Purely a function of `anim`, no state.
void ScattrPanel::paintIdleDrift (juce::Graphics& g, juce::Rectangle<float> area)
{
    if (idleFade < 0.01f) return;

    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip;
    clip.addRoundedRectangle (area.reduced (1.f), 9.f);
    g.reduceClipRegion (clip);

    // breathing light: a soft accent glow that swells and fades with the same slow rhythm as the waveform
    const float breath = 0.5f + 0.5f * std::sin (anim * 1.1f);
    g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha ((0.04f + 0.11f * breath) * idleFade), area.getCentreX(), area.getCentreY(),
                                             pal.acc.withAlpha (0.f), area.getX(), area.getCentreY(), true));
    g.fillRect (area);

    const float pi = juce::MathConstants<float>::pi;
    const juce::Colour mapleCols[4] = { pal.acc, pal.amber, pal.accDark.brighter (0.5f), pal.grass };

    for (int k = 0; k < 16; ++k)
    {
        const float depth = 0.6f + 0.2f * (float) (k % 3);                 // 0.6 .. 1.0: far ones are slower and smaller
        const float t = std::fmod (anim * 0.035f * depth + (float) k * 0.6180339f, 1.f);
        const float baseX = std::fmod ((float) k * 0.7548777f, 1.f);
        const float x = area.getX() + area.getWidth() * baseX + 18.f * std::sin (anim * 0.7f * depth + (float) k * 1.9f);
        const float y = area.getY() - 8.f + t * (area.getHeight() + 16.f);
        const float a = idleFade * std::sin (t * pi) * 0.85f;              // fades in at the top, out at the bottom
        const float rot = anim * (0.5f + 0.3f * depth) * ((k % 2) != 0 ? 1.f : -1.f) + (float) k;

        switch (curTheme)
        {
            default:
            case 0:   // summer
                if (k % 4 == 0)   // firefly
                {
                    const float fy = area.getY() + area.getHeight() * (0.2f + 0.6f * baseX) + 8.f * std::sin (anim * 0.9f + (float) k);
                    const float pulse = 0.5f + 0.5f * std::sin (anim * 1.6f + (float) k * 2.1f);
                    g.setGradientFill (juce::ColourGradient (pal.amber.withAlpha (0.6f * idleFade * pulse), x, fy,
                                                             pal.amber.withAlpha (0.f), x + 8.f, fy, true));
                    g.fillEllipse (x - 8.f, fy - 8.f, 16.f, 16.f);
                    g.setColour (juce::Colours::white.withAlpha (0.8f * idleFade * pulse));
                    g.fillEllipse (x - 1.2f, fy - 1.2f, 2.4f, 2.4f);
                }
                else
                    drawLeaf (g, x, y, rot, 9.f + 5.f * depth, (k % 2 == 0 ? pal.grass : pal.grassDark.brighter (0.25f)).withAlpha (a));
                break;

            case 1:   // autumn
                drawMaple (g, x, y, rot, 8.f + 4.f * depth, mapleCols[k % 4].withAlpha (a));
                break;

            case 2:   // winter
                drawSnowflake (g, x, y, 3.f + 3.f * depth, pal.ink, a);
                break;

            case 3:   // spring
            {
                juce::Path p;
                p.addEllipse (-3.5f * depth, -2.4f * depth, 7.f * depth, 4.8f * depth);
                g.setColour (pal.acc.brighter (0.5f).withAlpha (a));
                g.fillPath (p, juce::AffineTransform::rotation (rot).translated (x, y));
                break;
            }
        }
    }
}

// A glass card. The light and shadow around it are real blurs, kept outside the card so the glass stays
// see-through: the atmosphere shows through a dark tint, with a warm pool of light along the bottom edge and
// a rim that runs from white at the top through the accent colour to the theme light at the bottom.
// (Drawn into the cache, so the blurs cost nothing per frame.)
void ScattrPanel::drawGlass (juce::Graphics& g, juce::Rectangle<float> r, float radius, float tint, bool inset)
{
    const auto white = juce::Colours::white, black = juce::Colours::black;
    juce::Path card;
    card.addRoundedRectangle (r, radius);

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path outside;
        outside.addRectangle (r.expanded (60.f));
        outside.addRoundedRectangle (r, radius);
        outside.setUsingNonZeroWinding (false);   // even-odd: everything except the card itself
        g.reduceClipRegion (outside);

        juce::DropShadow (black.withAlpha (0.42f), 12, juce::Point<int> (0, 5)).drawForPath (g, card);
        juce::DropShadow (pal.glow.withAlpha (inset ? 0.08f : 0.15f), inset ? 10 : 18, juce::Point<int> (0, 0)).drawForPath (g, card);
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (card);

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
            g.setGradientFill (juce::ColourGradient (white.withAlpha (0.07f), 0.f, r.getY(),
                                                     white.withAlpha (0.f), 0.f, r.getY() + r.getHeight() * 0.55f, false));
            g.fillRect (r);
        }

        // a hot spot of light in the top-left corner
        g.setGradientFill (juce::ColourGradient (white.withAlpha (0.055f), r.getX() + 36.f, r.getY() + 4.f,
                                                 white.withAlpha (0.f), r.getX() + 220.f, r.getY() + 4.f, true));
        g.fillRect (r);

        // warm light pooling along the bottom edge
        g.setGradientFill (juce::ColourGradient (pal.glow.withAlpha (0.f), 0.f, r.getBottom() - r.getHeight() * 0.5f,
                                                 pal.glow.withAlpha (inset ? 0.04f : 0.07f), 0.f, r.getBottom(), false));
        g.fillRect (r);
    }

    juce::ColourGradient rim (white.withAlpha (inset ? 0.08f : 0.24f), 0.f, r.getY(),
                              pal.glow.withAlpha (0.15f), 0.f, r.getBottom(), false);
    rim.addColour (0.5, pal.acc.withAlpha (0.12f));
    g.setGradientFill (rim);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.2f);

    if (! inset)   // rim light along the top edge
    {
        g.setColour (white.withAlpha (0.11f));
        g.drawLine (r.getX() + radius, r.getY() + 1.6f, r.getRight() - radius, r.getY() + 1.6f, 1.f);
    }
}

void ScattrPanel::drawGroup (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawGlass (g, r, 12.f, 0.50f);
}

void ScattrPanel::drawGroupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
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
    paintIdleDrift (g, wr);   // fades out while a sample plays

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

        const float al = shownPlaying ? 0.85f : pinned >= 0 ? 0.65f : 0.40f;
        for (int pass = 0; pass < 2; ++pass)   // pass 0 = soft glow, pass 1 = the waveform itself
        {
            if (pass == 0)
                g.setColour (pal.acc.withAlpha (al * 0.14f));
            else
                g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (al), 0.f, wr.getY(),
                                                         pal.accDark.withAlpha (al), 0.f, wr.getBottom(), false));
            const float grow = pass == 0 ? 3.f : 0.f;
            for (int i = 0; i < n; ++i)
            {
                // idle "breathing": the whole shape swells and relaxes, with a slow ripple travelling along it
                const float mod = 1.f + idleFade * (0.045f * std::sin (anim * 1.1f) + 0.03f * std::sin (anim * 1.1f - (float) i * 0.035f));
                const float x = inner.getX() + inner.getWidth() * (float) i / (float) n;
                g.fillRect (x, cy - pk[(size_t) i].second * hh * mod - grow, inner.getWidth() / (float) n + 0.5f,
                            juce::jmax (1.5f, (pk[(size_t) i].second - pk[(size_t) i].first) * hh * mod) + grow * 2.f);
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
    g.setColour (pal.dim.withAlpha (1.f - 0.3f * idleFade * (0.5f + 0.5f * std::sin (anim * 1.1f))));
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

// ---------------------------------------------------------------- cached layer

void ScattrPanel::paintStatic (juce::Graphics& g)
{
    fillSky (g);
    paintAtmosphere (g);
    paintTitleGlow (g);

    drawGlass (g, eqRect(), 9.f, 0.60f, true);

    if (tab == 0)
    {
        drawGlass (g, waveRect(), 10.f, 0.68f, true);
        drawGlass (g, sampleRect(), 10.f, 0.56f);
        drawGroup (g, grainPanel());
        drawGroup (g, motionPanel());
    }
    else
    {
        for (int k = 0; k < 3; ++k) drawGroup (g, fxPanel (k));
    }
}

void ScattrPanel::rebuildCache()
{
    cache = juce::Image (juce::Image::RGB, juce::roundToInt ((float) baseW * cacheScale),
                         juce::roundToInt ((float) baseH * cacheScale), true);
    juce::Graphics cg (cache);
    cg.addTransform (juce::AffineTransform::scale (cacheScale));
    const auto live = pal;   // the backdrop is always painted in the destination palette; the crossfade does the blending
    pal = targetPal;
    paintStatic (cg);
    pal = live;
    cacheTheme = curTheme;
    cacheTab = tab;
}

// ---------------------------------------------------------------- live layer

// Slow light particles over the whole window: pollen and fireflies (summer), embers rising (autumn),
// sparkles settling (winter), drifting pollen (spring). Purely a function of `anim`.
void ScattrPanel::paintMotes (juce::Graphics& g)
{
    const float W = (float) baseW, H = (float) baseH;
    const bool falls = (curTheme == 2 || curTheme == 3);
    const float pi = juce::MathConstants<float>::pi;

    for (int k = 0; k < 16; ++k)
    {
        const float speed = 0.010f + 0.006f * (float) (k % 4);
        const float t = std::fmod (anim * speed + (float) k * 0.37f, 1.f);
        const float x = std::fmod ((float) k * 1.0506f, 1.f) * W + 26.f * std::sin (anim * 0.25f + (float) k * 1.3f);
        const float y = falls ? -10.f + t * (H + 20.f) : H + 10.f - t * (H + 20.f);
        const float r = 3.f + 5.f * std::fmod ((float) k * 0.413f, 1.f);
        const float twinkle = 0.5f + 0.5f * std::sin (anim * 1.3f + (float) k * 2.1f);
        const float a = std::sin (t * pi) * (0.06f + 0.06f * twinkle);
        const auto col = (k % 2 == 0) ? pal.glow : pal.amber;

        g.setGradientFill (juce::ColourGradient (col.withAlpha (a), x, y, col.withAlpha (0.f), x + r * 2.f, y, true));
        g.fillEllipse (x - r * 2.f, y - r * 2.f, r * 4.f, r * 4.f);
        g.setColour (juce::Colours::white.withAlpha (a * 0.9f));
        g.fillEllipse (x - 0.9f, y - 0.9f, 1.8f, 1.8f);
    }
}

// A thin highlight that slowly travels along the top edge of every card, then rests for a while
void ScattrPanel::paintGlints (juce::Graphics& g)
{
    juce::Rectangle<float> cards[5];
    int n = 0;
    cards[n++] = eqRect();
    if (tab == 0) { cards[n++] = waveRect(); cards[n++] = sampleRect(); cards[n++] = grainPanel(); cards[n++] = motionPanel(); }
    else          { cards[n++] = fxPanel (0); cards[n++] = fxPanel (1); cards[n++] = fxPanel (2); }

    for (int i = 0; i < n; ++i)
    {
        const auto r = cards[i];
        const float p = std::fmod (anim * 0.16f + (float) i * 0.37f, 1.8f);
        if (p > 1.f) continue;

        const float gx = r.getX() + p * r.getWidth(), hw = 60.f;
        const float a = 0.22f * std::sin (p * juce::MathConstants<float>::pi);

        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.reduced (8.f, 0.f).toNearestInt());
        juce::ColourGradient gr (juce::Colours::white.withAlpha (0.f), gx - hw, 0.f, juce::Colours::white.withAlpha (0.f), gx + hw, 0.f, false);
        gr.addColour (0.5, juce::Colours::white.withAlpha (a));
        g.setGradientFill (gr);
        g.fillRect (gx - hw, r.getY() + 1.f, hw * 2.f, 1.6f);
    }
}

void ScattrPanel::paintLabels (juce::Graphics& g)
{
    // title: soft shadow, then letters that warm from ivory to the highlight colour (the glow is in the cache)
    g.setFont (titleFont (50.f));
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawText ("Nature", juce::roundToInt (titleX() + 1.f), 16, 300, 50, juce::Justification::centredLeft);
    g.setGradientFill (juce::ColourGradient (pal.ink, 0.f, 18.f, pal.ink.interpolatedWith (pal.amber, 0.55f), 0.f, 60.f, false));
    g.drawText ("Nature", juce::roundToInt (titleX()), 14, 300, 50, juce::Justification::centredLeft);

    if (tab == 0)
    {
        drawGroupTitle (g, grainPanel(),  "grove: grains");
        drawGroupTitle (g, motionPanel(), "bloom: motion");
    }
    else
    {
        drawGroupTitle (g, fxPanel (0), "canopy: filter");
        drawGroupTitle (g, fxPanel (1), "canyon: delay");
        drawGroupTitle (g, fxPanel (2), "cavern: reverb");

        const int sw = (int) ((fxPanel (0).getWidth() - 24.f) / 5.f);
        g.setColour (pal.dim);
        g.setFont (serif (14.f, true));
        g.drawText ("mode",  (int) fxPanel (0).getX() + 12 + 4 * sw, (int) fxPanel (0).getY() + 40, sw, 16, juce::Justification::centred);
        g.drawText ("space", (int) fxPanel (2).getX() + 12 + 4 * sw, (int) fxPanel (2).getY() + 40, sw, 16, juce::Justification::centred);
    }
}

// ---------------------------------------------------------------- equalizer

// Reads the last 2048 output samples, takes their spectrum and folds it into log-spaced bands (50 Hz - 14 kHz).
// The display auto-ranges against the loudest recent band so it moves nicely at any volume.
void ScattrPanel::updateEq()
{
    const int w = proc.scopeW.load (std::memory_order_acquire);
    scopeStale = (w == lastScopeW) ? scopeStale + 1 : 0;   // host stopped calling the plugin: let the bars settle
    lastScopeW = w;

    std::array<float, (size_t) eqBands> target {};
    if (scopeStale < 4)
    {
        constexpr int mask = ScattrProcessor::scopeSize - 1;
        for (int i = 0; i < eqN; ++i)
            fftData[(size_t) i] = proc.scopeBuf[(w - eqN + i) & mask].load (std::memory_order_relaxed);
        std::fill (fftData.begin() + eqN, fftData.end(), 0.f);

        window.multiplyWithWindowingTable (fftData.data(), (size_t) eqN);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float binHz = proc.uiSampleRate.load() / (float) eqN;
        const float fMin = 50.f, fMax = 14000.f;
        std::array<float, (size_t) eqBands> db {};
        float frameMax = -200.f;

        for (int b = 0; b < eqBands; ++b)
        {
            const float f0 = fMin * std::pow (fMax / fMin, (float) b / (float) eqBands);
            const float f1 = fMin * std::pow (fMax / fMin, (float) (b + 1) / (float) eqBands);
            const float lo = f0 / binHz, hi = f1 / binHz;

            float m = 0.f;
            if (hi - lo < 1.f)   // low bands are narrower than one bin: interpolate between neighbours
            {
                const float cb = 0.5f * (lo + hi);
                const int i0 = juce::jlimit (0, eqN / 2 - 2, (int) cb);
                const float fr = cb - (float) i0;
                m = fftData[(size_t) i0] * (1.f - fr) + fftData[(size_t) i0 + 1] * fr;
            }
            else
            {
                for (int i = (int) std::ceil (lo); i <= (int) hi && i < eqN / 2; ++i)
                    m = juce::jmax (m, fftData[(size_t) i]);
            }

            const float amp = m / ((float) eqN * 0.25f);   // a full-scale sine reads 1.0
            const float tilt = juce::jlimit (-4.f, 12.f, 3.f * std::log2 (std::sqrt (f0 * f1) / 300.f));   // highs are quieter in most material
            db[(size_t) b] = 20.f * std::log10 (amp + 1.0e-7f) + tilt;
            frameMax = juce::jmax (frameMax, db[(size_t) b]);
        }

        eqRef = juce::jmax (eqRef - 0.15f, frameMax, -32.f);
        for (int b = 0; b < eqBands; ++b)
            target[(size_t) b] = juce::jlimit (0.f, 1.f, (db[(size_t) b] - (eqRef - 36.f)) / 36.f);
    }

    for (size_t b = 0; b < (size_t) eqBands; ++b)
    {
        eqLevel[b] += (target[b] - eqLevel[b]) * (target[b] > eqLevel[b] ? 0.5f : 0.12f);
        eqPeak[b] = juce::jmax (eqPeak[b] - 0.014f, eqLevel[b]);
    }
}

void ScattrPanel::paintEq (juce::Graphics& g)
{
    const auto b = eqRect().reduced (8.f, 6.f);
    const float gap = 1.8f, bw = (b.getWidth() - gap * (float) (eqBands - 1)) / (float) eqBands;
    const float baseY = b.getBottom(), maxH = b.getHeight();

    for (int i = 0; i < eqBands; ++i)
    {
        const float x = b.getX() + (float) i * (bw + gap);
        const float lv = eqLevel[(size_t) i];
        const float idle = 0.5f + 0.5f * std::sin (anim * 1.3f + (float) i * 0.55f);   // keeps the bars faintly alive in silence
        const float h = 2.f + (maxH - 2.f) * lv + 1.4f * idle * (1.f - lv);
        const juce::Rectangle<float> bar (x, baseY - h, bw, h);

        // soft halo at the tip, brighter the louder the band
        const float tx = bar.getCentreX(), ty = bar.getY();
        g.setGradientFill (juce::ColourGradient (pal.glow.withAlpha (0.10f + 0.50f * lv), tx, ty,
                                                 pal.glow.withAlpha (0.f), tx + 7.f, ty, true));
        g.fillEllipse (tx - 7.f, ty - 7.f, 14.f, 14.f);

        g.setGradientFill (juce::ColourGradient (pal.acc.interpolatedWith (pal.amber, lv), 0.f, bar.getY(),
                                                 pal.accDark, 0.f, baseY, false));
        g.fillRoundedRectangle (bar, bw * 0.5f);

        if (eqPeak[(size_t) i] > lv + 0.04f)   // falling peak cap
        {
            const float py = baseY - (2.f + (maxH - 2.f) * eqPeak[(size_t) i]) - 3.f;
            g.setColour (pal.amber.withAlpha (0.85f));
            g.fillRoundedRectangle (x, py, bw, 1.6f, 0.8f);
        }
    }
}

// ---------------------------------------------------------------- glows

// A soft bloom behind every knob that grows with its value, plus a wider halo around the tip of its arc.
// Painted here (behind the sliders) because a slider cannot draw outside its own bounds.
void ScattrPanel::paintKnobGlows (juce::Graphics& g)
{
    const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;

    std::array<juce::Slider*, 23> all {};
    for (size_t i = 0; i < knobs.size(); ++i) all[i] = &knobs[i].slider;
    all[22] = &specKnob;

    for (auto* sp : all)
    {
        auto& s = *sp;
        if (! s.isVisible()) continue;

        const float al = s.getAlpha();
        const float pos = (float) s.valueToProportionOfLength (s.getValue());
        const auto rb = s.getLookAndFeel().getSliderLayout (s).sliderBounds.toFloat();
        const auto c = rb.getCentre() + s.getPosition().toFloat();
        const float rad = (juce::jmin (rb.getWidth(), rb.getHeight()) - 2.f * Scat::knobPad) * 0.5f;
        const bool hot = s.isMouseOverOrDragging();
        const float breathe = 0.9f + 0.1f * std::sin (anim * 1.7f + c.x * 0.03f);

        const float R = rad * 2.3f;
        const float strength = (0.03f + 0.09f * pos + (hot ? 0.04f : 0.f)) * breathe * al;
        juce::ColourGradient amb (pal.acc.withAlpha (strength), c.x, c.y, pal.acc.withAlpha (0.f), c.x + R, c.y, true);
        amb.addColour (0.45, pal.acc.withAlpha (strength * 0.35f));
        g.setGradientFill (amb);
        g.fillEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f);

        if (pos > 0.001f)
        {
            const float ang = a0 + pos * (a1 - a0), ar = rad + 4.f;
            const float bx = c.x + std::sin (ang) * ar, by = c.y - std::cos (ang) * ar;
            const float br = hot ? 20.f : 16.f;
            g.setGradientFill (juce::ColourGradient (pal.glow.withAlpha (0.26f * al), bx, by, pal.glow.withAlpha (0.f), bx + br, by, true));
            g.fillEllipse (bx - br, by - br, br * 2.f, br * 2.f);
        }
    }
}

// The wider glow behind an on/off button. It swells as the button switches (see drawKeyBody for the key itself).
void ScattrPanel::paintToggleGlows (juce::Graphics& g)
{
    for (auto* b : { &droneBtn, &tuneBtn, &fonBtn, &donBtn, &ronBtn })
    {
        if (! b->isVisible()) continue;

        const float lit = buttonLit (*b), e = smooth01 (lit);
        const float flip = (lit > 0.001f && lit < 0.999f) ? std::sin (juce::MathConstants<float>::pi * lit) : 0.f;
        if (e < 0.01f && flip < 0.01f) continue;

        const auto r = b->getBounds().toFloat().reduced (2.f);
        for (int i = 6; i >= 1; --i)
        {
            const float t = (float) i / 6.f;   // 1 = outermost layer
            g.setColour (pal.acc.withAlpha ((0.015f * e + 0.03f * flip) * (1.f - 0.6f * t)));
            g.fillRoundedRectangle (r.expanded (2.f + t * (7.f + 6.f * flip)), 10.f + t * 6.f);
        }
    }
}

// ---------------------------------------------------------------- hover tips

int ScattrPanel::hoveredControl() const
{
    if (spectral != nullptr && spectral->isVisible()) return -1;

    for (size_t i = 0; i < knobs.size(); ++i)
        if (knobs[i].slider.isVisible() && (knobs[i].slider.isMouseOver (true) || knobs[i].label.isMouseOver (false)))
            return (int) i;

    if (specKnob.isVisible() && (specKnob.isMouseOver (true) || specLabel.isMouseOver (false)))
        return 29;

    const juce::Component* extra[7] = { &droneBtn, &tuneBtn, &typeBtn, &spaceBtn, &fonBtn, &donBtn, &ronBtn };
    for (int k = 0; k < 7; ++k)
        if (extra[k]->isVisible() && extra[k]->isMouseOver (true))
            return 22 + k;

    return -1;
}

// A small field note that rises into the footer, clear of every control
void ScattrPanel::paintTip (juce::Graphics& g)
{
    if (tipAlpha < 0.01f || tipId < 0 || tipId >= 30) return;

    const float a = smooth01 (tipAlpha);
    const auto black = juce::Colours::black;
    const juce::Rectangle<float> r (130.f, 519.f + (1.f - a) * 6.f, 520.f, 42.f);

    for (int i = 3; i >= 1; --i)
    {
        g.setColour (black.withAlpha (0.12f * a));
        g.fillRoundedRectangle (r.expanded ((float) i * 2.f).translated (0.f, 3.f), 12.f + (float) i * 2.f);
    }

    g.setGradientFill (juce::ColourGradient (pal.moss.withAlpha (0.95f * a), 0.f, r.getY(), pal.soil.withAlpha (0.97f * a), 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 11.f);
    g.setColour (pal.glow.withAlpha (0.10f * a));
    g.fillRoundedRectangle (r.reduced (1.f).withHeight (r.getHeight() * 0.45f), 10.f);
    g.setColour (pal.acc.withAlpha (0.55f * a));
    g.drawRoundedRectangle (r.reduced (0.5f), 11.f, 1.2f);

    drawLeaf (g, r.getX() + 24.f, r.getBottom() - 9.f, -0.5f, 14.f, pal.grassDark.brighter (0.2f).withAlpha (a));
    drawLeaf (g, r.getX() + 24.f, r.getBottom() - 9.f,  0.6f, 20.f, pal.grass.withAlpha (a));

    auto ti = r.toNearestInt().withTrimmedLeft (48).withTrimmedRight (14).reduced (0, 4);
    const auto nameArea = ti.removeFromTop (17);
    g.setFont (serif (14.f, true));
    g.setColour (pal.glow.brighter (0.4f).withAlpha (a));
    g.drawText (juce::String (kTips[tipId].name), nameArea, juce::Justification::centredLeft, true);
    g.setFont (serif (13.f));
    g.setColour (pal.ink.withAlpha (0.92f * a));
    g.drawText (juce::String (kTips[tipId].text), ti, juce::Justification::centredLeft, true);
}

// ---------------------------------------------------------------- main paint

void ScattrPanel::paint (juce::Graphics& g)
{
    if (cache.isNull() || cacheTheme != curTheme || cacheTab != tab) rebuildCache();
    if (themeT < 1.f && prevCache.isValid())
    {
        g.drawImage (prevCache, 0, 0, baseW, baseH, 0, 0, prevCache.getWidth(), prevCache.getHeight());
        g.setOpacity (smooth01 (themeT));
    }
    g.drawImage (cache, 0, 0, baseW, baseH, 0, 0, cache.getWidth(), cache.getHeight());
    g.setOpacity (1.f);

    paintMotes (g);
    paintGlints (g);
    paintLabels (g);

    // little sprout beside the title
    {
        const float sw = 0.08f * std::sin (anim * 0.9f), bx = sproutX();
        drawLeaf (g, bx, 52.f, 0.7f + sw, 20.f, pal.grassDark.brighter (0.2f));
        drawLeaf (g, bx, 52.f, -0.5f + sw, 16.f, pal.grass.darker (0.3f));
        drawLeaf (g, bx, 52.f, 0.1f + sw, 26.f, pal.grass);

        const float fx = bx + std::sin (0.1f + sw) * 28.f, fy = 52.f - 28.f;
        g.setGradientFill (juce::ColourGradient (pal.glow.withAlpha (0.6f), fx, fy, pal.glow.withAlpha (0.f), fx + 18.f, fy, true));
        g.fillEllipse (fx - 18.f, fy - 18.f, 36.f, 36.f);
        drawFlower (g, fx, fy, 5.5f, pal.acc, pal.amber);
    }

    paintEq (g);

    if (tab == 0)
    {
        paintWave (g);
        paintSamples (g);
    }

    paintKnobGlows (g);
    paintToggleGlows (g);

    drawVine (g);   // lives in its own footer strip, clear of every control
    paintTip (g);
}

// ---------------------------------------------------------------- spectral compressor window

namespace
{
// A cheap, soft blur: shrink the snapshot, run a few box-blur passes on the small copy, and let the
// resampler smooth it back up when it is drawn. Done once per opening, so cost is irrelevant.
juce::Image blurSnapshot (const juce::Image& src, int w, int h, int radius, int passes)
{
    juce::Image img = src.rescaled (w, h, juce::Graphics::highResamplingQuality).convertedToFormat (juce::Image::RGB);
    img.duplicateIfShared();

    std::vector<float> a ((size_t) w * (size_t) h * 3u), b (a.size());
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const auto* px = bd.getPixelPointer (x, y);
                for (int ch = 0; ch < 3; ++ch) a[((size_t) y * (size_t) w + (size_t) x) * 3u + (size_t) ch] = (float) px[ch];
            }
    }

    const float inv = 1.f / (float) (2 * radius + 1);
    for (int pass = 0; pass < passes; ++pass)
    {
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                for (int ch = 0; ch < 3; ++ch)
                {
                    float s = 0.f;
                    for (int d = -radius; d <= radius; ++d)
                        s += a[((size_t) y * (size_t) w + (size_t) juce::jlimit (0, w - 1, x + d)) * 3u + (size_t) ch];
                    b[((size_t) y * (size_t) w + (size_t) x) * 3u + (size_t) ch] = s * inv;
                }
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                for (int ch = 0; ch < 3; ++ch)
                {
                    float s = 0.f;
                    for (int d = -radius; d <= radius; ++d)
                        s += b[((size_t) juce::jlimit (0, h - 1, y + d) * (size_t) w + (size_t) x) * 3u + (size_t) ch];
                    a[((size_t) y * (size_t) w + (size_t) x) * 3u + (size_t) ch] = s * inv;
                }
    }

    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                auto* px = bd.getPixelPointer (x, y);
                for (int ch = 0; ch < 3; ++ch)
                    px[ch] = (juce::uint8) juce::jlimit (0, 255, juce::roundToInt (a[((size_t) y * (size_t) w + (size_t) x) * 3u + (size_t) ch]));
            }
    }
    return img;
}

// readouts for the window's knobs
double plainNumber (const juce::String& t) { return t.retainCharacters ("-0123456789.").getDoubleValue(); }

struct SpecDef
{
    const char* id;
    const char* text;
    juce::String (*show) (double);
    double (*read) (const juce::String&);
};

const SpecDef kSpecDefs[12] = {
    // globals
    { "smix",   "mix",        [] (double v) { return juce::String (juce::roundToInt ((float) (v * 100.0))) + "%"; },
                              [] (const juce::String& t) { return plainNumber (t) / 100.0; } },
    { "sgain",  "output",     [] (double v) { return juce::String (v, 1) + " dB"; },  plainNumber },
    { "satk",   "attack",     [] (double v) { return juce::String (v, 1) + " ms"; },  plainNumber },
    { "srel",   "release",    [] (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, 2) + " s" : juce::String (juce::roundToInt ((float) v)) + " ms"; },
                              [] (const juce::String& t) { const double n = plainNumber (t); return (t.containsIgnoreCase ("s") && ! t.containsIgnoreCase ("ms")) ? n * 1000.0 : n; } },
    // threshold
    { "sthr",   "threshold",  [] (double v) { return juce::String (v, 1) + " dB"; },  plainNumber },
    { "scen",   "center",     [] (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, 2) + " kHz" : juce::String (juce::roundToInt ((float) v)) + " Hz"; },
                              [] (const juce::String& t) { const double n = plainNumber (t); return t.containsIgnoreCase ("k") ? n * 1000.0 : n; } },
    { "sslope", "slope",      [] (double v) { return juce::String (v, 2) + " dB/oct"; }, plainNumber },
    { "scurve", "curve",      [] (double v) { return juce::String (v, 2) + " dB/oct" + juce::String (juce::CharPointer_UTF8 ("\xc2\xb2")); }, plainNumber },
    // dynamics
    { "sratio",   "down ratio", [] (double v) { return juce::String (v, 1) + ":1"; }, plainNumber },
    { "supratio", "up ratio",   [] (double v) { return juce::String (v, 1) + ":1"; }, plainNumber },
    { "supoff",   "up offset",  [] (double v) { return juce::String (v, 1) + " dB"; }, plainNumber },
    { "sknee",    "knee",       [] (double v) { return juce::String (v, 1) + " dB"; }, plainNumber }
};

constexpr float kFloorDb = -100.f;   // bottom of the analyzer
}

SpectralOverlay::SpectralOverlay (ScattrPanel& p, ScattrProcessor& pr) : panel (p), proc (pr)
{
    setOpaque (true);                 // the window paints every pixel, so the panel underneath need not repaint
    setWantsKeyboardFocus (true);
    setVisible (false);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        const auto& d = kSpecDefs[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 16);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (k.slider);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, k.slider);
        k.slider.textFromValueFunction = d.show;
        k.slider.valueFromTextFunction = d.read;
        k.slider.updateText();

        k.label.setText (d.text, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (Scat::serif (14.f, true));
        addAndMakeVisible (k.label);
    }

    addAndMakeVisible (onBtn);
    onAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "son", onBtn);

    addAndMakeVisible (winBtn);
    winBtn.onClick = [this]
    {
        auto* par = proc.apvts.getParameter ("swin");
        const int cur = (int) proc.apvts.getRawParameterValue ("swin")->load();
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) ((cur + 1) % 4)));
        par->endChangeGesture();
    };

    addAndMakeVisible (doneBtn);
    doneBtn.onClick = [this] { close(); };

    for (auto& v : shIn)  v = kFloorDb;
    for (auto& v : shOut) v = kFloorDb;
}

SpectralOverlay::~SpectralOverlay()
{
    stopTimer();
}

void SpectralOverlay::open (const juce::Image& snapshot)
{
    sharp = snapshot;
    buildLayers();

    progress = 0.f;
    closing = false;
    lastMs = juce::Time::getMillisecondCounterHiRes();
    stale = 100;
    for (auto& v : shIn)  v = kFloorDb;
    for (auto& v : shOut) v = kFloorDb;

    for (auto* c : getChildren()) c->setAlpha (0.f);
    setBounds (panel.getLocalBounds());
    setVisible (true);
    toFront (false);
    grabKeyboardFocus();
    startTimerHz (45);
}

void SpectralOverlay::close()
{
    if (! isVisible() || closing) return;
    closing = true;
}

bool SpectralOverlay::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { close(); return true; }
    return true;   // swallow everything else so keys don't leak to the host while the window is up
}

void SpectralOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! card().contains (e.position)) close();   // click on the blurred backdrop = dismiss
}

void SpectralOverlay::resized()
{
    // knob grid: three rows of four, the same rhythm as the main panel
    static const size_t order[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int r = 0; r < 3; ++r)
    {
        const auto gr = group (r);
        const int sw = (int) ((gr.getWidth() - 24.f) / 4.f);
        for (int c = 0; c < 4; ++c)
        {
            auto& k = knobs[order[r * 4 + c]];
            const int x = (int) gr.getX() + 12 + c * sw, y = (int) gr.getY();
            k.label.setBounds (x, y + 22, sw, 16);
            k.slider.setBounds (x, y + 38, sw, 88);
        }
    }

    const auto cd = card();
    doneBtn.setBounds ((int) cd.getRight() - 16 - 76, (int) cd.getY() + 12, 76, 30);
    winBtn.setBounds  (doneBtn.getX() - 8 - 118, (int) cd.getY() + 12, 118, 30);
    onBtn.setBounds   (winBtn.getX() - 8 - 64, (int) cd.getY() + 12, 64, 30);
}

SpectralCompressor::Params SpectralOverlay::readParams() const
{
    auto v = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    SpectralCompressor::Params p;
    p.mix = v ("smix");        p.outGainDb = v ("sgain");   p.thrDb = v ("sthr");      p.centerHz = v ("scen");
    p.slope = v ("sslope");    p.curve = v ("scurve");      p.downRatio = v ("sratio"); p.upRatio = v ("supratio");
    p.upOffsetDb = v ("supoff"); p.kneeDb = v ("sknee");    p.attackMs = v ("satk");    p.releaseMs = v ("srel");
    return p;
}

void SpectralOverlay::timerCallback()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit (0.f, 0.1f, (float) ((nowMs - lastMs) * 0.001));
    lastMs = nowMs;
    anim += dt;

    // fade the window in and out
    const float before = progress;
    progress = juce::jlimit (0.f, 1.f, progress + (closing ? -1.f : 1.f) * dt / 0.22f);
    if (progress != before || progress < 1.f)
    {
        const float a = smooth01 (progress);
        for (auto* c : getChildren()) c->setAlpha (a >= 0.999f ? 1.f : a);
    }
    if (closing && progress <= 0.f)
    {
        stopTimer();
        closing = false;
        setVisible (false);
        sharp = {};
        backdrop = {};
        cardImg = {};
        panel.repaint();
        return;
    }

    // the ON button glides between its looks like the panel's buttons do
    {
        const float target = onBtn.getToggleState() ? 1.f : 0.f;
        float v = buttonLit (onBtn);
        const float step = dt / 0.2f;
        v = v < target ? juce::jmin (target, v + step) : juce::jmax (target, v - step);
        onBtn.getProperties().set ("lit", v);
    }

    static const char* wn[4] = { "512", "1024", "2048", "4096" };
    const juce::String wt = juce::String ("WINDOW ") + wn[juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("swin")->load())];
    if (winBtn.getButtonText() != wt) winBtn.setButtonText (wt);

    // analyzer: follow the audio thread's frames; fall away when nothing is being processed
    const unsigned f = proc.spec.frames.load (std::memory_order_acquire);
    stale = (f == lastFrames) ? stale + 1 : 0;
    lastFrames = f;
    live = stale < 8 && proc.apvts.getRawParameterValue ("son")->load() > 0.5f;
    for (size_t b = 0; b < shIn.size(); ++b)
    {
        const float ti = live ? juce::jlimit (kFloorDb, 6.f, proc.spec.dispIn[b].load (std::memory_order_relaxed))  : kFloorDb;
        const float to = live ? juce::jlimit (kFloorDb, 6.f, proc.spec.dispOut[b].load (std::memory_order_relaxed)) : kFloorDb;
        shIn[b]  += (ti - shIn[b])  * (ti > shIn[b]  ? 0.55f : 0.14f);
        shOut[b] += (to - shOut[b]) * (to > shOut[b] ? 0.55f : 0.14f);
    }

    repaint();
}

// The soft bloom behind every knob in the window (a slider can't draw outside its own bounds)
void SpectralOverlay::paintKnobGlows (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;

    for (auto& k : knobs)
    {
        auto& s = k.slider;
        const float pos = (float) s.valueToProportionOfLength (s.getValue());
        const auto rb = s.getLookAndFeel().getSliderLayout (s).sliderBounds.toFloat();
        const auto c = rb.getCentre() + s.getPosition().toFloat();
        const float rad = (juce::jmin (rb.getWidth(), rb.getHeight()) - 2.f * Scat::knobPad) * 0.5f;
        const bool hot = s.isMouseOverOrDragging();
        const float breathe = 0.9f + 0.1f * std::sin (anim * 1.7f + c.x * 0.03f);

        const float R = rad * 2.3f;
        const float strength = (0.03f + 0.09f * pos + (hot ? 0.04f : 0.f)) * breathe;
        juce::ColourGradient amb (pal.acc.withAlpha (strength), c.x, c.y, pal.acc.withAlpha (0.f), c.x + R, c.y, true);
        amb.addColour (0.45, pal.acc.withAlpha (strength * 0.35f));
        g.setGradientFill (amb);
        g.fillEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f);

        if (pos > 0.001f)
        {
            const float ang = a0 + pos * (a1 - a0), ar = rad + 4.f;
            const float bx = c.x + std::sin (ang) * ar, by = c.y - std::cos (ang) * ar;
            const float br = hot ? 20.f : 16.f;
            g.setGradientFill (juce::ColourGradient (pal.glow.withAlpha (0.26f), bx, by, pal.glow.withAlpha (0.f), bx + br, by, true));
            g.fillEllipse (bx - br, by - br, br * 2.f, br * 2.f);
        }
    }
}

// The parts of the analyzer that never move: glass, grid, labels and legend
void SpectralOverlay::paintAnalyzerShell (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const auto an = analyzer(), plot = plotRect();
    auto dbY = [&] (float db) { return plot.getY() + juce::jlimit (0.f, 1.f, -db / -kFloorDb) * plot.getHeight(); };
    auto fx  = [&] (float hz) { return plot.getX() + std::log10 (hz / SpectralCompressor::fMinHz) / 3.f * plot.getWidth(); };

    panel.drawGlass (g, an, 10.f, 0.70f, true);
    g.setFont (Scat::serif (11.f));

    for (int db = 0; db >= -80; db -= 20)
    {
        const float y = dbY ((float) db);
        g.setColour (pal.line.withAlpha (db == 0 ? 0.55f : 0.30f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (pal.dim.withAlpha (0.8f));
        g.drawText (juce::String (db), juce::Rectangle<float> (an.getX() + 2.f, y - 7.f, 26.f, 14.f), juce::Justification::centredLeft);
    }
    static const float tickHz[] = { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f };
    for (float hz : tickHz)
    {
        const float x = fx (hz);
        const bool major = (hz == 100.f || hz == 1000.f || hz == 10000.f);
        g.setColour (pal.line.withAlpha (major ? 0.45f : 0.22f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        if (major)
        {
            g.setColour (pal.dim.withAlpha (0.85f));
            g.drawText (hz >= 1000.f ? juce::String ((int) (hz / 1000.f)) + "k" : juce::String ((int) hz),
                        juce::Rectangle<float> (x - 16.f, plot.getBottom() + 2.f, 32.f, 14.f), juce::Justification::centred);
        }
    }

    // legend
    const float ly = an.getBottom() - 22.f;
    g.setFont (Scat::serif (12.f, true));
    float lx = plot.getX();
    auto textW = [&] (const juce::String& t)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (g.getCurrentFont(), t, 0.f, 0.f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    };
    auto key = [&] (juce::Colour col, const juce::String& txt, bool line)
    {
        g.setColour (col);
        if (line) g.fillRect (lx, ly + 6.f, 12.f, 2.f); else g.fillRoundedRectangle (lx, ly + 2.f, 10.f, 9.f, 2.f);
        g.setColour (pal.dim);
        g.drawText (txt, juce::Rectangle<float> (lx + 15.f, ly - 1.f, 80.f, 15.f), juce::Justification::centredLeft);
        lx += 17.f + textW (txt) + 10.f;
    };
    key (pal.ink.withAlpha (0.55f), "in", false);
    key (pal.acc, "out", false);
    key (pal.amber, "squeeze above", true);
    key (pal.grass, "lift below", true);
}

// The moving parts: the spectrum bars and the two threshold curves
void SpectralOverlay::paintAnalyzerLive (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const auto plot = plotRect();
    auto dbY = [&] (float db) { return plot.getY() + juce::jlimit (0.f, 1.f, -db / -kFloorDb) * plot.getHeight(); };

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (plot.toNearestInt());

    // spectrum: what comes in (pale, behind) and what leaves (bright, in front)
    const float bwAll = plot.getWidth() / (float) SpectralCompressor::numBands;
    for (size_t b = 0; b < shIn.size(); ++b)
    {
        const float x = plot.getX() + (float) b * bwAll;
        const float yi = dbY (shIn[b]), yo = dbY (shOut[b]);

        const juce::Rectangle<float> inBar (x + 0.4f, yi, bwAll - 0.8f, plot.getBottom() - yi);
        g.setGradientFill (juce::ColourGradient (pal.ink.withAlpha (0.40f), 0.f, yi, pal.dim.withAlpha (0.07f), 0.f, plot.getBottom(), false));
        g.fillRect (inBar);

        const juce::Rectangle<float> outBar (x + 1.1f, yo, bwAll - 2.2f, plot.getBottom() - yo);
        g.setGradientFill (juce::ColourGradient (pal.acc.interpolatedWith (pal.amber, 0.35f).withAlpha (0.95f), 0.f, yo,
                                                 pal.accDark.withAlpha (0.65f), 0.f, plot.getBottom(), false));
        g.fillRect (outBar);

        if (shOut[b] > kFloorDb + 1.f)   // a little bead on each bar
        {
            g.setColour (pal.glow.brighter (0.4f).withAlpha (0.9f));
            g.fillRect (outBar.getX(), yo, outBar.getWidth(), 1.4f);
        }
    }

    // threshold curves, drawn from the very same formula the audio uses
    const auto p = readParams();
    juce::Path down, up;
    bool first = true;
    for (int i = 0; i <= (int) plot.getWidth(); i += 2)
    {
        const float hz = SpectralCompressor::fMinHz * std::pow (1000.f, (float) i / plot.getWidth());
        const float thr = SpectralCompressor::thresholdDb (p, hz);
        const float x = plot.getX() + (float) i;
        if (first) { down.startNewSubPath (x, dbY (thr)); up.startNewSubPath (x, dbY (thr - p.upOffsetDb)); first = false; }
        else       { down.lineTo (x, dbY (thr));          up.lineTo (x, dbY (thr - p.upOffsetDb)); }
    }

    const bool downOn = p.downRatio > 1.01f, upOn = p.upRatio > 1.01f;
    for (int i = 3; i >= 1; --i)   // glow
    {
        g.setColour (pal.amber.withAlpha ((downOn ? 0.07f : 0.03f) * (float) (4 - i)));
        g.strokePath (down, juce::PathStrokeType (1.6f + (float) i * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (pal.amber.withAlpha (downOn ? 0.95f : 0.45f));
    g.strokePath (down, juce::PathStrokeType (1.6f));

    juce::Path dashed;
    const float dashes[] = { 5.f, 4.f };
    juce::PathStrokeType (1.4f).createDashedStroke (dashed, up, dashes, 2);
    g.setColour (pal.grass.withAlpha (upOn ? 0.95f : 0.35f));
    g.fillPath (dashed);

    if (! live)
    {
        g.setFont (Scat::serif (15.f, true));
        g.setColour (pal.dim.withAlpha (0.9f));
        const bool isOn = proc.apvts.getRawParameterValue ("son")->load() > 0.5f;
        g.drawText (isOn ? "waiting for sound..." : "resting - press ON to wake it", plot.reduced (10.f), juce::Justification::centred);
    }
}

// Everything that sits still while the window is open
void SpectralOverlay::paintCardStatic (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const auto cd = card();
    panel.drawGlass (g, cd, 16.f, 0.86f);

    g.setFont (Scat::titleFont (28.f));
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawText ("Spectral Compressor", (int) cd.getX() + 23, (int) cd.getY() + 5, 360, 34, juce::Justification::centredLeft);
    g.setGradientFill (juce::ColourGradient (pal.ink, 0.f, cd.getY() + 4.f, pal.ink.interpolatedWith (pal.amber, 0.55f), 0.f, cd.getY() + 38.f, false));
    g.drawText ("Spectral Compressor", (int) cd.getX() + 22, (int) cd.getY() + 4, 360, 34, juce::Justification::centredLeft);

    g.setFont (Scat::serif (13.f, true));
    g.setColour (pal.dim);
    g.drawText ("every frequency gets its own compressor", (int) cd.getX() + 24, (int) cd.getY() + 36, 360, 16, juce::Justification::centredLeft);

    static const char* titles[3] = { "globals", "threshold", "dynamics" };
    for (int r = 0; r < 3; ++r)
    {
        panel.drawGroup (g, group (r));
        panel.drawGroupTitle (g, group (r), titles[r]);
    }

    paintAnalyzerShell (g);
}

// Renders the two cached layers. Called once each time the window opens.
void SpectralOverlay::buildLayers()
{
    const auto& pal = panel.pal;
    const int iw = juce::roundToInt ((float) ScattrPanel::baseW * layerScale), ih = juce::roundToInt ((float) ScattrPanel::baseH * layerScale);
    const float W = (float) ScattrPanel::baseW, H = (float) ScattrPanel::baseH;

    // backdrop: a blurred, dimmed copy of the plugin with a soft vignette
    backdrop = juce::Image (juce::Image::RGB, iw, ih, true);
    {
        juce::Graphics bg (backdrop);
        bg.addTransform (juce::AffineTransform::scale (layerScale));
        bg.fillAll (pal.bgBot);
        if (sharp.isValid())
        {
            const auto blurred = blurSnapshot (sharp, ScattrPanel::baseW / 4, ScattrPanel::baseH / 4, 3, 3);
            bg.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            bg.drawImage (blurred, 0, 0, ScattrPanel::baseW, ScattrPanel::baseH, 0, 0, blurred.getWidth(), blurred.getHeight());
        }
        bg.setColour (juce::Colours::black.withAlpha (0.36f));
        bg.fillAll();
        juce::ColourGradient vig (juce::Colours::transparentBlack, W * 0.5f, H * 0.5f, juce::Colours::black.withAlpha (0.38f), 0.f, 0.f, true);
        bg.setGradientFill (vig);
        bg.fillAll();
    }

    // card: drawn on a transparent image so its shadow and glow fall onto whatever is behind it
    cardImg = juce::Image (juce::Image::ARGB, iw, ih, true);
    {
        juce::Graphics cg (cardImg);
        cg.addTransform (juce::AffineTransform::scale (layerScale));
        paintCardStatic (cg);
    }
}

void SpectralOverlay::paint (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const float e = smooth01 (progress);
    const int W = getWidth(), H = getHeight();

    // while opening, the real plugin melts into its blurred backdrop; once open the snapshot is not needed
    if (e < 0.999f)
    {
        g.fillAll (pal.bgBot);
        if (sharp.isValid()) g.drawImage (sharp, 0, 0, W, H, 0, 0, sharp.getWidth(), sharp.getHeight());
    }
    if (backdrop.isValid())
    {
        g.setOpacity (e);
        g.drawImage (backdrop, 0, 0, W, H, 0, 0, backdrop.getWidth(), backdrop.getHeight());
    }
    if (cardImg.isValid())
    {
        g.setOpacity (e);
        g.drawImage (cardImg, 0, 0, W, H, 0, 0, cardImg.getWidth(), cardImg.getHeight());
    }
    g.setOpacity (1.f);

    // live parts (the controls themselves are children and paint after this)
    const bool fading = e < 0.999f;
    if (fading) g.beginTransparencyLayer (e);

    const float lit = buttonLit (onBtn), en = smooth01 (lit);
    if (en > 0.01f)   // glow behind the ON key
    {
        const auto r = onBtn.getBounds().toFloat().reduced (2.f);
        for (int i = 6; i >= 1; --i)
        {
            const float t = (float) i / 6.f;
            g.setColour (pal.acc.withAlpha (0.015f * en * (1.f - 0.6f * t)));
            g.fillRoundedRectangle (r.expanded (2.f + t * 8.f), 10.f + t * 6.f);
        }
    }

    paintAnalyzerLive (g);
    paintKnobGlows (g);

    if (fading) g.endTransparencyLayer();
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
