#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

struct Theme
{
    const char* name;
    juce::Colour bg, bg2, panel, panelEdge, text, dim, accent, knob, knobCap, pointer;
    int style;            // 0 Spectrum, 1 Console, 2 Eclipse, 3 Moon Dust, 4 Tape Room
    bool spectrumAccents; // each module gets its own spectral colour
    bool light;
};

const Theme& themeAt (int index);
int numThemes();

class PenumbraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PenumbraLookAndFeel() { setTheme (0); }
    void setTheme (int index);
    const Theme& theme() const { return *th; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

private:
    const Theme* th = nullptr;
};

// A rotary knob with its name and value painted underneath.
class Knob : public juce::Component
{
public:
    Knob (PenumbraProcessor&, const juce::String& paramId, const juce::String& label);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
    juce::Colour accent;
    bool onHeader = false;

private:
    juce::String label;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

class ModulePanel : public juce::Component
{
public:
    ModulePanel (PenumbraProcessor&, int moduleIndex, const juce::String& title, const juce::String& onId, int knobColumns);
    void addKnob (const juce::String& id, const juce::String& label);
    void addChoice (const juce::String& id);
    void addToggle (const juce::String& id, const juce::String& label);

    void paint (juce::Graphics&) override;
    void resized() override;
    void setAccent (juce::Colour c);
    juce::Rectangle<int> indicatorArea() const { return { getWidth() - 64, 4, 56, 20 }; }

    int columns;
    int module;

private:
    void paintIndicator (juce::Graphics&, juce::Rectangle<float>);

    PenumbraProcessor& proc;
    juce::String title;
    juce::Colour accent;
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttach;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttach;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAttach;
};

class PenumbraEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PenumbraEditor (PenumbraProcessor&);
    ~PenumbraEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;

    static constexpr int kDesignW = 1180, kDesignH = 650;

private:
    class Rack : public juce::Component
    {
    public:
        explicit Rack (PenumbraEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        PenumbraEditor& ed;
    };

    void timerCallback() override;
    void applyTheme (int index);
    void refreshPresetBox();
    void paintRack (juce::Graphics&);
    void layoutRack();

    PenumbraProcessor& proc;
    PenumbraLookAndFeel lnf;
    Rack rack { *this };
    std::vector<std::unique_ptr<ModulePanel>> panels;
    std::unique_ptr<Knob> inKnob, outKnob;
    juce::ComboBox presetBox, themeBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    float inMeter = 0, outMeter = 0;
    int lastProgram = -1;

    friend class Rack;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PenumbraEditor)
};
