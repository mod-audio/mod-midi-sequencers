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



void metronome(Data *self)
{
  if (self->beat < 0.5 && !self->preCountTrigger) {
    self->ARStatus = ATTACK;
    self->preCountTrigger = true;
  } else if (self->beat > 0.5 && self->preCountTrigger) {
    self->preCountTrigger = false;
  }
}



//TODO make suitable for all time signatures 
int barCounter(Data *self, float beatInMeasure, int barCounter)
{
    if (beatInMeasure < 3.9 && !self->barNotCounted) {
        self->barNotCounted = true;
        barCounter++;
    } else if (beatInMeasure > 3.9) {
        self->barNotCounted = false;
    }
    return barCounter;
}



void recordNotes(Data *self, uint8_t midiNote, uint8_t noteType, long int notePos)
{
    //recordedEvents[0] = midiNote
    //recordedEvents[1] = note On/Off
    //recordedEvents[2] = recordedPosition
    //recordedEvents[3] = calculated noteLength

    size_t rIndex = self->writeEvents.amountRecordedEvents;
    self->writeEvents.recordedEvents[rIndex][0] = (long int)midiNote;
    self->writeEvents.recordedEvents[rIndex][1] = (long int)noteType;
    self->writeEvents.recordedEvents[rIndex][2] = notePos;
    self->writeEvents.amountRecordedEvents++;
}



EventList calculateNoteLength(EventList events, float sampleRate, long int totalAmountOfTime)
{
    size_t noteOffIndex;
    float noteLength = 0;
    size_t searchIndex = 0;
    bool noteFound = false;
    long int foundNote[1][2];

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
                    events.recordedEvents[searchIndex][3] = noteLength; 
                } else {
                    noteLength = matchingNoteOffPos - foundNote[0][1];
                    events.recordedEvents[searchIndex][3] = noteLength;
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
    int recIndex = 0;

    for (size_t recordedNote = 0; recordedNote < events.amountRecordedEvents; recordedNote++) {
        debug_print("events.amountRecordedEvents = %li\n", events.amountRecordedEvents);
        if (events.recordedEvents[recordedNote][1] == 144) {
            long int note = events.recordedEvents[recordedNote][0];
            long int startPos = events.recordedEvents[recordedNote][2];

            long int noteLength = events.recordedEvents[recordedNote][3];
            long int velocity = 120; 

            events.eventList[0][recIndex][0] = (uint32_t)note;
            events.eventList[0][recIndex][1] = (uint32_t)noteLength;
            events.eventList[0][recIndex][2] = (uint32_t)velocity;
            events.eventList[0][recIndex][3] = (uint32_t)startPos;
            recIndex++;
            debug_print("recIndex = %i\n", recIndex);
        }
    }
    events.amountRecordedEvents = recIndex;
    return events;
}



//make copy of events from eventList A to eventList B
EventList copyEvents(EventList eventListA, EventList eventListB)
{
  eventListB.amountRecordedEvents = eventListA.amountRecordedEvents;
  for (size_t voices = 0; voices < 4; voices++) {
    for (size_t noteIndex = 0; noteIndex < eventListA.amountRecordedEvents; noteIndex++) {
      for (size_t noteMeta = 0; noteMeta < 4; noteMeta++) {
        eventListB.eventList[voices][noteIndex][noteMeta] = eventListA.eventList[voices][noteIndex][noteMeta];
      }
    }
  }   
  return eventListB;
}



EventList mergeEvents(EventList eventListA, EventList eventListB)
{

    eventListB.used = eventListA.used;

    bool noteFoundMerge = false;
    float temp[3] = {0, 0, 0};
    for (size_t note = 0; note < eventListA.used; note++) {
        for (int voice = 0; voice < 4; voice++) {
            if (eventListA.eventList[voice][note][0] > 0 && eventListA.eventList[voice][note][0] < 128) {
                for (int noteProps = 0; noteProps < 3; noteProps++) 
                    temp[noteProps] = eventListA.eventList[voice][note][noteProps];
                noteFoundMerge = true;
            }
            int voice = 0;
            while (voice < 4 && noteFoundMerge) {
                if ((eventListB.eventList[voice][note][0]) == 0 || (eventListB.eventList[voice][note][0] > 128)) {
                    for (int noteProps = 0; noteProps < 3; noteProps++) {
                        eventListB.eventList[voice][note][noteProps] = temp[noteProps];
                    }
                    noteFoundMerge = false;
                }
                voice++;
            }

        }
    }
    return eventListB;
}



EventList clearSequence(EventList events)
{
    events.amountRecordedEvents = 0;
    events.used = 0;

    for (size_t voice = 0; voice < 4; voice++) {
        for (uint8_t note = 0; note < 248; note++) {
            for (size_t noteProp = 0; noteProp < 4; noteProp++) {
                events.eventList[voice][note][noteProp] = 0;
            }
        }
    }
    for (uint8_t note = 0; note < 248; note++) {
        for (size_t noteProp = 0; noteProp < 4; noteProp++) {
            events.recordedEvents[note][noteProp] = 0;
        }
    }
    return events;
}
