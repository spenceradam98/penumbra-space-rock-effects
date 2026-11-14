#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"

class PenumbraProcessor : public juce::AudioProcessor
{
public:
    PenumbraProcessor();
    ~PenumbraProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const pn::Engine& getEngine() const { return engine; }

    // UI settings stored with the session (not automatable)
    int getTheme() const { return theme; }
    void setTheme (int t) { theme = t; }
    float getUiScale() const { return uiScale; }
    void setUiScale (float s) { uiScale = s; }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    pn::Params readParams() const;

    pn::Engine engine;
    std::vector<std::atomic<float>*> raw;    // parallel to pn::paramTable
    std::vector<float> scratchL, scratchR;
    int currentProgram = 0;
    int theme = 0;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PenumbraProcessor)
};
