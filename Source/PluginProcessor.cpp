#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout TapermateAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    auto addF = [&] (const char* id, const char* name, float lo, float hi, float step, float skew, float def)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, step, skew), def));
    };

    l.add (std::make_unique<AudioParameterBool> (ParameterID { "power", 1 }, "Power", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "speed", 1 }, "Speed", StringArray { "7.5 ips", "15 ips" }, 1));
    addF ("vari",    "Vari",     -50.f, 50.f, 0.1f, 1.f, 0.f);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "pitch", 1 }, "Pitch", -12, 12, 0));
    addF ("lpitch",  "L.Pitch",  -100.f, 100.f, 0.1f, 1.f, 0.f);
    addF ("lfpitch", "LF Pitch", 0.f, 100.f, 0.1f, 1.f, 0.f);
    addF ("hiss",    "Hiss",     0.f, 100.f, 0.1f, 1.f, 0.f);
    addF ("noise",   "Noise",    0.f, 100.f, 0.1f, 1.f, 0.f);
    addF ("satmix",  "Sat Mix",  0.f, 100.f, 0.1f, 1.f, 50.f);
    addF ("delay",   "Delay",    20.f, 1200.f, 1.f, 0.5f, 300.f);
    addF ("dlyfb",   "Delay FB", 0.f, 95.f, 0.1f, 1.f, 35.f);
    addF ("dlymix",  "Delay Mix", 0.f, 100.f, 0.1f, 1.f, 0.f);
    addF ("input",   "Input",    -24.f, 24.f, 0.1f, 1.f, 0.f);
    addF ("output",  "Output",   -24.f, 12.f, 0.1f, 1.f, 0.f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "metersrc", 1 }, "Meter In/Out", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "loop", 1 }, "Loop", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "tilt", 1 }, "Tilt", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "Sync", false));
    // extras de la v1.0
    addF ("reverb",  "Reverb",   0.f, 100.f, 0.1f, 1.f, 0.f);
    addF ("locut",   "Low Cut",  10.f, 500.f, 1.f, 0.4f, 10.f);
    addF ("midcut",  "Mid Cut",  -24.f, 0.f, 0.1f, 1.f, 0.f);
    addF ("hicut",   "High Cut", 1000.f, 20000.f, 1.f, 0.4f, 20000.f);
    return l;
}

TapermateAudioProcessor::TapermateAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool TapermateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void TapermateAudioProcessor::prepareToPlay (double sampleRate, int)
{
    machine.prepare (sampleRate);       // reserva los 60 s de cinta (~23 MB) una sola vez
    transport.store (tapermate_dsp::Stop);
    recArmed.store (false);
    lastHostPlaying = false;
}

tapermate_dsp::Params TapermateAudioProcessor::readParams() const
{
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    tapermate_dsp::Params p;
    p.power = P ("power") > 0.5f;
    p.speed = (int) std::round (P ("speed"));
    p.vari = P ("vari") * 0.01f;
    p.pitchSemis = P ("pitch");
    p.fineCents = P ("lpitch");
    p.lfPitch = P ("lfpitch") * 0.01f;
    p.hiss = P ("hiss") * 0.01f;
    p.noise = P ("noise") * 0.01f;
    p.satMix = P ("satmix") * 0.01f;
    p.delayMs = P ("delay");
    p.dlyFb = P ("dlyfb") * 0.01f;
    p.dlyMix = P ("dlymix") * 0.01f;
    p.inDb = P ("input");
    p.outDb = P ("output");
    p.loop = P ("loop") > 0.5f;
    p.tilt = P ("tilt") > 0.5f;
    p.reverb = P ("reverb") * 0.01f;
    p.locut = P ("locut");
    p.midDb = P ("midcut");
    p.hicut = P ("hicut");
    return p;
}

void TapermateAudioProcessor::requestTransport (int t)
{
    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    if (t == tapermate_dsp::Rec && sync) { recArmed.store (! recArmed.load()); return; }     // con SYNC, REC solo arma
    if (t == tapermate_dsp::Stop) recArmed.store (false);
    transport.store (t);
}

// Hilo de audio: sin asignaciones de memoria (toda la cinta se reserva en prepareToPlay).
void TapermateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    // SYNC: la maquina sigue el transporte del host (empieza desde el principio de la cinta)
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) hostPlaying = pos->getIsPlaying();
    if (apvts.getRawParameterValue ("sync")->load() > 0.5f)
    {
        if (hostPlaying && ! lastHostPlaying)
        {
            if (recArmed.load()) machine.clearTape();
            machine.rewind();
            transport.store (recArmed.load() ? tapermate_dsp::Rec : tapermate_dsp::Play);
        }
        else if (! hostPlaying && lastHostPlaying) { transport.store (tapermate_dsp::Stop); recArmed.store (false); }
    }
    lastHostPlaying = hostPlaying;

    machine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), n, readParams(), transport);
}

juce::AudioProcessorEditor* TapermateAudioProcessor::createEditor()
{
    return new TapermateAudioProcessorEditor (*this);
}

void TapermateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void TapermateAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapermateAudioProcessor();
}
