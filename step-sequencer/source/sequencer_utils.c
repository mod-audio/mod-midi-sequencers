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

    float frequency = ((bpm * getDivider((int)division)) / 60) * 0.5;

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



EventList insertNote(EventList events, uint8_t note, uint8_t noteTie)
{
    events.eventList[events.used][0] = note;
    events.eventList[events.used][1] = noteTie;
    events.used = (events.used + 1) % 248;

    return events;
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



EventList recordMetaData(EventList metaData, EventList events, uint8_t midiNote)
{
    metaData.eventList[metaData.used][0] = midiNote;
    for (size_t noteMeta = 1; noteMeta < metaData.amountOfProps; noteMeta++) {
        metaData.eventList[metaData.used][noteMeta] = events.eventList[metaData.used][noteMeta];
    }
    metaData.used += 1;

    return metaData;
}



EventList renderMetaRecording(EventList metaEvents, EventList currentEvents, uint8_t transpose, float playPos, 
        size_t notePlayed, float phase, int snapMode, size_t barLimit, size_t metaBegin, int quantizeValue) 
{

    switch (snapMode) 
    {
        case 0: //snap by bar
            //round the amount of notes that can fir in a bar, I think ceil?
            if (metaEvents.used < barLimit) {
                metaEvents.used = barLimit;
            } else {
                for (size_t i = metaEvents.used; i < barLimit; i++) { //TODO check this for error by one
                    for (size_t noteData = 0; noteData < currentEvents.amountOfProps; noteData++) {
                        metaEvents.eventList[i][noteData] = currentEvents.eventList[i % currentEvents.used][noteData] + transpose;
                    }
                }
            }
            break;
        case 1: //snap by note
            if (phase > 0.5) { //TODO check for error by one and correct phase
                for (size_t noteData = 0; noteData < currentEvents.amountOfProps; noteData++) {
                    metaEvents.eventList[metaEvents.used][noteData] = currentEvents.eventList[metaEvents.used % currentEvents.used][noteData] + transpose;
                }
            }
            break;
    }
    return metaEvents;
}



float calculateNewPhase(StepSeq* self, float noteLengthInSeconds, float currentBeatPosition, float bpm)
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



bool checkForFirstBar(StepSeq *self)
{
    float velInitVal      = 0.000000009;

    if (self->playing != self->previousPlaying) {
        if (*self->modeParam > 1) {
            self->velPhase = velInitVal;
            self->firstBar = true;
        }
        self->previousPlaying = self->playing;
    }
    return self->firstBar;
}



float resetPhase(StepSeq *self)
{
    if (self->frequency != self->previousFrequency) {
        self->barLimit = ceil(getDivider(self->division) * 4);
        float noteLengthInSeconds = 1.0 / self->frequency;
        self->phase        = calculateNewPhase(self, noteLengthInSeconds, self->beatInMeasure, self->bpm);
        self->previousFrequency = self->frequency;
    }

    if (self->beatInMeasure < 0.5 && self->resetPhase) {

        if (self->phase > 0.989 || self->phase < 0.01) {
            self->phase = 0.0;
        }
        self->firstBar = checkForFirstBar(self);
        self->resetPhase  = false;

    } else {
        if (self->beatInMeasure > 0.5) {
            self->resetPhase = true;
        }
    }
    return self->phase;
}



EventList clearSequence(EventList events)
{
    events.used = 0;

    return events;
}
