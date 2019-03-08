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

#include "structs.h"

float calculateFrequency(uint8_t bpm, float division);
bool checkDifference(uint8_t* arrayA, uint8_t* arrayB, size_t lengthA, size_t lengthB);
void insertNote(Array* arr, uint8_t note);
void clearSequence(Array *arr);
void copyEvents(Array* eventListA, Array* eventListB);
void resetPhase(Data* self);

#endif //_H_SEQ_UTILS_
