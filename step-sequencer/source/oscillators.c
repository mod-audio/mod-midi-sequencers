/*
 * =====================================================================================
 *
 *       Filename:  oscillators.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07-03-19 00:05:11
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Bram Giesen (), bram@moddevices.com
 *   Organization:  MOD Devices
 *
 * =====================================================================================
 */

#include "oscillators.h"

//this is a LFO to use for timing of the beatsync
float* phaseOsc(float frequency, float* phase, float rate, float swing)
{
  static float noteLength[2];
  static int switchLength = 0;
  
  float phaseParam = swing - 50;
  noteLength[0] = (phaseParam * -1 + 100) * 0.01;  
  noteLength[1] = (phaseParam + 100) * 0.01;

  *phase += frequency / rate;

  if(*phase >= noteLength[switchLength]){ 
    *phase = *phase - noteLength[switchLength];
    switchLength ^= 1;
  }

  return phase;
}


//phase distortion oscillator, used example from 'GroovyDSP: Introduction to Phase Distortion Synthesis', to control the velocity of the sequencer
float* velOsc(float frequency, float* velocityLFO, float rate, 
  const float* velocityCurve, const float* curveDepth, const float* curveLength, const float* curveClip, Data* self)
{

	double x1 = (*velocityCurve > 0) ? *velocityCurve * 0.01 : 0.0000001;

	static double phase;
	static double pos = 0;
	static double warpedpos;
	static double m1;
	static double m2;

	static double a = 1.0;
	static double phaseLenght = 1.0;

  phase = (frequency / *curveLength) / rate;
  m1 = a / x1;
  m2 = a / ( a - x1 );

  if(pos < x1) {
    warpedpos = m1*pos;
  }
  else { 
    warpedpos = (m2*pos * -1) + m2;
  }
  *velocityLFO = 127 * warpedpos;
  //"clip" signal
  if (*curveClip == 1) {
   // *velocityLFO = (*velocityLFO >= 100) ? 127 : 50;
    if (*velocityLFO >= 126 || self->clip)
    {
      *velocityLFO = 127;
      self->clip = true;
    } else if (!self->clip) {
      *velocityLFO = 50;
    } 
  }
    

  pos+=phase;

  while(pos >= phaseLenght )
    pos-= phaseLenght;
   
	return velocityLFO;
}
