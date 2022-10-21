/****************************************
 * Made by Shahar Pickman and Or Dadosh *
*****************************************/

/*********************************************************************************
 * This file gives an example of how to use the 'Effects' class in a Bela program.
***********************************************************************************/

#include "Effects.h"

std::vector<float> input_buffer;
int rd_ptr = 0;
std::string song_path = "../Californication_Instrumental.wav";		// change path for different track
std::string song_path_2 = "../speech.wav";

Scope scope;
Gui gui;
GuiController controller;

// 1. Declare global pointers for each of the effects we will use.
Distortion* distortion = nullptr;	// Effects* will work as well
WahWah* wahwah = nullptr;
Reverb* reverb = nullptr;

bool is_live = false; // set to true to process live input

bool setup(BelaContext *context, void *userData)
{
	gui.setup(context->projectName);
	controller.setup(&gui, "Effects");
	
	scope.setup(1, context->audioSampleRate);
	
	// 2. Alllocate effects' classes
	distortion = new Distortion(context, &controller);
	wahwah = new WahWah(context, &controller);
	reverb = new Reverb(context, &controller);

	if (!is_live) { 
		input_buffer = AudioFileUtilities::loadMono(song_path_2);
	}
	
	return true;
}

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
    	
	    if(++rd_ptr >= input_buffer.size()) {
	        rd_ptr = 0;
	    }
	            
		float in = 0;
		if (is_live){
			in = audioRead(context, n, 0);
		}
		
	    else {
	    	in = input_buffer[rd_ptr];
	    }
	    
	    float out = 0;
    	// 3. Activate the effects (in this case one after the other in a row).
    	out = distortion->process(in, &controller);
    	out = wahwah->process(out, &controller);
    	out = reverb->process(out, &controller);
    	
    	// Out comment to activate effects with potentiometers 
    	//out = distortion->process_hardware(in, n, context);
    	//out = reverb->process_hardware(in, n, context);
    	
    	scope.log(out);
    	
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
    }
}

void cleanup(BelaContext *context, void *userData)
{
	// 4. Deallocate memory
	delete distortion;
	delete wahwah;
	delete reverb;
}

