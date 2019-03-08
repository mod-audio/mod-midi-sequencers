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

#define PI_2 6.28318530717959

float* phaseOsc(float frequency, float* phase, float rate, float swing);
float* velOsc(float frequency, float* velocityLFO, float rate, const float* velocityCurve, 
    const float* velocityDepth, const float* curveLength, const float* curveClip);

#endif // _H_OSCILLATORS_
