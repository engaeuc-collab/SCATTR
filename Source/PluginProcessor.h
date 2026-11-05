#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "SpectralGate.h"
#include "EqCore.h"
#include <atomic>
#include <memory>
#include <vector>

struct Sample
{
    juce::String path, name;
    juce::AudioBuffer<float> data;
    double sampleRate = 44100.0;
    int length = 0;
    float rootNote = -1.f;   // detected pitch as a (fractional) MIDI note, -1 = no clear pitch
    std::vector<std::pair<float, float>> peaks;
    std::atomic<bool> enabled { true };

    // Playable region, normalised 0..1 across the file. Written by the UI, read by the audio thread.
    std::atomic<float> startN { 0.f }, endN { 1.f };

    // smallest allowed region (about 1024 source samples) so grains always have something to read
    float minGapN() const { return juce::jlimit (0.002f, 0.5f, 1024.f / (float) juce::jmax (1, length)); }
};

// A looping background texture used by the "wind" knob
struct Texture
{
    juce::AudioBuffer<float> data;
    double sampleRate = 44100.0;
    int length = 0, xfade = 0;   // xfade = loop crossfade length in samples
};

class ScattrProcessor : public juce::AudioProcessor,
                        private juce::AsyncUpdater
{
public:
    ScattrProcessor();
    ~ScattrProcessor() override { cancelPendingUpdate(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nature"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Message-thread only
    bool addSampleFile (const juce::File& f);
    void clearSamples();
    void removeSample (int index);

    juce::AudioProcessorValueTreeState apvts;

    // Only mutated on the message thread (under `lock`); the editor may read it on the message thread.
    std::vector<std::shared_ptr<Sample>> samples;

    // Values for the UI
    std::atomic<int>   uiSample   { -1 };
    std::atomic<float> uiProgress { 0.f };
    std::atomic<bool>  uiPlaying  { false };
    std::atomic<float> uiGrain    { 0.f };

    // Editor zoom (1.0 = 100%), saved with the plugin state
    std::atomic<float> editorScale { 1.f };

    // Colour theme index, saved with the plugin state
    std::atomic<int> themeIdx { 0 };

    // Output tap for the header equalizer. The audio thread writes, the UI thread reads; a torn read
    // only blurs a single frame of the display.
    static constexpr int scopeSize = 4096;   // power of two
    std::atomic<float> scopeBuf[scopeSize];
    std::atomic<int>   scopeW { 0 };         // next write index, always kept in 0..scopeSize-1
    std::atomic<float> uiSampleRate { 44100.f };

    // Spectral gate (the pop-out window). The audio thread runs it; the editor reads its analyzer arrays.
    SpectralGate spec;

    // Taps for the FX-tab equalizer display: the mono signal going into and coming out of the EQ.
    // The audio thread writes, the UI thread reads (a torn read only blurs a single frame).
    static constexpr int eqTapSize = 8192;   // power of two
    std::atomic<float> eqPreBuf[eqTapSize], eqPostBuf[eqTapSize];
    std::atomic<int>   eqTapW { 0 };         // next write index, always kept in 0..eqTapSize-1

private:
    void handleAsyncUpdate() override { setLatencySamples (pendingLatency.load()); }   // the host hears about latency on the message thread
    std::atomic<int> pendingLatency { 0 };
    int reportedLatency = 0;

    struct Grain
    {
        bool active = false;
        const Sample* s = nullptr;
        double pos = 0.0, inc = 1.0;
        int age = 0, len = 0;
        float gl = 1.f, gr = 1.f;
    };

    int pickSample();
    int bag[256] {};          // shuffled play order: every sample once per round
    int bagCount = 0;

    juce::AudioFormatManager formats;
    juce::CriticalSection lock;
    int generation = 0, lastGen = -1;

    static constexpr int maxGrains = 128;
    Grain grains[maxGrains];
    float env[1025] {};

    enum class Phase { Gap, Play };
    Phase phase = Phase::Gap;
    int remaining = 0, holdTotal = 1, cur = -1, last = -1;
    double grainCountdown = 0.0, currentSr = 44100.0;
    int notesHeld = 0, heldNote = 60;
    double playhead = 0.0;    // while a key is held: 0..1 position of the sweep through the sample's playable region
    bool noteDone = false;    // the held key's sample has played all the way through (silent until the next key)
    bool retrigger = false;   // set on every note-on: jump to a fresh random sample right away
    juce::Random rng;

    // wind texture layer
    void loadTextures();
    std::vector<Texture> textures;
    int texIdx = -1, texNext = -1;
    double texPos = 0.0;
    float texLevel = 0.f;

    // effects
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Chorus<float> chorus;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pOnSm, cOnSm;   // bypass fades
    bool phaserActive = false, chorusActive = false;

    EqCore::Processor eq;
    std::atomic<float>* eqParam[EqCore::numBands][6] {};   // on, type, freq, gain, q, slope (cached so the audio thread never looks them up by name)

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dOnSm, rOnSm;   // bypass fades
    std::vector<float> revL, revR, preBuf[2];                                           // reverb scratch + pre-delay
    int preW = 0, preSize = 1;
    bool revActive = false;
    juce::Reverb reverb;
    std::vector<float> delayBuf[2];
    int delayW = 0, delaySize = 1;
    float fbLp[2] {};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayTimeSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScattrProcessor)
};
