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

//float resetPhaseFunction(Data* self, float division)
//{
//    float rateValues[11] = {8, 6, 4, 3, 2, 1.5, 1, 0.75, 0.5, 0.375, 0.25};
//    float phaseLength = ((60 / self->bpm) / ((rateValues[(int)division] / 2)));
//    float pos = fmod((60 / self->bpm) * self->beatInMeasure, phaseLength) / phaseLength;
//    //set notePlacement var;
//    debug_print(" \n");
//    debug_print("rateValue = %f\n", rateValues[(int)division]);
//    debug_print("pos = %f\n", pos);
//    debug_print("phaseLength = %f\n", phaseLength);
//    debug_print("self->beatInMeasure = %f\n", self->beatInMeasure);
//
//    return pos;
//}


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

    debug_print("self->beatInMeasure = %f\n", self->beatInMeasure);
    debug_print("phase %f\n", phase);

    return phase;
}


void resetPhase(Data *self)
{
    float velInitVal      = 0.000000009;

    if (self->frequency != self->previousFrequency) {
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



void clearSequence(Array *arr)
{
    arr->used = 0;
}
