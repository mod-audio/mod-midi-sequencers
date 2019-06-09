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

float     reCalcPhase(int bpm, float beatInMeasure, float sampleRate, float divisions);
float     calculateFrequency(uint8_t bpm, float division);
float*    envelope(Data *self, float *amplitude);
void      metronome(Data *self);
int       barCounter(Data* self, float beatInMeasure, int barCounter);
void      recordNotes(Data *self, uint8_t midiNote, uint8_t noteType, long int notePos);
void      resetPhase(Data* self);
EventList quantizeNotes(EventList events);
EventList mergeEvents(EventList playEvents, EventList writeEvents, EventList mergedEvents);
EventList copyEvents(EventList eventListA, EventList eventListB);
EventList calculateNoteLength(EventList events, float sampleRate, long int totalAmountOftime);
EventList clearSequence(EventList events);


#endif //_H_SEQ_UTILS_
