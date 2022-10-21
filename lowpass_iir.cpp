/****************************************
 * Made by Shahar Pickman and Or Dadosh *
*****************************************/

/****************************************************
 * This is an example of how to use the 'IIRFilter'
 * class in a Bela program.
 * In this case we will implement a lowpass filter
 * The filter difference equation is:
 * y[n] = (1-a)*x[n] + a*y[n-1]
*****************************************************/

#include "Effects.h"

std::vector<float> input_buffer;
RingBuffer<float>* inputs = nullptr;
RingBuffer<float>* outputs = nullptr;
int rd_ptr = 0;
std::string song_path = "../Californication_Instrumental.wav";

Scope scope;

// Declare a global pointer for the filter.
IIRFilter* schroeder_allpass = nullptr;

bool setup(BelaContext *context, void *userData)
{
	scope.setup(1, context->audioSampleRate);
	
	float a = 0.99;

	// Initialize coefficients arrays:
	// Inputs:
	FilterElement input_elements[1];
	// (1-a)*x[n]
	input_elements[0].delay = 0;
	input_elements[0].coefficient = 1-a;

	//Outputs:
	FilterElement output_elements[1];
	// a*y[n-1]
	output_elements[0].delay = 1;
	output_elements[0].coefficient = -a;
	
	// Initialize the filter
	schroeder_allpass = new IIRFilter(1, input_elements, 1, output_elements);
	
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

	    scope.log(out);
	    
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
