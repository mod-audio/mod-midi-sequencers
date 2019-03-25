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
  *phase += frequency / rate;

  if(*phase >= 1){ 
    *phase = *phase - 1;
  }

  return phase;
}


//phase distortion oscillator, used example from 'GroovyDSP: Introduction to Phase Distortion Synthesis', to control the velocity of the sequencer
float* velOsc(float frequency, float* velocityLFO, float rate, 
  const float* velocityCurve, const float* curveDepth, const float* curveLength, const float* curveClip, Data* self)
{

  static double phase;
	static double warpedpos;
	static double m1;
	static double m2;

	static double a = 1.0;
	static double phaseLenght = 1.0;

  phase = (frequency / (double)*curveLength) / rate;
  m1 = a / self->x1;
  m2 = a / ( a - self->x1 );

  if(self->velPhase < self->x1) {
    warpedpos = m1*self->velPhase;
  }
  else { 
    warpedpos = (m2*self->velPhase * -1) + m2;
  }
  *velocityLFO = 127 * warpedpos;
  //"clip" signal
  if (*curveClip == 1) {
   // *velocityLFO = (*velocityLFO >= 100) ? 127 : 50;
    if (*velocityLFO >= 126.88 || self->clip)
    {
      *velocityLFO = 127;
      self->clip = true;
    } else if (!self->clip) {
      *velocityLFO = 50;
    } 
  }

  self->velPhase+=phase;

  while (self->velPhase >= phaseLenght) {
    self->velPhase-= phaseLenght;
    self->x1 = (*self->velocityCurve > 0) ? *self->velocityCurve * 0.01 : 0.00000009;
  }
   
	return velocityLFO;
}
