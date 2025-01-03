/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Oscillator.hpp"


enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};


struct ChainSettings
{
    float highCutFreq{ 0 };
    int highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class BasicOscillatorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    BasicOscillatorAudioProcessor();
    ~BasicOscillatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    OscWaveforms BasicOscillatorAudioProcessor::setOscillatorWaveform(int waveIndex);

private:
   // float rate = 0.5f; //Modulation rate in Hz
   // float depth = 0.5f; //Modulation depth (0.0 to 1.0)

   OscillatorProcessor myOsc;

   OscillatorProcessor myLfo;

   using Filter = juce::dsp::IIR::Filter<float>;

   using CutFilter = juce::dsp::ProcessorChain<Filter, Filter>;

   using MonoChain = juce::dsp::ProcessorChain<CutFilter>;

   MonoChain leftChain, rightChain;

   enum ChainPositions
   {
       HighCut
   };

    // return std::sin (x); //Sine Wave
    // return x / MathConstants<float>::pi // Saw Wave
    // return x < 0.0f ? -1.0f : 1.0f; // Square Wave
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicOscillatorAudioProcessor)
};
