/*
 * =====================================================================================
 *
 *       Filename:  Step-Sequencer.c
 *
 *    Description:  Main file of the MIDI sequencer plugin, I have used some parts of
 *                  the eg-metro lv2 plugin and the mod-midi-switchbox plugin.
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
#include "oscillators.h"

// Struct for a 3 byte MIDI event
typedef struct {
    LV2_Atom_Event event;
    uint8_t        msg[3];
} LV2_Atom_MIDI;


static LV2_Handle instantiate(const LV2_Descriptor*     descriptor,
        double                    rate,
        const char*               path,
        const LV2_Feature* const* features)
{
    StepSeq* self = (StepSeq*)calloc(1, sizeof(StepSeq));

    // Get host features
    const LV2_URID_Map* map = NULL;

    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            map = (const LV2_URID_Map*)features[i]->data;
            break;
        }
    }
    if (!map) {
        free(self);
        return NULL;
    }

    // Map URIs
    self->urid_midiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

    MetroURIs* const    uris  = &self->uris;
    uris->atom_Blank          = map->map(map->handle, LV2_ATOM__Blank);
    uris->atom_Float          = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Object         = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Path           = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_Resource       = map->map(map->handle, LV2_ATOM__Resource);
    uris->atom_Sequence       = map->map(map->handle, LV2_ATOM__Sequence);
    uris->time_Position       = map->map(map->handle, LV2_TIME__Position);
    uris->time_barBeat        = map->map(map->handle, LV2_TIME__barBeat);
    uris->time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
    uris->time_beatsPerBar    = map->map(map->handle, LV2_TIME__beatsPerBar);
    uris->time_speed          = map->map(map->handle, LV2_TIME__speed);

    self->rate             = rate;
    self->nyquist          = rate / 2;
    self->bpm              = 120.0f;
    self->beatInMeasure    = 0;
    self->phase            = 0;
    self->velPhase         = 0.000000009;
    self->x1               = 0.00000001;
    self->velocityLFO      = 0;
    self->velocity         = 0;
    self->octaveIndex      = 0;
    self->noteOffIndex     = 0;
    self->noteOffSendIndex = 0;
    self->countTicks       = 0;
    self->patternIndex     = 0;
    self->modeHandle       = 0;
    self->prevMod          = 100;
    self->prevLatch        = 100;
    self->count            = 0;
    self->inputIndex       = 0;
    self->notesPressed     = 0;
    self->previousFrequency = 0;
    //check this value
    self->prevThrough      = 0;

    self->placementIndex   = 0;
    self->notePlacement[0] = 0;
    self->notePlacement[1] = 0.5;

    //resetPhase vars:
    self->previousPlaying = false;
    self->resetPhase      = true;

    for (size_t i = 0; i < 16; i++) {
        self->midiThroughInput[i] = 0;
    }

    debug_print("DEBUG MODE\n");

    for (size_t i = 0; i < 4; i++) {
        self->noteOffArr[i] = 0;
    }

    for (size_t row = 0; row < 4; row++) {
        for (size_t index = 0; index < 2; index++) {
            self->noteOffTimer[row][index] = 0;
        }
    }
    //init vars
    self->writeEvents.used  = 0;
    self->writeEvents.amountOfProps = 2;
    self->playEvents.used   = 0;
    self->playEvents.amountOfProps = 2;

    self->notePlayed  = 0;
    self->playHead    = 0;
    self->transpose   = 0;
    self->lfo1        = 0;
    self->lfo2        = 0;

    self->recordingMetaData = false;
    self->firstRecordedNote = false;
    self->trigger           = false;
    self->triggerSet        = false;
    self->cleared           = true;
    self->through           = true;
    self->firstBar          = false;
    self->playing           = false;
    self->clip              = false;

    self->parameters[0]   = NULL;
    self->parameters[1]   = &self->divisionParam;
    self->parameters[2]   = &self->noteLengthParam;
    self->parameters[3]   = &self->octaveSpreadParam;
    self->parameters[4]   = &self->swingParam;
    self->parameters[5]   = &self->randomizeTimmingParam;
    self->parameters[6]   = &self->velocityModeParam;
    self->parameters[7]   = &self->velocityCurveParam;
    self->parameters[8]   = &self->curveDepthParam;
    self->parameters[9]   = &self->curveClipParam;
    self->parameters[10]  = &self->patternVel1Param;
    self->parameters[11]  = &self->patternVel2Param;
    self->parameters[12]  = &self->patternVel3Param;
    self->parameters[13]  = &self->patternVel4Param;
    self->parameters[14]  = &self->patternVel5Param;
    self->parameters[15]  = &self->patternVel6Param;
    self->parameters[16]  = &self->patternVel7Param;
    self->parameters[17]  = &self->patternVel8Param;

    return self;
}



static void
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
    StepSeq* self = (StepSeq*)instance;

    switch (port)
    {
        case PORT_ATOM_IN:
            self->port_events_in = (const LV2_Atom_Sequence*)data;
            break;
        case PORT_ATOM_OUT1:
            self->port_events_out1 = (LV2_Atom_Sequence*)data;
            break;
        case METRO_CONTROL:
            self->control = (LV2_Atom_Sequence*)data;
            break;
        case CVLFO1:
            self->lfo1PortParam = (const float*)data;
            break;
        case CVLFO2:
            self->lfo2PortParam = (const float*)data;
            break;
        case NOTEMODE:
            self->noteModeParam = (const float*)data;
            break;
        case MODE:
            self->modeParam = (const float*)data;
            break;
        case DIVISION:
            self->divisionParam = (const float*)data;
            break;
        case NOTELENGTH:
            self->noteLengthParam = (const float*)data;
            break;
        case OCTAVESPREAD:
            self->octaveSpreadParam = (const float*)data;
            break;
        case TRANSPOSE:
            self->latchTransposeParam = (const float*)data;
            break;
        case SWING:
            self->swingParam = (const float*)data;
            break;
        case RANDOMIZETIMMING:
            self->randomizeTimmingParam = (const float*)data;
            break;
        case VELOCITYMODE:
            self->velocityModeParam = (const float*)data;
            break;
        case VELOCITYCURVE:
            self->velocityCurveParam = (const float*)data;
            break;
        case CURVEDEPTH:
            self->curveDepthParam = (const float*)data;
            break;
        case CURVECLIP:
            self->curveClipParam = (const float*)data;
            break;
        case CURVELENGTH:
            self->curveLengthParam = (const float*)data;
            break;
        case VELOCITYPATTERNLENGTH:
            self->velocityPatternLengthParam = (const float*)data;
            break;
        case PATTERNVEL1:
            self->patternVel1Param = (const float*)data;
            break;
        case PATTERNVEL2:
            self->patternVel2Param = (const float*)data;
            break;
        case PATTERNVEL3:
            self->patternVel3Param = (const float*)data;
            break;
        case PATTERNVEL4:
            self->patternVel4Param = (const float*)data;
            break;
        case PATTERNVEL5:
            self->patternVel5Param = (const float*)data;
            break;
        case PATTERNVEL6:
            self->patternVel6Param = (const float*)data;
            break;
        case PATTERNVEL7:
            self->patternVel7Param = (const float*)data;
            break;
        case PATTERNVEL8:
            self->patternVel8Param = (const float*)data;
            break;
        case LFO1CONNECT:
            self->lfo1ConnectParam = (const float*)data;
            break;
        case LFO1DEPTH:
            self->lfo1DepthParam = (const float*)data;
            break;
        case LFO2CONNECT:
            self->lfo2ConnectParam = (const float*)data;
            break;
        case LFO2DEPTH:
            self->lfo2DepthParam = (const float*)data;
            break;
    }
}



static void
activate(LV2_Handle instance)
{

}


//create a midi message
static LV2_Atom_MIDI
createMidiEvent(StepSeq* self, uint8_t status, uint8_t note, uint8_t velocity)
{
    LV2_Atom_MIDI msg;
    memset(&msg, 0, sizeof(LV2_Atom_MIDI));

    msg.event.body.size = 3;
    msg.event.body.type = self->urid_midiEvent;

    msg.msg[0] = status;
    msg.msg[1] = note;
    msg.msg[2] = velocity;

    return msg;
}


static float
getParamMinRange(int param)
{
    float maxParamValue[18] =
    {0, 0, 0, 1, 25, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};

    return maxParamValue[param];
}



static float
getParamMaxRange(int param)
{
    float maxParamValue[18] =
    {0, 10, 0.99, 4, 75, 1, 2, 70, 1, 0, 127, 127, 127, 127, 127, 127, 127, 127};

    return maxParamValue[param];
}



static void
applyLfoToParameters(StepSeq* self)
{
    size_t amountParam = 18;
    float lfoValue = 0;
    int param;


    for (size_t parameters = 1; parameters < amountParam; parameters++) {
        self->variables[parameters] = **self->parameters[parameters]; 
    }


    param = (int)*self->lfo1ConnectParam;
    if (param > 0) {
        lfoValue = getParamMaxRange(param) * *self->lfo1DepthParam * *self->lfo1PortParam; 
        self->variables[param] = **self->parameters[param] + lfoValue; 
        self->variables[param] = applyRange(self->variables[param], getParamMinRange(param), getParamMaxRange(param));
    }
    param = (int)*self->lfo2ConnectParam;
    if (param > 0) {
        lfoValue = getParamMaxRange(param) * *self->lfo2DepthParam * *self->lfo2PortParam; 
        self->variables[param] = **self->parameters[param] + lfoValue; 
        self->variables[param] = applyRange(self->variables[param], getParamMinRange(param), getParamMaxRange(param));
    }


    self->division           = self->variables[1];
    self->noteLength         = self->variables[2];
    //debug_print("note length = %f\n", self->noteLength);
    self->octaveSpread       = self->variables[3];
    self->swing              = self->variables[4];
    self->randomizeTimming   = self->variables[5];
    self->velocityMode       = (int)self->variables[6];
    self->velocityCurve      = self->variables[7];
    self->curveDepth         = self->variables[8];
    self->curveClip          = self->variables[9];
    self->velocityPattern[0] = (uint8_t)self->variables[10];
    self->velocityPattern[1] = (uint8_t)self->variables[11];
    self->velocityPattern[2] = (uint8_t)self->variables[12];
    self->velocityPattern[3] = (uint8_t)self->variables[13];
    self->velocityPattern[4] = (uint8_t)self->variables[14];
    self->velocityPattern[5] = (uint8_t)self->variables[15];
    self->velocityPattern[6] = (uint8_t)self->variables[16];
    self->velocityPattern[7] = (uint8_t)self->variables[17];
}



    static void
update_position(StepSeq* self, const LV2_Atom_Object* obj)
{
    const MetroURIs* uris = &self->uris;

    // Received new transport position/speed
    LV2_Atom *beat = NULL, *bpm = NULL, *speed = NULL, *barsize = NULL;
    lv2_atom_object_get(obj,
            uris->time_barBeat, &beat,
            uris->time_beatsPerMinute, &bpm,
            uris->time_speed, &speed, uris->time_beatsPerBar, &barsize,
            NULL);

    static int previousSpeed = 0;

    if (bpm && bpm->type == uris->atom_Float) {
        // Tempo changed, update BPM
        self->bpm = ((LV2_Atom_Float*)bpm)->body;
    }
    if (speed && speed->type == uris->atom_Float) {
        // Speed changed, e.g. 0 (stop) to 1 (play)
        self->speed = ((LV2_Atom_Float*)speed)->body;
    }
    if (beat && beat->type == uris->atom_Float) {
        const float bar_beats       = ((LV2_Atom_Float*)beat)->body;
        const float beat_beats      = bar_beats - floorf(bar_beats);
        const float beat_barsize    = ((LV2_Atom_Float*)barsize)->body;
        self->beatInMeasure = ((LV2_Atom_Float*)beat)->body;
        self->barsize = beat_barsize;

        if (self->speed != previousSpeed) {
            self->phase = beat_beats;
            previousSpeed = self->speed;
        }
    }
}


//clear all arrays and set values back to initial state TODO check adding trigger?
static void
clearNotes(StepSeq *self, const uint32_t outCapacity)
{
    //TODO only note offs for notes that are currently being played
    for (size_t mNotes = 0; mNotes < 127; mNotes++) {
        LV2_Atom_MIDI msg = createMidiEvent(self, 128, mNotes, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&msg);
    }
}


static void
stopSequence(StepSeq* self)
{
    clearSequence(self->writeEvents);
    clearSequence(self->playEvents);

    self->writeEvents.used  = 0;
    self->playEvents.used   = 0;
    self->activeNotes        = 0;
    self->transpose          = 0;
    self->firstBar           = false;
    self->trigger            = false;
    self->octaveIndex        = 0;
    self->notePlayed         = 0;
    self->octaveIndex        = 0;
    self->noteTie            = 0;

    for (int y = 0; y < 2; y++) {
        self->noteStarted[y] = 0;
    }

    self->noteOffIndex     = 0;
    self->noteOffSendIndex = 0;
    self->activeNotes      = 0;
    self->cleared          = true;
}



static uint32_t
handlePorts(StepSeq* self)
{
    const uint32_t outCapacity = self->port_events_out1->atom.size;
    // Write an empty Sequence header to the outputs
    lv2_atom_sequence_clear(self->port_events_out1);
    self->port_events_out1->atom.type = self->port_events_in->atom.type;

    return outCapacity;
}


static EventList 
applyDifference(EventList play, EventList write)
{
    static bool different;
    //makes a copy of the event list if the there are new events
    different = checkDifference(play.eventList, write.eventList, play.used, write.used);

    if (different)
    {
        return play = copyEvents(write, play);
        different = false;
    } else {
        return play;
    }
}


static float
applyRandomTiming(float depth)
{
    float direction;

    if ((rand() % 2) == 1)
        direction = 1;
    else
        direction = -1;
        
    return depth * direction * ((rand() % 50) * 0.06);
}


static uint8_t
octaveHandler(StepSeq* self)
{
    uint8_t octave = 12 * self->octaveIndex;
    self->octaveIndex = (self->octaveIndex + 1) % (int)self->octaveSpread;

    return octave;
}


static uint8_t
velocityHandler(StepSeq* self)
{

    //TODO create function for velocity handling
    if (self->velocityMode == 0) {
        self->velocity = 80;
    } else if (self->velocityMode == 1) { 
        self->velocity = 127 + (int)floor(((self->velocityLFO) - 127) * self->curveDepth);
    } else if (self->velocityMode == 2) {
        self->velocity = floor(self->velocityPattern[self->patternIndex]);
    }

    self->patternIndex = (self->patternIndex + 1) % (int)*self->velocityPatternLengthParam; 

    if (self->clip)
        self->clip = false;

    return self->velocity;
}

static void
handleNoteOn(StepSeq* self, const uint32_t outCapacity)
{
    static bool   alreadyPlaying = false;
    static size_t noteFound      = 0;
    //get octave and velocity

    if ( self->playEvents.eventList[self->notePlayed][0] > 0 && self->playEvents.eventList[self->notePlayed][0] < 128)
    {
        uint8_t octave = octaveHandler(self);
        uint8_t velocity = velocityHandler(self);

        //create MIDI note on message
        uint8_t midiNote = self->playEvents.eventList[self->notePlayed][0] + self->transpose + octave;

        if (self->recordingMetaData) {
            self->metaData = recordTranspose(self->metaData, self->transpose, self->notePlayed, self->phase);
        }
        //check if note is already playing
        for (size_t i = 0; i < 4; i++) {
            if ((uint8_t)self->noteOffTimer[i][0] == midiNote) {
                self->noteOffTimer[i][1] = 0;
                alreadyPlaying = true;
                noteFound = i;
            } else {
                alreadyPlaying = false;
            }
        }
        //TODO remove static
        static size_t activeNoteIndex = 0;

        if (!alreadyPlaying) {
            //send MIDI note on message
            LV2_Atom_MIDI onMsg = createMidiEvent(self, 144, midiNote, velocity);
            lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&onMsg);

            if (self->noteTie > 0) {
                //sendNoteOff
                LV2_Atom_MIDI noteTieMsg = createMidiEvent(self, 128, self->noteTie, 0);
                lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&noteTieMsg);
                self->noteTie = 0;
            }

            if (self->playEvents.eventList[self->notePlayed][1] != 2) {
                self->noteOffTimer[activeNoteIndex][0] = (float)midiNote;
                activeNoteIndex = (activeNoteIndex + 1) % 4;
            } else {
                self->noteTie = midiNote;
            }
        } else {
            activeNoteIndex = (noteFound + 1) % 4;
        }

        //check for note tie else add to noteOffTimer
    } else if (self->noteTie > 0) {
        //TODO check for better structure, this code is included twice because it wasn't working when playing a rest
        LV2_Atom_MIDI noteTieMsg = createMidiEvent(self, 128, self->noteTie, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&noteTieMsg);
        self->noteTie = 0;
    }

    //set boolean for active notes send and set boolean for trigger to prevent multiple triggers
    self->cleared = false;
    self->trigger = true;

    //increment sequence index
    self->notePlayed++;
    self->notePlayed = (self->notePlayed > (self->playEvents.used - 1)) ? 0 : self->notePlayed;
    self->playHead++;
    self->playHead = ((self->phase > 0 && self->phase < 0.001) && (self->beatInMeasure < 0.01)) ? 0 : self->playHead;
}



static void
handleNoteOff(StepSeq* self, const uint32_t outCapacity)
{
    for (int i = 0; i < 4; i++) {
        if (self->noteOffTimer[i][0] > 0) {
            self->noteOffTimer[i][1] += self->frequency / self->rate;
            if (self->noteOffTimer[i][1] > self->noteLength) {
                LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, (uint8_t)self->noteOffTimer[i][0], 0);
                lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&offMsg);
                self->noteOffTimer[i][0] = 0;
                self->noteOffTimer[i][1] = 0;
            }
        }
    }
}



static int
switchMode(StepSeq* self, const uint32_t outCapacity)
{
    ModeEnum modeStatus    = (int)*self->modeParam;
    //TODO set normal value
    if ((int)*self->modeParam != self->prevMod || (int)*self->latchTransposeParam != self->prevLatch)
    {
        switch (modeStatus)
        {
            case CLEAR_ALL:
                clearNotes(self, outCapacity);
                stopSequence(self);
                self->playing    = false;
                self->modeHandle = 0;
                self->through    = true;
                break;
            case STOP:
                self->playing    = false;
                self->modeHandle = 0;
                self->through    = true;
                self->notePlayed = 0;
                break;
            case RECORD:
                self->firstRecordedNote = false;
                self->through           = true;
                self->modeHandle        = 6;
                break;
            case PLAY:

                if (self->writeEvents.used > 0)
                    self->playing = true;

                //set MIDI input through
                if ((int)*self->latchTransposeParam == 1 && self->playing == true) {
                    self->modeHandle = 3;
                    self->through = false;
                }
                else if ((int)*self->latchTransposeParam == 0 ) {
                    self->modeHandle = 5;
                    self->through = false;
                    self->playing = false;
                    clearNotes(self, outCapacity);
                } else {
                    self->through = true;
                    self->modeHandle = 0;
                }
                break;
            case RECORD_OVERWRITE:
                self->modeHandle    = 2;
                self->through = false;

                if ((int)*self->latchTransposeParam == 0 ) {
                    self->through = false;
                    self->playing = false;
                    clearNotes(self, outCapacity);
                } else {
                    self->playing = true;
                }
                break;
            case RECORD_APPEND:
                self->modeHandle    = 1;
                self->through = false;

                if ((int)*self->latchTransposeParam == 0 ) {
                    self->playing = false;
                    clearNotes(self, outCapacity);
                } else {
                    self->playing = true;
                }
                break;
            case UNDO_LAST:
                //TODO works but it should be aware of sequence
                self->writeEvents.used--;
                break;
        }
        self->prevMod = (int)*self->modeParam;
        self->prevLatch = (int)*self->latchTransposeParam;
    }
    //  if (*self->noteModeParam == 0) {
    //    modeHandle = 4;
    //  }

    return self->modeHandle;
}



static void
handleNotes(StepSeq* self, const uint8_t* const msg, uint8_t status, int modeHandle, uint32_t outCapacity, void* ev)
{

    switch (status)
    {
        uint8_t midiNote;

        case LV2_MIDI_MSG_NOTE_ON:
        self->notesPressed++;

        if ((uint8_t)*self->noteModeParam == 0)
            midiNote = 200;
        else
            midiNote = msg[1];

        switch (modeHandle)
        {
            case 0:
                break;
            case 1:
                self->writeEvents = insertNote(self->writeEvents, midiNote, (uint8_t)*self->noteModeParam);
                self->playing=true;
                break;
            case 2:
                //TODO does count needs to be reset?
                self->playing = true;
                self->writeEvents.eventList[self->count++ % self->writeEvents.used][0] = midiNote;
                self->writeEvents.eventList[self->count][1] = (uint8_t)*self->noteModeParam;
                break;
            case 3:
                if (midiNote < 128)
                    self->transpose = midiNote - self->writeEvents.eventList[0][0];
                break;
            case 4:
                //200 = rest
                self->writeEvents = insertNote(self->writeEvents, 200, (uint8_t)*self->noteModeParam);
                break;
            case 5:
                self->playing = true;
                self->transpose = midiNote - self->writeEvents.eventList[0][0];
                break;
            case 6:
                if (!self->firstRecordedNote) {
                    self->playing = false;
                    stopSequence(self);
                    self->writeEvents = insertNote(self->writeEvents, midiNote, (uint8_t)*self->noteModeParam);
                    self->firstRecordedNote = true;
                } else {
                    self->writeEvents = insertNote(self->writeEvents, midiNote, (uint8_t)*self->noteModeParam);
                }
                break;
        }
        break;

        case LV2_MIDI_MSG_NOTE_OFF:
        self->notesPressed--;
        if ((modeHandle == 5 || modeHandle == 2 || modeHandle == 1) && self->notesPressed == 0 && *self->latchTransposeParam == 0) {
            //debug_print("self->playing=false");
            self->playing = false;
            self->notePlayed = 0;
            clearNotes(self, outCapacity);
        }
        break;
    }

    //MIDI through
    if (self->through && (int)*self->noteModeParam > 0.0) {
        self->midiThroughInput[self->inputIndex++ % 16] = msg[1];
        lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, ev);
        self->prevThrough = 1;
    }
    else if (self->prevThrough == 1) {
        for (size_t i = 0; i < self->inputIndex + 1; i++) {
            //send note off
            //TODO does this need an init value?
            LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, self->midiThroughInput[i], 0);
            lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&offMsg);
        }
        self->inputIndex  = 0;
        self->prevThrough = 0;
    }
}



static void
sequenceProcess(StepSeq* self, const uint32_t outCapacity)
{
    self->playEvents = applyDifference(self->playEvents, self->writeEvents);
    //placement is used to control the amount of swing, the place of the of [0] is static the placement of note [1] can -
    //be moved
    self->notePlacement[1] = self->swing * 0.01;

    if (self->playing && self->firstBar)
    {
        float offset = applyRandomTiming(*self->randomizeTimmingParam);
        if (self->phase >= self->notePlacement[self->placementIndex] + offset && self->phase < (self->notePlacement[self->placementIndex] + 0.2 + offset)
                && !self->trigger && self->playEvents.used > 0)
        {
            handleNoteOn(self, outCapacity);
            self->triggerSet = false;
        } else
        { //if this is false: (self->phase < 0.2 && !trigger && self->writeEvents->used > 0)
            if (self->phase > self->notePlacement[self->placementIndex] + 0.2 + offset && !self->triggerSet)
            {
                self->placementIndex ^= 1;
                self->trigger = false;
                //TODO does this trigger has to be reset as well?
                self->triggerSet = true;
            }
        }
    }
    handleNoteOff(self, outCapacity);
}


//sequence the MIDI notes that are written into an array
static void
handleEvents(StepSeq* self, const uint32_t outCapacity)
{
    int modeHandle = switchMode(self, outCapacity);

    // Read incoming events
    LV2_ATOM_SEQUENCE_FOREACH(self->port_events_in, ev)
    {
        if (ev->body.type == self->urid_midiEvent)
        {
            const uint8_t* const msg = (const uint8_t*)(ev + 1);
            const uint8_t status = msg[0] & 0xF0;
            handleNotes(self, msg, status, modeHandle, outCapacity, ev);
        }
    }
}



static void
run(LV2_Handle instance, uint32_t n_samples)
{
    StepSeq* self = (StepSeq*)instance;
    self->port_events_out1->atom.type = self->port_events_in->atom.type;

    const MetroURIs* uris = &self->uris;

    // Work forwards in time frame by frame, handling events as we go
    const LV2_Atom_Sequence* in     = self->control;

    for (const LV2_Atom_Event* ev = lv2_atom_sequence_begin(&in->body);
            !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
            ev = lv2_atom_sequence_next(ev)) {

        if (ev->body.type == uris->atom_Object ||
                ev->body.type == uris->atom_Blank) {
            const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == uris->time_Position) {
                // Received position information, update
                update_position(self, obj);
            }
        }
    }
    self->frequency = calculateFrequency(self->bpm, self->division);
    //halftime speed when frequency goes out of range
    if (self->frequency > self->nyquist)
        self->frequency = self->frequency / 2;


    // Get the capacity
    const uint32_t outCapacity = handlePorts(self);

    //a phase Oscillator that we use for the tempo of the midi-sequencer
    for (uint32_t pos = 0; pos < n_samples; pos++) {
        applyLfoToParameters(self);
        self->phase = resetPhase(self);
        self->phase = *phaseOsc(self->frequency, &self->phase, self->rate);
        self->velocityLFO = *velOsc(self->frequency, &self->velocityLFO, self->rate, self->velocityCurve, self->curveDepth,
                self->curveLengthParam, self->curveClip, self);
        sequenceProcess(self, outCapacity);
    }
    handleEvents(self, outCapacity);
}



static void cleanup(LV2_Handle instance)
{
    free(instance);
}



static const LV2_Descriptor descriptor = {
    .URI = "http://moddevices.com/plugins/mod-devel/Step-Sequencer",
    .instantiate = instantiate,
    .connect_port = connect_port,
    .activate = activate,
    .run = run,
    .deactivate = NULL,
    .cleanup = cleanup,
    .extension_data = NULL
};




LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    return (index == 0) ? &descriptor : NULL;
}


