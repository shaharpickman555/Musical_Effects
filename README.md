# Musical_Effects

Real-time audio effects class in C++ for the Bela.io platform (https://bela.io/). 
The class contains three effects - Distortion, Wha-Wha & Reverb.
Also contains easy-to-use tools for creating more audio effects in this class, such as ring buffer structure and generic FIR/IIR filter.

Effects.h                    - header file for the Effects class. Include it in your project in order to use its features.
Effects.cpp                  - implementation file of the Effects class.
effects_render.cpp           - an example Bela project (render file) that uses the Effects class.
render_for_IIR & lowpass_iir - an example Bela project that uses the generic FIR/IIR filter for creating LPF.

Created as part of Technion University final project in Electrical Engineering, under the supervision of Dr. Lior Arbel in the High Speed Digital Systems Laboratory.

Made by Or Dadosh (ordadush100@gmail.com) & Shahar Pickman (pickman555@gmail.com)
