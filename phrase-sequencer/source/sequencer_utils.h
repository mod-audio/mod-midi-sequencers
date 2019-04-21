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
#include "oscillators.h"


float calculateFrequency(uint8_t bpm, float division);
void attackRelease(Data *self);
void precount(Data *self);
void handleBarSyncRecording(Data *self);
void clearSequence(Array *arr);
void recordNotes(Data *self, uint8_t midiNote);
void copyEvents(Array* eventListA, Array* eventListB);
void resetPhase(Data* self);

#endif //_H_SEQ_UTILS_
