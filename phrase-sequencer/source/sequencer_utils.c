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
  static int barCounter = 0;
  static bool barNotCounted = false;

  if (self->beatInMeasure < 3.9 && !barNotCounted) {
    barNotCounted = true;
    barCounter++;
  } else if (self->beatInMeasure > 3.9) {
    barNotCounted = false;
  }
  return barCounter;
}



void recordNotes(Data *self, uint8_t midiNote)
{
}



void calculateNoteLength(Data* self, int recordingLength)
{

    bool noteFound = false;
    static float foundNote[1][2];
    float totalamountOftime;
    float noteLength;
    float totalAmountOfTime;
    int recordingLength = 16;
    int searchIndex = 0;
    int noteOffIndex;

    FindNoteEnum calculateNoteLength;
    calculateNoteLength = FIND_NOTE_ON;
    float matchingNoteOffPos = 0;

    while (searchIndex < recordingLength) {


        switch (calculateNoteLength)
        {
            case FIND_NOTE_ON:
                if (self->writeEvents.recordedEvents[searchIndex][0] < 128 &&
                        self->writeEvents.recordedEvents[searchIndex][1] == 144)
                {
                    foundNote[0][0] = self->writeEvents.recordedEvents[searchIndex][0];
                    foundNote[0][1] = self->writeEvents.recordedEvents[searchIndex][2];
                    calculateNoteLength = FIND_NOTE_OFF;
                } else {
                    calculateNoteLength = NEXT_INDEX;
                }
                break;
            case FIND_NOTE_OFF:
                noteOffIndex = searchIndex;
                while (noteOffIndex < recordingLength) {
                    noteOffIndex++;
                    if (self->writeEvents.recordedEvents[noteOffIndex][0] == foundNote[0][0] &&
                            self->writeEvents.recordedEvents[noteOffIndex][1] == 128)
                    {
                        matchingNoteOffPos = self->writeEvents.recordedEvents[noteOffIndex][2];
                        noteFound = true;
                        calculateNoteLength = CALCULATE_NOTE_LENGTH;
                    } 
                }
                calculateNoteLength = CALCULATE_NOTE_LENGTH;
                break;
            case CALCULATE_NOTE_LENGTH:
                if (!noteFound) {
                    noteLength = totalAmountOfTime - foundNote[0][1]; 
                    self->writeEvents.recordedEvents[searchIndex][3] = noteLength; 
                } else {
                    noteLength = matchingNoteOffPos - foundNote[0][1];
                    self->writeEvents.recordedEvents[searchIndex][3] = noteLength;
                    noteFound = false;
                }
                calculateNoteLength = NEXT_INDEX;
                break;
            case NEXT_INDEX:
                searchIndex++;
                calculateNoteLength = FIND_NOTE_ON;
                break;
        }
    }
}



void quantizeNotes(Array events, float startPos, uint8_t midiNote)
{
    int snappedIndex    = 0;
    static int recIndex = 0;
    
    for (int recordedNote = 0; recordedNote < self->events.writeEvents.amountRecordedNotes; recordedNote++) {
        float note = events.writeEvents.recordedNotes[recordedNote][0];
        float startPos = events.writeEvents.recordedNotes[recordedNote][2];
        float noteLength = events.writeEvents.recordedNotes[recordedNote][3];
        float velocity = 120; 
        snappedIndex = (int)roundf(startPos);
        self->writeEvents->eventList[recIndex++ % 4][snappedIndex][0] = midiNote;
        self->writeEvents->eventList[recIndex][snappedIndex][1]       = noteLength;
        self->writeEvents->eventList[recIndex][snappedIndex][2]       = velocity;
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
