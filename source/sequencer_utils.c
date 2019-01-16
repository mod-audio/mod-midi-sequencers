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


//check difference between array A and B
bool checkDifference(uint8_t* arrayA, uint8_t* arrayB, size_t lengthA, size_t lengthB)
{
  if (lengthA != lengthB) {
    return true;
  } else {  
    for (size_t index = 0; index < lengthA; index++) {
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



void recordNote(Array *arr, uint8_t note)
{
  if (arr->used == arr->size) {
    arr->size *= 2;
    arr->eventList = (uint8_t *)realloc(arr->eventList, arr->size * sizeof(uint8_t));
  }
  arr->eventList[arr->used++] = note;
}



void recordNotes(Data* self, uint8_t note)
{ 
  static bool  wasRecording     = false;
  static bool  recordingStarted = false;
  
  if (self->beatInMeasure < 0.5 && *self->recordBars == 1)
  {
    self->recording = true;
    wasRecording    = true;
  }

  if (self->recording)
  { 
    if (!recordingStarted){
      //clearSequence(self->recordEvents);
      recordingStarted = true;
    }

    recordNote(self->recordEvents, note);
  }

  if (*self->recordBars == 0 && wasRecording)
  {
    self->recording = false;
    //TODO get numerator from host.
    int countAmount  = 0;
    //TODO remove hardcoded stuff, now this is set 4/4 with a div a 8th's
    size_t numerator = 4 * 2;
    
    self->recordEvents->used += 1;

    while (self->recordEvents->used >= numerator)
    {
      self->recordEvents->used = self->recordEvents->used - numerator;

      ++countAmount;
    }

//    debug_print("recordEvents size = %li\n", self->recordEvents->size);
//    for (size_t in = 0; in < self->recordEvents->used; in++) {
//      debug_print("record index = %li ", in);
//      debug_print("note = %i\n", self->recordEvents->eventList[in]); 
//    }

		if ( self->recordEvents->used  < (numerator/2))
		{
    	self->recordEvents->used = countAmount * numerator;
			//copyEvents(self->recordEvents, self->playEvents);
			copyEvents(self->recordEvents, self->writeEvents);
		} else {
			int shortage = (self->recordEvents->used - numerator) * -1;
			//add notes:
			int totalRecordedNotes = (countAmount * numerator) + self->recordEvents->used;
			
			for (int i = totalRecordedNotes ; i < totalRecordedNotes + shortage; i++) {
				insertNote(self->recordEvents, self->playEvents->eventList[i % self->playEvents->used] + self->transpose);
				debug_print(" appended note = %i\n", self->playEvents->eventList[i % self->playEvents->used]);
			}
			
			for (int i = 0; i < totalRecordedNotes -1; i++) {
				copyEvents(self->recordEvents, self->writeEvents);
			}
		}    
    
    countAmount      = 0; 
    self->transpose  = 0;
    self->notePlayed = self->notePlayed % self->writeEvents->used;
    recordingStarted = false;
    wasRecording     = false;
    clearSequence(self->recordEvents);
  }  
}


//make copy of events from eventList A to eventList B
void copyEvents(Array* eventListA, Array* eventListB)
{
  eventListB->eventList = (uint8_t *)realloc(eventListB->eventList, eventListA->used * sizeof(uint8_t));

  eventListB->used = eventListA->used;

  for (size_t noteIndex = 0; noteIndex < eventListA->used; noteIndex++) {
    eventListB->eventList[noteIndex] = eventListA->eventList[noteIndex];
  }   
}



void resetPhase(Data *self)
{
  static float previousDevision;
  static bool  previousPlaying = false;
  static bool  resetPhase      = true;

  if (self->beatInMeasure < 0.5 && resetPhase) {
    //TODO move elsewhere
    if (self->playing != previousPlaying) {
      self->firstBar = true;  
      previousPlaying = self->playing;
    }

    if (*self->division != previousDevision) {
      self->divisionRate = *self->division;  
      previousDevision   = *self->division; 
    }

    self->phase = 0.0;
    resetPhase  = false;

  } else {
    if (self->beatInMeasure > 0.5) {
      resetPhase = true;
    } 
  }
}



void clearSequence(Array *arr)
{
  free(arr->eventList);
  arr->eventList = NULL;
  arr->used = arr->size = 0;
}
