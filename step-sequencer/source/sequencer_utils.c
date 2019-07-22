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

float getDivider(int division)
{
    float rateValues[11] = {0.25, 0.375, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0};
    float divider = rateValues[division]; 

    return divider;
}



float applyRange(float numberToCheck, float min, float max)
{
    if (numberToCheck < min)
        return min;
    else if (numberToCheck > max)
        return max;
    else
        return numberToCheck;
}


float remap(float input, float low1, float high1, float low2, float high2)
{
    return low2 + (input - low1) * (high2 - low2) / (high1 - low1);
}


float calculateFrequency(uint8_t bpm, float division)
{
    float rateValues[11] = {0.25, 0.375, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0};
    float frequency = ((bpm * rateValues[(int)division]) / 60) * 0.5;

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



float calculateNewPhase(Data* self, float noteLengthInSeconds, float currentBeatPosition, float bpm)
{
    float beatsInSeconds = (60 / bpm) * currentBeatPosition;
    float barLengthMinusNoteLength = beatsInSeconds;

    while (barLengthMinusNoteLength > 0)
    {
        barLengthMinusNoteLength -= noteLengthInSeconds;
    }
    float phase = (noteLengthInSeconds + barLengthMinusNoteLength) / noteLengthInSeconds;

    if(phase > 0.5)
        self->placementIndex = 1;
    else
        self->placementIndex = 0;

    return phase;
}


void resetPhase(Data *self)
{
    float velInitVal      = 0.000000009;

    if (self->frequency != self->previousFrequency) {
        self->numNotesInBar = ceil(getDivider(self->division) * 4);
        float noteLengthInSeconds = 1.0 / self->frequency;
        self->phase        = calculateNewPhase(self, noteLengthInSeconds, self->beatInMeasure, self->bpm);
        self->previousFrequency = self->frequency;
    }

    if (self->beatInMeasure < 0.5 && self->resetPhase) {

        //TODO move elsewhere
        if (self->playing != self->previousPlaying) {
            if (*self->modeParam > 1) {
                self->velPhase = velInitVal;
                self->firstBar = true;
            }
            self->previousPlaying = self->playing;
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


//Does this also need to Record the octave control?
void renderMetaRecording(Array *metaEvents, Array *writeEvents, Array *playEvents, size_t metaBegin, size_t numNotesInBar, uint8_t *transpose, size_t *notePlayed)
{
    numNotesInBar = (numNotesInBar > 0) ? numNotesInBar : 1;
    size_t amountMetaBars = (size_t)roundf((float)metaEvents->used / (float)numNotesInBar);
    size_t notesToWrite = amountMetaBars * numNotesInBar;
    size_t notePos = (numNotesInBar * amountMetaBars) - (numNotesInBar - metaBegin); 
    notePos = (metaBegin < numNotesInBar * amountMetaBars * 0.5) ? notePos % numNotesInBar : notePos;
    size_t notesRecorded = metaEvents->used;
    size_t metaIndex = 0;

    while (notesRecorded > 0) {
        for (unsigned prop = 0; prop < NUM_NOTE_PROPS; prop++) {
            writeEvents->eventList[notePos][prop] = metaEvents->eventList[metaIndex][prop];
        }
        metaIndex++;
        notePos++;
        notesRecorded--;
        notesToWrite--;
        notePos = (notePos >= numNotesInBar * amountMetaBars) ? 0 : notePos;
        //barsToWrite = (notePos % numNotesInBar == 0) ? barsToWrite -= 1 : barsToWrite;
    }

    writeEvents->used = metaEvents->used;
    *notePlayed = notePos;

    while (notesToWrite > 0) {
            for (unsigned prop = 0; prop < NUM_NOTE_PROPS; prop++) { 
                writeEvents->eventList[notePos][prop] = playEvents->eventList[notePos % playEvents->used][prop] + *transpose;
            }
            notePos++;
            writeEvents->used++;
            notesToWrite--;
    }
    *transpose = 0;
}



void clearSequence(Array *arr)
{
    arr->used = 0;
}


//size_t remainer = metaEvents.used % numNotesInBar;
//size_t preBarNotes = numNoteInBar - metaBegin;
//size_t toEndOfBar = metaEvents.used - preBarNotes;
//size_t notesToWrite = metaEvents.used - remainer;
//size_t recordingLength = notesToWrite;
