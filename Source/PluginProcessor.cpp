#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "TexturesData.h"   // embedded wind recordings (Textures/ folder)

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using NR = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto add = [&p] (const char* id, const char* name, float lo, float hi, float def, float skew)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, NR (lo, hi, 0.0f, skew), def));
    };

    add ("size",  "Grain Size",  40.f, 500.f, 120.f, 0.5f);
    add ("dens",  "Density",     1.f,  80.f,  25.f,  0.5f);
    {   // playhead speed: 1x in the middle of the knob, 0.1x .. 4x
        NR r (0.1f, 4.f);
        r.setSkewForCentre (1.f);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "speed", 1 }, "Speed", r, 1.f));
    }
    add ("wind",  "Wind Texture", 0.f, 1.f,   0.f,   1.f);   // level of the looping texture layer
    add ("spray", "Spray",        0.f, 1.f,   0.1f,  1.f);
    add ("hold",  "Hold",         0.5f, 20.f, 4.f,   0.5f);
    add ("gap",   "Gap",          0.f, 5.f,   0.3f,  1.f);
    add ("cycle", "Key Cycle",    0.05f, 20.f, 1.5f, 0.5f);   // seconds per sample while a key is held
    add ("rand",  "Randomizer",   0.f, 1.f,   0.f,   1.f);     // chance that a grain comes from a random sample
    add ("width", "Random Pan",   0.f, 1.f,   0.5f,  1.f);   // per-grain random pan amount (id kept as "width" for old projects)
    add ("amp",   "Amp",          0.f, 1.f,   0.7f,  1.f);
    add ("mtune", "Master Tune", -100.f, 100.f, 0.f, 1.f);   // cents

    // effects
    add ("cutoff", "Filter Cutoff", 20.f, 20000.f, 20000.f, 0.3f);
    add ("reso",   "Filter Reso",   0.5f, 12.f,    0.7f,    0.5f);
    add ("drive",  "Drive",         0.f,  1.f,     0.f,     1.f);
    add ("fmix",   "Filter Mix",    0.f,  1.f,     1.f,     1.f);
    add ("dfb",    "Delay Feedback",0.f,  0.95f,   0.35f,   1.f);
    add ("dmix",   "Delay Mix",     0.f,  1.f,     0.15f,   1.f);
    add ("rsize",  "Reverb Size",   0.f,  1.f,     0.6f,    1.f);
    add ("rdamp",  "Reverb Damp",   0.f,  1.f,     0.5f,    1.f);
    add ("rmix",   "Reverb Mix",    0.f,  1.f,     0.25f,   1.f);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ftype", 1 }, "Filter Type", juce::StringArray { "LP", "HP", "BP" }, 0));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "dnote", 1 }, "Delay Time",
        juce::StringArray { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2", "1/1" }, 4));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rtype", 1 }, "Reverb Space",
        juce::StringArray { "Room", "Plate", "Hall", "Cathedral" }, 2));

    // every effect starts bypassed
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "fon", 1 }, "Filter On", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "don", 1 }, "Delay On",  false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "ron", 1 }, "Reverb On", false));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "drone", 1 }, "Drone (free-run)", false));   // off by default: keys play it

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "tune", 1 }, "Tune Samples", true));

    return { p.begin(), p.end() };
}

// linear-interpolated stereo read from a texture
inline void readTexAt (const Texture& t, double pos, float& l, float& r)
{
    const int i = juce::jlimit (0, t.length - 1, (int) pos);
    const int j = juce::jmin (i + 1, t.length - 1);
    const float fr = (float) (pos - (double) i);
    const float* d0 = t.data.getReadPointer (0);
    const float* d1 = t.data.getNumChannels() > 1 ? t.data.getReadPointer (1) : d0;
    l = d0[i] + (d0[j] - d0[i]) * fr;
    r = d1[i] + (d1[j] - d1[i]) * fr;
}

// Estimates a sample's pitch (YIN) from its loudest sections.
// Returns a fractional MIDI note, or -1 if there is no clear pitch (drums, noise...).
float detectRootNote (const Sample& s)
{
    const int n = s.length;
    const double sr = s.sampleRate;
    const int tauMax = juce::jlimit (64, 4096, (int) (sr / 40.0));
    const int tauMin = juce::jmax (2, (int) (sr / 2000.0));
    const int W = tauMax;
    const int frameLen = W + tauMax;
    if (n < frameLen + 8) return -1.f;

    const float* c0 = s.data.getReadPointer (0);
    const float* c1 = s.data.getNumChannels() > 1 ? s.data.getReadPointer (1) : nullptr;

    std::vector<float> x ((size_t) frameLen);
    auto load = [&] (int start)
    {
        for (int j = 0; j < frameLen; ++j)
            x[(size_t) j] = c1 != nullptr ? 0.5f * (c0[start + j] + c1[start + j]) : c0[start + j];
    };

    // find the loudest frames
    const int numCand = 64, span = n - frameLen;
    std::vector<std::pair<float, int>> cands;
    for (int c = 0; c < numCand; ++c)
    {
        const int start = (int) ((double) span * (double) c / (double) (numCand - 1));
        load (start);
        double e = 0.0;
        for (float v : x) e += (double) v * v;
        cands.emplace_back ((float) std::sqrt (e / frameLen), start);
    }
    std::sort (cands.begin(), cands.end(), [] (const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<float> found, d ((size_t) tauMax), cm ((size_t) tauMax);
    for (int k = 0; k < 6; ++k)
    {
        if (cands[(size_t) k].first < 0.005f) break;
        load (cands[(size_t) k].second);

        for (int tau = 1; tau < tauMax; ++tau)
        {
            float sum = 0.f;
            for (int j = 0; j < W; ++j)
            {
                const float df = x[(size_t) j] - x[(size_t) (j + tau)];
                sum += df * df;
            }
            d[(size_t) tau] = sum;
        }

        cm[0] = 1.f;
        float run = 0.f;
        for (int tau = 1; tau < tauMax; ++tau)
        {
            run += d[(size_t) tau];
            cm[(size_t) tau] = run > 0.f ? d[(size_t) tau] * (float) tau / run : 1.f;
        }

        int tau = -1;
        for (int t = tauMin; t < tauMax - 1; ++t)
            if (cm[(size_t) t] < 0.15f)
            {
                while (t + 1 < tauMax - 1 && cm[(size_t) t + 1] < cm[(size_t) t]) ++t;
                tau = t;
                break;
            }
        if (tau < 0) continue;

        const float a = cm[(size_t) tau - 1], b = cm[(size_t) tau], c = cm[(size_t) tau + 1];
        const float den = a - 2.f * b + c;
        const float shift = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.f;
        const double freq = sr / ((double) tau + (double) shift);
        found.push_back ((float) (69.0 + 12.0 * std::log2 (freq / 440.0)));
    }

    if (found.size() < 2) return -1.f;
    std::sort (found.begin(), found.end());
    return found[found.size() / 2];
}
} // namespace

ScattrProcessor::ScattrProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", makeLayout())
{
    formats.registerBasicFormats();
    for (auto& s : scopeBuf) s.store (0.f, std::memory_order_relaxed);
    for (int i = 0; i <= 1024; ++i)
        env[i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 1024.f);
    loadTextures();
}

// The wind layer is built from the audio files in the project's Textures/ folder (CMake embeds them).
// Each file becomes one looping texture; the wind knob sets its level.
void ScattrProcessor::loadTextures()
{
    for (int i = 0; i < TexturesData::namedResourceListSize; ++i)
    {
        int size = 0;
        const char* data = TexturesData::getNamedResource (TexturesData::namedResourceList[i], size);
        if (data == nullptr || size <= 0) continue;

        std::unique_ptr<juce::AudioFormatReader> r (formats.createReaderFor (
            std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false)));
        if (r == nullptr || r->lengthInSamples < 256) continue;

        const int n = (int) juce::jmin<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 90.0));
        Texture t;
        t.sampleRate = r->sampleRate;
        t.length = n;
        t.data.setSize (juce::jmin (2, (int) r->numChannels), n);
        t.data.clear();
        r->read (&t.data, 0, n, 0, true, true);

        double sum = 0.0;
        float peak = 0.f;
        for (int ch = 0; ch < t.data.getNumChannels(); ++ch)
        {
            const float* d = t.data.getReadPointer (ch);
            for (int k = 0; k < t.length; ++k) { sum += (double) d[k] * d[k]; peak = juce::jmax (peak, std::abs (d[k])); }
        }
        const float rms = (float) std::sqrt (sum / (double) ((juce::int64) t.length * t.data.getNumChannels()));
        if (rms < 1.0e-6f || peak < 1.0e-6f) continue;

        // even out levels between textures (about -14 dBFS RMS, never clipping)
        t.data.applyGain (juce::jmin (0.2f / rms, 0.95f / peak));

        // wind is slow-moving, so hide the loop seam with a long (1.5 s) equal-power crossfade
        t.xfade = juce::jmin ((int) (1.5 * t.sampleRate), t.length / 4);
        textures.push_back (std::move (t));
    }
}

void ScattrProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSr = sampleRate;

    filter.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 });
    filter.reset();
    reverb.setSampleRate (sampleRate);
    reverb.reset();

    delaySize = (int) (sampleRate * 4.1) + 8;   // room for a whole note at 60 bpm
    for (auto& b : delayBuf) b.assign ((size_t) delaySize, 0.f);
    delayW = 0;
    fbLp[0] = fbLp[1] = 0.f;
    delayTimeSm.reset (sampleRate, 0.08);
    delayTimeSm.setCurrentAndTargetValue ((float) (0.35 * sampleRate));

    for (auto* sm : { &fOnSm, &dOnSm, &rOnSm })
    {
        sm->reset (sampleRate, 0.02);
        sm->setCurrentAndTargetValue (0.f);
    }
    preSize = (int) (sampleRate * 0.1) + 8;
    for (auto& b : preBuf) b.assign ((size_t) preSize, 0.f);
    preW = 0;
    revL.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.f);
    revR.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.f);
    revActive = false;

    texIdx = texNext = -1;
    texPos = 0.0;
    texLevel = 0.f;
    cutoffSm.reset (sampleRate, 0.03);
    cutoffSm.setCurrentAndTargetValue (20000.f);

    for (auto& g : grains) g.active = false;
    phase = Phase::Gap;
    remaining = 0;
    lastGen = -1;
}

bool ScattrProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

// ---------------------------------------------------------------- samples

bool ScattrProcessor::addSampleFile (const juce::File& f)
{
    if (samples.size() >= 256 || ! f.existsAsFile()) return false;

    std::unique_ptr<juce::AudioFormatReader> r (formats.createReaderFor (f));
    if (r == nullptr || r->lengthInSamples < 256) return false;

    const int n = (int) juce::jmin<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 300.0));

    auto s = std::make_shared<Sample>();
    s->path = f.getFullPathName();
    s->name = f.getFileNameWithoutExtension();
    s->sampleRate = r->sampleRate;
    s->length = n;
    s->data.setSize (juce::jmin (2, (int) r->numChannels), n + 4);
    s->data.clear();
    r->read (&s->data, 0, n, 0, true, true);

    // Quiet files are normalised on load: if the loudest peak is below -6 dBFS the whole sample is raised
    // so its peak sits near -1 dBFS (gain capped at +30 dB so near-silent files don't become a wall of hiss).
    // Louder files are left exactly as they are.
    {
        float peak = 0.f;
        for (int ch = 0; ch < s->data.getNumChannels(); ++ch)
            peak = juce::jmax (peak, s->data.getMagnitude (ch, 0, n));
        if (peak > 1.0e-4f && peak < 0.5f)
            s->data.applyGain (juce::jmin (0.9f / peak, 31.6f));
    }

    const int cols = 700;
    const int step = juce::jmax (1, n / cols);
    const float* d = s->data.getReadPointer (0);
    for (int i = 0; i < cols; ++i)
    {
        float mn = 1.f, mx = -1.f;
        for (int j = i * step; j < (i + 1) * step && j < n; j += juce::jmax (1, step / 64))
        {
            mn = juce::jmin (mn, d[j]);
            mx = juce::jmax (mx, d[j]);
        }
        if (mn > mx) mn = mx = 0.f;
        s->peaks.emplace_back (mn, mx);
    }

    s->rootNote = detectRootNote (*s);

    const juce::ScopedLock sl (lock);
    samples.push_back (s);
    ++generation;
    return true;
}

void ScattrProcessor::clearSamples()
{
    const juce::ScopedLock sl (lock);
    samples.clear();
    ++generation;
}

void ScattrProcessor::removeSample (int index)
{
    const juce::ScopedLock sl (lock);
    if (index < 0 || index >= (int) samples.size()) return;
    samples.erase (samples.begin() + index);
    ++generation;   // the audio thread drops its grains and rebuilds its play order on the next block
}

int ScattrProcessor::pickSample()
{
    // Shuffled rounds: every enabled sample plays once, in random order, before any repeats.
    for (int pass = 0; pass < 2; ++pass)
    {
        if (bagCount == 0)
        {
            for (int i = 0; i < (int) samples.size() && bagCount < 256; ++i)
                if (samples[(size_t) i]->enabled.load()) bag[bagCount++] = i;

            for (int i = bagCount - 1; i > 0; --i)
                std::swap (bag[i], bag[rng.nextInt (i + 1)]);

            // don't start a new round with the sample that just played
            if (bagCount > 1 && bag[bagCount - 1] == last)
                std::swap (bag[bagCount - 1], bag[rng.nextInt (bagCount - 1)]);
        }

        while (bagCount > 0)
        {
            const int i = bag[--bagCount];
            if (i < (int) samples.size() && samples[(size_t) i]->enabled.load())
                return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------- audio

void ScattrProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())       { ++notesHeld; heldNote = m.getNoteNumber(); retrigger = true; }
        else if (m.isNoteOff()) notesHeld = juce::jmax (0, notesHeld - 1);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) notesHeld = 0;
    }

    const juce::ScopedTryLock tl (lock);
    if (! tl.isLocked()) return;

    if (lastGen != generation)
    {
        for (auto& g : grains) g.active = false;
        lastGen = generation;
        phase = Phase::Gap;
        remaining = 0;
        cur = last = -1;
        bagCount = 0;
        noteDone = false;
    }

    auto prm = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float sizeMs = prm ("size"), dens = prm ("dens"), speed = prm ("speed"), wind = prm ("wind"),
                spray = prm ("spray"), holdS = prm ("hold"), gapS = prm ("gap"), cycleS = prm ("cycle"), randAmt = prm ("rand"), width = prm ("width"),
                amp = prm ("amp");
    const bool active = prm ("drone") > 0.5f || notesHeld > 0;
    juce::ignoreUnused (cycleS);   // the cycle knob is retired; held keys play each sample through
    const bool keyMode = notesHeld > 0;   // a key is down: the sample plays through once, then waits for the next key

    const double sr = currentSr;
    const int numSamples = buffer.getNumSamples();
    const int sizeSamples = juce::jmax (32, (int) (sizeMs * 0.001 * sr));
    // Keep grains overlapping by at least 2x: short seeds at a low rain rate used to leave gaps and
    // sound choppy, so the rate is raised automatically as the seed shrinks.
    const float densEff = juce::jmax (dens, 2.0f / (sizeMs * 0.001f));
    const float norm = amp / std::sqrt (juce::jmax (1.0f, densEff * sizeMs * 0.001f));
    const float pitchOff = notesHeld > 0 ? (float) (heldNote - 60) : 0.f;
    const bool tuneOn = prm ("tune") > 0.5f;
    const float masterTune = prm ("mtune") * 0.01f;   // cents -> semitones
    const float targetNote = notesHeld > 0 ? (float) heldNote : 60.f;   // C4 when drifting with no key held

    // effect settings
    const float cutoff = prm ("cutoff"), reso = prm ("reso"), drive = prm ("drive"), fMix = prm ("fmix"),
                dFb = prm ("dfb"), dMix = prm ("dmix"),
                rSize = prm ("rsize"), rDamp = prm ("rdamp"), rMix = prm ("rmix");
    const int fType = juce::jlimit (0, 2, (int) prm ("ftype"));
    const bool fOn = prm ("fon") > 0.5f, dOn = prm ("don") > 0.5f, rOn = prm ("ron") > 0.5f;
    fOnSm.setTargetValue (fOn ? 1.f : 0.f);
    dOnSm.setTargetValue (dOn ? 1.f : 0.f);
    rOnSm.setTargetValue (rOn ? 1.f : 0.f);

    // tempo-synced delay time (falls back to 120 bpm when the host gives no tempo)
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
    {
        const auto pos = ph->getPosition();
        if (pos.hasValue())
        {
            const auto b = pos->getBpm();
            if (b.hasValue() && *b > 20.0) bpm = *b;
        }
    }
    static const double noteBeats[11] = { 0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 };
    const int dIdx = juce::jlimit (0, 10, (int) prm ("dnote"));
    const float delaySamples = juce::jlimit (16.f, (float) (delaySize - 8), (float) (noteBeats[dIdx] * 60.0 / bpm * sr));
    const float texCoef = 1.f - std::exp (-1.f / (0.25f * (float) sr));   // ~250 ms texture fades
    filter.setType (fType == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                  : fType == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                               : juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.setResonance (reso);
    cutoffSm.setTargetValue (juce::jlimit (20.f, (float) (0.45 * sr), cutoff));
    delayTimeSm.setTargetValue (delaySamples);
    const float drv = 1.f + drive * 12.f;
    const float driveMakeup = 0.5f / std::tanh (drv * 0.5f);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    auto spawn = [&]()
    {
        if (cur < 0 || cur >= (int) samples.size()) return;
        const int len0 = juce::jmin (sizeSamples, remaining);
        if (len0 <= 32) return;

        Grain* slot = nullptr;
        for (auto& g : grains) if (! g.active) { slot = &g; break; }
        if (slot == nullptr) return;

        // randomizer: with probability `randAmt`, this grain is taken from a random enabled sample
        // instead of the current one (0 = never, 1 = every grain)
        int idx = cur;
        if (randAmt > 0.f && samples.size() > 1 && rng.nextFloat() < randAmt)
        {
            for (int t = 0; t < 8; ++t)
            {
                const int c = rng.nextInt ((int) samples.size());
                if (samples[(size_t) c]->enabled.load()) { idx = c; break; }
            }
        }

        const Sample& s = *samples[(size_t) idx];
        const double srcLen = (double) s.length;
        const double regS = (double) s.startN.load() * srcLen;
        const double regE = (double) s.endN.load() * srcLen;
        const double regLen = juce::jmax (1.0, regE - regS);
        // tuned: shift this sample from its own detected pitch to the target note
        const float keyOff = (tuneOn && s.rootNote >= 0.f) ? targetNote - s.rootNote : pitchOff;
        const float semis = keyOff + masterTune;
        const double inc = std::pow (2.0, semis / 12.0) * s.sampleRate / sr;
        const int len = juce::jmin (len0, (int) ((regLen - 4.0) / inc));
        if (len <= 32) return;

        // the playhead sweeps the region once per bloom / cycle at 1x; the speed knob scales that (and wraps)
        double prog;
        if (keyMode)
            prog = playhead;   // key held: the real playhead, 0 -> 1 across the whole region
        else
        {
            prog = (1.0 - (double) remaining / (double) holdTotal) * (double) speed;
            prog -= std::floor (prog);
        }
        double start = regS + (prog + (rng.nextFloat() - 0.5f) * spray) * regLen;
        start = juce::jlimit (regS, juce::jmax (regS, regE - (double) len * inc - 3.0), start);

        // full knob = grains land anywhere from 10% to 90% across the field (never hard left/right), so it stays gentle
        const float pan = 0.5f + (rng.nextFloat() - 0.5f) * width * 0.8f;
        slot->active = true;
        slot->s = &s;
        slot->pos = start;
        slot->inc = inc;
        slot->age = 0;
        slot->len = len;
        slot->gl = std::cos (pan * juce::MathConstants<float>::halfPi) * 1.4142f;
        slot->gr = std::sin (pan * juce::MathConstants<float>::halfPi) * 1.4142f;
        if (idx == cur) uiGrain.store ((float) (start / srcLen));
    };

    // a key was just pressed: jump to a new random sample immediately (no waiting out the rest/bloom)
    if (retrigger)
    {
        retrigger = false;
        const int i = pickSample();
        if (i >= 0)
        {
            cur = last = i;
            phase = Phase::Play;
            holdTotal = remaining = 1 << 30;   // the sweep ends when the playhead reaches the end, not on a timer
            playhead = 0.0;
            noteDone = false;
            grainCountdown = 0.0;
        }

        // a new key also brings in a different wind texture (it fades across, no click)
        if (textures.size() > 1 && texIdx >= 0)
        {
            int t = texIdx;
            for (int k = 0; k < 6 && t == texIdx; ++k) t = rng.nextInt ((int) textures.size());
            if (t != texIdx) texNext = t;
        }
    }

    for (int n = 0; n < numSamples; ++n)
    {
        if (active)
        {
            if (phase == Phase::Gap)
            {
                if (keyMode && noteDone) { /* finished: stay quiet until the key is released and pressed again */ }
                else if (remaining > 0) --remaining;
                else
                {
                    const int i = pickSample();
                    if (i >= 0)
                    {
                        cur = last = i;
                        phase = Phase::Play;
                        if (keyMode) { holdTotal = remaining = 1 << 30; playhead = 0.0; noteDone = false; }
                        else         holdTotal = remaining = juce::jmax (1, (int) (holdS * sr));
                        grainCountdown = 0.0;
                    }
                }
            }
            if (phase == Phase::Play)
            {
                if (keyMode ? (playhead >= 1.0) : (remaining <= 0))
                {
                    phase = Phase::Gap;
                    if (keyMode) { noteDone = true; remaining = 0; }   // played all the way through
                    else           remaining = (int) (gapS * sr);
                }
                else
                {
                    if (grainCountdown <= 0.0)
                    {
                        spawn();
                        grainCountdown += sr / (double) densEff * (0.7 + 0.6 * rng.nextFloat());
                    }
                    grainCountdown -= 1.0;
                    --remaining;

                    if (keyMode && cur >= 0 && cur < (int) samples.size())
                    {
                        // advance the playhead: 1x = the sample's own speed, so a whole file takes its own length to play
                        const Sample& cs = *samples[(size_t) cur];
                        const double regLen = juce::jmax (1.0, (double) (cs.endN.load() - cs.startN.load()) * (double) cs.length);
                        playhead += (double) speed * cs.sampleRate / (sr * regLen);
                    }
                }
            }
        }
        else
        {
            phase = Phase::Gap;
            remaining = 0;
            noteDone = false;
        }

        float oL = 0.f, oR = 0.f;
        for (auto& g : grains)
        {
            if (! g.active) continue;
            const int i = (int) g.pos;
            if (i + 1 >= g.s->length) { g.active = false; continue; }

            const float fr = (float) (g.pos - i);
            const float* d0 = g.s->data.getReadPointer (0);
            const float* d1 = g.s->data.getNumChannels() > 1 ? g.s->data.getReadPointer (1) : d0;
            const float sL = d0[i] + (d0[i + 1] - d0[i]) * fr;
            const float sR = d1[i] + (d1[i + 1] - d1[i]) * fr;
            const float e = env[(int) (1024.f * (float) g.age / (float) g.len)];

            oL += sL * e * g.gl;
            oR += sR * e * g.gr;

            g.pos += g.inc;
            if (++g.age >= g.len) g.active = false;
        }

        float yL = oL * norm, yR = oR * norm;

        // wind: a looping texture layer whose level is the wind knob
        {
            const bool wantTex = active && wind > 0.001f && ! textures.empty();
            if (wantTex && texIdx < 0)
            {
                texIdx = rng.nextInt ((int) textures.size());
                const auto& t0 = textures[(size_t) texIdx];
                texPos = rng.nextDouble() * (double) (t0.length - t0.xfade);
            }
            if (texNext >= 0 && texLevel < 0.003f)
            {
                texIdx = texNext;
                texNext = -1;
                const auto& t0 = textures[(size_t) texIdx];
                texPos = rng.nextDouble() * (double) (t0.length - t0.xfade);
            }
            texLevel += (((wantTex && texNext < 0) ? 1.f : 0.f) - texLevel) * texCoef;

            if (texLevel > 0.0005f && texIdx >= 0)
            {
                const auto& tx = textures[(size_t) texIdx];
                const double loopLen = (double) (tx.length - tx.xfade);
                float tl, tr;
                readTexAt (tx, texPos, tl, tr);
                if (texPos < (double) tx.xfade)   // blend the loop seam
                {
                    float l2, r2;
                    readTexAt (tx, texPos + loopLen, l2, r2);
                    const float th = (float) (texPos / (double) tx.xfade) * juce::MathConstants<float>::halfPi;
                    tl = tl * std::sin (th) + l2 * std::cos (th);
                    tr = tr * std::sin (th) + r2 * std::cos (th);
                }
                texPos += tx.sampleRate / sr;
                if (texPos >= loopLen) texPos -= loopLen;

                const float tg = wind * texLevel;
                yL += tl * tg;
                yR += tr * tg;
            }
        }

        if (R == nullptr) yL = yR = 0.5f * (yL + yR);

        // filter (+ drive), faded in/out by its on button and blended by the filter mix
        const float cf = cutoffSm.getNextValue();
        const float fw = fOnSm.getNextValue();
        if (fw > 0.0005f)
        {
            const float dryL = yL, dryR = yR;
            if ((n & 7) == 0) filter.setCutoffFrequency (cf);
            yL = filter.processSample (0, yL);
            yR = filter.processSample (1, yR);

            if (drive > 0.001f)
            {
                yL = std::tanh (yL * drv) * driveMakeup;
                yR = std::tanh (yR * drv) * driveMakeup;
            }

            const float m = fMix * fw;
            yL = dryL + (yL - dryL) * m;
            yR = dryR + (yR - dryR) * m;
        }

        // ping-pong delay
        const float dw = dOnSm.getNextValue();
        float readPos = (float) delayW - delayTimeSm.getNextValue();
        if (readPos < 0.f) readPos += (float) delaySize;
        const int i0 = (int) readPos, i1 = (i0 + 1) % delaySize;
        const float dfr = readPos - (float) i0;
        const float dL = delayBuf[0][(size_t) i0] + (delayBuf[0][(size_t) i1] - delayBuf[0][(size_t) i0]) * dfr;
        const float dR = delayBuf[1][(size_t) i0] + (delayBuf[1][(size_t) i1] - delayBuf[1][(size_t) i0]) * dfr;
        fbLp[0] += 0.45f * (dL - fbLp[0]);
        fbLp[1] += 0.45f * (dR - fbLp[1]);
        delayBuf[0][(size_t) delayW] = (yL + fbLp[1] * dFb) * dw;
        delayBuf[1][(size_t) delayW] = (yR + fbLp[0] * dFb) * dw;
        delayW = (delayW + 1) % delaySize;
        yL += dL * dMix * dw;
        yR += dR * dMix * dw;

        L[n] = yL;
        if (R != nullptr) R[n] = yR;
    }

    // reverb: runs on a wet-only copy and is blended in per sample, so bypassing fades instead of clicking
    const bool revRunning = rOn || rOnSm.getCurrentValue() > 0.0005f;
    if (revRunning)
    {
        revActive = true;

        struct Space { float sizeBase, sizeRange, dampBase, dampRange, preMs, width; };
        static const Space spaces[4] = {
            { 0.10f, 0.35f, 0.35f, 0.55f,  0.f, 0.8f },   // room
            { 0.40f, 0.35f, 0.00f, 0.35f,  0.f, 1.0f },   // plate: dense and bright
            { 0.62f, 0.30f, 0.20f, 0.55f, 18.f, 1.0f },   // hall
            { 0.82f, 0.17f, 0.30f, 0.50f, 42.f, 1.0f }    // cathedral: huge, long, late
        };
        const auto& sp = spaces[juce::jlimit (0, 3, (int) prm ("rtype"))];

        juce::Reverb::Parameters rp;
        rp.roomSize = juce::jmin (0.99f, sp.sizeBase + sp.sizeRange * rSize);
        rp.damping = juce::jlimit (0.f, 1.f, sp.dampBase + sp.dampRange * rDamp);
        rp.wetLevel = 1.f / 3.f;   // wet only (the reverb scales wet by 3)
        rp.dryLevel = 0.f;
        rp.width = sp.width;
        rp.freezeMode = 0.f;
        reverb.setParameters (rp);

        if ((int) revL.size() < numSamples) { revL.resize ((size_t) numSamples); revR.resize ((size_t) numSamples); }
        const int preN = juce::jlimit (0, preSize - 2, (int) (sp.preMs * 0.001 * sr));
        for (int n = 0; n < numSamples; ++n)
        {
            const float inL = L[n], inR = R != nullptr ? R[n] : L[n];
            preBuf[0][(size_t) preW] = inL;
            preBuf[1][(size_t) preW] = inR;
            int rd = preW - preN;
            if (rd < 0) rd += preSize;
            revL[(size_t) n] = preBuf[0][(size_t) rd];
            revR[(size_t) n] = preBuf[1][(size_t) rd];
            preW = (preW + 1) % preSize;
        }

        reverb.processStereo (revL.data(), revR.data(), numSamples);

        for (int n = 0; n < numSamples; ++n)
        {
            const float m = rMix * rOnSm.getNextValue();
            L[n] = L[n] * (1.f - m) + revL[(size_t) n] * m;
            if (R != nullptr) R[n] = R[n] * (1.f - m) + revR[(size_t) n] * m;
        }
    }
    else if (revActive)
    {
        reverb.reset();
        for (auto& b : preBuf) std::fill (b.begin(), b.end(), 0.f);
        revActive = false;
    }

    // feed the header equalizer with the finished (post-effects) mono signal
    {
        int w = scopeW.load (std::memory_order_relaxed);
        for (int n = 0; n < numSamples; ++n)
        {
            scopeBuf[w].store (R != nullptr ? 0.5f * (L[n] + R[n]) : L[n], std::memory_order_relaxed);
            w = (w + 1) & (scopeSize - 1);
        }
        scopeW.store (w, std::memory_order_release);
        uiSampleRate.store ((float) sr, std::memory_order_relaxed);
    }

    uiSample.store (cur);
    uiPlaying.store (active && phase == Phase::Play);
    {
        float pr = 0.f;
        if (keyMode)
            pr = phase == Phase::Play ? (float) juce::jmin (1.0, playhead) : (noteDone ? 1.f : 0.f);
        else if (phase == Phase::Play)
        {
            pr = (1.f - (float) remaining / (float) holdTotal) * speed;
            pr -= std::floor (pr);
        }
        uiProgress.store (pr);
    }
}

// ---------------------------------------------------------------- state

void ScattrProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("uiScale", editorScale.load(), nullptr);
    st.setProperty ("theme", themeIdx.load(), nullptr);
    st.setProperty ("themeV", 2, nullptr);   // 2 = twilight removed (summer is index 0)
    juce::ValueTree sl ("SAMPLES");
    for (auto& s : samples)
    {
        juce::ValueTree v ("S");
        v.setProperty ("path", s->path, nullptr);
        v.setProperty ("on", s->enabled.load(), nullptr);
        v.setProperty ("start", s->startN.load(), nullptr);
        v.setProperty ("end", s->endN.load(), nullptr);
        sl.appendChild (v, nullptr);
    }
    st.appendChild (sl, nullptr);
    if (auto xml = st.createXml())
        copyXmlToBinary (*xml, dest);
}

void ScattrProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto st = juce::ValueTree::fromXml (*xml);
        if (! st.hasType (apvts.state.getType())) return;

        editorScale = juce::jlimit (0.5f, 2.0f, (float) st.getProperty ("uiScale", 1.0f));
        int th = juce::jmax (0, (int) st.getProperty ("theme", 0));
        if ((int) st.getProperty ("themeV", 1) < 2) th = juce::jmax (0, th - 1);   // older saves counted twilight as 0
        themeIdx = th;

        auto sl = st.getChildWithName ("SAMPLES");
        st.removeChild (sl, nullptr);
        apvts.replaceState (st);

        clearSamples();
        for (auto v : sl)
            if (addSampleFile (juce::File (v["path"].toString())))
            {
                auto& s = *samples.back();
                s.enabled = (bool) v["on"];
                const float a = juce::jlimit (0.f, 1.f, (float) v.getProperty ("start", 0.f));
                const float b = juce::jlimit (0.f, 1.f, (float) v.getProperty ("end", 1.f));
                if (b - a >= s.minGapN()) { s.startN = a; s.endN = b; }
            }
    }
}

juce::AudioProcessorEditor* ScattrProcessor::createEditor() { return new ScattrEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ScattrProcessor(); }
