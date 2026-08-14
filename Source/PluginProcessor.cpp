#include "PluginProcessor.h"
#include "PluginEditor.h"

Sa16SpectrumProcessor::Sa16SpectrumProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int i = 0; i < 6; ++i)
        channelStates[(size_t) i].on = true;

    for (int i = 0; i < Sa16::kMaxChannels; ++i)
        engine.setChannelEnabled (i, channelStates[(size_t) i].on);
}

Sa16SpectrumProcessor::~Sa16SpectrumProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout Sa16SpectrumProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> ("bypass", "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("freeze", "Freeze", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("linear", "Linear Freq", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "fft_size", "FFT Size", Sa16::fftSizeChoiceLabels(), Sa16::kDefaultFftSizeIndex));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "window", "Window",
        juce::StringArray { "Hann", "Hamming", "Blackman", "Flat-Top", "Kaiser" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "smoothing", "Smoothing", juce::NormalisableRange<float> (0.0f, 1.0f), 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reactivity", "Reactivity", juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "channel_gain", "Channel Gain", juce::NormalisableRange<float> (-100.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "preamp", "Preamp", juce::NormalisableRange<float> (-20.0f, 20.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "zoom", "Zoom", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    return { params.begin(), params.end() };
}

const juce::String Sa16SpectrumProcessor::getName() const { return JucePlugin_Name; }
bool Sa16SpectrumProcessor::acceptsMidi() const { return false; }
bool Sa16SpectrumProcessor::producesMidi() const { return false; }
bool Sa16SpectrumProcessor::isMidiEffect() const { return false; }
double Sa16SpectrumProcessor::getTailLengthSeconds() const { return 0.0; }

int Sa16SpectrumProcessor::getNumPrograms() { return 1; }
int Sa16SpectrumProcessor::getCurrentProgram() { return 0; }
void Sa16SpectrumProcessor::setCurrentProgram (int) {}
const juce::String Sa16SpectrumProcessor::getProgramName (int) { return {}; }
void Sa16SpectrumProcessor::changeProgramName (int, const juce::String&) {}

void Sa16SpectrumProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate, std::memory_order_relaxed);
    if (auto* p = apvts.getRawParameterValue ("fft_size"))
        engine.setFftSize (Sa16::kFftSizes[juce::jlimit (0, Sa16::kNumFftSizes - 1, (int) p->load())]);
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void Sa16SpectrumProcessor::releaseResources() {}

bool Sa16SpectrumProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;
    if (in != out)
        return false;

    const int n = in.size();
    return n >= 1 && n <= Sa16::kMaxChannels;
}

void Sa16SpectrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Sync engine params from APVTS
    if (auto* p = apvts.getRawParameterValue ("fft_size"))
        engine.setFftSize (Sa16::kFftSizes[juce::jlimit (0, Sa16::kNumFftSizes - 1, (int) p->load())]);
    if (auto* p = apvts.getRawParameterValue ("window"))
        engine.setWindowType (static_cast<Sa16::WindowType> ((int) p->load()));
    if (auto* p = apvts.getRawParameterValue ("smoothing"))
        engine.setSmoothing (p->load());
    if (auto* p = apvts.getRawParameterValue ("reactivity"))
        engine.setReactivity (p->load());
    if (auto* p = apvts.getRawParameterValue ("channel_gain"))
        engine.setChannelGainDb (p->load());
    if (auto* p = apvts.getRawParameterValue ("preamp"))
        engine.setPreampDb (p->load());
    if (auto* p = apvts.getRawParameterValue ("freeze"))
        engine.setFrozen (p->load() > 0.5f);
    if (auto* p = apvts.getRawParameterValue ("bypass"))
        bypassed.store (p->load() > 0.5f, std::memory_order_relaxed);

    for (int i = 0; i < Sa16::kMaxChannels; ++i)
    {
        engine.setChannelEnabled (i, channelStates[(size_t) i].on);
        engine.setChannelHold (i, channelStates[(size_t) i].hold);
    }

    if (! bypassed.load (std::memory_order_relaxed))
        engine.process (buffer);
}

bool Sa16SpectrumProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* Sa16SpectrumProcessor::createEditor()
{
    return new Sa16SpectrumEditor (*this);
}

void Sa16SpectrumProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("viewMode", viewMode.load (std::memory_order_relaxed));

    for (int i = 0; i < Sa16::kMaxChannels; ++i)
    {
        auto* ch = xml->createNewChildElement ("CH");
        ch->setAttribute ("i", i);
        ch->setAttribute ("on", channelStates[(size_t) i].on ? 1 : 0);
        ch->setAttribute ("solo", channelStates[(size_t) i].solo ? 1 : 0);
        ch->setAttribute ("hold", channelStates[(size_t) i].hold ? 1 : 0);
        ch->setAttribute ("mids", channelStates[(size_t) i].mids ? 1 : 0);
    }

    copyXmlToBinary (*xml, destData);
}

void Sa16SpectrumProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

        viewMode.store (xml->getIntAttribute ("viewMode", 0), std::memory_order_relaxed);

        for (auto* ch : xml->getChildWithTagNameIterator ("CH"))
        {
            const int i = ch->getIntAttribute ("i", -1);
            if (! juce::isPositiveAndBelow (i, Sa16::kMaxChannels))
                continue;
            channelStates[(size_t) i].on = ch->getIntAttribute ("on", 0) != 0;
            channelStates[(size_t) i].solo = ch->getIntAttribute ("solo", 0) != 0;
            channelStates[(size_t) i].hold = ch->getIntAttribute ("hold", 0) != 0;
            channelStates[(size_t) i].mids = ch->getIntAttribute ("mids", 0) != 0;
        }
    }
}

void Sa16SpectrumProcessor::setViewMode (Sa16::ViewMode mode)
{
    viewMode.store ((int) mode, std::memory_order_relaxed);
}

void Sa16SpectrumProcessor::setBypass (bool bypass)
{
    if (auto* p = apvts.getParameter ("bypass"))
        p->setValueNotifyingHost (bypass ? 1.0f : 0.0f);
    bypassed.store (bypass, std::memory_order_relaxed);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Sa16SpectrumProcessor();
}
