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
#ifndef _H_SEQ_UTILS
#define _H_SEQ_UTILS

#include "structs.h"

float getDivider(int division);
float remap(float input, float low1, float high1, float low2, float high2);
float calculateFrequency(uint8_t bpm, float division);
float applyRange(float numberToCheck, float min, float max);
bool  checkDifference(uint8_t (*arrayA) [2],  uint8_t (*arrayB) [2], size_t lengthA, size_t lengthB);
void  insertNote(Array *arr, uint8_t note, uint8_t noteMode);
void  clearSequence(Array *arr);
void  copyEvents(Array* eventListA, Array* eventListB);
float calculateNewPhase(Data* self, float noteLengthInSeconds, float currentBeatPosition, float bpm);
void  resetPhase(Data* self);
void  renderMetaRecording(Array *metaEvents, Array *writeEvents, Array *playEvents, size_t metaBegin, size_t numNotesInBar, uint8_t *transpose, uint32_t *notePlayed, size_t metaMode);
#endif //_H_SEQ_UTILS
