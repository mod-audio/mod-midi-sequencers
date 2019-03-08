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
float* phaseOsc(float frequency, float* phase, float rate, float swing)
{
  static float noteLength[2];
  static int switchLength = 0;
  
  float phaseParam = swing - 50;
  noteLength[0] = (phaseParam * -1 + 100) * 0.01;  
  noteLength[1] = (phaseParam + 100) * 0.01;

  *phase += frequency / rate;

  if(*phase >= noteLength[switchLength]){ 
    *phase = *phase - noteLength[switchLength];
    switchLength ^= 1;
  }

  return phase;
}



float calculateFrequency(uint8_t bpm, float division)
{
  float rateValues[11] = {7.5,10,15,20,30,40,60,80,120,160.0000000001,240};
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



void resetRecord(Data* self)
{
  *self->recordBars = 0;
  self->transpose   = 0;
  self->recording   = false;
  //self->notePlayed = self->notePlayed % self->writeEvents->used;
  clearSequence(self->recordEvents);
  //init object
  self->recordEvents = (Array *)malloc(sizeof(Array));
  self->recordEvents->eventList = (uint8_t *)malloc(sizeof(uint8_t));
  self->recordEvents->used = 0;
  self->recordEvents->size = 1;
}



void renderRecordedNotes(Data* self)
{
  static bool wasRecording = false;
 //static float counterForRecording = 0;
  //static float prevPosBeat = 0;
  //static float dividers[11] = {8, 6, 4, 3, 2, 1.5, 1, 0.75, 0.5, 0.375, 0.25};
 
  //pre-count
  if (self->beatInMeasure < 0.5 && *self->recordBars == 1) {
    self->preCount = true;
  }

  if (self->beatInMeasure > self->barsize - 0.02 && self->preCount) {
    self->recording = true;
  }

  if (self->recording)
  {
    self->preCount = false;
    
    static size_t amountOfBars = 0;
    static bool barCounted = false;
    
    if (self->beatInMeasure > self->barsize - 0.02 && !barCounted)
    {
      amountOfBars++;
      barCounted = true;
    } 
    else if (self->beatInMeasure < self->barsize -0.02) {
      barCounted = false;
    }

    if (amountOfBars > *self->recordLength && *self->recordBars != 0)
    {
      self->recording = false;
      if (*self->recordLength > 8) {
        wasRecording = true;
      } else {
        for (size_t i = 0; i < self->recordEvents->used; i++) {\
        }
        copyEvents(self->recordEvents, self->writeEvents);
        resetRecord(self);
        //counterForRecording = 0;
        amountOfBars = 0;
      }  
    }
  }

  if (*self->recordBars == 0 && wasRecording)
  {
//    self->recording = false;
//    //TODO get numerator from host.
//    int countAmount  = 0;
//    //TODO remove hardcoded stuff, now this is set 4/4 with a div a 8th's
//    size_t numerator = self->barsize * 2;
//
//    while (self->recordEvents->used >= numerator)
//    {
//      self->recordEvents->used = self->recordEvents->used - numerator;
//      ++countAmount;
//    }
//
//
//		if ( self->recordEvents->used  < (int)(round(numerator * 0.25)))
//		{
//    	self->recordEvents->used = (countAmount * numerator);
//			//copyEvents(self->recordEvents, self->playEvents);
//			copyEvents(self->recordEvents, self->writeEvents);
//		} else {
//			int shortage = (self->recordEvents->used - numerator) * - 1;
//			//add notes:
//			int totalRecordedNotes = (countAmount * numerator) + self->recordEvents->used;
//		
//      for (int i = totalRecordedNotes ; i < totalRecordedNotes + shortage; i++) {
//				insertNote(self->recordEvents, self->playEvents->eventList[i % self->playEvents->used] + self->transpose);
//			}
//
//			for (int i = 0; i < totalRecordedNotes; i++) {
//				copyEvents(self->recordEvents, self->writeEvents);
//			}
//    }    
//    resetRecord(self);
    wasRecording = false;
  }  
}



//make copy of events from eventList A to eventList B
void copyEvents(Array* eventListA, Array* eventListB)
{
  eventListB->eventList = (uint8_t *)realloc(eventListB->eventList, eventListA->used * sizeof(uint8_t));

  eventListB->used = eventListA->used;
  eventListB->size = eventListB->size;

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
    //debug_print("self->mode = %f\n", *self->mode);
    //debug_print("self->playing = %i\n", self->playing);
    //debug_print("previousPlaying = %i\n", previousPlaying);
    if (self->playing != previousPlaying) {
      if (*self->mode > 1) {
        debug_print("ja ik ben er hoor");
        self->phase = 0.0;
        self->firstBar = true;
      }  
      previousPlaying = self->playing;
    }

    if (*self->division != previousDevision) {
      self->phase        = 0.0;
      self->divisionRate = *self->division;  
      previousDevision   = *self->division; 
    }
    if (self->phase > 0.989 || self->phase < 0.01) {
      self->phase = 0.0;
    }

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
