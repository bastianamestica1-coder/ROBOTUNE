#pragma once
#include "PluginProcessor.h"

class RoboTuneEditor : public juce::AudioProcessorEditor
{
public:
    explicit RoboTuneEditor (RoboTuneProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setupLabel (juce::Label&, const juce::String&);

    RoboTuneProcessor& processor;
    juce::ComboBox keyBox, scaleBox;
    juce::Slider retuneSlider;
    juce::Label keyLabel, scaleLabel, retuneLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retuneAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoboTuneEditor)
};
