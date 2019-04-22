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


//TODO make suitable for all time signatures 
int barCounter(Data *self, uint8_t recordingLength)
{
  static int previousBeat = 10;
  static size_t counter = 0;
  //return three when the amount of bars has been reached
  if (self->barCount > recordingLength) {
    self->barCount = 0;
    self->barCounted = false;
    debug_print("self->barCount %i\n", self->barCount);
    debug_print("BARS ARE DONE\n");
    return 2;
  } else {
      if((int)self->beatInMeasure != previousBeat) {
        debug_print("self->beatInMeasure in INT BEAT = %f\n", self->beatInMeasure);
        previousBeat = (int)self->beatInMeasure;
        counter++;
      }
      if (counter > 4) {
        self->barCount += 1;
        counter = 0;
        previousBeat = 10;
        debug_print("self->beatInMeasure in BARCOUNT = %f\n", self->beatInMeasure);
        debug_print("self->barCount = %i\n", self->barCount);
      }
    return 1;
  }
}



void recordNotes(Data *self, uint8_t midiNote)
{
  static int   snappedIndex = 0;
  static int   recIndex     = 0;
  
  if (midiNote > 0) {
    snappedIndex = (int)roundf(self->phaseRecord);
    self->writeEvents->eventList[recIndex++ % 4][snappedIndex][0] = midiNote;
    debug_print("snappedIndex = %i\n", snappedIndex);
    debug_print("recordIndex = %i\n", recIndex);
    debug_print("self->writeEvents->eventList = %i\n", self->writeEvents->eventList[0][snappedIndex][0]);
    debug_print("self->writeEvents->eventList = %i\n", self->writeEvents->eventList[1][snappedIndex][0]);
    debug_print("self->writeEvents->eventList = %i\n", self->writeEvents->eventList[2][snappedIndex][0]);
    debug_print("self->writeEvents->eventList = %i\n", self->writeEvents->eventList[3][snappedIndex][0]);
    self->writeEvents->used = (int)self->phaseRecord;
  }

}


//make copy of events from eventList A to eventList B
void copyEvents(Array* eventListA, Array* eventListB)
{
  eventListB->used = eventListA->used;
  for (size_t voices = 0; voices < 4; voices++) {
    for (size_t noteIndex = 0; noteIndex < eventListA->used; noteIndex++) {
      for (size_t noteMeta = 0; noteMeta < 2; noteMeta++) {
        eventListB->eventList[voices][noteIndex][noteMeta] = eventListA->eventList[voices][noteIndex][noteMeta];
      }
    }
  }   
}


void clearSequence(Array *arr)
{
  arr->used = 0;
}
