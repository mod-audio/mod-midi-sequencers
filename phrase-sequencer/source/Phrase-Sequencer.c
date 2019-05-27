/*
 * =====================================================================================
 *
 *       Filename:  Phrase-Sequencer.c
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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
    Data* self = (Data*)calloc(1, sizeof(Data));

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
    self->beat             = 0;
    self->barCount         = 0;
    self->recordingStatus  = 0;
    self->division         = 0;
    self->phase            = 0;
    self->sinePhase        = 0;
    self->amplitude        = 0;
    self->phaseRecord      = 0;
    self->velPhase         = 0.000000009;
    self->x1               = 0.00000001; 
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
    self->activeNoteIndex  = 0;
    self->inputIndex       = 0;
    self->notesPressed     = 0;
    //check this value
    self->prevThrough      = 0;

    self->placementIndex   = 0;
    self->notePlacement[0] = 0;
    self->notePlacement[1] = 0.5;
    self->previousSpeed    = 0;

    //resetPhase vars:
    self->previousDevision = 12;
    self->previousPlaying = false;
    self->resetPhase      = true;

    for (size_t i = 0; i < 16; i++) {
        self->midiThroughInput[i] = 0;
    }

    debug_print("DEBUG MODE\n");
    //init objects
    self->ARStatus     = IDLE;

    self->recordingLengths[0] = &self->preCountLength;
    self->recordingLengths[1] = &self->recordingLength;

    for (size_t row = 0; row < 4; row++) {
        for (size_t index = 0; index < 2; index++) {
            self->noteOffTimer[row][index] = 0;
        }
    }

    for (size_t voice = 0; voice < 4; voice++) {
        for (uint8_t note = 0; note < 248; note++) {
            for (size_t noteProp = 0; noteProp < 2; noteProp++) {
                self->writeEvents.eventList[voice][note][noteProp] = 0;
                self->playEvents.eventList[voice][note][noteProp] = 0;
            }
        }
    }
    //init vars
    self->writeEvents.used  = 0;
    self->playEvents.used   = 0;

    self->notePlayed  = 0;
    self->transpose   = 0;
    self->noteFound   = 0;


    self->firstRecordedNote  = false; 
    self->barCounted         = false;
    self->startPreCount      = false;
    self->recording          = false; 
    self->trigger            = false;
    self->triggerSet         = false;
    self->preCountTrigger    = false;
    self->cleared            = true; 
    self->through            = true;
    self->firstBar           = false;
    self->playing            = false;
    self->clip               = false;
    self->alreadyPlaying     = false;
    self->recordingTriggered = false;

	self->pos = 0;
	self->period = 0;
	self->h_wavelength = 0;

    self->barCounter = 0;
    self->barNotCounted = false;
    
    return self;
}



static void 
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
    Data* self = (Data*)instance;

    switch (port)
    {
        case PORT_ATOM_IN:
            self->port_events_in = (const LV2_Atom_Sequence*)data;
            break;
        case PORT_ATOM_OUT1:
            self->port_events_out1 = (LV2_Atom_Sequence*)data;
            break;
        case PORT_METROME_OUT:
            self->metroOut = (float*)data; 
            break;
        case METRO_CONTROL:
            self->control = (LV2_Atom_Sequence*)data;
            break;
        case MODE:
            self->mode = (const float*)data;
            break;
        case ACTIVATERECORDING:
            self->recordTrigger = data;
            break;
        case PRECOUNT:
            self->preCountLength = (const float*)data;
            break;
        case RECORDINGLENGTH:
            self->recordingLength = (const float*)data;
            break;
        case DIVISION:
            self->changeDiv = (const float*)data;
            break;
        case NOTELENGTH:
            self->noteLengthParam = (const float*)data;
            break;
        case OCTAVESPREAD:
            self->octaveSpread = (const float*)data;
            break;
        case TRANSPOSE:
            self->latchTranspose = (const float*)data;
            break;
        case SWING:
            self->swing = (const float*)data;
            break;
        case RANDOMIZETIMMING:
            self->randomizeTimming = (const float*)data;
            break;
    }
}



static void 
activate(LV2_Handle instance)
{

}


//create a midi message 
static LV2_Atom_MIDI 
createMidiEvent(Data* self, uint8_t status, uint8_t note, uint8_t velocity)
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



static void
update_position(Data* self, const LV2_Atom_Object* obj)
{
    const MetroURIs* uris = &self->uris;

    // Received new transport position/speed
    LV2_Atom *beat = NULL, *bpm = NULL, *speed = NULL, *barsize = NULL;
    lv2_atom_object_get(obj,
            uris->time_barBeat, &beat,
            uris->time_beatsPerMinute, &bpm,
            uris->time_speed, &speed, uris->time_beatsPerBar, &barsize, 
            NULL);

    self->previousSpeed = 0; 

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
        self->beat                  = bar_beats - floorf(bar_beats);
        self->beatInMeasure = ((LV2_Atom_Float*)beat)->body; 
        self->barsize = beat_barsize; 

        if (self->speed != self->previousSpeed) {
            self->phase = beat_beats;
            self->previousSpeed = self->speed;
        }
    }
}


//clear all arrays and set values back to initial state TODO check adding trigger?
static void
clearNotes(Data *self, const uint32_t outCapacity)
{
    //TODO only note offs for notes that are currently being played
    for (size_t mNotes = 0; mNotes < 127; mNotes++) {
        LV2_Atom_MIDI msg = createMidiEvent(self, 128, mNotes, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&msg);
    }
}


static void
stopSequence(Data* self)
{
}



static uint32_t 
handlePorts(Data* self)
{
    const uint32_t outCapacity = self->port_events_out1->atom.size;
    // Write an empty Sequence header to the outputs
    lv2_atom_sequence_clear(self->port_events_out1);
    self->port_events_out1->atom.type = self->port_events_in->atom.type;

    return outCapacity; 
}



static float
applyRandomTiming(Data* self)
{
    return *self->randomizeTimming * ((rand() % 100) * 0.003);
}



static void 
handleNoteOn(Data* self, const uint32_t outCapacity)
{
    //get octave and velocity
    for (size_t voice = 0; voice < 4; voice++)
    {
        if ( self->playEvents.eventList[voice][self->notePlayed][0] > 0 
                && self->playEvents.eventList[voice][self->notePlayed][0] < 128)
        {
            uint8_t octave = 0;
            uint8_t velocity = 100;

            //create MIDI note on message
            uint8_t midiNote = (uint8_t)self->playEvents.eventList[voice][self->notePlayed][0] 
                + self->transpose + octave;

            //check if note is already playing
            for (size_t i = 0; i < 16; i++) {
                if ((uint8_t)self->noteOffTimer[i][0] == midiNote) { 
                    self->noteOffTimer[i][1] = 0;
                    self->alreadyPlaying = true;
                    self->noteFound = i; 
                } else {
                    self->alreadyPlaying = false;
                }
            }

            if (!self->alreadyPlaying) {
                //send MIDI note on message
                LV2_Atom_MIDI onMsg = createMidiEvent(self, 144, midiNote, velocity);
                lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&onMsg);
                self->noteOffTimer[self->activeNoteIndex][0] = (float)midiNote;
                self->noteOffTimer[self->activeNoteIndex][2] = self->playEvents.eventList[voice][self->notePlayed][1];
                self->activeNoteIndex = (self->activeNoteIndex + 1) % 16; 
            } else {
                self->activeNoteIndex = (self->noteFound + 1) % 16; 
            }
        } 
    } 
    //set boolean for active notes send and set boolean for trigger to prevent multiple triggers
    self->cleared = false;
    self->trigger = true;
    //increment sequence index 
    self->notePlayed++;
    self->notePlayed = (self->notePlayed > (self->playEvents.used - 1)) ? 0 : self->notePlayed;
}



static void
handleNoteOff(Data* self, const uint32_t outCapacity)
{
    for (int i = 0; i < 16; i++) {
        if (self->noteOffTimer[i][0] > 0) {
            self->noteOffTimer[i][1] += self->frequency;
            //self->noteOffTimer[i][2]
            if (self->noteOffTimer[i][1] > (self->noteOffTimer[i][2])) {
                //debug_print("self->noteOffTimer[%i][2] = %f\n", i, self->noteOffTimer[i][2]);
                //debug_print("note Off given for = %i\n", (uint8_t)self->noteOffTimer[i][0]);
                LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, (uint8_t)self->noteOffTimer[i][0], 0);
                lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&offMsg);
                self->noteOffTimer[i][0] = 0;
                self->noteOffTimer[i][1] = 0;
                self->noteOffTimer[i][2] = 0;
            }
        }  
    }
}



static void 
setMode(Data* self, const uint32_t outCapacity)
{
    ModeEnum modeStatus = (int)*self->mode;
    //TODO set normal value 
    if ((int)*self->mode != self->prevMod || (int)*self->latchTranspose != self->prevLatch) 
    {
        switch (modeStatus)
        {
            case CLEAR_ALL:
                clearNotes(self, outCapacity);
                stopSequence(self);
                self->playing    = false;
                self->through    = true; 
                break;
            case STOP:
                self->recordingStatus = 0;
                self->notePlayed      = 0;
                self->through         = true; 
                self->playing         = false;
                break;
            case PLAY:
                if (self->writeEvents.used > 0 && !self->recording)
                    self->playing = true;
                break;
            case RECORD_OVERWRITE:
                break;
            case RECORD_APPEND: 
                break;
            case UNDO_LAST:
                //TODO works but it should be aware of sequence
                self->writeEvents.used--;
                break;
        }
        self->prevMod = (int)*self->mode;
        self->prevLatch = (int)*self->latchTranspose;
    }
    if (*self->recordTrigger == 1 && !self->recordingTriggered) {
        self->recordingTriggered = true;
        self->startPreCount = true;
    }
}


/*function that handles the process of starting the pre-count at the beginning of next bar,
  pre-count length and recording length.*/
static void 
handleBarSyncRecording(Data *self, uint32_t pos)
{
    static bool countBars   = false;
    static int barsCounted  = 0;
    static int fullRecordingLength = 0;
    double frequency;

    //TODO change 3.9 to a procentage of the total size of the bar
    //enable counting of bars right before the of the current bar
    if (self->beatInMeasure > 3.9 && self->startPreCount) {
        countBars = true;
    }

    //count amount of bars to record included pre-count
    if (countBars) {
        barsCounted = barCounter(self, 1);
        if (barsCounted > **self->recordingLengths[0]) {
            self->recordingStatus = 3;
        } else if (barsCounted == (**self->recordingLengths[0] - 1)) {
            self->recordingStatus = 2;
        } else if (barsCounted > (**self->recordingLengths[0] - 2)) {
            self->recordingStatus = 1;
        }
    }

    //stop recording 
    if (barsCounted > (**self->recordingLengths[1] + **self->recordingLengths[0])) {
        countBars = false;
        self->recordingStatus = 4;
        barsCounted = 0;
    }

    RecordEnum recordMode = self->recordingStatus;
    
    switch(recordMode)
    {
        case R_IDLE:
            frequency = 0;
            break;
        case R_PRE_COUNT:
            precount(self);
            frequency = 660;
            break;
        case R_PRE_RECORDING:
            frequency = 660;
            precount(self);
            if (self->beatInMeasure > 3.5) {
                self->recording = true;
            }
            break;
        case R_RECORDING: //record
            frequency = 440;
            precount(self);
            self->phaseRecord = *phaseRecord(self->frequency, &self->phaseRecord, self->rate);
            fullRecordingLength = (int)self->phaseRecord;
            //debug_print("self->frequency = %f\n", self->frequency);
            break;
        case R_STOP_RECORDING: //stop recording 
            //TODO check if this is always safe
            fullRecordingLength += 1;
            self->phaseRecord = 0;
            self->writeEvents.used = fullRecordingLength;
            debug_print("phaseRecord = %i\n", fullRecordingLength);
            countBars = false;
            frequency = 0;
            self->recording = false;
            self->recordingTriggered = false;
            self->startPreCount = false;
            self->writeEvents = calculateNoteLength(self->writeEvents, self->rate);
            quantizeNotes(self); //TODO has to return a strutct
            //if (dub) {
            //    mergeEvents(&self->writeEvents, &self->playEvents);
            //}
            copyEvents(&self->writeEvents, &self->playEvents);
            //for (size_t i = 0; i < self->playEvents.used; i++) {
            //    for (size_t y = 0; y < 4; y++) {
            //        debug_print("self->playEvents->eventList[%li][%li][0] = %f\n", y, i, self->playEvents.eventList[y][i][0]);
            //    }
            //}
            //    for (size_t i = 0; i < self->playEvents.used; i++) {
            //        for (size_t y = 0; y < 4; y++) {
            //        debug_print("self->playEvents->eventList[%li][%li][1] = %f\n", y, i, self->playEvents.eventList[y][i][1]);
            //    }
            //}
            //    for (size_t i = 0; i < self->playEvents.used; i++) {
            //        for (size_t y = 0; y < 4; y++) {
            //        debug_print("self->playEvents->eventList[%li][%li][2] = %f\n", y, i, self->playEvents.eventList[y][i][2]);
            //    }
            //}
            self->playing = true;
            self->recordingStatus = 0;
            break;
       // case R_DUB:
       //     self->recording = true;
       //     self->phaseRecord = *phaseRecord(self->frequency, &self->phaseRecord, self->rate);
       //     break;

    }
    self->metroOut[pos] = 0.1 * *envelope(self, &self->amplitude) * (float)sinOsc(frequency, &self->sinePhase, self->rate);
    //TODO maybe return current switch state, so the call to the self->phaseRecord can be moved to the run function 
}



static size_t*
countNotesPressed(uint8_t status, size_t *notesPressed)
{
    switch (status)
    {
        case LV2_MIDI_MSG_NOTE_ON:
            notesPressed++;  
            break;
        case LV2_MIDI_MSG_NOTE_OFF:
            notesPressed--;
            break;
    }

    return notesPressed;
}



static void 
midiThrough(Data* self, const uint8_t* const msg, uint8_t status, int modeHandle, uint32_t outCapacity, void* ev)
{
    if (self->through) {
        self->midiThroughInput[self->inputIndex++ % 16] = msg[1]; 
        lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, ev);
        self->prevThrough = 1;
    } else if (self->prevThrough == 1) {
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
sequencerProcess(Data* self, const uint32_t outCapacity)
{
    self->notePlacement[1] = *self->swing * 0.01;

    float offset = applyRandomTiming(self);
    if (self->phase >= self->notePlacement[self->placementIndex] && 
            self->phase < (self->notePlacement[self->placementIndex] + 0.2) && !self->trigger) 
    { 
        handleNoteOn(self, outCapacity); 
        self->triggerSet = false;
    } else if (self->phase > self->notePlacement[self->placementIndex] + 0.2 && !self->triggerSet) 
    {
        self->placementIndex ^= 1;
        self->trigger = false;
        //TODO does this trigger has to be reset as well?
        self->triggerSet = true;    
    }
    handleNoteOff(self, outCapacity); 
}

static float 
getDivisionHz(int divisionIndex)
{
  float rateValues[11] = {240,160.0000000001,120,80,60,40,30,20,15,10,7.5};

  return rateValues[divisionIndex];
}

static float
getDivisionFrames(int divisionIndex)
{
  float rateValues[11] = {0.5,0.75,1,1.5,2,3,4,6,8,12,16};

  return rateValues[divisionIndex];
}

    
static void 
run(LV2_Handle instance, uint32_t n_samples)
{
    Data* self = (Data*)instance;
    self->port_events_out1->atom.type = self->port_events_in->atom.type;
    const MetroURIs* uris = &self->uris;

    // Work forwards in time frame by frame, handling events as we go
    const LV2_Atom_Sequence* in     = self->control;

    //get the capacity
    const uint32_t outCapacity = handlePorts(self); 
        
    for (uint32_t i = 0; i < n_samples; i++) {
        //reset phase when playing starts or stops
        if (self->speed != self->previousSpeed) {
            self->pos = reCalcPhase(self->bpm, self->beatInMeasure, self->rate, getDivisionFrames(self->division)); 
            self->previousSpeed = self->speed;
            debug_print("transport changed\n");
        }

        debug_print("self->division = %f\n", self->division);
        debug_print("self->changeDiv = %f\n", *self->changeDiv);
        //reset phase when there is a new division
        if (self->division != *self->changeDiv) {
            self->division = *self->changeDiv;
            self->pos = reCalcPhase(self->bpm, self->beatInMeasure, self->rate, getDivisionFrames(self->division)); 
            debug_print("Division changed\n");
        }

        self->period = (uint32_t)(self->rate * (60.0f / (self->bpm * (self->division / 2.0f))));
        self->h_wavelength = (self->period/2.0f);

        //update host information
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
        setMode(self, outCapacity);
        self->frequency = calculateFrequency(self->bpm, getDivisionHz(self->division));
        self->frequency = (self->frequency > self->nyquist) ? self->frequency / 2 : self->frequency; 
        //self->phase = *phaseOsc(self->frequency, &self->phase, self->rate, *self->swing);
        handleBarSyncRecording(self, i);
        if (self->playing) { 

            if(self->pos >= self->period && i < n_samples) {
                self->pos = 0;
            } else if(self->pos < self->h_wavelength && !self->trigger) {
                debug_print("NOTE ON send!");
                handleNoteOn(self, outCapacity);
                self->trigger = true;
            } else if(self->pos > self->h_wavelength && self->trigger) {
                self->trigger = false;
            }
            self->pos += 1;
        }           
        handleNoteOff(self, outCapacity);
        //sequencerProcess(self, outCapacity);
    }
    // Read incoming events
    LV2_ATOM_SEQUENCE_FOREACH(self->port_events_in, ev)
    {
        if (ev->body.type == self->urid_midiEvent)
        {
            const uint8_t* const msg = (const uint8_t*)(ev + 1);
            const uint8_t status = msg[0] & 0xF0;
            uint8_t midiNote = msg[1];
            uint8_t noteType = msg[0];
            int modeHandle = self->modeHandle;
            midiThrough(self, msg, status, modeHandle, outCapacity, ev);
            self->notesPressed = *countNotesPressed(status, &self->notesPressed);
            if (self->recording) {
                recordNotes(self, midiNote, noteType, self->phaseRecord);
            }
        }
    }
}



static void cleanup(LV2_Handle instance)
{
    free(instance);
}



static const LV2_Descriptor descriptor = {
    .URI = "http://moddevices.com/plugins/mod-devel/Phrase-Sequencer",
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


