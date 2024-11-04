#ifndef ProcessorBase_hpp
#define ProcessorBase_hpp

namespace MODID
{
	#define PARAMETER_ID(str) constexpr const char* str { #str };
	PARAMETER_ID (GainBypass)
	PARAMETER_ID (Gain)
	PARAMETER_ID (GainPhase)
}

//==============================================================================

static juce::String valueToTextFunction (float x) { return juce::String (x, 1); }
static float textToValueFunction (const juce::String& str) { return str.getFloatValue(); }

static juce::String shortStringFromValue (float value) { return juce::String (value, 2); }	

static juce::String stringFromValueHz (float value) {
	if (value < 10.0f)
		return juce::String (value, 2) + " Hz";
	else if (value < 1000.0f)
		return juce::String (value, 1) + " Hz";
	else
		return juce::String (value/1000.f, 1) + " kHz";
}

static juce::String stringFromValueDB (float value) {
	if (value < 10.0f)
		return juce::String (value, 2) + " dB";
	else if (value < 100.0f)
		return juce::String (value, 1) + " dB";
	else
		return juce::String (value, 0) + " dB";
}

static juce::String stringFromValueMS (float value) {
	if (value < 10.0f)
		return juce::String (value, 2) + " ms";
	else if (value < 100.0f)
		return juce::String (value, 1) + " ms";
	else
		return juce::String (value, 0) + " ms";
}

static juce::String stringFromValueRatio (float value, int) {
	if (value < 10.0f)
		return "1 : " + juce::String (value, 1);
	else
		return "1 : " + juce::String (value, 0);
}

//==============================================================================


class ProcessorBase
{
public:
    //==============================================================================
    ProcessorBase() {}
	
	virtual ~ProcessorBase() {}

	virtual void prepare (const juce::dsp::ProcessSpec&) = 0;
	
	virtual void reset() = 0;
	
	virtual const juce::String getName() const = 0;
	
	virtual void addParams(juce::AudioProcessorParameterGroup&) = 0;

	virtual bool update(juce::AudioProcessorValueTreeState&) = 0;
	
	virtual void process (juce::dsp::ProcessContextReplacing<float>&) = 0;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorBase)
};


//==============================================================================
class GainProcessor : public ProcessorBase
{
public:
	GainProcessor()
	{
		gain.setGainDecibels (-6.0f);
	}
	
	virtual ~GainProcessor() { }
	
	const juce::String getName() const override { return "Gain"; }

	void addParams(juce::AudioProcessorParameterGroup& params) override
	{
		params.addChild(std::make_unique<juce::AudioParameterBool> (juce::ParameterID(MODID::GainBypass, 1), "Gain Bypass", false));
		params.addChild(std::make_unique<juce::AudioParameterBool> (juce::ParameterID(MODID::GainPhase, 1), "Phase", false));
		params.addChild(std::make_unique<juce::AudioProcessorValueTreeState::Parameter> (juce::ParameterID(MODID::Gain, 1), "Gain", " dB", juce::NormalisableRange<float> (-24.0, 24.0), 0.0, valueToTextFunction, textToValueFunction));
	}
	
	bool update(juce::AudioProcessorValueTreeState& parameters) override
	{
		bool byp = (bool)parameters.getRawParameterValue(MODID::GainBypass)->load();
		float g = juce::Decibels::decibelsToGain(parameters.getRawParameterValue(MODID::Gain)->load());
		int phase = parameters.getRawParameterValue(MODID::GainPhase)->load();
		if (phase)
			g *= -1.f;
		setGain(byp? 1.f : g);
		
		return byp;
	}

	void prepare (const juce::dsp::ProcessSpec& spec) override
	{
		gain.prepare (spec);
	}
	
	void setdBGain(float g) 
	{
		gain.setGainDecibels(g);
	}
	
	void setGain(float g) 
	{
		gain.setGainLinear(g);
	}

	void process (juce::dsp::ProcessContextReplacing<float>& context) override
	{		
		gain.process (context);
	}

	void reset() override
	{
		gain.reset();
	}


private:
	juce::dsp::Gain<float> gain;
};

#endif // ProcessorBase.hpp
