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


float calculateFrequency(uint8_t bpm, float division)
{
  float rateValues[11] = {7.5,10,15,20,30,40,60,80,120,160.0000000001,240};
  float frequency = (bpm / rateValues[(int)division]) * 0.5;

  return frequency;
}


//check difference between array A and B
bool checkDifference(uint8_t (*arrayA) [2], uint8_t (*arrayB) [2], size_t lengthA, size_t lengthB)
{
  if (lengthA != lengthB) {
    return true;
  } else {  
    for (size_t index = 0; index < lengthA; index++) {
      for (size_t y = 0; y < 2; y++) {
        if (arrayA[index] != arrayB[index]) {
          return true;
        } 
        else if (arrayA[index][y] != arrayB[index][y]) {
          return true;    
        }
      }
    }
  }
  return false;
}


//simple linear attack release envelope, this is used for the clicktrack
void attackRelease(Data *self)
{
  switch(self->ARStatus)
  {
    case IDLE:
      break;
    case ATTACK:
      self->amplitude += 0.001;
      if (self->amplitude >= 1.0) {
        self->ARStatus = RELEASE;
      } 
      break;
    case RELEASE:
      if (self->amplitude >= 0.0) {
        self->amplitude -= 0.001;
      } else {
        self->ARStatus = IDLE;
      }
      break;
  }
}



void precount(Data *self)
{
  if (self->beat < 0.5 && !self->preCountTrigger) {
    self->ARStatus = ATTACK;
    self->preCountTrigger = true;
  } else if (self->beat > 0.5 && self->preCountTrigger) {
    self->preCountTrigger = false;
  }
}



int barCounter(Data *self, uint8_t recordingLength)
{
  //return three when the amount of bars has been reached
  if (self->barCount > recordingLength) {
    self->barCount = 0;
    return 2;
  } else {
    if (self->beatInMeasure < 0.5 && !self->barCounted) {
      self->barCount += 1;
      self->barCounted = true;
    } else if (self->beatInMeasure > 0.5 && self->barCounted) {
      self->barCounted = false;
    }
    return 1;
  }
}


/*function that handles the process of starting the pre-count at the beginning of next bar,
pre-count length and recording length.*/
void handleBarSyncRecording(Data *self)
{
  switch(self->recordingStatus)
  {
    case 0: //start pre-counting at next bar
      if (self->beatInMeasure < 0.1 && self->startPreCount) {
        self->startPreCount = false;
        self->recordingStatus = 2;
      }
      //debug_print("WAITING FOR FIRST BAR\n"); 
      break;
    case 1: //count bars while pre-counting
      precount(self);
      self->recordingStatus = barCounter(self, 1);
      //debug_print("PRE-COUNTING\n"); 
      break;
    case 2: //record
      self->recording = true;
      self->startPreCount = false;
      self->recordingStatus = (barCounter(self, 4)) + 1;
      //debug_print("RECORDING\n"); 
      break;
    case 3: //stop recording 
      //debug_print("STOP RECORDING\n"); 
      self->recording = false;
      self->recordingStatus = 1;
      break;
  }
}



void recordNotes(Data *self, uint8_t midiNote)
{
  static float midiNotes[4][248][2];
  static int   snappedIndex = 0;
  static int   recIndex     = 0;
  
  if(midiNote > 0) {
    snappedIndex = (int)roundf(self->phaseRecord);
    midiNotes[recIndex++ % 4][snappedIndex][0] = midiNote;
  }

}


//make copy of events from eventList A to eventList B
void copyEvents(Array* eventListA, Array* eventListB)
{
  eventListB->used = eventListA->used;

  for (size_t noteIndex = 0; noteIndex < eventListA->used; noteIndex++) {
    for (size_t noteMeta = 0; noteMeta < 2; noteMeta++) {
      eventListB->eventList[noteIndex][noteMeta] = eventListA->eventList[noteIndex][noteMeta];
    }
  }   
}

void resetPhase(Data *self)
{
  float velInitVal      = 0.000000009;

  if (self->beatInMeasure < 0.5 && self->resetPhase) {

    //TODO move elsewhere
    if (self->playing != self->previousPlaying) {
      if (*self->mode > 1) {
        self->velPhase = velInitVal;
        self->firstBar = true;
      }  
      self->previousPlaying = self->playing;
    }

    if (*self->division != self->previousDevision) {
      self->phase        = 0.0;
      self->velPhase     = velInitVal;
      self->divisionRate = *self->division;  
      self->previousDevision   = *self->division; 
    }
    if (self->phase > 0.989 || self->phase < 0.01) {
      self->phase = 0.0;
    }

    self->resetPhase  = false;

  } else {
    if (self->beatInMeasure > 0.5) {
      self->resetPhase = true;
    } 
  }
}



void clearSequence(Array *arr)
{
  arr->used = 0;
}
