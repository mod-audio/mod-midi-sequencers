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

bool      checkDifference(uint8_t (*arrayA) [2],  uint8_t (*arrayB) [2], size_t lengthA, size_t lengthB);
float     remap(float input, float low1, float high1, float low2, float high2);
float     calculateFrequency(uint8_t bpm, float division);
float     applyRange(float numberToCheck, float min, float max);
float     calculateNewPhase(StepSeq* self, float noteLengthInSeconds, float currentBeatPosition, float bpm);
float     resetPhase(StepSeq* self);
float     getDivider(int division);
EventList insertNote(EventList events, uint8_t note, uint8_t noteTie);
EventList clearSequence(EventList events);
EventList copyEvents(EventList eventListA, EventList eventListB);
EventList recordMetaData(EventList metaData, EventList events, uint8_t midiNote);
EventList renderMetaRecording(EventList metaEvents, EventList currentEvents, uint8_t transpose, float playPos, 
          size_t notePlayed, float phase, int snapMode, size_t barLimit, size_t metaBegin, int quantizeValue);

#endif //_H_SEQ_UTILS_
