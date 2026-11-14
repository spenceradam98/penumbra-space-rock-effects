#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static String formatValue (const pn::ParamDesc& d, float v)
{
    const String u (d.unit);
    if (u == "%") return String (roundToInt (v * 100.0f)) + " %";
    if (u == "Hz") return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : (v < 10.0f ? String (v, 2) : String (roundToInt (v))) + " Hz";
    if (u == "ms") return String (roundToInt (v)) + " ms";
    if (u == "s") return String (v, v < 10.0f ? 1 : 0) + " s";
    if (u == "dB") return (v > 0.05f ? "+" : "") + String (v, 1) + " dB";
    return String (v, 2);
}

AudioProcessorValueTreeState::ParameterLayout PenumbraProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> out;
    int n = 0; auto* t = pn::paramTable (n);
    const pn::Params def;
    for (int i = 0; i < n; ++i)
    {
        const auto& d = t[i];
        const ParameterID id { d.id, 1 };
        const float dv = d.get (def);
        switch (d.kind)
        {
            case pn::Kind::Float:
            {
                NormalisableRange<float> range (d.lo, d.hi);
                if (d.centre > d.lo && d.centre < d.hi && std::abs (d.centre - 0.5f * (d.lo + d.hi)) > 1e-3f)
                    range.setSkewForCentre (d.centre);
                auto attrs = AudioParameterFloatAttributes()
                                 .withStringFromValueFunction ([&d] (float v, int) { return formatValue (d, v); })
                                 .withLabel (d.unit);
                out.push_back (std::make_unique<AudioParameterFloat> (id, d.name, range, dv, attrs));
                break;
            }
            case pn::Kind::Choice:
                out.push_back (std::make_unique<AudioParameterChoice> (id, d.name, StringArray::fromTokens (d.choices, "|", ""), (int) dv));
                break;
            case pn::Kind::Toggle:
                out.push_back (std::make_unique<AudioParameterBool> (id, d.name, dv > 0.5f));
                break;
        }
    }
    return { out.begin(), out.end() };
}

PenumbraProcessor::PenumbraProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Penumbra", createLayout())
{
    int n = 0; auto* t = pn::paramTable (n);
    for (int i = 0; i < n; ++i) raw.push_back (apvts.getRawParameterValue (t[i].id));
}

bool PenumbraProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet(), in = layouts.getMainInputChannelSet();
    const bool okOut = out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
    const bool okIn = in == AudioChannelSet::mono() || in == AudioChannelSet::stereo();
    return okOut && okIn;
}

pn::Params PenumbraProcessor::readParams() const
{
    int n = 0; auto* t = pn::paramTable (n);
    pn::Params p;
    for (int i = 0; i < n; ++i) t[i].set (p, raw[(size_t) i]->load());
    return p;
}

void PenumbraProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.setParams (readParams());
    engine.prepare (sampleRate, samplesPerBlock);
    scratchL.assign ((size_t) jmax (1, samplesPerBlock), 0.0f);
    scratchR.assign ((size_t) jmax (1, samplesPerBlock), 0.0f);
}

void PenumbraProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    if ((int) scratchL.size() < n) { scratchL.resize ((size_t) n); scratchR.resize ((size_t) n); }   // hosts may exceed the announced size

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm()) engine.setTempo (*bpm);

    const float* inL = inCh > 0 ? buffer.getReadPointer (0) : nullptr;
    const float* inR = inCh > 1 ? buffer.getReadPointer (1) : inL;
    for (int i = 0; i < n; ++i)
    {
        scratchL[(size_t) i] = inL ? inL[i] : 0.0f;
        scratchR[(size_t) i] = inR ? inR[i] : 0.0f;
    }

    engine.setParams (readParams());
    engine.process (scratchL.data(), scratchR.data(), n);

    if (outCh == 1)
    {
        auto* o = buffer.getWritePointer (0);
        for (int i = 0; i < n; ++i) o[i] = 0.5f * (scratchL[(size_t) i] + scratchR[(size_t) i]);
    }
    else
    {
        buffer.copyFrom (0, 0, scratchL.data(), n);
        buffer.copyFrom (1, 0, scratchR.data(), n);
        for (int c = 2; c < outCh; ++c) buffer.clear (c, 0, n);
    }
}

int PenumbraProcessor::getNumPrograms() { int n = 0; pn::factoryPresets (n); return n; }

const String PenumbraProcessor::getProgramName (int index)
{
    int n = 0; auto* ps = pn::factoryPresets (n);
    return isPositiveAndBelow (index, n) ? String (ps[index].name) : String();
}

void PenumbraProcessor::setCurrentProgram (int index)
{
    int n = 0; auto* ps = pn::factoryPresets (n);
    if (! isPositiveAndBelow (index, n)) return;
    currentProgram = index;
    int np = 0; auto* t = pn::paramTable (np);
    for (int i = 0; i < np; ++i)
        if (auto* prm = apvts.getParameter (t[i].id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (t[i].get (ps[index].p)));
}

void PenumbraProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("theme", theme, nullptr);
    state.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PenumbraProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            theme = tree.getProperty ("theme", 0);
            uiScale = (float) (double) tree.getProperty ("uiScale", 1.0);
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* PenumbraProcessor::createEditor() { return new PenumbraEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PenumbraProcessor(); }
