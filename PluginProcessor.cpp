/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Oscillator.hpp"


//==============================================================================
BasicOscillatorAudioProcessor::BasicOscillatorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

BasicOscillatorAudioProcessor::~BasicOscillatorAudioProcessor()
{
}

//==============================================================================
const juce::String BasicOscillatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BasicOscillatorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BasicOscillatorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BasicOscillatorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BasicOscillatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BasicOscillatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BasicOscillatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BasicOscillatorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BasicOscillatorAudioProcessor::getProgramName (int index)
{
    return {};
}

void BasicOscillatorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BasicOscillatorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();

    myOsc.prepare(spec);
}

void BasicOscillatorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BasicOscillatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BasicOscillatorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    

    auto sync = apvts.getRawParameterValue("InSync")->load();
    auto noteIndex = static_cast<int>(apvts.getRawParameterValue("NoteVal")->load());
    auto feelIndex = static_cast<int>(apvts.getRawParameterValue("Feel")->load());


    if (sync == true)
    {
        double bpm;
        auto tmp_bpm = getPlayHead()->getPosition()->getBpm();


        if (tmp_bpm.hasValue())
        {
            bpm = *tmp_bpm;
            //DBG("Got BPM");
        }
        else
        {
            //DBG("Host BPM could not be retrieved");
            bpm = 120.0;
        }

        myOsc.setBpm(bpm);

        auto rate = myOsc.setModulator(noteIndex, feelIndex, true);
        auto depth = apvts.getRawParameterValue("depth")->load();


        myOsc.setFrequency(rate);
        
        auto waveIndex = static_cast<int>(apvts.getRawParameterValue("OscShape")->load());

        if (waveIndex == SINE)
        {
            myOsc.setWaveForm(SINE);
        }
        else if (waveIndex == SQUARE)
        {
            myOsc.setWaveForm(SQUARE);
        }
        else if (waveIndex == TRIANGLE)
        {
            myOsc.setWaveForm(TRIANGLE);
        }
        else
        {
            myOsc.setWaveForm(SAWTOOTH);
        }



        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float modulation = 1.0f - depth * myOsc.processSample();
                channelData[sample] *= modulation;
            }
        }


    }
    else
    {
        auto rate = apvts.getRawParameterValue("rate")->load();
        auto depth = apvts.getRawParameterValue("depth")->load();


        myOsc.setFrequency(rate);

        auto waveIndex = static_cast<int>(apvts.getRawParameterValue("OscShape")->load());

        if (waveIndex == SINE)
        {
            myOsc.setWaveForm(SINE);
        }
        else if (waveIndex == SQUARE)
        {
            myOsc.setWaveForm(SQUARE);
        }
        else if (waveIndex == TRIANGLE)
        {
            myOsc.setWaveForm(TRIANGLE);
        }
        else
        {
            myOsc.setWaveForm(SAWTOOTH);
        }

    

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float modulation = 1.0f - depth * myOsc.processSample();
                channelData[sample] *= modulation;
            }   
        }
    }

    
  
} 

//==============================================================================
bool BasicOscillatorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BasicOscillatorAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void BasicOscillatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BasicOscillatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout BasicOscillatorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>("InSync", "InSync", true)); //In Sync with BPM


    juce::StringArray stringArray2;
    stringArray2.add("2");
    stringArray2.add("1");
    stringArray2.add("1/2");
    stringArray2.add("1/4");
    stringArray2.add("1/8");
    stringArray2.add("1/16");
    stringArray2.add("1/32");


    layout.add(std::make_unique<juce::AudioParameterChoice>("NoteVal", "NoteVal", stringArray2, 0)); //BPM

    
    juce::StringArray stringArray3;
    stringArray3.add("Straight");
    stringArray3.add("Dotted");
    stringArray3.add("Triplet");


    layout.add(std::make_unique<juce::AudioParameterChoice>("Feel", "Feel", stringArray3, 0)); //BPM
    


    juce::StringArray stringArray;
    stringArray.add("Sine");
    stringArray.add("Square");
    stringArray.add("Triangle");
    stringArray.add("SawTooth");


    layout.add(std::make_unique<juce::AudioParameterChoice>("OscShape", "OscShape", stringArray, 0)); //Shape of Wave



    layout.add(std::make_unique<juce::AudioParameterFloat>("rate", "Rate", 0.1f, 10.0f, 5.0f));  // Rate: min 0.1Hz, max 10Hz, default 5Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>("depth", "Depth", 0.0f, 1.0f, 0.5f)); // Depth: min 0.0, max 1.0, default 0.5

   

 


    


    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicOscillatorAudioProcessor();
}
