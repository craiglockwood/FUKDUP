#pragma once

#include "PluginProcessor.h"

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getLabelFont (juce::Label&) override;
};

class FukdupAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit FukdupAudioProcessorEditor (FukdupAudioProcessor&);
    ~FukdupAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Module
    {
        juce::String name;
        juce::Colour colour;
        juce::ToggleButton toggle;
        std::unique_ptr<ButtonAttachment> attachment;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds, visual;
    };

    void timerCallback() override;
    Knob& makeKnob (const juce::String& paramID, const juce::String& name, juce::Colour colour);
    static void layoutKnob (Knob&, juce::Rectangle<int> cell);
    float param (const char* id) const;

    void drawLogo   (juce::Graphics&, bool anyOn);
    void drawScope  (juce::Graphics&, bool anyOn);
    void drawModule (juce::Graphics&, int index);
    void drawMaster (juce::Graphics&);

    FukdupAudioProcessor& proc;
    NeonLookAndFeel laf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::array<Module, 5> modules;
    Knob* mixKnob = nullptr;
    Knob* fbKnob  = nullptr;
    Knob* outKnob = nullptr;

    juce::Rectangle<int> logoArea, scopeArea, masterArea, meterArea;
    std::array<float, FukdupAudioProcessor::scopeSize> scopeData {};
    float tapeAngle = 0.0f, level = 0.0f;
    int frame = 0;
    juce::Random rnd;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FukdupAudioProcessorEditor)
};
