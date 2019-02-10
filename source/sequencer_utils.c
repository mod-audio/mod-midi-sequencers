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



void resetRecord(Data* self)
{
  self->transpose  = 0;
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
  static float counterForRecording = 0;
  static float prevPosBeat = 0;
  //static float dividers[11] = {8, 6, 4, 3, 2, 1.5, 1, 0.75, 0.5, 0.375, 0.25};
 

  //pre-count
  if (self->beatInMeasure < 0.5 && *self->recordBars == 1)
    self->preCount = true;

  if (self->beatInMeasure > self->barsize - 0.02)
    self->recording = true;

 
 
  if (self->recording)
  {
    self->preCount = false;
    static int dummy = 0;
    if ( dummy == 0 ) {
      debug_print("beat in measure when started recording =  %f\n", self->beatInMeasure);
      dummy++;
    } 
    
    static size_t amountOfBars = 0;
    static bool barCounted = false;
    
    if (self->beatInMeasure > self->barsize - 0.02 && !barCounted)
    {
      amountOfBars++;
      debug_print("amount of bars = %li\n", amountOfBars);
      barCounted = true;
    } 
    else if (self->beatInMeasure < self->barsize -0.02) {
      barCounted = false;
    }

    if (amountOfBars > *self->recordLength)
    {
      self->recording = false;
      if (*self->recordLength > 8) {
        wasRecording = true;
      } else {
        debug_print("self->used = %li\n", self->recordEvents->used);
        for (size_t i = 0; i < self->recordEvents->used; i++) {\
          debug_print("recordEvent[%li]", i); 
          debug_print(" %i\n", self->recordEvents->eventList[i]);
        }
        copyEvents(self->recordEvents, self->writeEvents);
        resetRecord(self);
        *self->recordBars = 0;
        counterForRecording = 0;
        amountOfBars = 0;
        self->recording = false;
      }  
    }
  }

//  if (*self->recordBars == 0 && wasRecording)
//  {
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
//    wasRecording = false;
//  }  
}



//make copy of events from eventList A to eventList B
void copyEvents(Array* eventListA, Array* eventListB)
{
  eventListB->eventList = (uint8_t *)realloc(eventListB->eventList, eventListA->used * sizeof(uint8_t));

  eventListB->used = eventListA->used;
  eventListB->size = eventListB->size;

  for (size_t noteIndex = 0; noteIndex < eventListA->used; noteIndex++) {
    debug_print("note in copyEvents = %i\n", eventListA->eventList[noteIndex]); 
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

    //self->phase = 0.0;
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
