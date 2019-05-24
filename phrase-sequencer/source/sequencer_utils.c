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
float* envelope(Data *self, float *amplitude)
{
  switch(self->ARStatus)
  {
    case IDLE:
      break;
    case ATTACK:
      *amplitude += 0.001;
      if (self->amplitude >= 1.0) {
        self->ARStatus = RELEASE;
      } 
      break;
    case RELEASE:
      if (self->amplitude >= 0.0) {
        *amplitude -= 0.001;
      } else {
        self->ARStatus = IDLE;
      }
      break;
  }
  return amplitude;
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



void recordNotes(Data *self, uint8_t midiNote, uint8_t noteType, float notePos)
{
    //recordedEvents[0] = midiNote
    //recordedEvents[1] = note On/Off
    //recordedEvents[2] = recordedPosition
    //recordedEvents[3] = calculated noteLength

    size_t rIndex = self->writeEvents.amountRecordedEvents;
    debug_print("DEBUG 1");
    self->writeEvents.recordedEvents[rIndex][0] = (float)midiNote;
    debug_print("DEBUG 2");
    self->writeEvents.recordedEvents[rIndex][1] = (float)noteType;
    debug_print("DEBUG 3");
    self->writeEvents.recordedEvents[rIndex][2] = (float)notePos;
    debug_print("DEBUG 4");
    self->writeEvents.amountRecordedEvents++;
    debug_print("DEBUG 5");
}



void calculateNoteLength(Data* self, int recordingLength)
{

    bool noteFound = false;
    static float foundNote[1][2];
    float noteLength = 0;
    float totalAmountOfTime = 0;
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
                    debug_print("noteLength !found in search = %f\n", noteLength);
                } else {
                    noteLength = matchingNoteOffPos - foundNote[0][1];
                    self->writeEvents.recordedEvents[searchIndex][3] = noteLength;
                    debug_print("noteLength found in search = %f\n", noteLength);
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



void quantizeNotes(Data* self)
{
    int snappedIndex    = 0;
    static int recIndex = 0;
    
    self->writeEvents.used = 8;

    for (size_t recordedNote = 0; recordedNote < self->writeEvents.amountRecordedEvents; recordedNote++) {
        if (self->writeEvents.recordedEvents[recordedNote][1] == 144) {
            //debug_print("recordedNote = %li\n", recordedNote);
            float note = self->writeEvents.recordedEvents[recordedNote][0];

            debug_print("note in quantize notes = %f\n", note);
            float startPos = self->writeEvents.recordedEvents[recordedNote][2];
            debug_print("startPos = %f\n", startPos);
            float noteLength = self->writeEvents.recordedEvents[recordedNote][3];
            debug_print("noteLength =  %f\n", noteLength);
            float velocity = 120; 
            snappedIndex = (int)roundf(startPos);
            self->writeEvents.eventList[recIndex][snappedIndex][0] = note;
            self->writeEvents.eventList[recIndex][snappedIndex][1]       = noteLength;
            self->writeEvents.eventList[recIndex][snappedIndex][2]       = velocity;
            recIndex = (recIndex + 1) % 4;
        }
    }
}



//make copy of events from eventList A to eventList B
void copyEvents(Array *eventListA, Array *eventListB)
{
  //eventListB = &eventListA;
  //eventListA->used = 0;
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
