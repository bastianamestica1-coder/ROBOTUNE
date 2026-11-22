#include "PluginEditor.h"

static const juce::Colour kAccent (0xff00e5ff);
static const juce::Colour kBg     (0xff12121a);

RoboTuneEditor::RoboTuneEditor (RoboTuneProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (420, 250);

    keyBox.addItemList   (p.apvts.getParameter ("key")->getAllValueStrings(), 1);
    scaleBox.addItemList (p.apvts.getParameter ("scale")->getAllValueStrings(), 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);

    retuneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    retuneSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
    retuneSlider.setTextValueSuffix (" ms");
    retuneSlider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    retuneSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (retuneSlider);

    setupLabel (keyLabel, "KEY");
    setupLabel (scaleLabel, "SCALE");
    setupLabel (retuneLabel, "RETUNE SPEED");

    keyAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "key", keyBox);
    scaleAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "scale", scaleBox);
    retuneAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (p.apvts, "retune", retuneSlider);
}

void RoboTuneEditor::setupLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    l.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (l);
}

void RoboTuneEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("ROBOTUNE", getLocalBounds().removeFromTop (48), juce::Justification::centred);
    g.setColour (kAccent.withAlpha (0.4f));
    g.drawLine (20.0f, 48.0f, (float) getWidth() - 20.0f, 48.0f, 1.0f);
}

void RoboTuneEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (36);

    auto left = area.removeFromLeft (190);
    left.removeFromTop (14);
    keyLabel.setBounds   (left.removeFromTop (18));
    keyBox.setBounds     (left.removeFromTop (30));
    left.removeFromTop (16);
    scaleLabel.setBounds (left.removeFromTop (18));
    scaleBox.setBounds   (left.removeFromTop (30));

    retuneLabel.setBounds (area.removeFromTop (32).withTrimmedTop (14));
    retuneSlider.setBounds (area.reduced (10, 0));
}
