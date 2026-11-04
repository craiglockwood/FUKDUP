#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr double stutterDivBeats[] = { 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
    constexpr double gateDivBeats[]    = { 1.0, 0.5, 0.25, 0.125 };

    inline int wrapIndex (int i, int size)
    {
        i %= size;
        return i < 0 ? i + size : i;
    }
}

//==============================================================================
FukdupAudioProcessor::FukdupAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "FUKDUP", createLayout())
{
    pStutOn     = apvts.getRawParameterValue ("stutterOn");
    pStutDiv    = apvts.getRawParameterValue ("stutterDiv");
    pStutRep    = apvts.getRawParameterValue ("stutterRepeats");
    pRevOn      = apvts.getRawParameterValue ("reverseOn");
    pRevSize    = apvts.getRawParameterValue ("reverseSize");
    pTapeOn     = apvts.getRawParameterValue ("tapeOn");
    pTapeTime   = apvts.getRawParameterValue ("tapeTime");
    pCrushOn    = apvts.getRawParameterValue ("crushOn");
    pBits       = apvts.getRawParameterValue ("crushBits");
    pDown       = apvts.getRawParameterValue ("crushDown");
    pGateOn     = apvts.getRawParameterValue ("gateOn");
    pGateDiv    = apvts.getRawParameterValue ("gateDiv");
    pGateDens   = apvts.getRawParameterValue ("gateDensity");
    pGateSmooth = apvts.getRawParameterValue ("gateSmooth");
    pMix        = apvts.getRawParameterValue ("mix");
    pFeedback   = apvts.getRawParameterValue ("feedback");
    pOutput     = apvts.getRawParameterValue ("output");
}

juce::AudioProcessorValueTreeState::ParameterLayout FukdupAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pid = [] (const char* id) { return ParameterID { id, 1 }; };

    auto ms   = [] (float v, int) { return String (roundToInt (v)) + " ms"; };
    auto pct  = [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; };
    auto db   = [] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " dB"; };
    auto bits = [] (float v, int) { return String (v, 1) + " bit"; };

    // Stutter / buffer repeat
    layout.add (std::make_unique<AudioParameterBool>   (pid ("stutterOn"), "Stutter", false));
    layout.add (std::make_unique<AudioParameterChoice> (pid ("stutterDiv"), "Stutter Division",
                    StringArray { "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" }, 3));
    layout.add (std::make_unique<AudioParameterInt>    (pid ("stutterRepeats"), "Stutter Repeats", 2, 16, 4));

    // Reverse
    layout.add (std::make_unique<AudioParameterBool>  (pid ("reverseOn"), "Reverse", false));
    layout.add (std::make_unique<AudioParameterFloat> (pid ("reverseSize"), "Reverse Size",
                    NormalisableRange<float> (40.0f, 1000.0f, 1.0f, 0.5f), 250.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (ms)));

    // Tape stop
    layout.add (std::make_unique<AudioParameterBool>  (pid ("tapeOn"), "Tape Stop", false));
    layout.add (std::make_unique<AudioParameterFloat> (pid ("tapeTime"), "Tape Stop Time",
                    NormalisableRange<float> (50.0f, 3000.0f, 1.0f, 0.5f), 600.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (ms)));

    // Bitcrush + sample rate reduction
    layout.add (std::make_unique<AudioParameterBool>  (pid ("crushOn"), "Crush", false));
    layout.add (std::make_unique<AudioParameterFloat> (pid ("crushBits"), "Bit Depth",
                    NormalisableRange<float> (1.0f, 16.0f, 0.1f), 8.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (bits)));
    layout.add (std::make_unique<AudioParameterInt>   (pid ("crushDown"), "Downsample", 1, 64, 4));

    // Random gate / chop
    layout.add (std::make_unique<AudioParameterBool>   (pid ("gateOn"), "Gate", false));
    layout.add (std::make_unique<AudioParameterChoice> (pid ("gateDiv"), "Gate Division",
                    StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    layout.add (std::make_unique<AudioParameterFloat>  (pid ("gateDensity"), "Gate Density",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat>  (pid ("gateSmooth"), "Gate Smooth",
                    NormalisableRange<float> (0.5f, 50.0f, 0.1f, 0.5f), 3.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (ms)));

    // Master
    layout.add (std::make_unique<AudioParameterFloat> (pid ("mix"), "Mix",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (pid ("feedback"), "Feedback",
                    NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.3f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (pid ("output"), "Output",
                    NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (db)));

    return layout;
}

//==============================================================================
void FukdupAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    ringSize = juce::jmax (1024, (int) (sr * 10.0));

    ring.setSize (2, ringSize);     ring.clear();
    tapeRing.setSize (2, ringSize); tapeRing.clear();
    loopBuf.setSize (2, (int) (sr * 4.0) + 1); loopBuf.clear();

    writePos = tapeWrite = 0;
    loopLen = 1; loopPhase = 0; lastStutStep = -1; wasStutOn = false;
    revAnchor = revK = revLen = 0;
    tapeRead = 0.0; tapeSpeed = 1.0f; tapeBlend = 0.0f; wasTapeOn = false;
    crushCounter = 0; crushHold[0] = crushHold[1] = 0.0f;
    lastGateStep = -1; gateTarget = gateGain = 1.0f;
    fbState[0] = fbState[1] = 0.0f;

    mixSm.reset (sr, 0.02);
    outSm.reset (sr, 0.02);
    mixSm.setCurrentAndTargetValue (pMix->load());
    outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));
}

bool FukdupAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void FukdupAudioProcessor::captureLoop (int length, int numChannels)
{
    loopLen = juce::jlimit (16, loopBuf.getNumSamples(), length);
    const int start = wrapIndex (writePos - loopLen, ringSize);
    const int first = juce::jmin (loopLen, ringSize - start);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        loopBuf.copyFrom (ch, 0, ring, ch, start, first);
        if (first < loopLen)
            loopBuf.copyFrom (ch, first, ring, ch, 0, loopLen - first);
    }
    loopPhase = 0;
}

//==============================================================================
void FukdupAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n      = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    const int nCh = juce::jmin (numOut, buffer.getNumChannels(), 2);
    if (nCh <= 0 || n <= 0)
        return;

    // ---- Tempo / position (falls back to an internal 120 BPM clock) ----
    double bpm = 120.0;
    double beat = internalBeats;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = *b;
            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition())
                    beat = *p;
        }
    }
    bpm = juce::jlimit (30.0, 300.0, bpm);
    const double beatsPerSample = bpm / (60.0 * sr);
    const double samplesPerBeat = 60.0 * sr / bpm;

    // ---- Parameters ----
    const bool   stutOn     = pStutOn->load() > 0.5f;
    const double stutDiv    = stutterDivBeats[juce::jlimit (0, 5, (int) pStutDiv->load())];
    const int    repeats    = juce::jmax (1, (int) pStutRep->load());
    const double stutPeriod = stutDiv * repeats;
    const int    stutLen    = (int) (stutDiv * samplesPerBeat);

    const bool revOn = pRevOn->load() > 0.5f;
    const int  revN  = juce::jmax (64, (int) (pRevSize->load() * 0.001 * sr));

    const bool  tapeOn  = pTapeOn->load() > 0.5f;
    const float tapeDec = (float) (1.0 / juce::jmax (1.0, pTapeTime->load() * 0.001 * sr));

    const bool  crushOn = pCrushOn->load() > 0.5f;
    const float levels  = std::pow (2.0f, pBits->load() - 1.0f);
    const int   down    = juce::jmax (1, (int) pDown->load());

    const bool   gateOn   = pGateOn->load() > 0.5f;
    const double gateDiv  = gateDivBeats[juce::jlimit (0, 3, (int) pGateDiv->load())];
    const float  density  = pGateDens->load();
    const float  gateCoef = 1.0f - std::exp (-1.0f / (float) (juce::jmax (0.1f, pGateSmooth->load()) * 0.001 * sr));

    const float fb = pFeedback->load();
    mixSm.setTargetValue (pMix->load());
    outSm.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    const int   fadeLen   = juce::jmax (1, (int) (0.002 * sr));
    const float blendStep = (float) (1.0 / (0.005 * sr));
    const bool  timeFx    = stutOn || revOn;

    float peak = 0.0f;
    auto* const* data = buffer.getArrayOfWritePointers();

    for (int i = 0; i < n; ++i)
    {
        // ---- Stutter: grab a fresh slice at every grid period ----
        const auto stStep = (long long) std::floor (beat / stutPeriod);
        if (stutOn && (! wasStutOn || stStep != lastStutStep))
            captureLoop (stutLen, nCh);
        wasStutOn = stutOn;
        lastStutStep = stStep;

        // ---- Gate: roll the dice on each step ----
        const auto gStep = (long long) std::floor (beat / gateDiv);
        if (gStep != lastGateStep)
        {
            lastGateStep = gStep;
            const bool open = rng.nextFloat() < density;
            gateTarget = open ? 1.0f : 0.0f;
            gateBits = (gateBits << 1) | (open ? 1u : 0u);
        }
        gateGain += ((gateOn ? gateTarget : 1.0f) - gateGain) * gateCoef;

        // ---- Reverse: backwards grains anchored at the write head ----
        if (! revOn)
            revK = revLen;
        else if (revK >= revLen)
        {
            revAnchor = wrapIndex (writePos - 1, ringSize);
            revK = 0;
            revLen = revN;
        }
        const int   revFade = juce::jmax (1, juce::jmin (512, revLen / 4));
        const float revW    = juce::jmin (1.0f, (float) revK / (float) revFade, (float) (revLen - revK) / (float) revFade);

        // ---- Tape stop: read head decelerates to zero ----
        if (tapeOn && ! wasTapeOn)
        {
            tapeRead = (double) wrapIndex (tapeWrite - 1, ringSize);
            tapeSpeed = 1.0f;
        }
        wasTapeOn = tapeOn;
        if (tapeOn)
            tapeSpeed = juce::jmax (0.0f, tapeSpeed - tapeDec);
        tapeBlend = tapeOn ? juce::jmin (1.0f, tapeBlend + blendStep)
                           : juce::jmax (0.0f, tapeBlend - blendStep);

        const int   t0       = (int) tapeRead;
        const int   t1       = (t0 + 1 >= ringSize) ? 0 : t0 + 1;
        const float tFrac    = (float) (tapeRead - t0);
        const float tapeGain = juce::jmin (1.0f, tapeSpeed * 3.0f);

        const float mix     = mixSm.getNextValue();
        const float outGain = outSm.getNextValue();

        const int   loopFade = juce::jmax (1, juce::jmin (fadeLen, loopLen / 4));
        const float loopW    = juce::jmin (1.0f, (float) loopPhase / (float) loopFade,
                                           (float) (loopLen - loopPhase) / (float) loopFade);

        float mono = 0.0f;

        for (int ch = 0; ch < nCh; ++ch)
        {
            const float dry = data[ch][i];

            // Feedback re-injects the glitched output into the capture buffer
            ring.setSample (ch, writePos, dry + (timeFx ? fbState[ch] : 0.0f));

            float s = dry;
            if (stutOn)
                s = loopBuf.getSample (ch, loopPhase) * loopW;
            else if (revOn)
                s = ring.getSample (ch, wrapIndex (revAnchor - revK, ringSize)) * revW;

            tapeRing.setSample (ch, tapeWrite, s);
            if (tapeBlend > 0.0f)
            {
                const float* t = tapeRing.getReadPointer (ch);
                const float tv = (t[t0] + (t[t1] - t[t0]) * tFrac) * tapeGain;
                s += (tv - s) * tapeBlend;
            }

            if (crushOn)
            {
                if (crushCounter == 0)
                    crushHold[ch] = s;
                s = std::round (crushHold[ch] * levels) / levels;
            }

            s *= gateGain;
            fbState[ch] = std::tanh (s * fb);

            const float out = (dry * (1.0f - mix) + s * mix) * outGain;
            data[ch][i] = out;
            mono += out;
        }

        // ---- Advance heads ----
        if (++crushCounter >= down) crushCounter = 0;
        if (stutOn && ++loopPhase >= loopLen) loopPhase = 0;
        if (revOn) ++revK;
        if (tapeOn)
        {
            tapeRead += tapeSpeed;
            if (tapeRead >= ringSize) tapeRead -= ringSize;
        }
        if (++writePos  >= ringSize) writePos = 0;
        if (++tapeWrite >= ringSize) tapeWrite = 0;
        beat += beatsPerSample;
        internalBeats += beatsPerSample;

        // ---- Scope ----
        mono /= (float) nCh;
        peak = juce::jmax (peak, std::abs (mono));
        if (++scopeDecim >= 2)
        {
            scopeDecim = 0;
            const int w = scopeWrite.load (std::memory_order_relaxed);
            scope[(size_t) w].store (mono, std::memory_order_relaxed);
            scopeWrite.store ((w + 1) % scopeSize, std::memory_order_relaxed);
        }
    }

    // ---- Publish visual state ----
    const double sp = beat / stutPeriod;
    visStutterProg.store (stutOn ? (float) (sp - std::floor (sp)) : 0.0f);
    visRevProg.store (revOn && revLen > 0 ? (float) revK / (float) revLen : 0.0f);
    visTapeSpeed.store (tapeOn ? tapeSpeed : 1.0f);
    visGateGain.store (gateGain);
    visGateBits.store (gateBits);
    visBpm.store ((float) bpm);
    if (peak > visLevel.load())
        visLevel.store (peak);
}

//==============================================================================
juce::AudioProcessorEditor* FukdupAudioProcessor::createEditor()
{
    return new FukdupAudioProcessorEditor (*this);
}

void FukdupAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FukdupAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FukdupAudioProcessor();
}
