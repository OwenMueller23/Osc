#ifndef Oscillator_hpp
#define Oscillator_hpp

#include "ProcessorBase.hpp"

juce::StringArray OscWaveformNames =
{
	"Sine",
	"Square",
	"Triangle",
	"Sawtooth",
};

enum OscWaveforms
{
	SINE,
	SQUARE,
	TRIANGLE,
	SAWTOOTH
};

enum modTimeIndex
{
	Double = 0,
	Whole = 1,
	Half = 2,
	Quarter = 3,
	Eighth = 4,
	Sixteenth = 5,
	ThirtySecond = 6,
};

enum modAdjIndex
{
	Straight = 0,
	Dotted = 1,
	Triplet = 2,
};

class OscillatorProcessor  : public ProcessorBase
{
public:
     const juce::String getName() const override { return "Oscillator"; }
 
    OscillatorProcessor() {
      oscillator.setFrequency (440.0f);
      oscillator.initialise ([] (float x) { return std::sin (x); });
    }

	void setWaveForm(int wave)
	{
		switch (wave) {
			default:
			case SINE:
				oscillator.initialise ([] (float x) { return std::sin (x); });	
				break;
			case SQUARE:	
				oscillator.initialise ([] (float x) { return x < 0.f ? -1.f : 1.f; });	
				break;
			case TRIANGLE:
				oscillator.initialise ([] (float x) { return (abs(2*x) / juce::MathConstants<float>::pi) - 1.f; });	
				break;
			case SAWTOOTH:
				oscillator.initialise ([] (float x) { return (x / juce::MathConstants<float>::pi); });	
				break;


		}
	}
	
	// TODO: implement
	void addParams(juce::AudioProcessorParameterGroup&) override
	{
		;
	}
	
	bool update(juce::AudioProcessorValueTreeState&) override
	{
		return false;
	}
	
	void prepare (const juce::dsp::ProcessSpec& spec) override
	{
		oscillator.prepare (spec);
	}
    
	void setFrequency(float frequency)
	{
		oscillator.setFrequency(frequency);
	}
	
	void process (juce::dsp::ProcessContextReplacing<float>& context) override 
	{
        oscillator.process (context);
    }
	
	float processSample () 
	{
		return oscillator.processSample(0.f);
	}

    void reset() override {
       oscillator.reset();
    }

	void setBpm(double tempo)
	{
		bpm = tempo;
	}

	
	float setModulator(/*int func, int wave,*/ int noteVal, int adj, bool sync)
	 {
		float timing;
		switch (adj)
		{
			default:

			case Straight: 
				timing = 1.f; break;

			case Dotted: 
				timing = 1.5f; break;

			case Triplet: 
				timing = 0.33333333f; break;
		}

		float mpc; //MILISECONDS PER CYCLE
		//		if (sync) {
		switch (noteVal) {
			default:

			case Double: 
				mpc = (60000.f / bpm) * 16 * timing; break;

			case Whole: 
				mpc = (60000.f / bpm) * 8 * timing; break;

			case Half: 
				mpc = (60000.f / bpm) * 4 * timing; break;

			case Quarter: 
				mpc = (60000.f / bpm) * 2 * timing; break;

			case Eighth: 
				mpc = (60000.f / bpm) * timing; break;

			case Sixteenth:
				mpc = (60000.f / bpm) * 0.5f * timing; break;

			case ThirtySecond: 
				mpc = (60000.f / bpm) * 0.25f * timing; break;
		}
		return 1. / (mpc * 0.001f); // return a frequency
		//		}
		//		else { // TODO: free floating 
		//			switch (noteVal) {
		//				default:
		//				case mQuadruple: mpc = (60000.f / bpm) * 16 * timing; break;
		//				case mDouble: mpc = (60000.f / bpm) * 8 * timing; break;
		//				case mWhole: mpc = (60000.f / bpm) * 4 * timing; break;
		//				case mHalf: mpc = (60000.f / bpm) * 2 * timing; break;
		//				case mQuarter: mpc = (60000.f / bpm) * timing; break;
		//				case mEighth: mpc = (60000.f / bpm) * 0.5f * timing; break;
		//				case mSixteenth: mpc = (60000.f / bpm) * 0.25f * timing; break;
		//			}
		//			return 1./(mpc * 0.001f); // return a frequency
		//		}
	}
	

private:
    juce::dsp::Oscillator<float> oscillator;

	float bpm = 120.0;
};
//	float triangleOscillator(float triangleVal, float sampleRate, float freq)
//	{
//		if (triangleVal < 1.f && triangleSlope > 0.f)
//		{
//			triangleSlope = 2.f / (sampleRate * freq);
//		}
//		else
//		{
//			if (triangleSlope > 0.f) {
//				triangleSlope = -2.f / (sampleRate * freq);
//			}
//		}
//
//		if (triangleVal > -1.f && triangleSlope < 0.f)
//		{
//			triangleSlope = -2.f / (sampleRate * freq);
//		}
//		else
//		{
//			if (triangleSlope < 0.f) {
//				triangleSlope = 2.f / (sampleRate * freq);
//			}
//		}
//
//		triangleVal += triangleSlope;
//		return triangleVal;
//	}
//

#endif
