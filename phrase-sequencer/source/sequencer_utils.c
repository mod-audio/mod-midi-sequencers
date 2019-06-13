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



EventList recordNotes(EventList recordedEvents, uint8_t midiNote, uint8_t velocity, uint8_t noteType, long int notePos)
{
    //recordedEvents[0] = midiNote
    //recordedEvents[1] = note On/Off
    //recordedEvents[2] = recordedPosition
    //recordedEvents[3] = calculated noteLength
    //TODO add velocity
    size_t rIndex = recordedEvents.used;
    recordedEvents.eventList[rIndex][0] = (long int)midiNote;
    recordedEvents.eventList[rIndex][1] = (long int)velocity;
    recordedEvents.eventList[rIndex][2] = (long int)noteType;
    recordedEvents.eventList[rIndex][3] = notePos;
    recordedEvents.used++;

    return recordedEvents;
}



EventList calculateNoteLength(EventList recordedEvents, float sampleRate, long int totalAmountOfTime)
{
    size_t noteOffIndex;
    float noteLength = 0;
    size_t searchIndex = 0;
    bool noteFound = false;
    long int foundNote[1][2];

    FindNoteEnum calculateNoteLength;
    calculateNoteLength = FIND_NOTE_ON;
    float matchingNoteOffPos = 0;

    while (searchIndex < recordedEvents.used) {

        switch (calculateNoteLength)
        {
            case FIND_NOTE_ON:
                if (recordedEvents.eventList[searchIndex][0] < 128 &&
                        recordedEvents.eventList[searchIndex][2] == 144)
                {
                    foundNote[0][0] = recordedEvents.eventList[searchIndex][0];
                    foundNote[0][1] = recordedEvents.eventList[searchIndex][3];
                    calculateNoteLength = FIND_NOTE_OFF;
                } else {
                    calculateNoteLength = NEXT_INDEX;
                }
                break;
            case FIND_NOTE_OFF:
                noteOffIndex = searchIndex;
                while (noteOffIndex < recordedEvents.used && !noteFound) {
                    if (recordedEvents.eventList[noteOffIndex][0] == foundNote[0][0] &&
                            recordedEvents.eventList[noteOffIndex][2] == 128)
                    {
                        matchingNoteOffPos = recordedEvents.eventList[noteOffIndex][3];
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
                    recordedEvents.eventList[searchIndex][4] = noteLength; 
                } else {
                    noteLength = matchingNoteOffPos - foundNote[0][1];
                    recordedEvents.eventList[searchIndex][4] = noteLength;
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
    return recordedEvents;
}


//recordedEvents[0] = midiNote
//recordedEvents[1] = velocity
//recordedEvents[2] = note On/Off 
//recordedEvents[3] = recordedPosition
//recordedEvents[4] = calculated noteLength
EventList quantizeNotes(EventList recordedEvents)
{
    int recIndex = 0;

    //for (size_t recordedNote = 0; recordedNote < recordedEvents.used; recordedNote++) {
    //    if (recordedEvents.eventList[recordedNote][1] == 144) {
    //        long int note = recordedEvents.eventList[recordedNote][0];
    //        long int velocity = recordedEvents.eventList[recordedNote][1];
    //        long int startPos = recordedEvents.eventList[recordedNote][2];
    //        long int noteLength = recordedEvents.eventList[recordedNote][3];
    //        long int noteStatus; //TODO check order of elements

    //        recordedEvents.eventList[recIndex][0] = (uint32_t)note;
    //        recordedEvents.eventList[recIndex][1] = (uint32_t)velocity;
    //        recordedEvents.eventList[recIndex][2] = (uint32_t)noteStatus;
    //        recordedEvents.eventList[recIndex][3] = (uint32_t)startPos;
    //        recordedEvents.eventList[recIndex][4] = (uint32_t)noteLength;
    //        recIndex++;
    //    }
    //}
    //recordedEvents.used = recIndex;
    return recordedEvents;
}



//make copy of events from eventList A to eventList B
EventList copyEvents(EventList eventListA, EventList eventListB)
{
    eventListB.used = eventListA.used;
    for (size_t noteIndex = 0; noteIndex < eventListA.used; noteIndex++) {
        for (size_t noteMeta = 0; noteMeta < eventListA.amountOfProps; noteMeta++) {
            eventListB.eventList[noteIndex][noteMeta] = eventListA.eventList[noteIndex][noteMeta];
        }
    }
    return eventListB;
}




EventList mergeEvents(EventList recordedEvents, EventList storedEvents, EventList mergedEvents) 
{ 
    size_t amountOfProps = recordedEvents.amountOfProps;
	size_t i = 0, j = 0, k = 0; 
    mergedEvents.used = storedEvents.used + recordedEvents.used;

	while (i < recordedEvents.used && j < storedEvents.used) 
	{ 
        if (recordedEvents.eventList[i][0] == 0) {
            j++;
        } else {
            if (recordedEvents.eventList[i][3] < storedEvents.eventList[j][3]) { 
                for (size_t noteProps = 0; noteProps < amountOfProps; noteProps++)
                    mergedEvents.eventList[k][noteProps] = recordedEvents.eventList[i][noteProps]; 
                k++;
                i++;
            } 
            else {
                for (size_t noteProps = 0; noteProps < amountOfProps; noteProps++)
                    mergedEvents.eventList[k][noteProps] = storedEvents.eventList[j][noteProps]; 
                k++;
                j++;
            }
        }
	} 
    while (i < recordedEvents.used) {
        for (size_t noteProps = 0; noteProps < amountOfProps; noteProps++)
            mergedEvents.eventList[k][noteProps] = recordedEvents.eventList[i][noteProps]; 
        k++;
        i++;
    }
    while (j < storedEvents.used) {
        for (size_t noteProps = 0; noteProps < amountOfProps; noteProps++)
            mergedEvents.eventList[k][noteProps] = storedEvents.eventList[i][noteProps]; 
        k++;
        j++;
    }
    return mergedEvents;
} 



EventList clearSequence(EventList events)
{
    events.used = 0;

    for (uint8_t note = 0; note < 248; note++) {
        for (size_t noteProp = 0; noteProp < events.amountOfProps; noteProp++) {
            events.eventList[note][noteProp] = 0;
        }
    }
    return events;
}
