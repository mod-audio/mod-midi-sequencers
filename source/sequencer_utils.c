/*
 * =====================================================================================
 *
 *       Filename:  sequencer_utils.c
 *
 *    Description:  util functions for MIDI sequencer
 *
 *        Version:  1.0
 *        Created:  12/24/2018 03:29:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Bram Giesen (), bram@moddevices.com
 *   Organization:  MOD Devices
 *
 * =====================================================================================
 */


#include "sequencer_utils.h"



//phase oscillator to use for timing of the beatsync
float* phaseOsc(float frequency, float* phase, float rate)
{
  *phase += frequency / rate;

  if(*phase >= 1) *phase = *phase - 1;

  return phase;
}

float calculateFrequency(uint8_t bpm, float division)
{
  float rateValues[11] = {15,20,30,40,60,80,120,160.0000000001,240,320.0000000002,480};
  float frequency = bpm / rateValues[(int)division];

  return frequency;
}


//check differnece between array A and B
bool checkDifference(uint8_t* arrayA, uint8_t* arrayB, size_t length)
{
  if (sizeof(arrayA) != sizeof(arrayB)) {
    return true;
  } else {
    for (size_t index = 0; index < length; index++) {
      if (arrayA[index] != arrayB[index]) {
        return true;
      }
    }
  }
  return false;
}


void insertNote(Array *arr, uint8_t note)
{
  if (arr->used == arr->size) {
    arr->size *= 2;
    arr->eventList = (uint8_t *)realloc(arr->eventList, arr->size * sizeof(uint8_t));
  }
  arr->eventList[arr->used++] = note;
}


void recordNotes(Array* arr, uint8_t note)
{
  insertNote(arr, note);
}

//make copy of events from eventList A to eventList B
void copyEvents(Array* eventListA, Array* eventListB)
{
  eventListB->eventList = (uint8_t *)realloc(eventListB->eventList, eventListA->used * sizeof(uint8_t));

  for (size_t noteIndex = 0; noteIndex < eventListA->used; noteIndex++) {
    eventListB->eventList[noteIndex] = eventListA->eventList[noteIndex];
  }   
}


void clearSequence(Array *arr)
{
  free(arr->eventList);
  arr->eventList = NULL;
  arr->used = arr->size = 0;
}
