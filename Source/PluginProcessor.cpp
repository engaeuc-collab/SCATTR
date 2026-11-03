#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    add ("size",  "Grain Size",  10.f, 500.f, 120.f, 0.5f);
    add ("dens",  "Density",     1.f,  80.f,  25.f,  0.5f);
    add ("pitch", "Pitch",      -24.f, 24.f,  0.f,   1.f);
    add ("pjit",  "Pitch Jitter", 0.f, 12.f,  0.f,   1.f);
    add ("spray", "Spray",        0.f, 1.f,   0.1f,  1.f);
    add ("hold",  "Hold",         0.5f, 20.f, 4.f,   0.5f);
    add ("gap",   "Gap",          0.f, 5.f,   0.3f,  1.f);
    add ("cycle", "Key Cycle",    0.05f, 20.f, 1.5f, 0.5f);   // seconds per sample while a key is held
    add ("width", "Width",        0.f, 1.f,   0.5f,  1.f);
    add ("amp",   "Amp",          0.f, 1.f,   0.7f,  1.f);
    add ("mtune", "Master Tune", -100.f, 100.f, 0.f, 1.f);   // cents

    // effects
    add ("cutoff", "Filter Cutoff", 20.f, 20000.f, 20000.f, 0.3f);
    add ("reso",   "Filter Reso",   0.5f, 12.f,    0.7f,    0.5f);
    add ("drive",  "Drive",         0.f,  1.f,     0.f,     1.f);
    add ("dtime",  "Delay Time",    10.f, 1000.f,  350.f,   0.5f);
    add ("dfb",    "Delay Feedback",0.f,  0.95f,   0.35f,   1.f);
    add ("dmix",   "Delay Mix",     0.f,  1.f,     0.15f,   1.f);
    add ("rsize",  "Reverb Size",   0.f,  1.f,     0.6f,    1.f);
    add ("rdamp",  "Reverb Damp",   0.f,  1.f,     0.5f,    1.f);
    add ("rmix",   "Reverb Mix",    0.f,  1.f,     0.25f,   1.f);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ftype", 1 }, "Filter Type", juce::StringArray { "LP", "HP", "BP" }, 0));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "drone", 1 }, "Drone (free-run)", true));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "tune", 1 }, "Tune Samples", true));

    return { p.begin(), p.end() };
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
    for (int i = 0; i <= 1024; ++i)
        env[i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 1024.f);
}

void ScattrProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSr = sampleRate;

    filter.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 });
    filter.reset();
    reverb.setSampleRate (sampleRate);
    reverb.reset();

    delaySize = (int) (sampleRate * 1.1) + 8;
    for (auto& b : delayBuf) b.assign ((size_t) delaySize, 0.f);
    delayW = 0;
    fbLp[0] = fbLp[1] = 0.f;
    delayTimeSm.reset (sampleRate, 0.08);
    delayTimeSm.setCurrentAndTargetValue ((float) (0.35 * sampleRate));
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
    }

    auto prm = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float sizeMs = prm ("size"), dens = prm ("dens"), pitch = prm ("pitch"), pjit = prm ("pjit"),
                spray = prm ("spray"), holdS = prm ("hold"), gapS = prm ("gap"), cycleS = prm ("cycle"), width = prm ("width"),
                amp = prm ("amp");
    const bool active = prm ("drone") > 0.5f || notesHeld > 0;

    const double sr = currentSr;
    const int numSamples = buffer.getNumSamples();
    const int sizeSamples = juce::jmax (32, (int) (sizeMs * 0.001 * sr));
    const float norm = amp / std::sqrt (juce::jmax (1.0f, dens * sizeMs * 0.001f));
    const float pitchOff = notesHeld > 0 ? (float) (heldNote - 60) : 0.f;
    const bool tuneOn = prm ("tune") > 0.5f;
    const float masterTune = prm ("mtune") * 0.01f;   // cents -> semitones
    const float targetNote = notesHeld > 0 ? (float) heldNote : 60.f;   // C4 when drifting with no key held

    // effect settings
    const float cutoff = prm ("cutoff"), reso = prm ("reso"), drive = prm ("drive"),
                dTime = prm ("dtime"), dFb = prm ("dfb"), dMix = prm ("dmix"),
                rSize = prm ("rsize"), rDamp = prm ("rdamp"), rMix = prm ("rmix");
    const int fType = juce::jlimit (0, 2, (int) prm ("ftype"));
    const bool filterOn = ! ((fType == 0 && cutoff > 18000.f) || (fType == 1 && cutoff < 25.f));
    filter.setType (fType == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                  : fType == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                               : juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.setResonance (reso);
    cutoffSm.setTargetValue (juce::jlimit (20.f, (float) (0.45 * sr), cutoff));
    delayTimeSm.setTargetValue (dTime * 0.001f * (float) sr);
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

        const Sample& s = *samples[(size_t) cur];
        const double srcLen = (double) s.length;
        const double regS = (double) s.startN.load() * srcLen;
        const double regE = (double) s.endN.load() * srcLen;
        const double regLen = juce::jmax (1.0, regE - regS);
        // tuned: shift this sample from its own detected pitch to the target note
        const float keyOff = (tuneOn && s.rootNote >= 0.f) ? targetNote - s.rootNote : pitchOff;
        const float semis = pitch + keyOff + masterTune + (rng.nextFloat() * 2.f - 1.f) * pjit;
        const double inc = std::pow (2.0, semis / 12.0) * s.sampleRate / sr;
        const int len = juce::jmin (len0, (int) ((regLen - 4.0) / inc));
        if (len <= 32) return;

        const double prog = 1.0 - (double) remaining / (double) holdTotal;
        double start = regS + (prog + (rng.nextFloat() - 0.5f) * spray) * regLen;
        start = juce::jlimit (regS, juce::jmax (regS, regE - (double) len * inc - 3.0), start);

        const float pan = 0.5f + (rng.nextFloat() - 0.5f) * width;
        slot->active = true;
        slot->s = &s;
        slot->pos = start;
        slot->inc = inc;
        slot->age = 0;
        slot->len = len;
        slot->gl = std::cos (pan * juce::MathConstants<float>::halfPi) * 1.4142f;
        slot->gr = std::sin (pan * juce::MathConstants<float>::halfPi) * 1.4142f;
        uiGrain.store ((float) (start / srcLen));
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
            holdTotal = remaining = juce::jmax (1, (int) (cycleS * sr));
            grainCountdown = 0.0;
        }
    }

    for (int n = 0; n < numSamples; ++n)
    {
        if (active)
        {
            if (phase == Phase::Gap)
            {
                if (remaining > 0) --remaining;
                else
                {
                    const int i = pickSample();
                    if (i >= 0)
                    {
                        cur = last = i;
                        phase = Phase::Play;
                        holdTotal = remaining = juce::jmax (1, (int) ((notesHeld > 0 ? cycleS : holdS) * sr));
                        grainCountdown = 0.0;
                    }
                }
            }
            if (phase == Phase::Play)
            {
                if (remaining <= 0)
                {
                    // while a key is held, roll straight into the next sample (no silent rest)
                    phase = Phase::Gap;
                    remaining = notesHeld > 0 ? 0 : (int) (gapS * sr);
                }
                else
                {
                    if (grainCountdown <= 0.0)
                    {
                        spawn();
                        grainCountdown += sr / (double) dens * (0.7 + 0.6 * rng.nextFloat());
                    }
                    grainCountdown -= 1.0;
                    --remaining;
                }
            }
        }
        else
        {
            phase = Phase::Gap;
            remaining = 0;
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
        if (R == nullptr) yL = yR = 0.5f * (yL + yR);

        // filter
        const float cf = cutoffSm.getNextValue();
        if (filterOn)
        {
            if ((n & 7) == 0) filter.setCutoffFrequency (cf);
            yL = filter.processSample (0, yL);
            yR = filter.processSample (1, yR);
        }

        // drive
        if (drive > 0.001f)
        {
            yL = std::tanh (yL * drv) * driveMakeup;
            yR = std::tanh (yR * drv) * driveMakeup;
        }

        // ping-pong delay
        float readPos = (float) delayW - delayTimeSm.getNextValue();
        if (readPos < 0.f) readPos += (float) delaySize;
        const int i0 = (int) readPos, i1 = (i0 + 1) % delaySize;
        const float dfr = readPos - (float) i0;
        const float dL = delayBuf[0][(size_t) i0] + (delayBuf[0][(size_t) i1] - delayBuf[0][(size_t) i0]) * dfr;
        const float dR = delayBuf[1][(size_t) i0] + (delayBuf[1][(size_t) i1] - delayBuf[1][(size_t) i0]) * dfr;
        fbLp[0] += 0.45f * (dL - fbLp[0]);
        fbLp[1] += 0.45f * (dR - fbLp[1]);
        delayBuf[0][(size_t) delayW] = yL + fbLp[1] * dFb;
        delayBuf[1][(size_t) delayW] = yR + fbLp[0] * dFb;
        delayW = (delayW + 1) % delaySize;
        yL += dL * dMix;
        yR += dR * dMix;

        L[n] = yL;
        if (R != nullptr) R[n] = yR;
    }

    // reverb (in place, wet/dry handled by the reverb's own levels)
    if (rMix > 0.001f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = rSize;
        rp.damping = rDamp;
        rp.wetLevel = rMix / 3.f;
        rp.dryLevel = (1.f - rMix) / 2.f;
        rp.width = 1.f;
        rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        if (R != nullptr) reverb.processStereo (L, R, numSamples);
        else              reverb.processMono (L, numSamples);
    }

    uiSample.store (cur);
    uiPlaying.store (active && phase == Phase::Play);
    uiProgress.store (phase == Phase::Play ? 1.f - (float) remaining / (float) holdTotal : 0.f);
}

// ---------------------------------------------------------------- state

void ScattrProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("uiScale", editorScale.load(), nullptr);
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
