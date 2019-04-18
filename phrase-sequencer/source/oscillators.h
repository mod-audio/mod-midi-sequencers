/*
 * =====================================================================================
 *
 *       Filename:  oscillators.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07-03-19 00:04:38
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Bram Giesen (), bram@moddevices.com
 *   Organization:  MOD Devices
 *
 * =====================================================================================
 */
#ifndef _H_OSCILLATORS_
#define _H_OSCILLATORS_

#include <math.h>
#include "sequencer_utils.h"


#define PI_2 6.28318530717959

double* phaseOsc(float frequency, double* phase, float rate, float swing);
double* phaseRecord(float frequency, double* phase, float rate);
double* velOsc(float frequency, double* velocityLFO, float rate, const float* velocityCurve, 
    const float* velocityDepth, const float* curveLength, const float* curveClip, Data* self);

#endif // _H_OSCILLATORS_
