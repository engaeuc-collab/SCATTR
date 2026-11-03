#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <vector>

struct Sample
{
    juce::String path, name;
    juce::AudioBuffer<float> data;
    double sampleRate = 44100.0;
    int length = 0;
    std::vector<std::pair<float, float>> peaks;
    std::atomic<bool> enabled { true };
};

class ScattrProcessor : public juce::AudioProcessor
{
public:
    ScattrProcessor();
    ~ScattrProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SCATTR"; }
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

    juce::AudioProcessorValueTreeState apvts;

    // Only mutated on the message thread (under `lock`); the editor may read it on the message thread.
    std::vector<std::shared_ptr<Sample>> samples;

    // Values for the UI
    std::atomic<int>   uiSample   { -1 };
    std::atomic<float> uiProgress { 0.f };
    std::atomic<bool>  uiPlaying  { false };
    std::atomic<float> uiGrain    { 0.f };

private:
    struct Grain
    {
        bool active = false;
        const Sample* s = nullptr;
        double pos = 0.0, inc = 1.0;
        int age = 0, len = 0;
        float gl = 1.f, gr = 1.f;
    };

    int pickSample();

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
    juce::Random rng;

    // effects
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Reverb reverb;
    std::vector<float> delayBuf[2];
    int delayW = 0, delaySize = 1;
    float fbLp[2] {};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayTimeSm;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScattrProcessor)
};
