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


//simple attack release envelope
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
  debug_print("amplitude value = %f\n", self->amplitude);
}



void precount(Data *self)
{
  if (self->phase < 0.5 && !self->preCountTrigger)
  {
    debug_print("send attack\n");
    //play short sine ping 
    self->ARStatus = ATTACK;
    self->preCountTrigger = true;
    //triggerAttackRelease(self);
  }
  else if (self->phase > 0.5 && self->preCountTrigger)
  {
    self->preCountTrigger = false;
  }

  if (self->beatInMeasure < 0.2 && self->startPreCount == true)
  {
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


void insertNote(Array *arr, uint8_t note, uint8_t noteTie)
{
  arr->eventList[arr->used][0] = note;
  arr->eventList[arr->used][1] = noteTie;
  arr->used = (arr->used + 1) % 248;
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

void startRecording(Data *self)
{
  if (self->beatInMeasure < 0.2 && !self->recording)
    self->recording = true;

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
