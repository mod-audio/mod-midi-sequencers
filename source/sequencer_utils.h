/*
 * =====================================================================================
 *
 *       Filename:  sequencer_utils.h
 *
 *    Description:   
 *
 *        Version:  1.0
 *        Created:  12/24/2018 03:39:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Bram Giesen (), bram@moddevices.com
 *   Organization:  MOD Devices
 *
 * =====================================================================================
 */
#ifndef _H_SEQ_UTILS_
#define _H_SEQ_UTILS_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "structs.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#define debug_print(...) \
((void)((DEBUG) ? fprintf(stderr, __VA_ARGS__) : 0))

float* phaseOsc(float frequency, float* phase, float rate);
float calculateFrequency(uint8_t bpm, float division);
bool checkDifference(uint8_t* arrayA, uint8_t* arrayB, size_t length);
void insertNote(Array* arr, uint8_t note);
void clearSequence(Array *arr);
void copyEvents(Array* eventListA, Array* eventListB);
void recordNotes(Data* self, uint8_t note);
void resetPhase(Data* self);

#endif
