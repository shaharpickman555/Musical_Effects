/**************************************
 * Made by Shahar Pickman and Or Dadosh 
 * Guided by Lior Arbel
 * Pickman555@gmail.com
 * ordadush100@gmail.com
***************************************/

#include <Bela.h>
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <libraries/Scope/Scope.h>
#include <libraries/Biquad/Biquad.h>
#include <vector>
#include <cmath>
#include <cassert>
//#include <iostream>


/*********************************************************************************************
 * This class implements a generic ring buffer.
 * Buffer size is determined at initalization and cannot be changed.
 * Its purpose is to be a container for samples recording and it has
 * the ability to get a previous sample according to a given delay by using the read() method.
**********************************************************************************************/

template <typename T>
class RingBuffer
{
private:
	std::vector<T> ring_buffer;
	unsigned int size;
	unsigned int wr_ptr;
		
public:
	RingBuffer(unsigned int size, std::vector<T>* other = nullptr) : size(size),  wr_ptr(0)
	{
		ring_buffer.resize(size);
		if (other) {
			assert(size >= other->size());
			ring_buffer = *other;
			wr_ptr = other->size() % size;  
		}
	}
	
	/* Writes a new value to the ring buffer (and updates the write pointer).
	 * @param val - the value to write.
	 * @returns nothing (always succeeds).
	**/
	void write(const T val)
	{
		ring_buffer[wr_ptr] = val;
		if (++wr_ptr >= size)
        	wr_ptr = 0;
	}
	
	/**
	 * Reads a sample from the ring buffer.
	 * @param samples_delay - delay to read from, must not be bigger than the ring buffer size.
	 * @returns the value of the relevant sample.
	**/
	T read(unsigned int samples_delay = 0) const
	{
		assert(samples_delay <= size);
		return ring_buffer[(wr_ptr - 1 - samples_delay + size) % size];
	}
};

/************************************************************************************************************************
 * This class implements a generic IIR filter.
 * It must be initalized with two arrays of FilterElement, where each element consist of delay and coefficient.
 * Then, the process method performs the following calculation according to the given arrays:
 * y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2] + ...... bN*x[n - N]) - (a0*y[n] + a1*x[n-1] + a2*x[n-2] + ...... bN*x[n - M])
*************************************************************************************************************************/

struct FilterElement
{
	unsigned int delay;
	float coefficient;
};

class IIRFilter
{
private:
	unsigned int input_elements_num;
	FilterElement* input_elements;
	unsigned int output_elements_num;
	FilterElement* output_elements;

public:
	IIRFilter(unsigned int input_elements_num, FilterElement* input_elements, unsigned int output_elements_num, FilterElement* output_elements);
	~IIRFilter();
	float process(const RingBuffer<float>& inputs_buffer, const RingBuffer<float>& outputs_buffer) const;
};

/*********************************************************************************************************
 * 'Effects' is an abstract class that defines the basic common structure of
 * all audio effects according to how we implemented them on Bela.
 * It is possible to add members and methods which may be relevant for multiple effects.
 * In addition, we have implemented three audio effects in this file: distortion, wah-wah and reverb.
 * Note: One must initialize a gui controller and give it as a reference to the classes' constructors and
 * methods. Sliders are initialized by the classes' constructors.
**********************************************************************************************************/

class Effects
{
protected:
	float sample_rate;
	unsigned int audio_frames_per_analog_frame;
	
public:
	Effects(BelaContext *context);
	virtual ~Effects() = default;
	
	/**
	 * Every effect inherits from this class must override the process method.
	 * Normally, it should be called once for every sample.
	 * @param in - An input sample to be processed.
	 * @param controller - the gui controller defined for the project.
	 * @returns the processed output sample.
	**/
	virtual float process(float in, GuiController* controller = nullptr) = 0;
};


class Distortion : public Effects
{
private:

	// Members to hold gui sliders indexes
	unsigned int gain_slider_index;
	unsigned int volume_slider_index;
	unsigned int type_slider_index;

public:
	Distortion(BelaContext *context, GuiController* controller);
	float process(float in, GuiController* controller) override;
	/**
	 * This function is equivalent to the 'regular' process function.
	 * The only difference is that the effect's parameters are controlled
	 * by potentiometers connected to the bella rather than through the IDE gui controller.
	 * The distortion effect needs two potentiometers connected to 'analog in' channels 0 and 1.
	 * Channel 0 is mapped to the volume parameter.
	 * Channel 1 is mapped to the gain parameter.
	 * @param in - An input sample to be processed.
	 * @param index - index of the relevant sample in the current frame.
	 * @param context - the Bela context of the project.
	 * @returns the processed output sample.
	**/
	float process_hardware(float in, unsigned int index, BelaContext* context);
};


class WahWah : public Effects
{
private:
	Biquad bpFilter;	// Biquad band-pass
	double fc;			// Main frequency of the bandpass filter
	bool direction;		// direction of "movement" of the bandpass filter
	
	// Members to hold gui sliders indexes
	unsigned int q_slider_index;
	unsigned int movement_rate_slider_index;
	unsigned int minf_slider_index;
	unsigned int maxf_slider_index;
	unsigned int dry_wet_slider_index;
	
public:
	WahWah(BelaContext *context, GuiController* controller);
	float process(float in, GuiController* controller) override;
};


class Reverb : public Effects
{
private:
	RingBuffer<float> cf1;
	RingBuffer<float> cf2;
	RingBuffer<float> cf3;
	RingBuffer<float> cf4;
	RingBuffer<float> apf1_in;		// sum of cf 1-4
	RingBuffer<float> apf1_out;		// also apf2 in..
	RingBuffer<float> apf2_out;
	
	// Members to hold the delay of each filter (in units of samples)
	unsigned int cf1_delay;
	unsigned int cf2_delay;
	unsigned int cf3_delay;
	unsigned int cf4_delay;
	unsigned int apf1_delay;
	unsigned int apf2_delay;

	// Members to hold gui sliders indexes
	unsigned int reverb_time_slider_index;
	unsigned int mix_slider_index;
	
public:
	Reverb(BelaContext *context, GuiController* controller);
	float process(float in, GuiController* controller) override;
	/**
	 * This function is equivalent to the 'regular' process function.
	 * The only difference is that the effect's parameters are controlled
	 * by potentiometers connected to the bella rather than through the IDE gui controller.
	 * The reverb effect needs two potentiometers connected to 'analog in' channels 0 and 1.
	 * Channel 0 is mapped to the 'reverb time' parameter.
	 * Channel 1 is mapped to the 'mix percent' parameter.
	 * @param in - An input sample to be processed.
	 * @param index - index of the relevant sample in the current frame.
	 * @param context - the Bela context of the project.
	 * @returns the processed output sample.
	**/
	float process_hardware(float in, unsigned int index, BelaContext* context);
};

