#pragma once
#include <JuceHeader.h>
#include "TapeMachine.h"

class TapermateAudioProcessor : public juce::AudioProcessor
{
public:
    TapermateAudioProcessor();
    ~TapermateAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TAPERMATE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    tapermate_dsp::TapeMachine machine;                       // publico: la interfaz lee los VU, el motor y el contador
    std::atomic<int> transport { tapermate_dsp::Stop };       // STOP / PLAY / REC / REV / F.FWD / RWD
    std::atomic<bool> recArmed { false };          // con SYNC: REC "arma" y la grabacion empieza con el transporte del host

    void requestTransport (int t);
    void rewindTape() { machine.rewind(); }
    void clearTape() { machine.clearTape(); }

private:
    tapermate_dsp::Params readParams() const;
    bool lastHostPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapermateAudioProcessor)
};
