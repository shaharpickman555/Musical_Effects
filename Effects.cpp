/**************************************
 * Made by Shahar Pickman and Or Dadosh
 * Guided by Lior Arbel
***************************************/

#include "Effects.h"

# define DOWN (0)
# define UP (1)

/*************************constants for reverb***************************/
// These values considered example for a great medium concert hall Reverb
const float cf1_delay_ms = 29.7;
const float cf2_delay_ms = 37.1;
const float cf3_delay_ms = 41.1;
const float cf4_delay_ms = 43.7;
const float apf1_delay_ms = 5;
const float apf2_delay_ms = 1.7;
const float apf1_reverb_time_ms = 96.83;
const float apf2_reverb_time_ms = 32.92;

// Allpass filters have constant gain
const float apf1_gain = pow(0.001, apf1_delay_ms/apf1_reverb_time_ms);
const float apf2_gain = pow(0.001, apf2_delay_ms/apf2_reverb_time_ms);
/************************************************************************/

IIRFilter::IIRFilter(unsigned int input_elements_num, FilterElement* input_elements_vals, unsigned int output_elements_num, FilterElement* output_elements_vals) :
		input_elements_num(input_elements_num), input_elements(new FilterElement[input_elements_num]),
		output_elements_num(output_elements_num), output_elements(new FilterElement[output_elements_num])
{
	for (int i = 0; i < input_elements_num ; i++) {
		input_elements[i] = input_elements_vals[i];
	}
	for (int i = 0; i < output_elements_num ; i++) {
		output_elements[i] = output_elements_vals[i];
	}
}

IIRFilter::~IIRFilter()
{
	delete [] input_elements;
	delete [] output_elements;
}

float
IIRFilter::process(const RingBuffer<float>& inputs_buffer, const RingBuffer<float>& outputs_buffer) const
{
	float result = 0;
	
	for (int i = 0; i < input_elements_num ; i++) {
		result += input_elements[i].coefficient * inputs_buffer.read(input_elements[i].delay);
	}
	
	for (int i = 0; i < output_elements_num ; i++) {
		result -= output_elements[i].coefficient * outputs_buffer.read(output_elements[i].delay);
	}
	
	return result;
}

Effects::Effects(BelaContext *context) : sample_rate(context->audioSampleRate),
										 audio_frames_per_analog_frame(context->audioFrames/context->analogFrames)
{}

Distortion::Distortion(BelaContext *context, GuiController* controller) : Effects(context)
{
	// Arguments: name, default value, minimum, maximum, increment
	volume_slider_index = controller->addSlider("Volume", 1, 0.025, 1, 0.05);
	gain_slider_index = controller->addSlider("Gain", 1, 1, 50, 1);
	type_slider_index = controller->addSlider("Distortion/Overdrive", 0, 0, 1, 1);
}

// Called by the Distortion class process/process_hardware wrappers, and do the actual processing.
float static
__distortion_process(float in, float volume, float gain, bool is_overdrive)
{
	// Boost the amplitude
    in *= gain;
    
    if (!is_overdrive) {
    	// Hard - Clipping the samples higher than 1 or lower than -1
	    if(in > 1)
			in = 1;
		else if (in < -1)
			in = -1;
    }
    
    else {
		// soft clipping	
		int sign = in > 0 ? 1 : -1;
		in = sign*(1 - exp(-fabs(in)));
    }
	
	// Normalizing the clipped signal
	in *= volume;
	
	return in;
}

float
Distortion::process(float in, GuiController* controller)
{
	float volume = controller->getSliderValue(volume_slider_index);
    float gain = controller->getSliderValue(gain_slider_index);
	bool is_overdrive = controller->getSliderValue(type_slider_index);
    
	return __distortion_process(in, volume, gain, is_overdrive);
}

float
Distortion::process_hardware(float in, unsigned int index, BelaContext* context)
{
	float volume = 1;
	float gain = 1;
	if(!(index % audio_frames_per_analog_frame)) {
			// read analog inputs and update volume and gain
			volume = analogRead(context, index/2, 0);
			volume = map(volume, 0, 0.85, 0.1, 1);
			gain = analogRead(context, index/2, 1);
			gain = map(gain, 0, 0.85, 1, 50);
	}
	
	return __distortion_process(in, volume, gain, 0);
}


WahWah::WahWah(BelaContext *context, GuiController* controller) : Effects(context), fc(1000), direction(DOWN)
{
	double Fs = context->audioSampleRate;
	
	// Initialize bandpass filter
	Biquad::Settings settings{
			.fs =Fs,
			.cutoff = fc,
			.type = Biquad::bandpass,
			.q = 1,
			.peakGainDb = 0,
			};
	bpFilter.setup(settings);
	
	q_slider_index = controller->addSlider("Q", 2.5, 0.1, 10, 0.1);
	movement_rate_slider_index = controller->addSlider("Movement Rate", 2000, 1000, 10000, 500);
	minf_slider_index = controller->addSlider("Min Freq", 500, 100, 10000, 100);
	maxf_slider_index = controller->addSlider("Max Freq ", 5000, 1000, 10000, 100);
	dry_wet_slider_index = controller->addSlider("Dry/Wet ", 0, 0, 1, 0.05);
}

float
WahWah::process(float in, GuiController* controller)
{
	float out = 0.0;
 	double fc_wave = 0.0;
	double delta = controller->getSliderValue(movement_rate_slider_index)/sample_rate;
	double minf = controller->getSliderValue(minf_slider_index);
	double maxf = controller->getSliderValue(maxf_slider_index);
	
	if (direction == DOWN) {
		fc_wave = fc;
		fc = fc - delta;
		if (fc < minf) {
			fc = minf;
			direction = UP;
		}
	}
	
	else if (direction == UP) {
		fc_wave = fc;
		fc = fc + delta;
		if (fc > maxf) {
			fc = maxf;
			direction = DOWN;
		}
	}
	
    double q = controller->getSliderValue(q_slider_index);
	bpFilter.setQ(q);
	bpFilter.setFc(fc_wave);
	out = bpFilter.process(in);
	
	// Dry/Wet Mix
    float mix_percent = controller->getSliderValue(dry_wet_slider_index);
    out = ((1 - mix_percent) * in) + (mix_percent * out * 10);	// normalizing factor can be changed later
	
	return out;
}

Reverb::Reverb(BelaContext *context, GuiController* controller) : Effects(context),
		cf1(sample_rate), cf2(sample_rate), cf3(sample_rate), cf4(sample_rate),
		apf1_in(sample_rate), apf1_out(sample_rate), apf2_out(sample_rate),
		cf1_delay((int)( cf1_delay_ms * (sample_rate/1000))), cf2_delay((int)( cf2_delay_ms * (sample_rate/1000))),
		cf3_delay((int)( cf3_delay_ms * (sample_rate/1000))), cf4_delay((int)( cf4_delay_ms * (sample_rate/1000))),
		apf1_delay((int)( apf1_delay_ms * (sample_rate/1000))), apf2_delay((int)( apf2_delay_ms * (sample_rate/1000)))
{
	reverb_time_slider_index = controller->addSlider("Reverb Time (ms)", 1000, 0.1, 3000, 100);
	mix_slider_index = controller->addSlider("Mix Percentage", 0.0, 0.0, 1.0, 0.05);
}

float
Reverb::process(float in, GuiController* controller)
{
	float reverb_time = controller->getSliderValue(reverb_time_slider_index);
	float mix_percent = controller->getSliderValue(mix_slider_index);
	
	// g = 0.001 power of delay_time over reverb_time (-60db decrease, time to completely decay) 
	float cf1_gain = pow(0.001,cf1_delay_ms/reverb_time);
	float cf2_gain = pow(0.001,cf2_delay_ms/reverb_time);
	float cf3_gain = pow(0.001,cf3_delay_ms/reverb_time);
	float cf4_gain = pow(0.001,cf4_delay_ms/reverb_time);
    	
    float cf1_out_delay_sample = cf1.read(cf1_delay);			//y[n-D] for each CF
    float cf2_out_delay_sample = cf2.read(cf2_delay); 
    float cf3_out_delay_sample = cf3.read(cf3_delay); 
    float cf4_out_delay_sample = cf4.read(cf4_delay); 
    
    float apf1_input_delay_sample = apf1_in.read(apf1_delay);	//x[n-D] for APF1
    float apf1_output_delay_sample = apf1_out.read(apf1_delay); //y[n-D] for APF1
    
    float apf2_in_delay_sample = apf1_out.read(apf2_delay);		//x[n-D] for APF2
    float apf2_out_delay_sample = apf2_out.read(apf2_delay);	//y[n-D] for APF2

    cf1.write(in + cf1_out_delay_sample * cf1_gain);
    cf2.write(in + cf2_out_delay_sample * cf2_gain);
    cf3.write(in + cf3_out_delay_sample * cf3_gain);
    cf4.write(in + cf4_out_delay_sample * cf4_gain);
    
    // x[n] for APF1 + normalizing
    float apf1_in_curr_sample = (cf1.read() + cf2.read() + cf3.read() + cf4.read()) * 0.25;
    
    float apf1_in_mixed = (1 - mix_percent) * in + (mix_percent * apf1_in_curr_sample);
    apf1_in.write(apf1_in_mixed);

    // y[n] = x[n-D] - g * x[n] + g * y[n-D] (APF1)
    apf1_out.write(apf1_input_delay_sample - apf1_gain * apf1_in_mixed + apf1_gain * apf1_output_delay_sample);
    float apf2_in_curr_sample = apf1_out.read();				//x[n] for APF2
    
    // y[n] = x[n-D] - g * x[n] + g * y[n-D] (APF2)
    apf2_out.write(apf2_in_delay_sample - apf2_gain * apf2_in_curr_sample + apf2_gain * apf2_out_delay_sample);

	return apf2_out.read();
}

float
Reverb::process_hardware(float in, unsigned int index, BelaContext* context)
{
	float reverb_time = 1000;
	float mix_percent = 0;
	
	if(!(index % audio_frames_per_analog_frame)) {
			// read analog inputs and update reverb time and mix percent
			reverb_time = analogRead(context, index/2, 0);
			reverb_time = map(reverb_time, 0, 0.85, 1, 3000);
			mix_percent = analogRead(context, index/2, 1);
			mix_percent = map(mix_percent, 0, 0.85, 0, 1);
	}
	
	// g = 0.001 power of delay_time over reverb_time (-60db decrease, time to completely decay) 
	float cf1_gain = pow(0.001,cf1_delay_ms/reverb_time);
	float cf2_gain = pow(0.001,cf2_delay_ms/reverb_time);
	float cf3_gain = pow(0.001,cf3_delay_ms/reverb_time);
	float cf4_gain = pow(0.001,cf4_delay_ms/reverb_time);
    	
    float cf1_out_delay_sample = cf1.read(cf1_delay);			//y[n-D] for each CF
    float cf2_out_delay_sample = cf2.read(cf2_delay); 
    float cf3_out_delay_sample = cf3.read(cf3_delay); 
    float cf4_out_delay_sample = cf4.read(cf4_delay); 
    
    float apf1_input_delay_sample = apf1_in.read(apf1_delay);	//x[n-D] for APF1
    float apf1_output_delay_sample = apf1_out.read(apf1_delay); //y[n-D] for APF1
    
    float apf2_in_delay_sample = apf1_out.read(apf2_delay);		//x[n-D] for APF2
    float apf2_out_delay_sample = apf2_out.read(apf2_delay);	//y[n-D] for APF2

    cf1.write(in + cf1_out_delay_sample * cf1_gain);
    cf2.write(in + cf2_out_delay_sample * cf2_gain);
    cf3.write(in + cf3_out_delay_sample * cf3_gain);
    cf4.write(in + cf4_out_delay_sample * cf4_gain);
    
    // x[n] for APF1 + normalizing
    float apf1_in_curr_sample = (cf1.read() + cf2.read() + cf3.read() + cf4.read()) * 0.25;
    
    float apf1_in_mixed = (1 - mix_percent) * in + (mix_percent * apf1_in_curr_sample);
    apf1_in.write(apf1_in_mixed);

    // y[n] = x[n-D] - g * x[n] + g * y[n-D] (APF1)
    apf1_out.write(apf1_input_delay_sample - apf1_gain * apf1_in_mixed + apf1_gain * apf1_output_delay_sample);
    float apf2_in_curr_sample = apf1_out.read();				//x[n] for APF2
    
    // y[n] = x[n-D] - g * x[n] + g * y[n-D] (APF2)
    apf2_out.write(apf2_in_delay_sample - apf2_gain * apf2_in_curr_sample + apf2_gain * apf2_out_delay_sample);

	return apf2_out.read();
}

