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
void BasicOscillatorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();

    myOsc.prepare(spec);
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    // myLfo.prepare(spec);

    myOsc.reset();
    //myLfo.reset();

    auto chainSettings = getChainSettings(apvts);


    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = leftChain.get<ChainPositions::HighCut>();


    leftHighCut.setBypassed<0>(true);
    leftHighCut.setBypassed<1>(true);

    
    switch (chainSettings.highCutSlope)
    {
    case Slope_12:
    {
        *leftHighCut.get<0>().coefficients = *cutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *leftHighCut.get<0>().coefficients = *cutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *cutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        break;
    }
    }

    rightHighCut.setBypassed<0>(true);
    rightHighCut.setBypassed<1>(true);

    switch (chainSettings.highCutSlope)
    {
    case Slope_12:
    {
        *rightHighCut.get<0>().coefficients = *cutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *rightHighCut.get<0>().coefficients = *cutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *cutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        break;
    }
    }
    
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

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);

    auto leftBlock = audioBlock.getSingleChannelBlock(0);
    auto rightBlock = audioBlock.getSingleChannelBlock(1);


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
    auto lowPassCut = apvts.getRawParameterValue("LowPass")->load();
    auto mod = static_cast<int>(apvts.getRawParameterValue("Modulation")->load());
    auto depth = apvts.getRawParameterValue("depth")->load();


    auto waveIndex = static_cast<int>(apvts.getRawParameterValue("OscShape")->load());

    myOsc.setWaveForm(setOscillatorWaveform(waveIndex));

    myOsc.setLowPassFreq(lowPassCut);


    if (sync == true)
    {
        //INSYNC IS ON =================================================================================
        if (mod == 0) //Volume
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


            myOsc.setFrequency(rate);



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
        else // LowPassFilter modulation
        {

            /*
            auto rate = apvts.getRawParameterValue("rate")->load();


           // myOsc.setFrequency(rate);
            myLfo.setFrequency(0.5f);


            for (int channel = 0; channel < totalNumInputChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer(channel);

                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    // Process LFO or other modulation signal here
                    float lfoSample = myLfo.processSample(); // or another modulation source

                    // Calculate modulated frequency, for example using depth to control the modulation amount
                    float baseFrequency = 440.0f;  // Default base frequency (e.g., A4)
                    float modulatedFrequency = baseFrequency + (lfoSample * lowPassCut);  // Modulate frequency

                    // Clamp modulated frequency to avoid invalid values
                    modulatedFrequency = std::max(20.0f, std::min(modulatedFrequency, 20000.0f));  // Ensure valid range

                    // Set the modulated frequency to the oscillator
                    myOsc.setFrequency(0.5f);

                    // Now process the sample with the oscillator, outputting to the channel data
                    channelData[sample] = myOsc.processSample();  // Get the oscillator's output for this sample

                    DBG("Modulated Frequency: " << modulatedFrequency);
                    DBG("lfosample: " << lfoSample);

                }
            }
            */

            juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
            juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

            leftChain.process(leftContext);
            rightChain.process(rightContext);

            auto chainSettings = getChainSettings(apvts);


            auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, getSampleRate(), 2 * (chainSettings.highCutSlope + 1));

            auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
            auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();


            leftHighCut.setBypassed<0>(true);
            leftHighCut.setBypassed<1>(true);


            switch (chainSettings.highCutSlope)
            {
            case Slope_12:
            {
                *leftHighCut.get<0>().coefficients = *cutCoefficients[0];
                leftHighCut.setBypassed<0>(false);
                break;
            }
            case Slope_24:
            {
                *leftHighCut.get<0>().coefficients = *cutCoefficients[0];
                leftHighCut.setBypassed<0>(false);
                *leftHighCut.get<1>().coefficients = *cutCoefficients[1];
                leftHighCut.setBypassed<1>(false);
                break;
            }
            }

            rightHighCut.setBypassed<0>(true);
            rightHighCut.setBypassed<1>(true);

            switch (chainSettings.highCutSlope)
            {
            case Slope_12:
            {
                *rightHighCut.get<0>().coefficients = *cutCoefficients[0];
                rightHighCut.setBypassed<0>(false);
                break;
            }
            case Slope_24:
            {
                *rightHighCut.get<0>().coefficients = *cutCoefficients[0];
                rightHighCut.setBypassed<0>(false);
                *rightHighCut.get<1>().coefficients = *cutCoefficients[1];
                rightHighCut.setBypassed<1>(false);
                break;
            }
            }



        }
    }
    else
    {
        //INSYNC IS OFF =================================================================================
        if (mod == 0) //Volume
        {
            auto rate = apvts.getRawParameterValue("rate")->load();


            myOsc.setFrequency(rate);



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
        else // LowPass Filter modulation
        {
           
        }
    }

    /*
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPassFilter.process(context);
  */
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

OscWaveforms BasicOscillatorAudioProcessor::setOscillatorWaveform(int waveIndex)
{
    if (waveIndex == SINE)
    {
        return SINE;
    }
    else if (waveIndex == SQUARE)
    {
        return SQUARE;
    }
    else if (waveIndex == TRIANGLE)
    {
        return TRIANGLE;
    }
    else
    {
        return SAWTOOTH;
    }
}


ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.highCutFreq = static_cast<Slope>(apvts.getRawParameterValue("LowPass")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowPass Slope")->load());

    return settings;
}


juce::AudioProcessorValueTreeState::ParameterLayout BasicOscillatorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>("InSync", "InSync", true)); //In Sync with BPM


    juce::StringArray stringArray4;
    stringArray4.add("Volume"); //Index 0
    stringArray4.add("LowPass Filter"); //Index 1


    layout.add(std::make_unique<juce::AudioParameterChoice>("Modulation", "Modulation", stringArray4, 0)); //Choice



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

   

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowPass", 1), "LowPass", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));


    juce::StringArray stringArray5;
    for (int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray5.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowPass Slope", "LowPass Slope", stringArray5, 0)); //Shape of Wave


    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicOscillatorAudioProcessor();
}
