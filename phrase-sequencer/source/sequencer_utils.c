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
  //float rateValues[11] = {7.5,10,15,20,30,40,60,80,120,160.0000000001,240};
  debug_print("division in Hz = %f\n", division);
  float frequency = (bpm / division);

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
  if (self->beatInMeasure < 3.9 && !self->barNotCounted) {
    self->barNotCounted = true;
    self->barCounter++;
  } else if (self->beatInMeasure > 3.9) {
    self->barNotCounted = false;
  }
  return self->barCounter;
}



void recordNotes(Data *self, uint8_t midiNote, uint8_t noteType, float notePos)
{
    //recordedEvents[0] = midiNote
    //recordedEvents[1] = note On/Off
    //recordedEvents[2] = recordedPosition
    //recordedEvents[3] = calculated noteLength

    size_t rIndex = self->writeEvents.amountRecordedEvents;
    self->writeEvents.recordedEvents[rIndex][0] = (float)midiNote;
    self->writeEvents.recordedEvents[rIndex][1] = (float)noteType;
    self->writeEvents.recordedEvents[rIndex][2] = (float)notePos;
    self->writeEvents.amountRecordedEvents++;
}



EventList calculateNoteLength(EventList events, float sampleRate)
{
    bool noteFound = false;
    static float foundNote[1][2];
    float noteLength = 0;
    float totalAmountOfTime = 0;
    size_t searchIndex = 0;
    size_t noteOffIndex;

    FindNoteEnum calculateNoteLength;
    calculateNoteLength = FIND_NOTE_ON;
    float matchingNoteOffPos = 0;

    while (searchIndex < events.amountRecordedEvents) {

        switch (calculateNoteLength)
        {
            case FIND_NOTE_ON:
                if (events.recordedEvents[searchIndex][0] < 128 &&
                        events.recordedEvents[searchIndex][1] == 144)
                {
                    foundNote[0][0] = events.recordedEvents[searchIndex][0];
                    foundNote[0][1] = events.recordedEvents[searchIndex][2];
                    calculateNoteLength = FIND_NOTE_OFF;
                } else {
                    calculateNoteLength = NEXT_INDEX;
                }
                break;
            case FIND_NOTE_OFF:
                noteOffIndex = searchIndex;
                while (noteOffIndex < events.amountRecordedEvents && !noteFound) {
                    if (events.recordedEvents[noteOffIndex][0] == foundNote[0][0] &&
                            events.recordedEvents[noteOffIndex][1] == 128)
                    {
                        matchingNoteOffPos = events.recordedEvents[noteOffIndex][2];
                        noteFound = true;
                        calculateNoteLength = CALCULATE_NOTE_LENGTH;
                    } 
                    noteOffIndex++;
                }
                calculateNoteLength = CALCULATE_NOTE_LENGTH;
                break;
            case CALCULATE_NOTE_LENGTH:
                if (!noteFound) {
                    noteLength = totalAmountOfTime - foundNote[0][1]; 
                    events.recordedEvents[searchIndex][3] = noteLength * sampleRate; 
                    //debug_print("noteLength !found in search = %f\n", noteLength);
                } else {
                    noteLength = matchingNoteOffPos - foundNote[0][1];
                    events.recordedEvents[searchIndex][3] = noteLength * sampleRate;
                    //debug_print("noteLength found in search = %f\n", noteLength);
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
    return events;
}



EventList quantizeNotes(EventList events)
{
    int snappedIndex    = 0;
    static int recIndex = 0;
    int prevSnappedIndex = -1;

    for (size_t recordedNote = 0; recordedNote < events.amountRecordedEvents; recordedNote++) {
        if (events.recordedEvents[recordedNote][1] == 144) {
            float note = events.recordedEvents[recordedNote][0];
            float startPos = events.recordedEvents[recordedNote][2];
            float noteLength = events.recordedEvents[recordedNote][3];
            float velocity = 120; 
            snappedIndex = (int)roundf(startPos);
            if (snappedIndex == prevSnappedIndex) {
                recIndex = (recIndex + 1) % 4;
            } else {
                recIndex = 0;
            }
            events.eventList[recIndex][snappedIndex][0] = (uint32_t)note;
            events.eventList[recIndex][snappedIndex][1] = (uint32_t)noteLength;
            events.eventList[recIndex][snappedIndex][2] = (uint32_t)velocity;
            prevSnappedIndex = snappedIndex;

        }
    }
    return events;
}



//make copy of events from eventList A to eventList B
EventList copyEvents(EventList eventListA, EventList eventListB)
{
  eventListB.used = eventListA.used;
  for (size_t voices = 0; voices < 4; voices++) {
    for (size_t noteIndex = 0; noteIndex < eventListA.used; noteIndex++) {
      for (size_t noteMeta = 0; noteMeta < 3; noteMeta++) {
        eventListB.eventList[voices][noteIndex][noteMeta] = eventListA.eventList[voices][noteIndex][noteMeta];
      }
    }
  }   
  return eventListB;
}



EventList mergeEvents(EventList eventListA, EventList eventListB)
{
    if (eventListB.used > eventListA.used) {
        eventListA.used = eventListB.used;
    } else {
        eventListB.used = eventListA.used;
    }

    static bool noteFoundMerge = false;
    float temp[3] = {0, 0, 0};
    for (int note = 0; note < 9; note++) {
        for (int voice = 0; voice < 3; voice++) {
            if (eventListA.eventList[voice][note][0] > 0 && eventListA.eventList[voice][note][0] < 128) {
                for (int noteProps = 0; noteProps < 3; noteProps++) 
                    temp[noteProps] = eventListA.eventList[voice][note][noteProps];
                noteFoundMerge = true;
            }
        }
        int voice = 0;
        while (voice < 3 && noteFoundMerge) {
            if ((eventListB.eventList[voice][note][0]) == 0 || (eventListB.eventList[voice][note][0] > 128)) {
                for (int noteProps = 0; noteProps < 3; noteProps++) {
                    eventListB.eventList[voice][note][noteProps] = temp[noteProps];
                }
                noteFoundMerge = false;
            }
            voice++;
        }
    }
    return eventListB;
}



void clearSequence(EventList *arr)
{
  arr->used = 0;
}
