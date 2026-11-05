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

// The "nature" wordmark: each letter is drawn on its own (so it can breathe), so the layout of the letters is worked out once.
constexpr float kLogoSize = 56.f, kLogoBase = 54.f;   // font height, and the baseline's y in the header
struct LogoLayout { float x[6] {}, w[6] {}, total = 0.f; };
const LogoLayout& logoLayout()
{
    static const LogoLayout L = []
    {
        LogoLayout r;
        juce::GlyphArrangement ga;
        ga.addLineOfText (Scat::logoFont (kLogoSize), "nature", 0.f, 0.f);
        const int n = juce::jmin (6, ga.getNumGlyphs());
        for (int i = 0; i < n; ++i)
        {
            r.x[i] = ga.getGlyph (i).getLeft();
            r.w[i] = ga.getGlyph (i).getRight() - ga.getGlyph (i).getLeft();
        }
        r.total = ga.getBoundingBox (0, -1, true).getWidth();
        return r;
    }();
    return L;
}

// top of a box (height H) in which a left-aligned, vertically centred line of `f` has its baseline at baseY
float logoBoxTop (const juce::Font& f, float baseY, float H)
{
    return baseY - ((H - (f.getAscent() + f.getDescent())) * 0.5f + f.getAscent());
}

// Width of the title at its drawn size, so the title and the sprout beside it can be centred as one group.
float titleWidth() { return logoLayout().total; }
constexpr float sproutGap = 24.f, sproutHalf = 15.f;   // title -> sprout stem, and how far the sprout reaches right of its stem

// How "on" a toggle looks, 0..1. The panel glides this for the on/off buttons (the "lit" property, 0.2 s);
// every other button simply follows its toggle state.
inline float buttonLit (const juce::Button& b)
{
    const auto& props = b.getProperties();
    if (props.contains ("lit")) return (float) (double) props["lit"];
    return b.getToggleState() ? 1.f : 0.f;
}

// Hover notes. Indices 0..29 are the knobs (same order as the `knobs` array); then DRIFT, TUNE, (retired: filter mode),
// reverb space, (retired: filter on), the echo / reverb on buttons, the spectral gate button, phaser on, chorus on, and finally
// the equalizer plot and its PRE button. The retired entries are never shown; they only keep the numbering. ScattrPanel::hoveredControl() returns these indices.
struct Tip { const char* name; const char* text; };
const Tip kTips[42] = {
    { "seed size",     "Length of each grain. Short is gritty, long is smooth and recognisable." },
    { "rain",          "How many grains fall per second. More makes a thicker, denser cloud." },
    { "speed",         "How fast the playhead travels through each sample. 1x is normal; pitch stays put." },
    { "(retired)",     "" },
    { "scatter",       "How far grains stray from the playhead. Low is focused, high is wide." },
    { "bloom",         "Seconds each sample plays before the next one, when no key is held." },
    { "rest",          "Silence between samples, in seconds, when no key is held." },
    { "master",        "The overall output volume, after every effect." },
    { "(retired)",     "" },
    { "(retired)",     "" },
    { "(retired)",     "" },
    { "time",          "Echo delay time as a note length, synced to your host's tempo." },
    { "feedback",      "How many times the echo repeats." },
    { "echo mix",      "Level of the echoes against the dry sound." },
    { "size",          "How big the space is. Larger gives a longer tail." },
    { "damp",          "Absorbs highs in the tail. More damping sounds darker and softer." },
    { "reverb mix",    "Level of the reverb against the dry sound." },
    { "master tune",   "Fine tuning for everything, in cents." },
    { "cycle",         "(retired) Held keys now play each sample all the way through." },
    { "random",        "Chance that a grain is borrowed from a different sample." },
    { "(retired)",     "" },
    { "pan spread",    "Scatters grains randomly left and right. Zero keeps them centred." },
    { "phaser rate",   "How fast the phaser sweeps up and down." },
    { "phaser depth",  "How far the sweep reaches around its centre." },
    { "phaser center", "The middle of the sweep, in Hz. Lower is deeper and swirlier, higher is airier." },
    { "phaser feedback", "Feeds the phaser back into itself. More is sharper and more resonant; negative sounds hollow." },
    { "phaser mix",    "Blends the phased sound with the dry sound." },
    { "chorus rate",   "How fast the chorus wobbles." },
    { "chorus depth",  "How much the pitch wavers. A little is lush, a lot is seasick." },
    { "chorus mix",    "Blends the chorus with the dry sound." },
    { "drift",         "On: plays by itself. Off: only sounds while a key is held." },
    { "tune",          "On: samples retune to the key you play. Off: keys just transpose them." },
    { "(retired)",     "" },
    { "reverb space",  "Click to cycle between room, plate, hall and cathedral." },
    { "(retired)",     "" },
    { "echo",          "Turns the echo on or off." },
    { "reverb",        "Turns the reverb on or off." },
    { "spectral gate", "Opens the spectral gate, which keeps or removes each frequency depending on whether it is above or below a cutoff (measured in dB below the loudest frequency of the moment). Lit while on. Right-click to switch it on or off." },
    { "phaser",        "Turns the phaser on or off." },
    { "chorus",        "Turns the chorus on or off." },
    { "equalizer",     "Double-click to add a band (up to 24), drag a dot to move it, scroll over it to change its width, hold Shift and scroll to change a cut's slope, right-click it for its shape, slope and dynamic mode. Ctrl+Z undoes." },
    { "pre",           "Also shows the sound going into the equalizer, as a thin line behind the filled spectrum." }
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

// ---------------------------------------------------------------- theme props and knob details
namespace
{
// Little props used by the new themes (butterfly, bubble, fern frond, cactus, lavender sprig, dewdrop).
void drawButterfly (juce::Graphics& g, float x, float y, float s, float phase, juce::Colour wing, juce::Colour edge)
{
    const float flap = 0.25f + 0.75f * std::abs (std::sin (phase));
    for (int side = -1; side <= 1; side += 2)
    {
        const float sx = (float) side;
        g.setColour (wing.withAlpha (0.92f));
        g.fillEllipse (x + (sx > 0 ? 0.f : -s * flap) , y - s * 0.95f, s * flap, s * 0.95f);            // upper wing
        g.fillEllipse (x + (sx > 0 ? 0.f : -s * 0.75f * flap), y - s * 0.12f, s * 0.75f * flap, s * 0.7f);  // lower wing
        g.setColour (edge.withAlpha (0.55f));
        g.drawEllipse (x + (sx > 0 ? 0.f : -s * flap), y - s * 0.95f, s * flap, s * 0.95f, 0.7f);
    }
    g.setColour (edge.darker (0.7f));
    g.drawLine (x, y - s * 0.8f, x, y + s * 0.55f, 1.3f);
}

void drawBubble (juce::Graphics& g, float x, float y, float r, juce::Colour c, float a)
{
    g.setColour (c.withAlpha (a * 0.12f));
    g.fillEllipse (x - r, y - r, r * 2.f, r * 2.f);
    g.setColour (c.withAlpha (a * 0.75f));
    g.drawEllipse (x - r, y - r, r * 2.f, r * 2.f, 1.f);
    g.setColour (juce::Colours::white.withAlpha (a * 0.8f));
    g.fillEllipse (x - r * 0.55f, y - r * 0.6f, r * 0.45f, r * 0.45f);
}

// A fern frond: a curved rib with leaflets that shrink towards the tip. `dir` leans it left (-1) or right (1).
void drawFern (juce::Graphics& g, float x, float y, float len, float dir, juce::Colour c, float sway)
{
    const juce::Point<float> p0 (x, y), p1 (x + dir * len * 0.12f, y - len * 0.55f),
                             p2 (x + dir * len * 0.6f + sway * len, y - len * 0.92f);
    juce::Path rib;
    rib.startNewSubPath (p0);
    rib.quadraticTo (p1, p2);
    g.setColour (c);
    g.strokePath (rib, juce::PathStrokeType (1.3f));

    for (float t = 0.18f; t < 0.97f; t += 0.1f)
    {
        const float u = 1.f - t;
        const juce::Point<float> p (u * u * p0.x + 2.f * u * t * p1.x + t * t * p2.x, u * u * p0.y + 2.f * u * t * p1.y + t * t * p2.y);
        const juce::Point<float> d (2.f * u * (p1.x - p0.x) + 2.f * t * (p2.x - p1.x), 2.f * u * (p1.y - p0.y) + 2.f * t * (p2.y - p1.y));
        const float dl = std::sqrt (d.x * d.x + d.y * d.y) + 1.0e-4f;
        const juce::Point<float> n (-d.y / dl, d.x / dl);   // perpendicular
        const float ll = (1.f - t) * len * 0.3f + 2.f;
        g.drawLine (p.x, p.y, p.x + (n.x * 0.8f + d.x / dl * 0.5f) * ll, p.y + (n.y * 0.8f + d.y / dl * 0.5f) * ll, 1.f);
        g.drawLine (p.x, p.y, p.x - (n.x * 0.8f - d.x / dl * 0.5f) * ll, p.y - (n.y * 0.8f - d.y / dl * 0.5f) * ll, 1.f);
    }
}

void drawCactus (juce::Graphics& g, float x, float baseY, float h, juce::Colour c, juce::Colour bloom)
{
    g.setColour (c);
    g.fillRoundedRectangle (x - 2.6f, baseY - h, 5.2f, h + 2.f, 2.6f);                       // trunk
    g.fillRoundedRectangle (x - 8.f, baseY - h * 0.55f, 6.f, 3.2f, 1.6f);                    // left arm, out ...
    g.fillRoundedRectangle (x - 8.f, baseY - h * 0.55f - 6.f, 3.2f, 9.f, 1.6f);              // ... and up
    g.fillRoundedRectangle (x + 2.f, baseY - h * 0.4f, 6.f, 3.2f, 1.6f);                     // right arm
    g.fillRoundedRectangle (x + 4.8f, baseY - h * 0.4f - 5.f, 3.2f, 8.f, 1.6f);
    g.setColour (c.brighter (0.3f).withAlpha (0.5f));
    g.drawLine (x - 0.6f, baseY - h + 2.f, x - 0.6f, baseY, 0.8f);                           // a highlight rib
    g.setColour (bloom);
    g.fillEllipse (x - 2.f, baseY - h - 3.f, 4.f, 4.f);
}

void drawLavenderSprig (juce::Graphics& g, float x, float y, float h, float lean, juce::Colour stem, juce::Colour floret)
{
    g.setColour (stem);
    g.drawLine (x, y, x + lean, y - h, 1.3f);
    for (int k = 0; k < 7; ++k)
    {
        const float t = 0.42f + 0.085f * (float) k;
        const float px = x + lean * t + ((k % 2 == 0) ? -1.7f : 1.7f), py = y - h * t;
        g.setColour ((k % 2 == 0 ? floret : floret.brighter (0.25f)).withAlpha (0.95f));
        g.fillEllipse (px - 1.5f, py - 2.2f, 3.f, 4.4f);
    }
}

void drawDew (juce::Graphics& g, float x, float y, float r, juce::Colour tint)
{
    g.setColour (tint.withAlpha (0.28f));
    g.fillEllipse (x - r, y - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawEllipse (x - r, y - r, r * 2.f, r * 2.f, 0.8f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillEllipse (x - r * 0.55f, y - r * 0.6f, r * 0.5f, r * 0.5f);
}

// What the pointer on the knob looks like, per theme.
enum class Pointer { Leaf, Maple, Crystal, Petal, Drop, Blade, Crescent };
Pointer pointerFor (int theme)
{
    static const Pointer kinds[10] = { Pointer::Leaf, Pointer::Maple, Pointer::Crystal, Pointer::Petal, Pointer::Leaf,
                                       Pointer::Drop, Pointer::Blade, Pointer::Crescent, Pointer::Leaf, Pointer::Petal };
    return kinds[juce::jlimit (0, 9, theme)];
}

// A small themed ornament on the stump's upper-left shoulder (moss, fallen leaf, snow, blossom, ...).
// Everything is clipped to the stump, so nothing hangs off the knob.
void drawKnobDecor (juce::Graphics& g, int theme, juce::Point<float> c, float rad, const Palette& pal, float pulse)
{
    const float s = rad * 0.17f;
    const juce::Point<float> p (c.x - rad * 0.64f, c.y - rad * 0.58f);

    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip;
    clip.addEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f);
    g.reduceClipRegion (clip);

    switch (theme)
    {
        default:
        case 0:   // summer: a cushion of moss with a clover
            g.setColour (pal.grassDark.withAlpha (0.8f));
            g.fillEllipse (p.x - 1.7f * s, p.y - 0.4f * s, 2.8f * s, 1.6f * s);
            g.fillEllipse (p.x - 0.5f * s, p.y - 1.2f * s, 2.4f * s, 1.6f * s);
            g.fillEllipse (p.x + 0.3f * s, p.y - 0.1f * s, 2.2f * s, 1.4f * s);
            g.setColour (pal.grass.withAlpha (0.7f));
            g.fillEllipse (p.x - 0.9f * s, p.y - 0.2f * s, 1.2f * s, 0.7f * s);
            g.fillEllipse (p.x + 0.5f * s, p.y - 0.8f * s, 1.0f * s, 0.6f * s);
            for (int k = 0; k < 3; ++k)
            {
                const float a = (float) k * 2.094f + 0.5f;
                g.fillEllipse (p.x + 1.4f * s + std::sin (a) * 0.45f * s - 0.4f * s, p.y + 0.9f * s - std::cos (a) * 0.45f * s - 0.4f * s, 0.8f * s, 0.8f * s);
            }
            break;

        case 1:   // autumn: fallen maple leaves
            drawMaple (g, p.x, p.y, 0.9f, s * 1.5f, pal.amber.withAlpha (0.92f));
            drawMaple (g, p.x + 1.9f * s, p.y + 1.5f * s, -0.6f, s * 1.0f, pal.acc.withAlpha (0.9f));
            break;

        case 2:   // winter: a drift of snow with a few sparkles
            g.setColour (pal.ink.withAlpha (0.92f));
            g.fillEllipse (p.x - 2.0f * s, p.y - 0.6f * s, 4.0f * s, 1.9f * s);
            g.fillEllipse (p.x - 1.2f * s, p.y - 1.5f * s, 2.5f * s, 1.5f * s);
            g.setColour (pal.glow.withAlpha (0.35f));
            g.fillEllipse (p.x - 1.4f * s, p.y + 0.5f * s, 2.8f * s, 0.7f * s);
            g.setColour (juce::Colours::white.withAlpha (0.4f + 0.5f * pulse));
            g.fillEllipse (p.x + 2.1f * s, p.y + 0.6f * s, 0.55f * s, 0.55f * s);
            g.fillEllipse (p.x + 1.0f * s, p.y + 1.7f * s, 0.4f * s, 0.4f * s);
            break;

        case 3:   // spring: a blossom and a loose petal
            drawBlossom (g, p.x, p.y, s * 1.3f, pal.acc.brighter (0.45f), pal.amber, pal.accDark);
            g.setColour (pal.acc.brighter (0.5f).withAlpha (0.9f));
            g.fillEllipse (p.x + 1.6f * s, p.y + 1.0f * s, 1.5f * s, 0.9f * s);
            break;

        case 4:   // meadow: a daisy and a blade of grass
            drawLeaf (g, p.x + 1.3f * s, p.y + 1.5f * s, 2.2f, s * 1.8f, pal.grass.withAlpha (0.9f));
            drawFlower (g, p.x, p.y, s * 1.35f, pal.ink, pal.acc);
            break;

        case 5:   // tide: pearls and a bubble
            for (int k = 0; k < 3; ++k)
            {
                static const float ox[3] = { 0.f, 1.6f, -0.2f }, oy[3] = { 0.f, 0.9f, 1.6f }, rr[3] = { 0.9f, 0.65f, 0.5f };
                const float px = p.x + ox[k] * s, py = p.y + oy[k] * s, r = rr[k] * s;
                g.setGradientFill (juce::ColourGradient (pal.ink, px - r * 0.4f, py - r * 0.4f, pal.amber.darker (0.15f), px + r, py + r, true));
                g.fillEllipse (px - r, py - r, r * 2.f, r * 2.f);
            }
            g.setColour (pal.acc.withAlpha (0.6f));
            g.drawEllipse (p.x + 1.0f * s, p.y - 1.1f * s, 1.1f * s, 1.1f * s, 0.9f);
            break;

        case 6:   // dune: pebbles in the sand
            for (int k = 0; k < 3; ++k)
            {
                static const float ox[3] = { 0.f, 1.7f, 0.5f }, oy[3] = { 0.f, 0.6f, 1.5f }, rw[3] = { 1.7f, 1.2f, 1.0f };
                const float px = p.x + ox[k] * s, py = p.y + oy[k] * s;
                g.setColour (pal.bark.brighter (0.1f + 0.12f * (float) k).withAlpha (0.95f));
                g.fillEllipse (px - rw[k] * s * 0.5f, py - rw[k] * s * 0.32f, rw[k] * s, rw[k] * s * 0.64f);
                g.setColour (juce::Colours::white.withAlpha (0.18f));
                g.fillEllipse (px - rw[k] * s * 0.3f, py - rw[k] * s * 0.25f, rw[k] * s * 0.4f, rw[k] * s * 0.18f);
            }
            break;

        case 7:   // moonlit: a glowing mushroom
        {
            g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (0.20f + 0.14f * pulse), p.x, p.y, pal.acc.withAlpha (0.f), p.x + 3.4f * s, p.y, true));
            g.fillEllipse (p.x - 3.4f * s, p.y - 3.4f * s, 6.8f * s, 6.8f * s);
            g.setColour (pal.ink.withAlpha (0.8f));
            g.fillRoundedRectangle (p.x - 0.3f * s, p.y - 0.1f * s, 0.6f * s, 1.4f * s, 0.3f * s);
            g.setColour (pal.acc);
            g.fillEllipse (p.x - 1.2f * s, p.y - 1.3f * s, 2.4f * s, 1.5f * s);
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.fillEllipse (p.x - 0.5f * s, p.y - 0.9f * s, 0.4f * s, 0.4f * s);
            break;
        }

        case 8:   // rain: dewdrops on the wood
            drawDew (g, p.x, p.y, s * 0.95f, pal.glow);
            drawDew (g, p.x + 1.7f * s, p.y + 0.8f * s, s * 0.65f, pal.glow);
            drawDew (g, p.x - 0.2f * s, p.y + 1.7f * s, s * 0.5f, pal.glow);
            break;

        case 9:   // lavender: a sprig laid across the rim
            drawLavenderSprig (g, p.x - 1.5f * s, p.y + 2.0f * s, s * 4.2f, s * 2.4f, pal.grassDark, pal.acc);
            break;
    }
}

// The pointer: a leaf, maple leaf, frost crystal, petal, droplet, blade or crescent moon depending on the theme.
void drawKnobPointer (juce::Graphics& g, int theme, juce::Point<float> c, float rad, float a, const Palette& pal)
{
    const auto kind = pointerFor (theme);
    const float len = rad * 0.75f, wid = rad * 0.26f;
    const auto black = juce::Colours::black;

    if (kind == Pointer::Maple)
    {
        const float r = rad * 0.4f, o = rad * 0.55f;
        const float px = c.x + std::sin (a) * o, py = c.y - std::cos (a) * o;
        drawMaple (g, px + 1.2f, py + 2.f, a, r, black.withAlpha (0.3f));
        drawMaple (g, px, py, a, r, pal.amber.brighter (0.1f));
        return;
    }

    juce::Path shape, detail;
    switch (kind)
    {
        default:
        case Pointer::Leaf:
            shape = Scat::leafPath (len, wid);
            detail.startNewSubPath (0.f, 0.f);  detail.lineTo (0.f, -len * 0.85f);
            for (float t : { 0.32f, 0.55f })
            {
                detail.startNewSubPath (0.f, -len * t);  detail.lineTo ( wid * 0.85f, -len * (t + 0.13f));
                detail.startNewSubPath (0.f, -len * t);  detail.lineTo (-wid * 0.85f, -len * (t + 0.13f));
            }
            break;

        case Pointer::Blade:
            shape = Scat::leafPath (len * 1.05f, wid * 0.55f);
            detail.startNewSubPath (0.f, 0.f);  detail.lineTo (0.f, -len * 0.9f);
            break;

        case Pointer::Crystal:
            shape.startNewSubPath (0.f, 0.f);
            shape.lineTo (wid * 1.05f, -len * 0.38f);
            shape.lineTo (0.f, -len);
            shape.lineTo (-wid * 1.05f, -len * 0.38f);
            shape.closeSubPath();
            detail.startNewSubPath (0.f, 0.f);                     detail.lineTo (0.f, -len);
            detail.startNewSubPath (-wid * 1.05f, -len * 0.38f);   detail.lineTo (wid * 1.05f, -len * 0.38f);
            break;

        case Pointer::Petal:
            shape.startNewSubPath (0.f, 0.f);
            shape.cubicTo (wid * 1.8f, -len * 0.2f, wid * 1.4f, -len * 0.95f, 0.f, -len);
            shape.cubicTo (-wid * 1.4f, -len * 0.95f, -wid * 1.8f, -len * 0.2f, 0.f, 0.f);
            shape.closeSubPath();
            detail.startNewSubPath (0.f, -len * 0.1f);  detail.lineTo (0.f, -len * 0.8f);
            break;

        case Pointer::Drop:
        {
            const float rr = wid * 1.45f;
            shape.startNewSubPath (0.f, -len);
            shape.cubicTo (rr * 0.35f, -len * 0.7f, rr * 1.0f, -rr * 1.7f, rr, -rr);
            shape.cubicTo (rr, -rr * 0.45f, rr * 0.5f, 0.f, 0.f, 0.f);
            shape.cubicTo (-rr * 0.5f, 0.f, -rr, -rr * 0.45f, -rr, -rr);
            shape.cubicTo (-rr * 1.0f, -rr * 1.7f, -rr * 0.35f, -len * 0.7f, 0.f, -len);
            shape.closeSubPath();
            detail.addEllipse (-rr * 0.55f, -rr * 1.35f, rr * 0.5f, rr * 0.7f);   // a highlight
            break;
        }

        case Pointer::Crescent:
            shape.startNewSubPath (0.f, -len);
            shape.quadraticTo (wid * 2.7f, -len * 0.5f, 0.f, 0.f);
            shape.quadraticTo (wid * 0.55f, -len * 0.5f, 0.f, -len);
            shape.closeSubPath();
            break;
    }

    const auto t = juce::AffineTransform::translation (0.f, -rad * 0.2f)
                       .followedBy (juce::AffineTransform::rotation (a))
                       .followedBy (juce::AffineTransform::translation (c.x, c.y));
    g.setColour (black.withAlpha (0.3f));
    g.fillPath (shape, t.translated (1.2f, 2.f));
    g.setGradientFill (juce::ColourGradient (pal.amber.brighter (0.3f), c.x, c.y - rad,
                                             pal.amber.darker (0.25f), c.x, c.y + rad * 0.3f, false));
    g.fillPath (shape, t);

    if (kind == Pointer::Drop)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillPath (detail, t);
    }
    else if (! detail.isEmpty())
    {
        g.setColour (pal.amber.darker (0.6f).withAlpha (0.65f));
        g.strokePath (detail, juce::PathStrokeType (0.9f), t);
    }
}
} // namespace

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
        g.setColour (black.withAlpha (0.09f));
        g.fillEllipse (c.x - rad - e, c.y - rad - e + 4.f, (rad + e) * 2.f, (rad + e) * 2.f);
    }

    // wooden stump: domed, lit from the top-left
    {
        juce::ColourGradient body (pal.bark.brighter (0.45f), c.x - rad * 0.45f, c.y - rad * 0.55f,
                                   pal.soil.darker (0.6f),    c.x + rad * 0.55f, c.y + rad * 0.75f, true);
        body.addColour (0.55, pal.soil.brighter (0.1f));
        g.setGradientFill (body);
        g.fillEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f);

        g.setGradientFill (juce::ColourGradient (white.withAlpha (hot ? 0.28f : 0.17f), c.x, c.y - rad,
                                                 black.withAlpha (0.32f), c.x, c.y + rad, false));
        g.drawEllipse (c.x - rad + 0.8f, c.y - rad + 0.8f, rad * 2.f - 1.6f, rad * 2.f - 1.6f, 1.6f);

        // bark: a ring of short, uneven grooves just inside the rim
        if (rad > 20.f)
        {
            juce::Path grooves;
            const int n = 30;
            for (int k = 0; k < n; ++k)
            {
                const float ang = (float) k * juce::MathConstants<float>::twoPi / (float) n + 0.11f * (float) (k % 3);
                const float r0 = rad * (0.83f + 0.035f * (float) (k % 2)), r1 = rad * 0.95f;
                grooves.startNewSubPath (c.x + std::sin (ang) * r0, c.y - std::cos (ang) * r0);
                grooves.lineTo (c.x + std::sin (ang) * r1, c.y - std::cos (ang) * r1);
            }
            g.setColour (black.withAlpha (0.13f));
            g.strokePath (grooves, juce::PathStrokeType (1.f));
        }
    }

    // sunken centre cap (the opposite light direction makes it read as a dish)
    {
        const float ic = rad * 0.62f;
        g.setGradientFill (juce::ColourGradient (pal.soil.darker (0.5f), c.x, c.y - ic,
                                                 pal.moss.brighter (0.15f), c.x, c.y + ic, false));
        g.fillEllipse (c.x - ic, c.y - ic, ic * 2.f, ic * 2.f);
        g.setGradientFill (juce::ColourGradient (black.withAlpha (0.4f), c.x, c.y - ic,
                                                 white.withAlpha (0.14f), c.x, c.y + ic, false));
        g.drawEllipse (c.x - ic, c.y - ic, ic * 2.f, ic * 2.f, 1.2f);

        // tree rings, slightly off-centre like a real cross-section, round a small heartwood
        if (rad > 20.f)
        {
            const float ox = ic * 0.05f, oy = ic * 0.035f;
            g.setColour (black.withAlpha (0.10f));
            for (int k = 1; k <= 3; ++k)
            {
                const float rr = ic * (0.28f + 0.22f * (float) k);
                g.drawEllipse (c.x - rr + ox, c.y - rr + oy, rr * 2.f, rr * 2.f, 0.8f);
            }
            g.setColour (pal.bark.withAlpha (0.30f));
            g.fillEllipse (c.x - ic * 0.09f + ox, c.y - ic * 0.09f + oy, ic * 0.18f, ic * 0.18f);
        }
    }

    // the theme's little ornament on the stump's shoulder (moss, a fallen leaf, snow, a blossom, ...)
    drawKnobDecor (g, style, c, rad, pal, pulse);

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

    // five tiny seeds along the outside of the track mark the quarter points
    g.setColour (pal.dim.withAlpha (0.28f));
    for (int k = 0; k < 5; ++k)
    {
        const float ta = a0 + (a1 - a0) * (float) k * 0.25f, tr = ar + 4.6f;
        g.fillEllipse (c.x + std::sin (ta) * tr - 0.9f, c.y - std::cos (ta) * tr - 0.9f, 1.8f, 1.8f);
    }

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

    // the pointer: a leaf, maple leaf, frost crystal, petal, droplet, blade or crescent, by theme
    drawKnobPointer (g, style, c, rad, a, pal);
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

    const auto& props = b.getProperties();
    g.setFont (serif (props.contains ("fontSize") ? (float) (double) props["fontSize"] : 14.f));
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
    setWantsKeyboardFocus (true);   // so Ctrl+Z reaches the equalizer's undo
    setSize (baseW, baseH);

    struct Def { const char* id; const char* text; int dp; };
    // dp = digits shown under the knob (whole numbers for big ranges, 0.00 style for the small ones)
    static const Def defs[30] = {
        { "size", "seed size", 0 }, { "dens", "rain", 0 }, { "speed", "speed", 2 },
        { "", "", 0 }, { "spray", "scatter", 2 }, { "hold", "bloom", 2 },
        { "gap", "rest", 2 }, { "master", "master", 2 },   // 7: lives on the settings tab
        { "", "", 0 }, { "", "", 0 }, { "", "", 0 },   // 8..10: the old filter's knobs (retired)
        { "dnote", "time", -1 }, { "dfb", "feedback", 2 }, { "dmix", "echo mix", 2 },
        { "rsize", "size", 2 }, { "rdamp", "damp", 2 }, { "rmix", "reverb mix", 2 },
        { "mtune", "master tune", 0 }, { "cycle", "cycle", 2 }, { "rand", "random", 2 },
        { "", "", 0 }, { "width", "pan spread", 2 },
        { "prate", "rate", 2 }, { "pdepth", "depth", 2 }, { "pcenter", "center", 0 }, { "pfb", "feedback", 2 }, { "pmix", "mix", 2 },
        { "crate", "rate", 2 }, { "cdepth", "depth", 2 }, { "cmix", "mix", 2 }
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        if (retiredKnob (i)) continue;   // the filter is gone: these slots are never created, attached or shown

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
    static const char* tabNames[3] = { "GRAINS", "FX", "SETTINGS" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = tabBtn[(size_t) i];
        b.setButtonText (tabNames[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1001);
        b.onClick = [this, i] { if (tabBtn[(size_t) i].getToggleState()) setTab (i); };
        addAndMakeVisible (b);
    }
    tabBtn[0].setToggleState (true, juce::dontSendNotification);

    // settings tab
    {
        const std::atomic<bool>* flags[3] = { &proc.setNoTips, &proc.setNoAnim, &proc.setMono };
        for (int i = 0; i < 3; ++i)
        {
            auto& b = setBtn[(size_t) i];
            b.setButtonText ("ON");
            b.setToggleState (flags[i]->load(), juce::dontSendNotification);
            b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            b.onClick = [this, i]
            {
                const bool on = setBtn[(size_t) i].getToggleState();
                if (i == 0) proc.setNoTips = on;
                if (i == 1) proc.setNoAnim = on;
                if (i == 2) proc.setMono = on;
                proc.saveUiSettings();
                if (i == 2) applyTheme (curTheme, true, true);   // recolour (or restore) right away
                repaint();
            };
            addChildComponent (b);
        }

        static const char* startNames[6] = { "LAST USED", "50%", "75%", "100%", "150%", "200%" };
        startBtn.setButtonText (startNames[juce::jlimit (0, 5, proc.setStartSize.load())]);
        startBtn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        startBtn.onClick = [this]
        {
            juce::PopupMenu menu;
            const int cur = proc.setStartSize.load();
            for (int i = 0; i < 6; ++i) menu.addItem (i + 1, i == 0 ? juce::String ("Last used") : juce::String (startNames[i]), true, i == cur);

            juce::Component::SafePointer<ScattrPanel> safe (this);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&startBtn),
                                [safe] (int result)
                                {
                                    if (safe == nullptr || result <= 0) return;
                                    safe->proc.setStartSize = result - 1;
                                    safe->proc.saveUiSettings();
                                    safe->startBtn.setButtonText (startNames[result - 1]);
                                });
        };
        addChildComponent (startBtn);
    }

    addAndMakeVisible (spaceBtn);
    spaceBtn.onClick = [this]
    {
        auto* par = proc.apvts.getParameter ("rtype");
        const int cur = (int) proc.apvts.getRawParameterValue ("rtype")->load();
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) ((cur + 1) % 4)));
        par->endChangeGesture();
    };

    for (auto* b : { &donBtn, &ronBtn }) addAndMakeVisible (*b);
    donAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "don", donBtn);
    ronAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "ron", ronBtn);

    for (auto* b : { &ponBtn, &conBtn }) addAndMakeVisible (*b);
    ponAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "pon", ponBtn);
    conAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "con", conBtn);

    eqView = std::make_unique<EqDisplay> (*this, proc);
    addAndMakeVisible (*eqView);
    eqView->setBounds (eqPanel().toNearestInt());   // the panel's one-and-only resized() ran (in setSize, above) before this view existed, so without this it sits at 0 x 0: invisible and deaf to the mouse

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

    addAndMakeVisible (specBtn);
    specBtn.setClickingTogglesState (false);                 // the light follows the gate's on/off parameter; a press opens the window
    specBtn.getProperties().set ("fontSize", 12.0);          // the label is longer than DRIFT / TUNE, so it is set a little smaller
    specBtn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    specBtn.onClick = [this]
    {
        if (specRightDown) { specRightDown = false; return; }   // a right-click toggles the gate (see mouseDown); it does not open the window
        openSpectral();
    };
    specBtn.addMouseListener (this, false);                      // so the panel hears right-clicks on the button

    // the pop-out window goes in last so it sits above everything else
    spectral = std::make_unique<SpectralOverlay> (*this, proc);
    addChildComponent (*spectral);
    spectral->setBounds (getLocalBounds());

    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Select samples", juce::File(),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        juce::Component::SafePointer<ScattrPanel> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectMultipleItems,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr) return;   // the editor was closed while the dialog was open
                                  for (auto& f : fc.getResults()) safe->proc.addSampleFile (f);
                                  safe->repaint();
                                  safe->restoreFocusSoon();   // the native dialog took the keyboard; give it back
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

void ScattrPanel::applyTheme (int idx, bool animate, bool force)
{
    idx = juce::jlimit (0, (int) Scat::themes().size() - 1, idx);
    animate = animate && animOn();
    if (animate && idx == curTheme && themeT >= 1.f && ! force) return;

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
    lnf.setStyle (idx);
    proc.themeIdx = idx;
    const bool mono = proc.setMono.load();
    targetPal = mono ? Scat::grayPal (Scat::themes()[(size_t) idx].pal) : Scat::themes()[(size_t) idx].pal;
    themeT = animate ? 0.f : 1.f;
    if (! animate) pal = targetPal;

    cacheTheme = -1;   // the cached sky and cards are rebuilt with the new palette (see rebuildCache)
    lnf.setPalette (pal);
    themeBtn.setButtonText (mono ? "mono" : Scat::themes()[(size_t) idx].name);
    themeBtn.setEnabled (! mono);          // themes are switched off while black and white is on
    themeBtn.setAlpha (mono ? 0.5f : 1.f);
    sendLookAndFeelChange();   // refreshes the knob readouts' colours
    repaint();
}

void ScattrPanel::setTab (int t)
{
    tab = t;
    const bool grains = (t == 0), fx = (t == 1), sett = (t == 2);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const bool master = (i == 7);
        const bool vis = master ? sett
                                : ((grains && onGrainsTab (i)) || (fx && ! onGrainsTab (i))) && i != 18 && ! retiredKnob (i);   // 18 = "cycle": retired, samples now play all the way through
        knobs[i].slider.setVisible (vis);
        knobs[i].label.setVisible (vis && ! master);
    }
    spaceBtn.setVisible (fx);
    for (auto* b : { &donBtn, &ronBtn, &ponBtn, &conBtn }) b->setVisible (fx);
    if (eqView != nullptr) eqView->setVisible (fx);
    loadBtn.setVisible (grains);
    clearBtn.setVisible (grains);
    droneBtn.setVisible (grains);
    tuneBtn.setVisible (grains);
    specBtn.setVisible (grains);
    for (auto& b : setBtn) b.setVisible (sett);
    startBtn.setVisible (sett);
    tabBtn[(size_t) t].setToggleState (true, juce::dontSendNotification);
    repaint();
}

void ScattrPanel::resized()
{
    // header
    tabBtn[0].setBounds (18, 22, 80, 34);
    tabBtn[1].setBounds (102, 22, 58, 34);
    tabBtn[2].setBounds (164, 22, 92, 34);
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

    // grains tab: two rows of five equal columns, so nothing is left hanging empty at the right-hand end
    {
        static const size_t row1[5] = { 0, 1, 2, 4, 21 };         // seed size, rain, speed, scatter, pan spread
        static const size_t row2[4] = { 5, 6, 19, 17 };            // bloom, rest, random, master tune
        for (int k = 0; k < 5; ++k) place (row1[k], grainPanel(), 5, k, 88);

        for (int k = 0; k < 4; ++k) place (row2[k], motionPanel(), 5, k, 88);

        // the last of the motion row's five columns holds the three keys, stacked
        const auto mp = motionPanel();
        const int sw = (int) ((mp.getWidth() - 24.f) / 5.f);
        const int bw = 112, bx = (int) mp.getX() + 12 + 4 * sw + (sw - bw) / 2;
        droneBtn.setBounds (bx, (int) mp.getY() + 28, bw, 26);
        tuneBtn.setBounds  (bx, (int) mp.getY() + 60, bw, 26);
        specBtn.setBounds  (bx, (int) mp.getY() + 92, bw, 26);
    }

    // fx tab: the equalizer on top, then two rows of effect cards (see fxPanel() for the geometry)
    {
        if (eqView != nullptr) eqView->setBounds (eqPanel().toNearestInt());

        // Every card has a header row (its title on the left, its keys on the right) and the knobs sit entirely below it,
        // so a key never lands on a knob's name.
        auto placeFx = [this] (size_t idx, juce::Rectangle<float> panel, int n, int k)
        {
            const int sw = (int) ((panel.getWidth() - 24.f) / (float) n);
            const int x = (int) panel.getX() + 12 + k * sw, y = (int) panel.getY();
            knobs[idx].label.setBounds (x, y + 30, sw, 16);
            knobs[idx].slider.setBounds (x, y + 45, sw, 76);
        };
        for (int k = 0; k < 5; ++k) placeFx ((size_t) (22 + k), fxPanel (0), 5, k);   // phaser: rate, depth, center, feedback, mix
        for (int k = 0; k < 3; ++k) placeFx ((size_t) (27 + k), fxPanel (1), 3, k);   // chorus: rate, depth, mix
        for (int k = 0; k < 3; ++k) placeFx ((size_t) (11 + k), fxPanel (2), 3, k);   // delay: time, feedback, echo mix
        for (int k = 0; k < 3; ++k) placeFx ((size_t) (14 + k), fxPanel (3), 3, k);   // reverb: size, damp, reverb mix

        // each card's on button sits in its top-right corner, in the header row; the reverb space key sits just left of its own
        auto onBounds = [&] (int card) { return juce::Rectangle<int> ((int) fxPanel (card).getRight() - 12 - 50, (int) fxPanel (card).getY() + 5, 50, 22); };
        ponBtn.setBounds (onBounds (0));
        conBtn.setBounds (onBounds (1));
        donBtn.setBounds (onBounds (2));
        ronBtn.setBounds (onBounds (3));
        spaceBtn.setBounds (ronBtn.getX() - 6 - 70, ronBtn.getY(), 70, 22);
    }

    // settings tab: each row's control sits at the right-hand end of the row (row 0 is the master knob)
    for (int i = 0; i < 5; ++i)
    {
        const auto row = settingsRow (i).toNearestInt();
        const juce::Rectangle<int> c (row.getRight() - 12 - 132, row.getY() + 21, 132, 34);
        if (i == 0)      knobs[7].slider.setBounds (row.getRight() - 12 - 132, row.getY(), 132, row.getHeight());
        else if (i < 4)  setBtn[(size_t) (i - 1)].setBounds (c);
        else             startBtn.setBounds (c);
    }
}

void ScattrPanel::filesDropped (const juce::StringArray& files, int, int)
{
    if (spectral != nullptr && spectral->isVisible()) return;
    for (auto& f : files) proc.addSampleFile (juce::File (f));
    repaint();
    restoreFocusSoon();
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
float ScattrPanel::titleX() const  { return 0.5f * (262.f + eqRect().getX()) - (titleWidth() + sproutGap + sproutHalf) * 0.5f; }
float ScattrPanel::sproutX() const { return titleX() + titleWidth() + sproutGap; }

bool ScattrPanel::isInterestedInFileDrag (const juce::StringArray&)
{
    return spectral == nullptr || ! spectral->isVisible();   // nothing behind the spectral window may be dropped on
}

// A native file dialog (or a drag from the desktop) takes keyboard focus away from the plugin's window and the operating
// system does not always hand it back, so the next key press never reached the host's keyboard-to-MIDI. Bring the plugin's window
// forward and take the focus again right away, and once more a moment later, after the dialog has finished closing.
void ScattrPanel::restoreFocus()
{
    if (! isShowing()) return;
    if (auto* top = getTopLevelComponent()) top->toFront (true);
    grabKeyboardFocus();
}

void ScattrPanel::restoreFocusSoon()
{
    restoreFocus();
    juce::Component::SafePointer<ScattrPanel> safe (this);
    for (int ms : { 120, 450 })
        juce::Timer::callAfterDelay (ms, [safe] { if (safe != nullptr) safe->restoreFocus(); });
}

bool ScattrPanel::keyPressed (const juce::KeyPress& k)
{
    if (tab != 1 || eqView == nullptr || (spectral != nullptr && spectral->isVisible())) return false;

    const auto m = k.getModifiers();
    if (! m.isCommandDown()) return false;   // Ctrl on Windows / Linux, Cmd on a Mac

    const auto c = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k.getKeyCode());
    if (c == 'z') return m.isShiftDown() ? eqView->redo() : eqView->undo();
    if (c == 'y') return eqView->redo();
    return false;
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
    if (e.eventComponent == &specBtn)   // right-click on SPECTRAL GATE: switch the gate on / off without opening the window
    {
        specRightDown = e.mods.isPopupMenu();
        if (specRightDown)
        {
            auto* par = proc.apvts.getParameter ("son");
            const float next = proc.apvts.getRawParameterValue ("son")->load() > 0.5f ? 0.f : 1.f;
            par->beginChangeGesture();
            par->setValueNotifyingHost (par->convertTo0to1 (next));
            par->endChangeGesture();
        }
        return;
    }

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
    if (animOn()) anim += 0.033f;

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
    specBtn.setToggleState (proc.apvts.getRawParameterValue ("son")->load() > 0.5f, juce::dontSendNotification);
    for (auto* b : { &droneBtn, &tuneBtn, &donBtn, &ronBtn, &specBtn, &ponBtn, &conBtn, &setBtn[0], &setBtn[1], &setBtn[2] })
    {
        const float target = b->getToggleState() ? 1.f : 0.f;
        float v = buttonLit (*b);
        const float step = animOn() ? dt / 0.2f : 1.f;
        v = v < target ? juce::jmin (target, v + step) : juce::jmax (target, v - step);
        b->getProperties().set ("lit", v);
    }

    // hover tips: wait ~0.5 s on a control (0.1 s when moving between controls while a tip is already up)
    {
        const int hov = (proc.setNoTips.load() || juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown()) ? -1 : hoveredControl();
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

    updateEq();

    static const char* rt[4] = { "ROOM", "PLATE", "HALL", "CHURCH" };
    const int ri = juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("rtype")->load());
    if (spaceBtn.getButtonText() != rt[ri]) spaceBtn.setButtonText (rt[ri]);

    // bypassed effects look dimmed
    const float aD = 0.55f + 0.45f * smooth01 (buttonLit (donBtn));
    const float aR = 0.55f + 0.45f * smooth01 (buttonLit (ronBtn));
    const float aP = 0.55f + 0.45f * smooth01 (buttonLit (ponBtn));
    const float aC = 0.55f + 0.45f * smooth01 (buttonLit (conBtn));
    for (size_t idx : { (size_t) 11, (size_t) 12, (size_t) 13 })            { knobs[idx].slider.setAlpha (aD); knobs[idx].label.setAlpha (aD); }
    for (size_t idx : { (size_t) 14, (size_t) 15, (size_t) 16 })            { knobs[idx].slider.setAlpha (aR); knobs[idx].label.setAlpha (aR); }
    for (size_t idx : { (size_t) 22, (size_t) 23, (size_t) 24, (size_t) 25, (size_t) 26 }) { knobs[idx].slider.setAlpha (aP); knobs[idx].label.setAlpha (aP); }
    for (size_t idx : { (size_t) 27, (size_t) 28, (size_t) 29 })            { knobs[idx].slider.setAlpha (aC); knobs[idx].label.setAlpha (aC); }
    spaceBtn.setAlpha (aR);
    if (tab == 1 && eqView != nullptr) eqView->update();

    const int c = proc.uiSample.load();
    playIdx = c;
    if (pinned >= (int) proc.samples.size()) pinned = -1;
    if (pinned >= 0)    shown = pinned;
    else if (c >= 0)    shown = c;
    if (shown >= (int) proc.samples.size()) shown = -1;
    if (shown < 0 && ! proc.samples.empty()) shown = (int) proc.samples.size() - 1;

    playing  = proc.uiPlaying.load();
    shownPlaying = playing && playIdx == shown;
    playGlow += ((playing ? 1.f : 0.f) - playGlow) * (playing ? 0.25f : 0.07f);   // lights quickly, fades slowly
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
    const bool woody = (curTheme >= 1 && curTheme <= 3);   // autumn, winter, spring: bare branch
    juce::Path stem;
    stem.startNewSubPath (0.f, yAt (0.f));
    for (float x = 0.f; x <= W; x += 6.f) stem.lineTo (x, yAt (x));
    g.setColour (curTheme == 2 ? pal.bark.brighter (0.3f) : (woody || curTheme == 5 || curTheme == 6) ? pal.bark : pal.grassDark);
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
        case 4:   // meadow: grass tufts, daisies and a butterfly
            for (float x = 24.f; x < W - 18.f; x += 40.f, ++i)
            {
                const float sy = yAt (x), sw = sway (x);
                g.setColour (pal.grass.withAlpha (0.85f));
                for (int b = -1; b <= 1; ++b)
                    g.drawLine (x + (float) b * 3.f, sy, x + (float) b * 5.f + sw * 22.f, sy - 9.f - (b == 0 ? 4.f : 0.f), 1.4f);
                if (i % 2 == 0)
                {
                    const float fx = x + 18.f, fy = yAt (fx);
                    const float tx = fx + sw * 14.f, ty = fy - 16.f;
                    g.setColour (pal.grassDark);
                    g.drawLine (fx, fy, tx, ty, 1.5f);
                    drawLeaf (g, fx, fy - 5.f, 1.1f + sw, 8.f, pal.grass.withAlpha (0.9f));
                    drawFlower (g, tx, ty, 6.5f, pal.ink, pal.acc);
                }
            }
            drawButterfly (g, W * (0.06f + 0.88f * std::fmod (anim * 0.018f, 1.f)),
                           top - 2.f + 7.f * std::sin (anim * 1.1f) + 5.f * std::sin (anim * 2.7f), 6.f, anim * 11.f,
                           pal.acc.brighter (0.2f), pal.ink);
            break;

        case 5:   // tide: kelp swaying on the seabed, bubbles rising
            for (float x = 18.f; x < W - 12.f; x += 34.f, ++i)
            {
                const float h = 20.f + 8.f * (float) (i % 3), sw = 5.f * std::sin (anim * 0.9f + x * 0.07f);
                const juce::Point<float> p0 (x, yAt (x) + 3.f), p1 (x + sw * 0.5f, p0.y - h * 0.5f), p2 (x + sw * 1.6f, p0.y - h);
                juce::Path strand;
                strand.startNewSubPath (p0);
                strand.quadraticTo (p1, p2);
                g.setColour (pal.grass.withAlpha (0.9f));
                g.strokePath (strand, juce::PathStrokeType (1.8f));
                for (float t : { 0.45f, 0.8f })
                {
                    const float u = 1.f - t;
                    const float bx = u * u * p0.x + 2.f * u * t * p1.x + t * t * p2.x;
                    const float by = u * u * p0.y + 2.f * u * t * p1.y + t * t * p2.y;
                    drawLeaf (g, bx, by, (t < 0.6f ? 1.0f : -1.0f) + sw * 0.05f, 9.f + 3.f * (1.f - t), pal.grassDark.brighter (0.3f).withAlpha (0.9f));
                }
            }
            for (int k = 0; k < 8; ++k)
            {
                const float t = std::fmod (anim * 0.07f + (float) k * 0.125f, 1.f);
                const float bx = W * (0.05f + 0.12f * (float) k) + 8.f * std::sin (anim * 1.1f + (float) k * 2.f);
                drawBubble (g, bx, y0 - t * (y0 - top + 18.f), 2.f + (float) (k % 3), pal.glow, 1.f - t * 0.6f);
            }
            break;

        case 6:   // dune: cacti, agave and the odd desert bloom
            for (float x = 24.f; x < W - 18.f; x += 44.f, ++i)
            {
                const float sy = yAt (x);
                if (i % 3 == 0)
                    drawCactus (g, x, sy, 16.f + 4.f * (float) (i % 2), pal.grassDark.brighter (0.15f), pal.acc);
                else if (i % 3 == 1)
                    for (int b = -2; b <= 2; ++b)
                        drawLeaf (g, x, sy, (float) b * 0.45f + sway (x), 12.f - (float) std::abs (b) * 1.5f, pal.grass.withAlpha (0.9f));
                else
                {
                    const float tx = x + sway (x) * 12.f, ty = sy - 14.f;
                    g.setColour (pal.grassDark);
                    g.drawLine (x, sy, tx, ty, 1.4f);
                    drawFlower (g, tx, ty, 5.f, pal.amber, pal.accDark);
                }
            }
            break;

        case 7:   // moonlit: ferns and drifting fireflies
            for (float x = 22.f; x < W - 14.f; x += 44.f, ++i)
                drawFern (g, x, yAt (x), 24.f + 3.f * (float) (i % 3), (i % 2 == 0) ? 1.f : -1.f, pal.grassDark.brighter (0.2f), sway (x));
            for (int k = 0; k < 9; ++k)
            {
                const float fx = W * (0.05f + 0.11f * (float) k) + 22.f * std::sin (anim * 0.5f + (float) k * 1.7f);
                const float fy = top + (y0 - top) * 0.3f + 12.f * std::sin (anim * 0.7f + (float) k * 2.3f) - 6.f;
                const float pulse = 0.5f + 0.5f * std::sin (anim * 1.7f + (float) k * 2.1f);
                g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (0.6f * pulse), fx, fy, pal.acc.withAlpha (0.f), fx + 9.f, fy, true));
                g.fillEllipse (fx - 9.f, fy - 9.f, 18.f, 18.f);
                g.setColour (juce::Colours::white.withAlpha (0.35f + 0.5f * pulse));
                g.fillEllipse (fx - 1.2f, fy - 1.2f, 2.4f, 2.4f);
            }
            break;

        case 8:   // rain: big wet leaves with dewdrops, and rain falling into the vine
            for (float x = 26.f; x < W - 20.f; x += 48.f, ++i)
            {
                const float ang = ((i % 2 == 0) ? 1.05f : -1.05f) + sway (x);
                const float size = 24.f + 2.f * (float) (i % 3);
                drawLeaf (g, x, yAt (x), ang, size, (i % 2 == 0 ? pal.grass : pal.grassDark.brighter (0.3f)).withAlpha (0.92f));
                drawDew (g, x + std::sin (ang) * size, yAt (x) - std::cos (ang) * size + 4.f, 2.3f, pal.glow);
            }
            for (int k = 0; k < 10; ++k)
            {
                const float t = std::fmod (anim * 0.5f + (float) k * 0.1f, 1.f);
                const float rx = W * (0.04f + 0.098f * (float) k);
                if (t < 0.9f)
                {
                    const float ry = top - 10.f + t * (y0 - top + 10.f);
                    g.setColour (pal.ink.withAlpha (0.5f * (1.f - t * 0.4f)));
                    g.drawLine (rx, ry, rx - 1.6f, ry + 7.f, 1.1f);
                }
                else
                {
                    const float u = (t - 0.9f) / 0.1f;   // a ripple spreads where the drop lands
                    g.setColour (pal.glow.withAlpha (0.55f * (1.f - u)));
                    g.drawEllipse (rx - 2.f - 6.f * u, y0 - 1.f - 1.5f * u, 4.f + 12.f * u, 3.f * u + 1.f, 0.9f);
                }
            }
            break;

        case 9:   // lavender: sprigs and drifting petals
            for (float x = 20.f; x < W - 12.f; x += 28.f, ++i)
            {
                const float sy = yAt (x);
                drawLavenderSprig (g, x, sy, 20.f + 3.f * (float) (i % 3), sway (x) * 40.f + ((i % 2 == 0) ? 2.f : -2.f), pal.grassDark, pal.acc);
                if (i % 3 == 0) drawLeaf (g, x, sy, ((i % 2 == 0) ? 1.0f : -1.0f) + sway (x), 9.f, pal.grass.withAlpha (0.85f));
            }
            for (int k = 0; k < 6; ++k)
            {
                const float t = std::fmod (anim * 0.05f + (float) k * 0.167f, 1.f);
                const float fx = W * (0.08f + 0.16f * (float) k) + 20.f * std::sin (anim * 0.8f + (float) k * 2.3f);
                juce::Path p;
                p.addEllipse (-3.f, -1.6f, 6.f, 3.2f);
                g.setColour (pal.acc.brighter (0.4f).withAlpha (0.85f * (1.f - t * 0.6f)));
                g.fillPath (p, juce::AffineTransform::rotation (anim * 1.3f + (float) k).translated (fx, top + t * (y0 - top - 3.f)));
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

    auto wave = [&] (float y0, float amp, float freq, float phase, float thick, juce::Colour c, float alpha)
    {
        juce::Path p;
        bool first = true;
        for (float x = -10.f; x <= W + 10.f; x += 10.f)
        {
            const float y = y0 + amp * std::sin (x * freq + phase) + amp * 0.4f * std::sin (x * freq * 2.3f + phase * 1.7f);
            if (first) { p.startNewSubPath (x, y); first = false; }
            else         p.lineTo (x, y);
        }
        g.setColour (c.withAlpha (alpha));
        g.strokePath (p, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        case 4:   // meadow: high, clear light and a haze over the grass
            shaft (W * 0.44f, 120.f, -200.f, 0.05f, pal.glow);
            shaft (W * 0.62f,  80.f, -240.f, 0.07f, pal.glow);
            shaft (W * 0.78f, 140.f, -290.f, 0.05f, pal.amber);
            {
                juce::ColourGradient hz (pal.glow.withAlpha (0.10f), W * 0.5f, H + 20.f, pal.glow.withAlpha (0.f), W * 0.5f, H - 320.f, true);
                g.setGradientFill (hz);
                g.fillRect (0.f, 0.f, W, H);
            }
            break;

        case 5:   // tide: light falling through water, with faint caustic ribbons
            shaft (W * 0.40f,  70.f, -160.f, 0.08f, pal.glow);
            shaft (W * 0.58f, 120.f, -200.f, 0.07f, pal.glow);
            shaft (W * 0.78f,  60.f, -230.f, 0.06f, pal.glow);
            for (int k = 0; k < 4; ++k)
                wave (30.f + 34.f * (float) k, 9.f + 2.f * (float) k, 0.010f + 0.003f * (float) k, (float) k * 1.7f,
                      18.f + 6.f * (float) k, pal.glow, 0.035f);
            break;

        case 6:   // dune: a low sun on the horizon and layered ridges of sand
        {
            juce::ColourGradient sunLow (pal.glow.withAlpha (0.26f), W * 0.30f, H - 90.f, pal.glow.withAlpha (0.f), W * 0.30f, H - 390.f, true);
            g.setGradientFill (sunLow);
            g.fillRect (0.f, 0.f, W, H);
            for (int k = 0; k < 3; ++k)
            {
                juce::Path ridge;
                const float yb = H - 120.f + 36.f * (float) k;
                ridge.startNewSubPath (0.f, H);
                for (float x = 0.f; x <= W; x += 8.f)
                    ridge.lineTo (x, yb + 16.f * std::sin (x * 0.006f + (float) k * 2.3f) + 7.f * std::sin (x * 0.017f + (float) k));
                ridge.lineTo (W, H);
                ridge.closeSubPath();
                g.setColour (black.withAlpha (0.07f + 0.05f * (float) k));
                g.fillPath (ridge);
            }
            shaft (W * 0.66f, 100.f, -260.f, 0.05f, pal.acc);
            break;
        }

        case 7:   // moonlit: a moon, a scatter of stars and a pale beam
        {
            const float mx = W * 0.84f, my = 66.f;
            juce::ColourGradient halo (pal.glow.withAlpha (0.30f), mx, my, pal.glow.withAlpha (0.f), mx + 150.f, my, true);
            g.setGradientFill (halo);
            g.fillEllipse (mx - 150.f, my - 150.f, 300.f, 300.f);
            g.setColour (pal.amber.withAlpha (0.88f));
            g.fillEllipse (mx - 20.f, my - 20.f, 40.f, 40.f);
            g.setColour (pal.bgMid.withAlpha (0.20f));
            g.fillEllipse (mx - 8.f, my - 6.f, 11.f, 11.f);
            g.fillEllipse (mx + 5.f, my + 6.f, 7.f, 7.f);
            for (int k = 0; k < 30; ++k)
            {
                const float sx = std::fmod ((float) k * 0.6180339f, 1.f) * W, sy = std::fmod ((float) k * 0.4142f, 1.f) * H * 0.42f;
                const float sr = 0.7f + 0.9f * std::fmod ((float) k * 0.73f, 1.f);
                g.setColour (juce::Colours::white.withAlpha (0.14f + 0.22f * std::fmod ((float) k * 0.37f, 1.f)));
                g.fillEllipse (sx - sr, sy - sr, sr * 2.f, sr * 2.f);
            }
            shaft (W * 0.70f, 90.f, -250.f, 0.05f, pal.glow);
            break;
        }

        case 8:   // rain: mist hanging off the top and fine slanting rain
        {
            juce::ColourGradient mist (pal.glow.withAlpha (0.14f), 0.f, 0.f, pal.glow.withAlpha (0.f), 0.f, 260.f, false);
            g.setGradientFill (mist);
            g.fillRect (0.f, 0.f, W, 260.f);
            g.setColour (pal.ink.withAlpha (0.045f));
            for (int k = 0; k < 90; ++k)
            {
                const float rx = std::fmod ((float) k * 0.7548777f, 1.f) * (W + 80.f) - 20.f, ry = std::fmod ((float) k * 0.3819f, 1.f) * H;
                g.drawLine (rx, ry, rx - 6.f, ry + 16.f + 12.f * std::fmod ((float) k * 0.31f, 1.f), 1.f);
            }
            break;
        }

        case 9:   // lavender: a warm band of dusk light over the field
            shaft (W * 0.48f, 100.f, -230.f, 0.06f, pal.amber);
            shaft (W * 0.68f,  60.f, -260.f, 0.08f, pal.glow);
            shaft (W * 0.84f, 120.f, -300.f, 0.06f, pal.acc);
            {
                juce::ColourGradient hz (pal.acc.withAlpha (0.14f), W * 0.5f, H + 10.f, pal.acc.withAlpha (0.f), W * 0.5f, H - 280.f, true);
                g.setGradientFill (hz);
                g.fillRect (0.f, 0.f, W, H);
            }
            break;
    }

    {   // vignette
        juce::ColourGradient vg (black.withAlpha (0.f), W * 0.5f, H * 0.46f, black.withAlpha (0.26f), 0.f, 0.f, true);
        vg.addColour (0.55, black.withAlpha (0.f));
        vg.addColour (0.80, black.withAlpha (0.13f));
        g.setGradientFill (vg);
        g.fillRect (0.f, 0.f, W, H);
    }
    {   // mist along the bottom, lit from within
        g.setGradientFill (juce::ColourGradient (black.withAlpha (0.f), 0.f, H - 130.f, black.withAlpha (0.28f), 0.f, H, false));
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
    const auto f = logoFont (kLogoSize);
    g.setFont (f);
    const float top = logoBoxTop (f, kLogoBase, 80.f);
    for (int layer = 0; layer < 3; ++layer)
    {
        const float rr = 6.f - 2.f * (float) layer;   // 6, 4, 2 px
        g.setColour (pal.glow.withAlpha (0.02f + 0.012f * (float) layer));
        for (int k = 0; k < 12; ++k)
        {
            const float ang = (float) k * twoPi / 12.f;
            g.drawText ("nature", juce::Rectangle<float> (titleX() + std::cos (ang) * rr, top + std::sin (ang) * rr, 300.f, 80.f),
                        juce::Justification::centredLeft, false);
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

            case 4:   // meadow: grass blades and white petals
                if (k % 2 == 0)
                    drawLeaf (g, x, y, rot, 8.f + 4.f * depth, pal.grass.withAlpha (a));
                else
                {
                    juce::Path p;
                    p.addEllipse (-3.f * depth, -1.8f * depth, 6.f * depth, 3.6f * depth);
                    g.setColour (pal.ink.withAlpha (a));
                    g.fillPath (p, juce::AffineTransform::rotation (rot).translated (x, y));
                }
                break;

            case 5:   // tide: bubbles rising
                drawBubble (g, x, area.getBottom() - (y - area.getY()), 2.f + 3.f * depth, pal.glow, a);
                break;

            case 6:   // dune: wind-blown sand
                g.setColour (pal.amber.withAlpha (a * 0.8f));
                g.fillEllipse (x - 1.2f * depth, y - 1.2f * depth, 2.4f * depth, 2.4f * depth);
                if (k % 5 == 0) g.drawLine (x, y, x + 14.f * depth, y + 1.5f, 1.f);
                break;

            case 7:   // moonlit: fireflies
            {
                const float fy = area.getY() + area.getHeight() * (0.2f + 0.6f * baseX) + 8.f * std::sin (anim * 0.9f + (float) k);
                const float pulse = 0.5f + 0.5f * std::sin (anim * 1.6f + (float) k * 2.1f);
                g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (0.6f * idleFade * pulse), x, fy, pal.acc.withAlpha (0.f), x + 8.f, fy, true));
                g.fillEllipse (x - 8.f, fy - 8.f, 16.f, 16.f);
                g.setColour (juce::Colours::white.withAlpha (0.8f * idleFade * pulse));
                g.fillEllipse (x - 1.2f, fy - 1.2f, 2.4f, 2.4f);
                break;
            }

            case 8:   // rain: streaks
                g.setColour (pal.glow.withAlpha (a * 0.9f));
                g.drawLine (x, y, x - 2.f, y + 8.f + 4.f * depth, 1.1f);
                break;

            case 9:   // lavender: petals
            {
                juce::Path p;
                p.addEllipse (-3.5f * depth, -1.8f * depth, 7.f * depth, 3.6f * depth);
                g.setColour (pal.acc.brighter (0.4f).withAlpha (a));
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

        juce::DropShadow (black.withAlpha (0.28f), 12, juce::Point<int> (0, 5)).drawForPath (g, card);
        juce::DropShadow (pal.glow.withAlpha (inset ? 0.08f : 0.15f), inset ? 10 : 18, juce::Point<int> (0, 0)).drawForPath (g, card);
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (card);

        g.setColour (pal.soil.withAlpha (tint));
        g.fillRect (r);

        if (inset)   // recessed: shadow falling from the top edge
        {
            g.setGradientFill (juce::ColourGradient (black.withAlpha (0.30f), 0.f, r.getY(),
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

    juce::ColourGradient rim (white.withAlpha (inset ? 0.07f : 0.17f), 0.f, r.getY(),
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
    drawGlass (g, r, 12.f, 0.44f);
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
    if (animOn()) paintIdleDrift (g, wr);   // fades out while a sample plays

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

    // a faint glow under the waveform card while a sample is sounding
    if (playGlow > 0.01f)
    {
        const float a = smooth01 (playGlow) * (animOn() ? 0.86f + 0.14f * std::sin (anim * 2.4f) : 1.f);
        const float x0 = wr.getX() + 16.f, x1 = wr.getRight() - 16.f, y0 = wr.getBottom();

        g.setGradientFill (juce::ColourGradient (pal.glow.withAlpha (0.26f * a), 0.f, y0 - 1.f, pal.glow.withAlpha (0.f), 0.f, y0 + 20.f, false));
        g.fillRoundedRectangle (x0, y0 - 1.f, x1 - x0, 21.f, 10.f);

        juce::ColourGradient thread (pal.glow.withAlpha (0.f), x0, 0.f, pal.glow.withAlpha (0.f), x1, 0.f, false);   // a thin bright thread along the lower edge
        thread.addColour (0.5, pal.glow.withAlpha (0.55f * a));
        g.setGradientFill (thread);
        g.fillRect (x0, y0 - 0.5f, x1 - x0, 1.5f);
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
    else if (tab == 1)
    {
        drawGlass (g, eqPanel(), 12.f, 0.62f, true);
        for (int k = 0; k < 4; ++k) drawGroup (g, fxPanel (k));
    }
    else
    {
        drawGlass (g, settingsPanel(), 12.f, 0.55f);
        for (int i = 0; i < 4; ++i)   // a hairline between the rows
        {
            const auto r = settingsRow (i);
            g.setColour (pal.line.withAlpha (0.55f));
            g.drawHorizontalLine ((int) r.getBottom() + 2, r.getX() + 6.f, r.getRight() - 6.f);
        }
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
    const bool falls = (curTheme == 2 || curTheme == 3 || curTheme == 8);
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
    juce::Rectangle<float> cards[8];
    int n = 0;
    cards[n++] = eqRect();
    if (tab == 0) { cards[n++] = waveRect(); cards[n++] = sampleRect(); cards[n++] = grainPanel(); cards[n++] = motionPanel(); }
    else if (tab == 1) { cards[n++] = eqPanel(); for (int k = 0; k < 4; ++k) cards[n++] = fxPanel (k); }
    else               { cards[n++] = settingsPanel(); }

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
    // The wordmark: silver letters that breathe (each one drifts a hair up and down, a beat behind its neighbour),
    // with a little sun that now and then rises behind the word, crosses it, and sets, warming each letter it passes.
    {
        const auto& lay = logoLayout();
        const auto font = logoFont (kLogoSize);
        const float x0 = titleX(), pi = juce::MathConstants<float>::pi, boxH = 80.f;
        const float top = logoBoxTop (font, kLogoBase, boxH);
        static const char* word = "nature";

        // where the sun is: one slow pass of 8 s, then 8 s of rest
        const bool anims = animOn(), mono = proc.setMono.load();
        auto tone = [mono] (juce::Colour c) { return mono ? juce::Colour::greyLevel (c.getPerceivedBrightness()) : c; };

        const float ph = std::fmod (anim / 16.f, 1.f);
        float sunA = 0.f, sunX = 0.f, sunY = 0.f;
        if (anims && ph < 0.5f)
        {
            const float t = ph / 0.5f;
            sunA = smooth01 (juce::jmin (1.f, std::sin (t * pi) * 1.8f));   // fades in and out at the ends
            sunX = x0 - 26.f + t * (lay.total + 52.f);
            sunY = kLogoBase - 30.f - 7.f * std::sin (t * pi);              // rises a little as it crosses
        }

        auto sunBloom = [&] (float radius, juce::Colour c, float alpha)
        {
            c = tone (c);
            g.setGradientFill (juce::ColourGradient (c.withAlpha (alpha), sunX, sunY, c.withAlpha (0.f), sunX + radius, sunY, true));
            g.fillEllipse (sunX - radius, sunY - radius, radius * 2.f, radius * 2.f);
        };

        if (sunA > 0.005f)   // the light behind the letters
        {
            sunBloom (78.f, juce::Colour (0xffff9a2e), 0.20f * sunA);
            sunBloom (44.f, juce::Colour (0xffffb340), 0.42f * sunA);
            sunBloom (19.f, juce::Colour (0xffffd57a), 0.90f * sunA);
        }

        const auto silver = pal.ink.withSaturation (0.07f);
        const auto warmTop = tone (juce::Colour (0xfffff0c8)), warmBot = tone (juce::Colour (0xffe9953a));
        g.setFont (font);

        for (int i = 0; i < 6; ++i)
        {
            const float bob = anims ? 1.9f * std::sin (anim * 1.4f - (float) i * 0.5f) : 0.f;   // the breathing
            const float cx = x0 + lay.x[i] + lay.w[i] * 0.5f;
            const float dx = cx - sunX;
            const float lit = sunA * std::exp (-(dx * dx) / (2.f * 40.f * 40.f));

            const auto r = juce::Rectangle<float> (x0 + lay.x[i], top + bob, lay.w[i] + 24.f, boxH);
            const juce::String ch = juce::String::charToString ((juce::juce_wchar) word[i]);

            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawText (ch, r.translated (1.f, 2.f), juce::Justification::centredLeft, false);

            const auto tc = silver.brighter (0.2f).interpolatedWith (warmTop, lit * 0.9f);
            const auto bc = silver.darker (0.5f).interpolatedWith (warmBot, lit * 0.7f);
            g.setGradientFill (juce::ColourGradient (tc, 0.f, kLogoBase - 40.f + bob, bc, 0.f, kLogoBase + 2.f + bob, false));
            g.drawText (ch, r, juce::Justification::centredLeft, false);
        }

        if (sunA > 0.005f)   // and a touch of its light spilling over the tops of the letters
        {
            sunBloom (34.f, juce::Colour (0xffffe3a0), 0.26f * sunA);
            sunBloom (12.f, juce::Colour (0xfffff4d0), 0.40f * sunA);
        }
    }

    if (tab == 0)
    {
        drawGroupTitle (g, grainPanel(),  "grove: grains");
        drawGroupTitle (g, motionPanel(), "bloom: motion");
    }
    else if (tab == 2)
    {
        drawGroupTitle (g, settingsPanel(), "hollow: settings");
        struct Row { const char* title; const char* note; };
        static const Row rows[5] = {
            { "master",             "The overall output volume, applied after every effect." },
            { "disable tips",       "Hides the little notes that appear when you rest the pointer on a knob or button." },
            { "disable animations", "Stops the swaying, breathing, glints and drifting lights. The waveform and meters still move." },
            { "disable themes",     "Turns the whole plugin black and white. The theme button is locked until this is switched off." },
            { "startup size",       "How big the window is when it opens. Last used keeps whatever zoom you picked most recently." } };
        for (int i = 0; i < 5; ++i)
        {
            const auto r = settingsRow (i);
            g.setFont (serif (20.f));
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawText (rows[i].title, juce::Rectangle<float> (r.getX() + 15.f, r.getY() + 9.f, 340.f, 26.f), juce::Justification::centredLeft);
            g.setColour (pal.ink);
            g.drawText (rows[i].title, juce::Rectangle<float> (r.getX() + 14.f, r.getY() + 8.f, 340.f, 26.f), juce::Justification::centredLeft);
            g.setFont (serif (13.f, true));
            g.setColour (pal.dim);
            g.drawFittedText (rows[i].note, juce::Rectangle<float> (r.getX() + 14.f, r.getY() + 38.f, r.getWidth() - 200.f, 34.f).toNearestInt(),
                              juce::Justification::topLeft, 2);
        }
    }
    else
    {
        drawGroupTitle (g, eqPanel(),  "clearing: equalizer");
        drawGroupTitle (g, fxPanel (0), "ripple: phaser");
        drawGroupTitle (g, fxPanel (1), "birdsong: chorus");
        drawGroupTitle (g, fxPanel (2), "canyon: delay");
        drawGroupTitle (g, fxPanel (3), "cavern: reverb");
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
        // in silence the bars sway in a slow travelling swell, so the box never looks like an empty slot
        const float idle = 0.5f + 0.5f * std::sin (anim * 1.3f - (float) i * 0.55f);
        const float swell = (0.10f + 0.26f * idle) * (0.6f + 0.4f * std::sin (anim * 0.5f + (float) i * 0.21f));
        const float h = 2.f + (maxH - 2.f) * lv + (maxH - 2.f) * swell * (1.f - lv);
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

    std::array<juce::Slider*, 30> all {};
    for (size_t i = 0; i < knobs.size(); ++i) all[i] = &knobs[i].slider;

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
    for (auto* b : { &droneBtn, &tuneBtn, &donBtn, &ronBtn, &specBtn, &ponBtn, &conBtn, &setBtn[0], &setBtn[1], &setBtn[2] })
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

    const juce::Component* extra[7] = { &droneBtn, &tuneBtn, nullptr, &spaceBtn, nullptr, &donBtn, &ronBtn };   // 32 and 34 were the filter
    for (int k = 0; k < 7; ++k)
        if (extra[k] != nullptr && extra[k]->isVisible() && extra[k]->isMouseOver (true))
            return 30 + k;

    if (specBtn.isVisible() && specBtn.isMouseOver (true)) return 37;
    if (ponBtn.isVisible() && ponBtn.isMouseOver (true))   return 38;
    if (conBtn.isVisible() && conBtn.isMouseOver (true))   return 39;
    if (eqView != nullptr && eqView->isVisible())
    {
        if (eqView->preBtn.isMouseOver (true)) return 41;
        if (eqView->isMouseOver (true))        return 40;
    }

    return -1;
}

// A small field note that rises into the footer, clear of every control
void ScattrPanel::paintTip (juce::Graphics& g)
{
    if (tipAlpha < 0.01f || tipId < 0 || tipId >= 42) return;

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

    if (animOn()) { paintMotes (g); paintGlints (g); }
    paintLabels (g);

    // little sprout beside the title
    {
        const float sw = animOn() ? 0.08f * std::sin (anim * 0.9f) : 0.f, bx = sproutX();
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

// ---------------------------------------------------------------- equalizer display

namespace
{
constexpr float kSpecMin = -96.f, kSpecMax = -6.f;   // what the spectrum's vertical extent shows (dB)
constexpr float kPlotDb = EqCore::maxGainDb;         // the plot shows +-18 dB of EQ gain

juce::String hzText (float hz)
{
    return hz >= 1000.f ? juce::String (hz / 1000.f, hz >= 10000.f ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
}
}

EqDisplay::EqDisplay (ScattrPanel& pn, ScattrProcessor& pr) : panel (pn), proc (pr)
{
    for (int i = 0; i < EqCore::numBands; ++i)
    {
        const juce::String id ("eq" + juce::String (i));
        bp[(size_t) i].on   = proc.apvts.getParameter (id + "on");
        bp[(size_t) i].type = proc.apvts.getParameter (id + "ty");
        bp[(size_t) i].freq = proc.apvts.getParameter (id + "f");
        bp[(size_t) i].gain = proc.apvts.getParameter (id + "g");
        bp[(size_t) i].q    = proc.apvts.getParameter (id + "q");
        bp[(size_t) i].slope = proc.apvts.getParameter (id + "sl");
        bp[(size_t) i].dyn    = proc.apvts.getParameter (id + "dy");
        bp[(size_t) i].thresh = proc.apvts.getParameter (id + "dt");
        bp[(size_t) i].range  = proc.apvts.getParameter (id + "dr");
        bp[(size_t) i].speed  = proc.apvts.getParameter (id + "ds");
    }

    addAndMakeVisible (preBtn);
    preBtn.getProperties().set ("fontSize", 11.0);
    preBtn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void EqDisplay::resized()
{
    preBtn.setBounds (getWidth() - 36 - 46, 5, 46, 20);
}

float EqDisplay::xOfHz (float hz) const
{
    const auto p = plot();
    return p.getX() + std::log (juce::jmax (hz, 1.f) / EqCore::minHz) / std::log (EqCore::maxHz / EqCore::minHz) * p.getWidth();
}

float EqDisplay::hzOfX (float x) const
{
    const auto p = plot();
    const float t = juce::jlimit (0.f, 1.f, (x - p.getX()) / p.getWidth());
    return EqCore::minHz * std::pow (EqCore::maxHz / EqCore::minHz, t);
}

float EqDisplay::yOfDb (float db) const
{
    const auto p = plot();
    return p.getCentreY() - db / kPlotDb * p.getHeight() * 0.5f;
}

float EqDisplay::dbOfY (float y) const
{
    const auto p = plot();
    return (p.getCentreY() - y) / (p.getHeight() * 0.5f) * kPlotDb;
}

EqCore::Band EqDisplay::readBand (int i) const
{
    const auto& b = bp[(size_t) i];
    EqCore::Band r;
    r.on     = b.on->getValue() > 0.5f;
    r.type   = juce::jlimit (0, EqCore::numTypes - 1, juce::roundToInt (b.type->convertFrom0to1 (b.type->getValue())));
    r.freq   = b.freq->convertFrom0to1 (b.freq->getValue());
    r.gainDb = b.gain->convertFrom0to1 (b.gain->getValue());
    r.q      = b.q->convertFrom0to1 (b.q->getValue());
    r.slope  = juce::jlimit (0, EqCore::numSlopes - 1, juce::roundToInt (b.slope->convertFrom0to1 (b.slope->getValue())));
    r.dyn    = b.dyn->getValue() > 0.5f;
    r.thresh = b.thresh->convertFrom0to1 (b.thresh->getValue());
    r.range  = b.range->convertFrom0to1 (b.range->getValue());
    r.speed  = juce::jlimit (0, EqCore::numSpeeds - 1, juce::roundToInt (b.speed->convertFrom0to1 (b.speed->getValue())));
    return r;
}

juce::Point<float> EqDisplay::nodePos (int i) const
{
    const auto b = readBand (i);
    return { xOfHz (b.freq), yOfDb (EqCore::usesGain (b.type) ? b.gainDb : 0.f) };
}

int EqDisplay::hitBand (juce::Point<float> p) const
{
    int best = -1;
    float bestD = 12.f * 12.f;
    for (int i = 0; i < EqCore::numBands; ++i)
    {
        if (! readBand (i).on) continue;
        const auto n = nodePos (i);
        const float d = (n.x - p.x) * (n.x - p.x) + (n.y - p.y) * (n.y - p.y);
        if (d <= bestD) { bestD = d; best = i; }
    }
    return best;
}

juce::Colour EqDisplay::bandColour (int i)
{
    return juce::Colour::fromHSV (std::fmod (0.58f + 0.381966f * (float) i, 1.f), 0.42f, 0.98f, 1.f);   // soft pastels, spread by the golden angle so neighbouring bands differ
}

void EqDisplay::setNorm (juce::RangedAudioParameter* p, float plainValue)
{
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void EqDisplay::beginGesture (int band)
{
    beginDrag (bp[(size_t) band].freq, bp[(size_t) band].gain);
}

void EqDisplay::beginDrag (juce::RangedAudioParameter* a, juce::RangedAudioParameter* b)
{
    dragA = a;
    dragB = b;
    if (dragA != nullptr) dragA->beginChangeGesture();
    if (dragB != nullptr) dragB->beginChangeGesture();
}

void EqDisplay::endGesture()
{
    if (dragA != nullptr) dragA->endChangeGesture();
    if (dragB != nullptr) dragB->endChangeGesture();
    dragA = dragB = nullptr;
}

// where a dynamic band's threshold sits on the analyzer's own scale (the analyzer is tilted, so the line follows it)
float EqDisplay::yOfThreshold (const EqCore::Band& b) const
{
    const auto p = plot();
    const float shown = b.thresh + analyzerTilt (b.freq);
    return p.getBottom() - juce::jlimit (0.f, 1.f, (shown - kSpecMin) / (kSpecMax - kSpecMin)) * p.getHeight();
}

int EqDisplay::hitDynHandle (juce::Point<float> pt, int band) const
{
    if (band < 0) return 0;
    const auto b = readBand (band);
    if (! b.on || ! b.dyn || ! EqCore::usesGain (b.type)) return 0;

    const float x = xOfHz (b.freq);
    const float yR = yOfDb (juce::jlimit (-kPlotDb, kPlotDb, b.gainDb + b.range));
    if ((pt.x - x) * (pt.x - x) + (pt.y - yR) * (pt.y - yR) <= 10.f * 10.f) return 1;
    if (std::abs (pt.x - x) <= 38.f && std::abs (pt.y - yOfThreshold (b)) <= 6.f) return 2;
    return 0;
}

void EqDisplay::addBandAt (juce::Point<float> p)
{
    Scope undoScope (*this);
    for (int i = 0; i < EqCore::numBands; ++i)
    {
        if (readBand (i).on) continue;

        auto put = [this] (juce::RangedAudioParameter* par, float v)
        {
            par->beginChangeGesture();
            setNorm (par, v);
            par->endChangeGesture();
        };
        put (bp[(size_t) i].type, (float) EqCore::Bell);
        put (bp[(size_t) i].freq, hzOfX (p.x));
        put (bp[(size_t) i].gain, juce::jlimit (-kPlotDb, kPlotDb, dbOfY (p.y)));
        put (bp[(size_t) i].q, 1.f);
        put (bp[(size_t) i].slope, 0.f);
        put (bp[(size_t) i].dyn, 0.f);
        put (bp[(size_t) i].thresh, -30.f);
        put (bp[(size_t) i].range, -6.f);
        put (bp[(size_t) i].speed, 1.f);
        put (bp[(size_t) i].on, 1.f);
        selected = i;
        return;
    }
}

void EqDisplay::removeBand (int i)
{
    for (auto* par : { bp[(size_t) i].on, bp[(size_t) i].gain })
    {
        par->beginChangeGesture();
        setNorm (par, 0.f);
        par->endChangeGesture();
    }
    if (selected == i) selected = -1;
    if (hover == i) hover = -1;
}

void EqDisplay::showTypeMenu (int i)
{
    juce::PopupMenu m;
    const auto cur = readBand (i);
    for (int t = 0; t < EqCore::numTypes; ++t) m.addItem (t + 1, EqCore::typeName (t), true, t == cur.type);

    if (EqCore::isCut (cur.type))   // cuts can be made steeper
    {
        juce::PopupMenu sub;
        for (int k = 0; k < EqCore::numSlopes; ++k) sub.addItem (20 + k, EqCore::slopeName (k), true, k == cur.slope);
        m.addSeparator();
        m.addSubMenu ("Slope", sub);
    }

    m.addSeparator();
    m.addItem (60, "Dynamic", EqCore::usesGain (cur.type), cur.dyn);   // the band reacts to the level around it
    if (cur.dyn && EqCore::usesGain (cur.type))
    {
        juce::PopupMenu sp;
        for (int k = 0; k < EqCore::numSpeeds; ++k) sp.addItem (70 + k, EqCore::speedName (k), true, k == cur.speed);
        m.addSubMenu ("Dynamic speed", sp);
    }

    m.addSeparator();
    m.addItem (100, "Remove band");

    juce::Component::SafePointer<EqDisplay> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, i] (int result)
    {
        if (safe == nullptr || result <= 0) return;
        Scope undoScope (*safe);   // everything below is one undo step
        if (result == 100) { safe->removeBand (i); return; }

        if (result == 60 || (result >= 70 && result < 70 + EqCore::numSpeeds))
        {
            auto* par = result == 60 ? safe->bp[(size_t) i].dyn : safe->bp[(size_t) i].speed;
            const float v = result == 60 ? (safe->readBand (i).dyn ? 0.f : 1.f) : (float) (result - 70);
            par->beginChangeGesture();
            safe->setNorm (par, v);
            par->endChangeGesture();
            return;
        }

        if (result >= 20 && result < 20 + EqCore::numSlopes)
        {
            auto* par = safe->bp[(size_t) i].slope;
            par->beginChangeGesture();
            safe->setNorm (par, (float) (result - 20));
            par->endChangeGesture();
            return;
        }

        const auto before = safe->readBand (i);
        auto* par = safe->bp[(size_t) i].type;
        par->beginChangeGesture();
        safe->setNorm (par, (float) (result - 1));
        par->endChangeGesture();

        if (EqCore::isCut (result - 1) && ! EqCore::isCut (before.type))   // a fresh cut starts out flat (no resonant bump)
        {
            auto* q = safe->bp[(size_t) i].q;
            q->beginChangeGesture();
            safe->setNorm (q, 0.71f);
            q->endChangeGesture();
        }
    });
}

void EqDisplay::showPlotMenu (juce::Point<float> p)
{
    int used = 0;
    for (int i = 0; i < EqCore::numBands; ++i) if (readBand (i).on) ++used;

    juce::PopupMenu m;
    m.addItem (1, "Add band here", used < EqCore::numBands);
    m.addItem (2, "Clear all bands", used > 0);
    m.addSeparator();
    m.addItem (3, "Undo", ! undoStack.empty());
    m.addItem (4, "Redo", ! redoStack.empty());

    juce::Component::SafePointer<EqDisplay> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, p] (int result)
    {
        if (safe == nullptr || result <= 0) return;
        if (result == 1) safe->addBandAt (p);
        else if (result == 3) safe->undo();
        else if (result == 4) safe->redo();
        else if (result == 2)
        {
            Scope undoScope (*safe);
            for (int i = 0; i < EqCore::numBands; ++i)
                if (safe->readBand (i).on) safe->removeBand (i);
        }
    });
}

// ---- undo / redo

EqDisplay::Snap EqDisplay::snapshot() const
{
    Snap s;
    for (int i = 0; i < EqCore::numBands; ++i) s.b[(size_t) i] = readBand (i);
    return s;
}

void EqDisplay::record (const Snap& before)
{
    if (before == snapshot()) return;   // nothing actually changed
    undoStack.push_back (before);
    if (undoStack.size() > 200) undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void EqDisplay::applySnapshot (const Snap& s)
{
    auto put = [] (juce::RangedAudioParameter* par, float v)
    {
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 (v));
        par->endChangeGesture();
    };

    for (int i = 0; i < EqCore::numBands; ++i)
    {
        const auto& b = s.b[(size_t) i];
        if (readBand (i) == b) continue;
        auto& p = bp[(size_t) i];
        put (p.type, (float) b.type);
        put (p.freq, b.freq);
        put (p.gain, b.gainDb);
        put (p.q, b.q);
        put (p.slope, (float) b.slope);
        put (p.dyn, b.dyn ? 1.f : 0.f);
        put (p.thresh, b.thresh);
        put (p.range, b.range);
        put (p.speed, (float) b.speed);
        put (p.on, b.on ? 1.f : 0.f);
    }
    selected = hover = -1;
    dragging = false;
}

bool EqDisplay::undo()
{
    if (undoStack.empty() || dragging) return false;
    redoStack.push_back (snapshot());
    const auto s = undoStack.back();
    undoStack.pop_back();
    applySnapshot (s);
    return true;
}

bool EqDisplay::redo()
{
    if (redoStack.empty() || dragging) return false;
    undoStack.push_back (snapshot());
    const auto s = redoStack.back();
    redoStack.pop_back();
    applySnapshot (s);
    return true;
}

void EqDisplay::mouseMove (const juce::MouseEvent& e)
{
    hover = hitBand (e.position);
    hoverPt = e.position;
    const int fb = selected >= 0 ? selected : hover;
    const bool overHandle = hover < 0 && hitDynHandle (e.position, fb) != 0;
    setMouseCursor (hover >= 0 ? juce::MouseCursor::PointingHandCursor
                               : overHandle ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
}

void EqDisplay::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
}

void EqDisplay::mouseDown (const juce::MouseEvent& e)
{
    panel.grabKeyboardFocus();   // so Ctrl+Z works right after clicking here
    const int hit = hitBand (e.position);
    if (e.mods.isPopupMenu())
    {
        if (hit >= 0) { selected = hit; showTypeMenu (hit); }
        else if (plot().expanded (6.f).contains (e.position)) showPlotMenu (e.position);
        return;
    }
    if (hit < 0)
    {
        // not on a node: maybe on the dynamic handles of the band that is selected (or under the pointer)
        const int fb = selected >= 0 ? selected : hover;
        const int h = hitDynHandle (e.position, fb);
        if (h == 0) { selected = -1; return; }

        selected = fb;
        dragging = true;
        dragKind = h;
        dragBefore = snapshot();
        beginDrag (h == 1 ? bp[(size_t) fb].range : bp[(size_t) fb].thresh);
        return;
    }

    selected = hit;
    dragging = true;
    dragKind = 0;
    dragBefore = snapshot();
    beginGesture (hit);
}

void EqDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging || selected < 0) return;

    const auto b = readBand (selected);
    if (dragKind == 1)   // the range handle: its height is the gain the band moves to
    {
        setNorm (bp[(size_t) selected].range, juce::jlimit (-kPlotDb, kPlotDb, dbOfY (e.position.y) - b.gainDb));
        return;
    }
    if (dragKind == 2)   // the threshold line, read off the analyzer's scale
    {
        const auto pl = plot();
        const float shown = kSpecMin + (pl.getBottom() - e.position.y) / pl.getHeight() * (kSpecMax - kSpecMin);
        setNorm (bp[(size_t) selected].thresh, juce::jlimit (-80.f, 0.f, shown - analyzerTilt (b.freq)));
        return;
    }
    setNorm (bp[(size_t) selected].freq, hzOfX (e.position.x));
    if (EqCore::usesGain (b.type))
        setNorm (bp[(size_t) selected].gain, juce::jlimit (-kPlotDb, kPlotDb, dbOfY (e.position.y)));
}

void EqDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragging) { endGesture(); record (dragBefore); }
    dragging = false;
    dragKind = 0;
}

void EqDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hit = hitBand (e.position);
    if (hit < 0 && hitDynHandle (e.position, selected >= 0 ? selected : hover) != 0) return;   // a handle, not empty space
    if (hit >= 0) { Scope undoScope (*this); removeBand (hit); }
    else if (plot().expanded (6.f).contains (e.position)) addBandAt (e.position);
}

void EqDisplay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int b = dragging ? selected : hitBand (e.position);
    if (b < 0) { Component::mouseWheelMove (e, w); return; }

    const auto before = snapshot();

    if (e.mods.isShiftDown())   // shift + scroll: change the slope of a cut band (up = steeper), one step per wheel notch
    {
        if (! EqCore::isCut (readBand (b).type)) return;   // only cuts have a slope; leave the width alone

        const float d = w.deltaY != 0.f ? w.deltaY : w.deltaX;   // some systems turn a shifted scroll sideways
        slopeWheelAcc += d;
        if (std::abs (slopeWheelAcc) < 0.1f) return;   // fine-grained wheels and trackpads: wait until it adds up to one notch
        const int dir = slopeWheelAcc > 0.f ? 1 : -1;
        slopeWheelAcc = 0.f;

        const int cur = readBand (b).slope;
        const int next = juce::jlimit (0, EqCore::numSlopes - 1, cur + dir);
        selected = b;
        if (next == cur) return;

        auto* sp = bp[(size_t) b].slope;
        sp->beginChangeGesture();
        setNorm (sp, (float) next);
        sp->endChangeGesture();

        const double nowS = juce::Time::getMillisecondCounterHiRes();
        if (b != lastWheelBand || nowS - lastWheelMs > 600.0) record (before);
        lastWheelBand = b;
        lastWheelMs = nowS;
        return;
    }

    auto* par = bp[(size_t) b].q;
    const float q = juce::jlimit (0.1f, 18.f, readBand (b).q * std::exp (w.deltaY * 2.5f));   // scroll up = narrower
    par->beginChangeGesture();
    setNorm (par, q);
    par->endChangeGesture();
    selected = b;

    const double now = juce::Time::getMillisecondCounterHiRes();   // a burst of scrolling on one band is a single undo step
    if (b != lastWheelBand || now - lastWheelMs > 600.0) record (before);
    lastWheelBand = b;
    lastWheelMs = now;
}

// ---- the spectrum

void EqDisplay::computeSpectrum (const std::atomic<float>* ring, std::vector<float>& raw, std::vector<float>& sm, bool stale)
{
    const int cols = (int) sm.size();
    if (! stale)
    {
        const int w = proc.eqTapW.load (std::memory_order_acquire);
        for (int i = 0; i < 4096; ++i)
            fftData[(size_t) i] = ring[(w - 4096 + i) & (ScattrProcessor::eqTapSize - 1)].load (std::memory_order_relaxed);
        std::fill (fftData.begin() + 4096, fftData.end(), 0.f);
        window.multiplyWithWindowingTable (fftData.data(), 4096);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float binHz = juce::jmax (1.f, proc.uiSampleRate.load()) / 4096.f;
        const float ratio = EqCore::maxHz / EqCore::minHz;
        for (int c = 0; c < cols; ++c)
        {
            const float f0 = EqCore::minHz * std::pow (ratio, (float) c / (float) cols);
            const float f1 = EqCore::minHz * std::pow (ratio, (float) (c + 1) / (float) cols);
            const float b0 = f0 / binHz, b1 = f1 / binHz;
            float m = 0.f;
            if (b1 - b0 < 1.f)   // a narrow column: interpolate between the two nearest bins
            {
                const float bc = 0.5f * (b0 + b1);
                const int i0 = juce::jlimit (0, 2045, (int) bc);
                const float fr = bc - (float) i0;
                m = fftData[(size_t) i0] * (1.f - fr) + fftData[(size_t) i0 + 1] * fr;
            }
            else
                for (int b = (int) std::ceil (b0); b <= juce::jmin (2046, (int) std::floor (b1)); ++b)
                    m = juce::jmax (m, fftData[(size_t) b]);

            const float amp = m * 4.f / 4096.f;   // 0 dB = a full-scale sine
            raw[(size_t) c] = 20.f * std::log10 (amp + 1.0e-9f) + 4.5f * std::log2 (std::sqrt (f0 * f1) / 1000.f);   // 4.5 dB/oct tilt, like most analyzers
        }
    }
    else
        std::fill (raw.begin(), raw.end(), kSpecMin);

    for (int c = 0; c < cols; ++c)   // fast attack, slow release
    {
        const float t = raw[(size_t) c], s = sm[(size_t) c];
        sm[(size_t) c] = t > s ? s + (t - s) * 0.55f : s + (t - s) * 0.10f;
    }
}

void EqDisplay::update()
{
    if (! isVisible()) return;

    const int cols = juce::jmax (8, (int) (plot().getWidth() / 2.f));
    if ((int) specPost.size() != cols)
    {
        specPost.assign ((size_t) cols, kSpecMin);  specPre.assign ((size_t) cols, kSpecMin);
        rawPost.assign ((size_t) cols, kSpecMin);   rawPre.assign ((size_t) cols, kSpecMin);
    }

    const int w = proc.eqTapW.load (std::memory_order_acquire);
    staleTicks = (w == lastTapW) ? staleTicks + 1 : 0;   // the host stopped calling the plugin: let the spectrum settle
    lastTapW = w;
    const bool stale = staleTicks > 3;

    computeSpectrum (proc.eqPostBuf, rawPost, specPost, stale);
    if (preBtn.getToggleState()) computeSpectrum (proc.eqPreBuf, rawPre, specPre, stale);
}

// ---- painting

void EqDisplay::paint (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const auto pl = plot();
    const double sr = (double) juce::jmax (8000.f, proc.uiSampleRate.load());

    // ---------- grid: decades of frequency, steps of 6 dB
    {
        for (float base : { 10.f, 100.f, 1000.f, 10000.f })
            for (int m = 1; m <= 9; ++m)
            {
                const float hz = base * (float) m;
                if (hz < EqCore::minHz || hz > EqCore::maxHz) continue;
                const bool major = (m == 1 && base >= 100.f);
                g.setColour (pal.ink.withAlpha (major ? 0.10f : 0.035f));
                g.drawVerticalLine ((int) std::round (xOfHz (hz)), pl.getY(), pl.getBottom());
            }

        g.setFont (serif (11.f, true));
        for (int db = -18; db <= 18; db += 6)
        {
            const float y = yOfDb ((float) db);
            g.setColour (pal.ink.withAlpha (db == 0 ? 0.22f : 0.06f));
            g.drawHorizontalLine ((int) std::round (y), pl.getX(), pl.getRight());
            g.setColour (pal.dim.withAlpha (0.75f));
            g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (pl.getRight() + 5.f, y - 7.f, 30.f, 14.f), juce::Justification::centredLeft);
        }

        static const float labelHz[9] = { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
        static const char* labelTx[9] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
        g.setColour (pal.dim.withAlpha (0.75f));
        for (int k = 0; k < 9; ++k)
            g.drawText (labelTx[k], juce::Rectangle<float> (xOfHz (labelHz[k]) - 16.f, pl.getBottom() + 3.f, 32.f, 14.f), juce::Justification::centred);
    }

    g.saveState();   // everything up to the band nodes is clipped to the plot; the nodes and readout may spill past it
    g.reduceClipRegion (pl.expanded (1.f, 0.f).toNearestInt());

    // ---------- spectrum
    auto specPath = [&] (const std::vector<float>& s)
    {
        juce::Path p;
        const int cols = (int) s.size();
        for (int c = 0; c < cols; ++c)
        {
            const float x = pl.getX() + ((float) c + 0.5f) / (float) cols * pl.getWidth();
            const float y = pl.getBottom() - juce::jlimit (0.f, 1.f, (s[(size_t) c] - kSpecMin) / (kSpecMax - kSpecMin)) * pl.getHeight();
            if (c == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        return p;
    };

    if (! specPost.empty())
    {
        if (preBtn.getToggleState())
        {
            g.setColour (pal.dim.withAlpha (0.55f));
            g.strokePath (specPath (specPre), juce::PathStrokeType (1.f));
        }

        auto line = specPath (specPost);
        juce::Path fill (line);
        fill.lineTo (pl.getRight(), pl.getBottom());
        fill.lineTo (pl.getX(), pl.getBottom());
        fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (pal.acc.withAlpha (0.34f), 0.f, pl.getY(), pal.acc.withAlpha (0.02f), 0.f, pl.getBottom(), false));
        g.fillPath (fill);
        g.setColour (pal.acc.brighter (0.3f).withAlpha (0.7f));
        g.strokePath (line, juce::PathStrokeType (1.2f));
    }

    // ---------- the equalizer curve, from the same filter maths the audio uses
    std::array<EqCore::Band, EqCore::numBands> bands;
    std::array<EqCore::Sections, EqCore::numBands> cf;
    bool any = false;
    const bool idle = staleTicks > 3;   // nothing is playing: dynamic bands rest at their own gain
    for (int i = 0; i < EqCore::numBands; ++i)
    {
        bands[(size_t) i] = readBand (i);
        if (bands[(size_t) i].on)
        {
            auto eff = bands[(size_t) i];   // what the audio is doing right now: a dynamic band is drawn where it currently is
            if (eff.dyn && EqCore::usesGain (eff.type) && ! idle)
                eff.gainDb += eff.range * proc.eqDynAmt[i].load (std::memory_order_relaxed);
            cf[(size_t) i] = EqCore::designBand (eff, sr);
            any = true;
        }
    }

    if (any)
    {
        const int focus = dragging ? selected : (hover >= 0 ? hover : selected);

        juce::Path curve, area, one;
        const float y0 = yOfDb (0.f);
        for (int i = 0; i <= (int) pl.getWidth(); i += 2)
        {
            const float x = pl.getX() + (float) i, hz = hzOfX (x);
            double db = 0.0;
            for (int b = 0; b < EqCore::numBands; ++b)
                if (bands[(size_t) b].on) db += EqCore::sectionsDb (cf[(size_t) b], hz, sr);
            const float y = yOfDb (juce::jlimit (-kPlotDb * 1.2f, kPlotDb * 1.2f, (float) db));
            if (i == 0) { curve.startNewSubPath (x, y); area.startNewSubPath (x, y0); area.lineTo (x, y); }
            else        { curve.lineTo (x, y); area.lineTo (x, y); }

            if (focus >= 0 && bands[(size_t) focus].on)
            {
                const float yb = yOfDb ((float) EqCore::sectionsDb (cf[(size_t) focus], hz, sr));
                if (i == 0) one.startNewSubPath (x, yb); else one.lineTo (x, yb);
            }
        }
        area.lineTo (pl.getRight(), y0);
        area.closeSubPath();

        g.setColour (pal.amber.withAlpha (0.14f));
        g.fillPath (area);

        if (focus >= 0 && bands[(size_t) focus].on)   // the focused band's own shape, faint, in its colour
        {
            juce::Path oneFill (one);
            oneFill.lineTo (pl.getRight(), y0);
            oneFill.lineTo (pl.getX(), y0);
            oneFill.closeSubPath();
            g.setColour (bandColour (focus).withAlpha (0.16f));
            g.fillPath (oneFill);
        }

        for (int k = 3; k >= 1; --k)   // glow
        {
            g.setColour (pal.amber.withAlpha (0.07f * (float) (4 - k)));
            g.strokePath (curve, juce::PathStrokeType (1.8f + (float) k * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (pal.amber.brighter (0.3f));
        g.strokePath (curve, juce::PathStrokeType (2.f));
    }
    else
    {
        g.setColour (pal.dim.withAlpha (0.55f));
        g.setFont (serif (14.f, true));
        g.drawText ("double-click to add a band  -  up to 24", pl.withTrimmedBottom (pl.getHeight() * 0.18f), juce::Justification::centred);
    }

    // ---------- dynamic bands: a ghost node where the gain goes at full effect, joined to the band by a dashed line;
    //            the focused band also shows its threshold, which can be dragged against the analyzer
    g.restoreState();
    {
        const int fb = dragging ? selected : (hover >= 0 ? hover : selected);
        for (int i = 0; i < EqCore::numBands; ++i)
        {
            const auto& b = bands[(size_t) i];
            if (! b.on || ! b.dyn || ! EqCore::usesGain (b.type)) continue;

            const bool hot = (i == fb);
            const auto col = bandColour (i);
            const auto n = nodePos (i);
            const float yT = yOfDb (juce::jlimit (-kPlotDb, kPlotDb, b.gainDb + b.range));
            const float a = hot ? 1.f : 0.4f;

            const float dash[2] = { 3.f, 3.f };
            g.setColour (col.withAlpha (0.75f * a));
            g.drawDashedLine (juce::Line<float> (n.x, n.y, n.x, yT), dash, 2, 1.2f);
            g.setColour (juce::Colours::black.withAlpha (0.35f * a));
            g.fillEllipse (n.x - 5.f, yT - 4.f, 10.f, 10.f);
            g.setColour (col.withAlpha (0.25f * a));
            g.fillEllipse (n.x - 6.f, yT - 6.f, 12.f, 12.f);
            g.setColour (col.withAlpha (0.95f * a));
            g.drawEllipse (n.x - 6.f, yT - 6.f, 12.f, 12.f, 1.5f);
            juce::Path arrow;   // a tiny triangle showing which way the gain moves
            const float dir = b.range < 0.f ? 1.f : -1.f;
            arrow.addTriangle (n.x - 3.f, yT - 1.5f * dir, n.x + 3.f, yT - 1.5f * dir, n.x, yT + 2.5f * dir);
            g.fillPath (arrow);

            if (hot)
            {
                const float yh = yOfThreshold (b);
                g.setColour (col.withAlpha (0.18f));
                g.fillRoundedRectangle (n.x - 38.f, yh - 4.f, 76.f, 8.f, 4.f);
                g.setColour (col.withAlpha (0.9f));
                g.fillRoundedRectangle (n.x - 38.f, yh - 1.f, 76.f, 2.f, 1.f);
                g.fillEllipse (n.x - 41.f, yh - 3.f, 6.f, 6.f);
                g.fillEllipse (n.x + 35.f, yh - 3.f, 6.f, 6.f);
            }
        }
    }

    // ---------- band nodes
    for (int i = 0; i < EqCore::numBands; ++i)
    {
        if (! bands[(size_t) i].on) continue;
        const auto n = nodePos (i);
        const bool hot = (i == hover) || (i == selected);
        const float r = hot ? 9.f : 7.5f;
        const auto col = bandColour (i);

        g.setGradientFill (juce::ColourGradient (col.withAlpha (hot ? 0.5f : 0.32f), n.x, n.y, col.withAlpha (0.f), n.x + r + 8.f, n.y, true));
        g.fillEllipse (n.x - r - 8.f, n.y - r - 8.f, (r + 8.f) * 2.f, (r + 8.f) * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillEllipse (n.x - r + 1.f, n.y - r + 2.f, r * 2.f, r * 2.f);
        g.setColour (col);
        g.fillEllipse (n.x - r, n.y - r, r * 2.f, r * 2.f);
        g.setColour (juce::Colours::white.withAlpha (hot ? 0.95f : 0.7f));
        g.drawEllipse (n.x - r, n.y - r, r * 2.f, r * 2.f, 1.3f);
        if (bands[(size_t) i].dyn && EqCore::usesGain (bands[(size_t) i].type))   // a second ring marks a dynamic band
        {
            g.setColour (col.withAlpha (0.8f));
            g.drawEllipse (n.x - r - 3.f, n.y - r - 3.f, (r + 3.f) * 2.f, (r + 3.f) * 2.f, 1.f);
        }
        g.setColour (pal.soil.darker (0.4f));
        g.setFont (serif (i >= 9 ? 9.5f : 11.f, true));
        g.drawText (juce::String (i + 1), juce::Rectangle<float> (n.x - r, n.y - r, r * 2.f, r * 2.f), juce::Justification::centred);
    }

    // ---------- a small readout for the band under the pointer (or being dragged)
    const int info = dragging ? selected : hover;
    if (info >= 0 && bands[(size_t) info].on)
    {
        const auto& b = bands[(size_t) info];
        const auto n = nodePos (info);
        juce::String l1 = juce::String (EqCore::typeName (b.type)) + "   " + hzText (b.freq);
        juce::String l2 = (EqCore::usesGain (b.type) ? (b.gainDb >= 0.f ? "+" : "") + juce::String (b.gainDb, 1) + " dB   " : juce::String())
                        + (EqCore::usesQ (b.type) ? "Q " + juce::String (b.q, 2) : juce::String())
                        + (EqCore::isCut (b.type) ? juce::String ("   ") + EqCore::slopeName (b.slope) : juce::String());
        l2 = l2.trim();
        const bool isDyn = b.dyn && EqCore::usesGain (b.type);
        const juce::String l3 = isDyn ? "dynamic  " + juce::String (b.thresh, 0) + " dB  " + (b.range >= 0.f ? "+" : "") + juce::String (b.range, 1)
                                          + " dB  " + EqCore::speedName (b.speed)
                                      : juce::String();

        g.setFont (serif (12.f));
        auto textW = [&] (const juce::String& t)
        {
            juce::GlyphArrangement ga;
            ga.addLineOfText (g.getCurrentFont(), t, 0.f, 0.f);
            return ga.getBoundingBox (0, -1, true).getWidth();
        };
        const float w = juce::jmax (juce::jmax (textW (l1), textW (l2)), textW (l3)) + 18.f, h = isDyn ? 52.f : 36.f;
        float bx = juce::jlimit (4.f, (float) getWidth() - w - 4.f, n.x - w * 0.5f);
        float by = n.y - 18.f - h;
        if (by < 2.f) by = n.y + 18.f;
        const juce::Rectangle<float> box (bx, by, w, h);

        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (box.translated (0.f, 2.f), 8.f);
        g.setGradientFill (juce::ColourGradient (pal.moss.withAlpha (0.96f), 0.f, box.getY(), pal.soil.withAlpha (0.97f), 0.f, box.getBottom(), false));
        g.fillRoundedRectangle (box, 8.f);
        g.setColour (bandColour (info).withAlpha (0.8f));
        g.drawRoundedRectangle (box.reduced (0.5f), 8.f, 1.1f);
        const float rh = isDyn ? 16.f : 17.f;
        g.setColour (pal.ink);
        g.drawText (l1, juce::Rectangle<float> (box.getX(), box.getY() + 3.f, w, rh), juce::Justification::centred);
        g.setColour (pal.dim);
        g.drawText (l2, juce::Rectangle<float> (box.getX(), box.getY() + 3.f + rh, w, rh), juce::Justification::centred);
        if (isDyn)
        {
            g.setColour (bandColour (info).withAlpha (0.9f));
            g.drawText (l3, juce::Rectangle<float> (box.getX(), box.getY() + 3.f + 2.f * rh, w, rh), juce::Justification::centred);
        }
    }
}

// ---------------------------------------------------------------- spectral gate window

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

const SpecDef kSpecDefs[7] = {
    { "sthr",     "cutoff",         [] (double v) { return juce::String (v, 1) + " dB"; }, plainNumber },
    { "sbal",     "weak / strong",  [] (double v) { return juce::String (v, 2); },         plainNumber },
    { "stilt",    "tilt",           [] (double v) { return juce::String (v, 2); },         plainNumber },
    { "sattack",  "attack",         [] (double v) { return juce::String (juce::roundToInt ((float) v)) + " ms"; }, plainNumber },
    { "shold",    "hold",           [] (double v) { return juce::String (juce::roundToInt ((float) v)) + " ms"; }, plainNumber },
    { "srelease", "release",        [] (double v) { return juce::String (juce::roundToInt ((float) v)) + " ms"; }, plainNumber },
    { "ssmooth",  "smooth",         [] (double v) { return juce::String (juce::roundToInt ((float) (v * 100.0))) + "%"; },
                                    [] (const juce::String& t) { return plainNumber (t) / 100.0; } }
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

    addAndMakeVisible (tiltBtn);
    tiltAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "stilton", tiltBtn);

    addAndMakeVisible (winBtn);
    winBtn.onClick = [this]
    {
        auto* par = proc.apvts.getParameter ("swin");
        const int cur = (int) proc.apvts.getRawParameterValue ("swin")->load();
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) ((cur + 1) % 7)));
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
    // "gate" card: the three main controls, large
    {
        const auto gr = group (0);
        const int sw = (int) ((gr.getWidth() - 24.f) / 3.f);
        for (int c = 0; c < 3; ++c)
        {
            auto& k = knobs[(size_t) c];
            const int x = (int) gr.getX() + 12 + c * sw, y = (int) gr.getY();
            k.label.setBounds (x, y + 22, sw, 16);
            k.slider.setBounds (x, y + 38, sw, 106);
        }
    }

    // "response" card: how the gate moves in time and spreads across neighbouring frequencies
    {
        const auto gr = group (1);
        const int sw = (int) ((gr.getWidth() - 24.f) / 4.f);
        for (int c = 0; c < 4; ++c)
        {
            auto& k = knobs[(size_t) (3 + c)];
            const int x = (int) gr.getX() + 12 + c * sw, y = (int) gr.getY();
            k.label.setBounds (x, y + 22, sw, 16);
            k.slider.setBounds (x, y + 38, sw, 112);
        }
    }

    // "tilt & window" card: the tilt enable key and the FFT size key, side by side
    {
        const auto gr = group (2);
        const int sw = (int) ((gr.getWidth() - 24.f) / 2.f);
        const int x = (int) gr.getX() + 12, y = (int) gr.getY();
        tiltBtn.setBounds (x + (sw - 130) / 2, y + 42, 130, 32);
        winBtn.setBounds  (x + sw + (sw - 130) / 2, y + 42, 130, 32);
    }

    const auto cd = card();
    doneBtn.setBounds ((int) cd.getRight() - 16 - 76, (int) cd.getY() + 12, 76, 30);
    onBtn.setBounds   (doneBtn.getX() - 8 - 64, (int) cd.getY() + 12, 64, 30);
}

SpectralGate::Params SpectralOverlay::readParams() const
{
    auto v = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    SpectralGate::Params p;
    p.cutoffDb  = v ("sthr");
    p.balance   = v ("sbal");
    p.tiltOn    = v ("stilton") > 0.5f;
    p.tilt      = v ("stilt");
    p.attackMs  = v ("sattack");
    p.holdMs    = v ("shold");
    p.releaseMs = v ("srelease");
    p.smooth    = v ("ssmooth");
    return p;
}

void SpectralOverlay::timerCallback()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit (0.f, 0.1f, (float) ((nowMs - lastMs) * 0.001));
    lastMs = nowMs;
    if (! proc.setNoAnim.load()) anim += dt;

    // fade the window in and out
    const float before = progress;
    progress = juce::jlimit (0.f, 1.f, progress + (closing ? -1.f : 1.f) * dt / (proc.setNoAnim.load() ? 0.001f : 0.22f));
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
        backdropOld = {};
        cardImg = {};
        panel.repaint();
        return;
    }

    // keep the picture behind the window alive: a fresh blurred copy of the plugin about ten times a second, cross-faded
    if (! proc.setNoAnim.load() || nowMs - lastSnapMs > 400.0)
        if (nowMs - lastSnapMs > 100.0 && ! closing)
        {
            refreshBackdrop();
            lastSnapMs = nowMs;
        }
    if (blend < 1.f)
    {
        blend = juce::jmin (1.f, blend + dt / 0.1f);
        if (blend >= 1.f) backdropOld = {};
    }

    // the keys glide between their looks like the panel's buttons do
    for (auto* b : { &onBtn, &tiltBtn })
    {
        const float target = b->getToggleState() ? 1.f : 0.f;
        float v = buttonLit (*b);
        const float step = dt / 0.2f;
        v = v < target ? juce::jmin (target, v + step) : juce::jmax (target, v - step);
        b->getProperties().set ("lit", v);
    }

    // the tilt knob looks asleep while tilt is off
    {
        const float fade = smooth01 (progress);
        const float a = fade * (0.4f + 0.6f * smooth01 (buttonLit (tiltBtn)));
        knobs[2].slider.setAlpha (a);
        knobs[2].label.setAlpha (a);
    }

    static const char* wn[7] = { "128", "256", "512", "1024", "2048", "4096", "8192" };
    const juce::String wt = wn[juce::jlimit (0, 6, (int) proc.apvts.getRawParameterValue ("swin")->load())];
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
        const float al = s.getAlpha();

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

// The parts of the analyzer that never move: glass, grid, labels and legend
void SpectralOverlay::paintAnalyzerShell (juce::Graphics& g)
{
    const auto& pal = panel.pal;
    const auto an = analyzer(), plot = plotRect();
    auto dbY = [&] (float db) { return plot.getY() + juce::jlimit (0.f, 1.f, -db / -kFloorDb) * plot.getHeight(); };
    auto fx  = [&] (float hz) { return plot.getX() + std::log10 (hz / SpectralGate::fMinHz) / 3.f * plot.getWidth(); };

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
    key (pal.amber, "cutoff", true);
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
    const float bwAll = plot.getWidth() / (float) SpectralGate::numBands;
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

    // the cutoff curve, drawn from the very same formula the audio uses (tilt included). The cutoff is measured
    // down from the loudest bin of the moment, so the curve rides along with it.
    const auto p = readParams();
    const float refDb = live ? juce::jlimit (kFloorDb, 6.f, proc.spec.dispRefDb.load (std::memory_order_relaxed)) : 0.f;
    juce::Path thrLine;
    bool first = true;
    for (int i = 0; i <= (int) plot.getWidth(); i += 2)
    {
        const float hz = SpectralGate::fMinHz * std::pow (1000.f, (float) i / plot.getWidth());
        const float x = plot.getX() + (float) i, y = dbY (refDb + SpectralGate::thresholdDb (p, hz));
        if (first) { thrLine.startNewSubPath (x, y); first = false; }
        else         thrLine.lineTo (x, y);
    }

    // below the curve is the weak side, above it the strong side. Each is tinted by how far it is held back,
    // so the balance setting can be read straight off the picture
    const float held = 1.f - SpectralGate::weakGain (p), heldS = 1.f - SpectralGate::strongGain (p);
    {
        juce::Path weak (thrLine);
        weak.lineTo (plot.getRight() + 2.f, plot.getBottom() + 2.f);
        weak.lineTo (plot.getX() - 2.f, plot.getBottom() + 2.f);
        weak.closeSubPath();
        g.setColour (pal.amber.withAlpha (0.03f + 0.15f * held));
        g.fillPath (weak);

        juce::Path strong (thrLine);
        strong.lineTo (plot.getRight() + 2.f, plot.getY() - 2.f);
        strong.lineTo (plot.getX() - 2.f, plot.getY() - 2.f);
        strong.closeSubPath();
        g.setColour (pal.grass.withAlpha (0.02f + 0.13f * heldS));
        g.fillPath (strong);
    }

    for (int i = 3; i >= 1; --i)   // glow
    {
        g.setColour (pal.amber.withAlpha (0.07f * (float) (4 - i)));
        g.strokePath (thrLine, juce::PathStrokeType (1.6f + (float) i * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (pal.amber.withAlpha (0.95f));
    g.strokePath (thrLine, juce::PathStrokeType (1.6f));

    g.setFont (Scat::serif (12.f, true));
    g.setColour (pal.grass.withAlpha (0.85f - 0.5f * heldS));
    g.drawText ("strong", plot.withTrimmedRight (6.f).withHeight (16.f).translated (0.f, 3.f), juce::Justification::centredRight);
    g.setColour (pal.amber.withAlpha (0.85f - 0.5f * held));
    g.drawText ("weak", plot.withTrimmedRight (6.f).withTop (plot.getBottom() - 19.f).withHeight (16.f), juce::Justification::centredRight);

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
    g.drawText ("Spectral Gate", (int) cd.getX() + 23, (int) cd.getY() + 5, 360, 34, juce::Justification::centredLeft);
    g.setGradientFill (juce::ColourGradient (pal.ink, 0.f, cd.getY() + 4.f, pal.ink.interpolatedWith (pal.amber, 0.55f), 0.f, cd.getY() + 38.f, false));
    g.drawText ("Spectral Gate", (int) cd.getX() + 22, (int) cd.getY() + 4, 360, 34, juce::Justification::centredLeft);


    static const char* titles[3] = { "gate", "response", "tilt & window" };
    for (int r = 0; r < 3; ++r)
    {
        panel.drawGroup (g, group (r));
        panel.drawGroupTitle (g, group (r), titles[r]);
    }

    // small captions over the two keys in the last card
    {
        const auto gr = group (2);
        const float sw = (float) (int) ((gr.getWidth() - 24.f) / 2.f);
        g.setFont (Scat::serif (13.f, true));
        g.setColour (pal.dim);
        g.drawText ("slant the cutoff", juce::Rectangle<float> (gr.getX() + 12.f, gr.getY() + 24.f, sw, 16.f), juce::Justification::centred);
        g.drawText ("fft size",         juce::Rectangle<float> (gr.getX() + 12.f + sw, gr.getY() + 24.f, sw, 16.f), juce::Justification::centred);
    }

    paintAnalyzerShell (g);
}

// Renders the two cached layers. Called once each time the window opens.
// A blurred, dimmed copy of the plugin with a soft vignette. Drawn at design size: it is blurred anyway, so more pixels would add nothing.
juce::Image SpectralOverlay::renderBackdrop (const juce::Image& src) const
{
    const auto& pal = panel.pal;
    const float W = (float) ScattrPanel::baseW, H = (float) ScattrPanel::baseH;

    juce::Image out (juce::Image::RGB, ScattrPanel::baseW, ScattrPanel::baseH, true);
    juce::Graphics bg (out);
    bg.fillAll (pal.bgBot);
    if (src.isValid())
    {
        const auto blurred = blurSnapshot (src, ScattrPanel::baseW / 4, ScattrPanel::baseH / 4, 3, 3);
        bg.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        bg.drawImage (blurred, 0, 0, ScattrPanel::baseW, ScattrPanel::baseH, 0, 0, blurred.getWidth(), blurred.getHeight());
    }
    bg.setColour (juce::Colours::black.withAlpha (0.36f));
    bg.fillAll();
    juce::ColourGradient vig (juce::Colours::transparentBlack, W * 0.5f, H * 0.5f, juce::Colours::black.withAlpha (0.38f), 0.f, 0.f, true);
    bg.setGradientFill (vig);
    bg.fillAll();
    return out;
}

// Paints the plugin (its own artwork and every visible child except this window) into a small image, so the
// animations behind the blur keep moving while the window is up.
juce::Image SpectralOverlay::grabPanel() const
{
    constexpr float sc = 0.5f;
    juce::Image img (juce::Image::ARGB, juce::roundToInt ((float) ScattrPanel::baseW * sc), juce::roundToInt ((float) ScattrPanel::baseH * sc), true);
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale (sc));
    panel.paint (g);
    for (auto* c : panel.getChildren())
    {
        if (c == this || ! c->isVisible()) continue;
        juce::Graphics::ScopedSaveState ss (g);
        g.setOrigin (c->getPosition());
        g.reduceClipRegion (0, 0, c->getWidth(), c->getHeight());
        c->paintEntireComponent (g, false);
    }
    return img;
}

void SpectralOverlay::refreshBackdrop()
{
    backdropOld = backdrop;
    backdrop = renderBackdrop (grabPanel());
    blend = 0.f;   // fade from the previous picture to this one
}

void SpectralOverlay::buildLayers()
{
    const int iw = juce::roundToInt ((float) ScattrPanel::baseW * layerScale), ih = juce::roundToInt ((float) ScattrPanel::baseH * layerScale);

    // backdrop: a blurred, dimmed copy of the plugin with a soft vignette (refreshed while the window is open)
    backdrop = renderBackdrop (sharp);
    backdropOld = {};
    blend = 1.f;
    lastSnapMs = juce::Time::getMillisecondCounterHiRes();

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
    if (backdropOld.isValid() && blend < 1.f)   // the previous picture, while the new one fades in over it
    {
        g.setOpacity (e);
        g.drawImage (backdropOld, 0, 0, W, H, 0, 0, backdropOld.getWidth(), backdropOld.getHeight());
    }
    if (backdrop.isValid())
    {
        g.setOpacity (e * (backdropOld.isValid() && blend < 1.f ? blend : 1.f));
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

    for (auto* b : { &onBtn, &tiltBtn })   // glow behind a lit key
    {
        const float en = smooth01 (buttonLit (*b));
        if (en <= 0.01f) continue;
        const auto r = b->getBounds().toFloat().reduced (2.f);
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
