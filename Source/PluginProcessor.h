#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>

class FukdupAudioProcessor : public juce::AudioProcessor
{
public:
    FukdupAudioProcessor();
    ~FukdupAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FUKDUP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // ---- Visual feedback: written by the audio thread, read by the UI ----
    static constexpr int scopeSize = 1024;
    std::array<std::atomic<float>, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> visStutterProg { 0.0f }, visRevProg { 0.0f }, visTapeSpeed { 1.0f },
                       visGateGain { 1.0f }, visLevel { 0.0f }, visBpm { 120.0f };
    std::atomic<juce::uint32> visGateBits { 0 };

private:
    void captureLoop (int length, int numChannels);

    std::atomic<float>* pStutOn = nullptr;
    std::atomic<float>* pStutDiv = nullptr;
    std::atomic<float>* pStutRep = nullptr;
    std::atomic<float>* pRevOn = nullptr;
    std::atomic<float>* pRevSize = nullptr;
    std::atomic<float>* pTapeOn = nullptr;
    std::atomic<float>* pTapeTime = nullptr;
    std::atomic<float>* pCrushOn = nullptr;
    std::atomic<float>* pBits = nullptr;
    std::atomic<float>* pDown = nullptr;
    std::atomic<float>* pGateOn = nullptr;
    std::atomic<float>* pGateDiv = nullptr;
    std::atomic<float>* pGateDens = nullptr;
    std::atomic<float>* pGateSmooth = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pOutput = nullptr;

    double sr = 44100.0;

    // capture buffers
    juce::AudioBuffer<float> ring, tapeRing, loopBuf;
    int ringSize = 1, writePos = 0, tapeWrite = 0;

    // stutter
    int loopLen = 1, loopPhase = 0;
    long long lastStutStep = -1;
    bool wasStutOn = false;

    // reverse
    int revAnchor = 0, revK = 0, revLen = 0;

    // tape stop
    double tapeRead = 0.0;
    float tapeSpeed = 1.0f, tapeBlend = 0.0f;
    bool wasTapeOn = false;

    // crush
    int crushCounter = 0;
    float crushHold[2] = { 0.0f, 0.0f };

    // gate
    long long lastGateStep = -1;
    float gateTarget = 1.0f, gateGain = 1.0f;
    juce::uint32 gateBits = 0;

    // feedback
    float fbState[2] = { 0.0f, 0.0f };

    double internalBeats = 0.0;
    int scopeDecim = 0;
    juce::Random rng;
    juce::SmoothedValue<float> mixSm, outSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FukdupAudioProcessor)
};
