/****************************************
 * Made by Shahar Pickman and Or Dadosh *
*****************************************/

/****************************************************
 * This is an example of how to use the 'IIRFilter'
 * class in a Bela program.
 * In this case we will implement an AllPass filter
 * that is also used in our Reverb implementation
 * (as part of the Schroeder's Reverb algorithm).
 * The filter difference equation is:
 * y[n] = x[n-D] - g * x[n] + g * y[n-D]
*****************************************************/

#include "Effects.h"

std::vector<float> input_buffer;
RingBuffer<float>* inputs = nullptr;
RingBuffer<float>* outputs = nullptr;
int rd_ptr = 0;
std::string song_path = "../Californication_Instrumental.wav";

const float apf1_delay_ms = 5;
const float apf1_reverb_time_ms = 96.83;

// Declare a global pointer for the filter.
IIRFilter* schroeder_allpass = nullptr;

bool setup(BelaContext *context, void *userData)
{
	// y[n] = x[n-D] - g * x[n] + g * y[n-D]
	const float g = pow(0.001, apf1_delay_ms/apf1_reverb_time_ms);
	const float D = (int)(apf1_delay_ms * (context->audioSampleRate/1000));

	// Initialize coefficients arrays:
	// Inputs:
	FilterElement input_elements[2];
	// -g*x[n]
	input_elements[0].delay = 0;
	input_elements[0].coefficient = -g;
	// x[n-D]
	input_elements[1].delay = D;
	input_elements[1].coefficient = 1;
	
	//Outputs:
	FilterElement output_elements[1];
	// g * y[n-D]
	output_elements[0].delay = D;
	output_elements[0].coefficient = -g;
	
	// Initialize the filter
	schroeder_allpass = new IIRFilter(2, input_elements, 1, output_elements);
	
	input_buffer = AudioFileUtilities::loadMono(song_path);
	
	// Initiaize RingBuffers for inputs and Outputs
	inputs = new RingBuffer<float>(input_buffer.size());
	outputs = new RingBuffer<float>(int(context->audioSampleRate));
	
	return true;
}

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
    	
	    inputs->write(input_buffer[rd_ptr]);
	    if(++rd_ptr >= input_buffer.size()) {
	        rd_ptr = 0;
	    }

	    float out = schroeder_allpass->process(*inputs, *outputs);
	    outputs->write(out);
	    
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
    }
}

void cleanup(BelaContext *context, void *userData)
{
	delete schroeder_allpass;
	delete inputs;
	delete outputs;
}

