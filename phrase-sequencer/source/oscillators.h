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

uint32_t reCalcPos(int bpm, float beatInMeasure, float sampleRate, float division);
float    reCalcPhase(int bpm, float beatInMeasure, float sampleRate, float division);
double   sinOsc(float frequency, double* phase, float rate);
double*  phaseOsc(float frequency, double* phase, float rate, float swing);
double*  phaseRecord(float frequency, double* phase, float rate, size_t length);

#endif // _H_OSCILLATORS_
