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
#include <unistd.h>
#include "sequencer_utils.h"
#include "oscillators.h"

// Struct for a 3 byte MIDI event
typedef struct {
    LV2_Atom_Event event;
    uint8_t        msg[3];
} LV2_Atom_MIDI;



static void
printEventList(EventList events)
{
    for (size_t i = 0; i < events.used; i++) {
        debug_print("self->events->eventList[%li][0] = %i\n", i, events.eventList[i][0]);
        debug_print("self->events->eventList[%li][1] = %i\n", i, events.eventList[i][1]);
        debug_print("self->events->eventList[%li][2] = %i\n", i, events.eventList[i][2]);
        debug_print("self->events->eventList[%li][3] = %i\n", i, events.eventList[i][3]);
        debug_print("self->events->eventList[%li][4] = %i\n", i, events.eventList[i][4]);
    }
}



static float
getDivisionFrames(int divisionIndex)
{
  float rateValues[11] = {0.5,0.75,1,1.5,2,3,4,6,8,12,16};

  return rateValues[divisionIndex];
}



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


    for (size_t i = 0; i < 16; i++) {
        self->midiThroughInput[i] = 0;
    }

    debug_print("DEBUG MODE\n");
    //init objects
    self->ARStatus     = IDLE;

    self->recordingLengths[0] = &self->preCountLength;
    self->recordingLengths[1] = &self->recordingLength;

    self->playEvents.amountOfProps     = 5;
    self->storedEvents.amountOfProps   = 5;
    self->mergedEvents.amountOfProps   = 5;
    self->recordedEvents.amountOfProps = 5;

    for (size_t row = 0; row < 4; row++) {
        for (size_t index = 0; index < 2; index++) {
            self->noteOffTimer[row][index] = 0;
        }
    }

    for (size_t voice = 0; voice < 4; voice++) {
        for (uint8_t note = 0; note < 248; note++) {
            for (size_t noteProp = 0; noteProp < self->storedEvents.amountOfProps; noteProp++) {
                self->recordedEvents.eventList[note][noteProp] = 0;
                self->mergedEvents.eventList[note][noteProp] = 0;
                self->storedEvents.eventList[note][noteProp] = 0;
                self->playEvents.eventList[note][noteProp] = 0;
            }
        }
    }
    //init vars
    self->playEvents.used     = 0;
    self->storedEvents.used   = 0;
    self->mergedEvents.used   = 0;
    self->recordedEvents.used = 0;
    self->notePlayed          = 0;
    self->transpose           = 0;
    self->noteFound           = 0;
    self->beat                = 0;
    self->barCount            = 0;
    self->recordingStatus     = 0;
    self->division            = 0;
    self->phase               = 0;
    self->sinePhase           = 0;
    self->amplitude           = 0;
    self->phaseRecord         = 0;
    self->velocity            = 0;
    self->octaveIndex         = 0;
    self->noteOffIndex        = 0;
    self->noteOffSendIndex    = 0;
    self->modeHandle          = 0;
    self->activeNoteIndex     = 0;
    self->inputIndex          = 0;
    self->notesPressed        = 0;
    self->prevThrough         = 0;//check this value
    self->previousSpeed       = 0;
    self->fullRecordingLength = 0;
    self->recordedFrames      = 0;
    self->applyMomentaryFx    = 0;
    self->prevRecordTrigger   = 0;
    self->pos                 = 0;
    self->period              = 0;
    self->h_wavelength        = 0;
    self->barCounter          = 0;
    self->recordingPos        = 0;
    self->periodCounter       = 0;
    self->previousDevision    = 12;
    self->prevMod             = 100;
    self->prevLatch           = 100;
    self->recordingBpm        = 120;
    self->rate                = rate;
    self->nyquist             = rate / 2;
    self->bpm                 = 120.0f;
    self->previousPlaying     = false;
    self->resetPhase          = true;
    self->cleared             = true;
    self->through             = true;
    self->recordingLengthSet  = false;
    self->firstRecordedNote   = false;
    self->barCounted          = false;
    self->startPreCount       = false;
    self->recording           = false;
    self->trigger             = false;
    self->triggerSet          = false;
    self->preCountTrigger     = false;
    self->firstBar            = false;
    self->playing             = false;
    self->playingEnabled      = false;
    self->alreadyPlaying      = false;
    self->recordingTriggered  = false;
    self->recordingEnabled    = false;
    self->barNotCounted       = false;

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
        case FX_MODE:
            self->fxMode = (const float*)data;
            break;
        case MOMENTARY_FX:
            self->momentaryFx = (const float*)data;
            break;
    }
}



static void
activate(LV2_Handle instance)
{

}



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
        static bool initBpm = false;
        if (!initBpm) {
            self->previousBpm = self->bpm;
            initBpm = true;
        }
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


static void
clearNotes(Data *self, const uint32_t outCapacity)
{
    //TODO only note offs for notes that are currently being played
    for (size_t mNotes = 0; mNotes < 127; mNotes++) {
        LV2_Atom_MIDI msg = createMidiEvent(self, 128, mNotes, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&msg);
    }
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
    if ( self->playEvents.eventList[self->notePlayed][0] > 0
            && self->playEvents.eventList[self->notePlayed][0] < 128)
    {
        uint8_t octave = 0;
        uint8_t velocity = 100;

        //create MIDI note on message
        uint8_t midiNote = (uint8_t)self->playEvents.eventList[self->notePlayed][0]
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
            self->noteOffTimer[self->activeNoteIndex][2] = self->playEvents.eventList[self->notePlayed][4];
            self->playEvents.eventList[self->notePlayed][0] = 0;
            self->playEvents.eventList[self->notePlayed][1] = 0;
            self->playEvents.eventList[self->notePlayed][2] = 0;
            self->playEvents.eventList[self->notePlayed][3] = 0;
            self->playEvents.eventList[self->notePlayed][4] = 0;
            self->activeNoteIndex = (self->activeNoteIndex + 1) % 16;
        } else {
            self->activeNoteIndex = (self->noteFound + 1) % 16;
        }
    }
    self->cleared = false;
    self->trigger = true;
}



static void
handleNoteOff(Data* self, const uint32_t outCapacity, float factor)
{
    for (int i = 0; i < 16; i++) {
        if (self->noteOffTimer[i][0] > 0) {
            self->noteOffTimer[i][1] += 1;
            if (self->noteOffTimer[i][1] > (self->noteOffTimer[i][2] * factor)) {
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
                self->recordingLengthSet = false;
                self->recordedEvents = clearSequence(self->recordedEvents);
                self->mergedEvents   = clearSequence(self->mergedEvents);
                self->storedEvents   = clearSequence(self->storedEvents);
                self->playEvents     = clearSequence(self->playEvents);
                self->notePlayed     = 0;
                self->playing        = false;
                self->playingEnabled = false;
                self->playing    = false;
                self->through    = true;
                //self->startPreCount        = false;
                break;
            case STOP:
                self->recordingStatus = 0;
                self->notePlayed      = 0;
                self->through         = true;
                self->playing         = false;
                //self->startPreCount   = false;
                self->playingEnabled  = false;
                break;
            case PLAY:
                self->playingEnabled = true;
            case RECORD_OVERWRITE:
                break;
            case RECORD_APPEND:
                break;
            case UNDO_LAST:
                //TODO write function that remembers state
                break;
        }
        self->prevMod = (int)*self->mode;
        self->prevLatch = (int)*self->latchTranspose;
    }
    if (*self->recordTrigger == 1 && !self->recordingTriggered) {
        self->recordingEnabled = !self->recordingEnabled;
        if (self->recordingEnabled) {
            self->startPreCount = true;
        }
        self->recordingTriggered = true;
    }
    if (*self->recordTrigger == 0) {
        self->recordingTriggered = false;
    }
    self->prevRecordTrigger = *self->recordTrigger;
}


static size_t
findClosestBarLength(int result, int div)
{
    int iteration = 1;
    while (true)
    {
        int numberToMatch1 = div * iteration;
        int difference1 = numberToMatch1 - result;
        iteration++;
        int numberToMatch2 = div * iteration;
        int difference2 = numberToMatch2 - result;

        if (difference2 > 0 || difference1 > 0) {
            if (abs(difference1) < abs(difference2)) {
                return (size_t)numberToMatch1;
            } else {
                return (size_t)numberToMatch2;
            }
        }
    }
}



static float
calculateCurrentRecordingPhase(float beatInMeasure, uint8_t division, size_t recordingLength, size_t barCounter)
{
    float phase = beatInMeasure * division + ((recordingLength * barCounter) % recordingLength);

    return phase;
}



static size_t
calculateCurrentPlayHeadPos(float bpm, float beatInMeasure, uint8_t division, size_t recordingLength, size_t barCounter)
{
    size_t notePlayed = (size_t)round((bpm / (bpm / (division * 2))) * (beatInMeasure / 4) * barCounter);

    return notePlayed;
}



static EventList 
renderRecording(Data* self, long int fullRecordingLength)
{
    //fullRecordingLength = findClosestBarLength(fullRecordingLength, 16);//TODO remove hardcoded value
    //self->writeEvents.used = fullRecordingLength;
    self->phaseRecord        = 0;
    //self->recording          = false;
    self->recordingTriggered = false;
    self->startPreCount      = false;
    self->recordedEvents     = calculateNoteLength(self->recordedEvents, self->rate, fullRecordingLength);
    printEventList(self->recordedEvents);
    self->recordedEvents     = quantizeNotes(self->recordedEvents);
    self->storedEvents       = mergeEvents(self->recordedEvents, self->storedEvents, self->mergedEvents);
    self->playEvents         = copyEvents(self->storedEvents, self->playEvents);
    self->mergedEvents       = clearSequence(self->mergedEvents);
    self->recordedEvents     = clearSequence(self->recordedEvents);
    self->notePlayed         = 0;

    return self->playEvents;
}



static float
calculateRecordingPhase(float beatInMeasure, uint8_t division, size_t recordingLength, size_t barCounter)
{
    float phase = beatInMeasure * division + (recordingLength * barCounter);

    return phase;
}



static void
resetRecordingValues(Data *self)
{
    self->startPreCount   = false;
    //self->recordingTriggered = false;
    self->recordingStatus = 0;
    self->phaseRecord     = 0;
    self->recordingStatus = 0;

    self->playing         = true;
}



/*function that handles the process of starting the pre-count at the beginning of next bar,
  pre-count length and recording length.*/
static void
handleBarSyncRecording(Data *self, uint32_t pos)
{
    //TODO remove all statics later
    int firstLoopLength;
    static bool counting           = false;
    static int fullRecordingLength = 0;
    static double frequency        = 0;

    //TODO change 3.9 to a procentage of the total size of the bar
    //enable counting of bars right before the of the current bar
    if (self->beatInMeasure > 3.9 && self->startPreCount) {
          counting = true;
    }

    //count amount of bars to record included pre-count
    if (counting) {
        self->barCounter = barCounter(self, self->beatInMeasure, self->barCounter);
        self->recordingPos = periodCounter(self->period, self->recordingPos, self->periodCounter);
        if (self->recordingEnabled) {
            if (self->barCounter > **self->recordingLengths[0]) {
                self->recordingStatus = 3;
            } else if (self->barCounter == (**self->recordingLengths[0] - 1)) {
                self->recordingStatus = 2;
            } else if (self->barCounter > (**self->recordingLengths[0] - 2)) {
                self->recordingStatus = 1;
            }
        }
        self->recordedFrames += 1;
    }

    //firstLoopLength = self->recordedEvents.used / 16; //TODO Doesn't work with frames

    if (**self->recordingLengths[1] > 0) {
        if (self->barCounter > (**self->recordingLengths[1] + **self->recordingLengths[0])) {
            self->barCounter      = **self->recordingLengths[0] + 1;
            self->recordingStatus = 4;
        }
    }

    if (!self->recordingEnabled && self->playing) {
        self->recordingStatus = 0;
        self->recording       = false;
    }


    //else {
    //    if (barsCounted > (**self->recordingLengths[1] + firstLoopLength ) && firstLoopLength > 0) {
    //        barsCounted = **self->recordingLengths[0]; //TODO lock this variable
    //        self->barCounter = **self->recordingLengths[0];
    //        //self->barNotCounted = false;
    //        self->recordingStatus = 4;
    //    } else if (!self->recordingEnabled && self->recording && self->beatInMeasure > 3.9) {
    //        self->recording = false;
    //        self->recordingStatus = 4;
    //    }
    //}

    RecordEnum recordMode = self->recordingStatus;


    static int previousRecordingStatus = -1;

    if (self->recordingStatus != previousRecordingStatus || self->recordingStatus == 2) {
        switch(recordMode)
        {
            case R_IDLE:
                frequency = 0;
                break;
            case R_PRE_COUNT:
                frequency = 660;
                break;
            case R_PRE_RECORDING:
                frequency            = 660;
                self->phaseRecord    = 0;
                if (self->beatInMeasure > 3.5) {
                    self->recording  = true;
                }
                self->recordedFrames = 0;
                break;
            case R_RECORDING:
                frequency            = 440;
                self->phaseRecord    = 0;
                self->recordedFrames = 0;
                self->recording      = true;
                break;
            case R_STOP_RECORDING:
                frequency = 0;
                if (!self->recordingLengthSet) {
                    debug_print("recording Length in frames = %li\n", self->recordedFrames);
                    debug_print("self->period * 32 = %i\n", self->period * 32);
                    self->fullRecordingLength = self->recordedFrames;
                    self->previousFactorBpm   = self->bpm;
                    self->recordingBpm        = self->bpm;
                    self->recordingLengthSet  = true;
                }
                self->playEvents = renderRecording(self, self->fullRecordingLength);
                self->recordedFrames = 0;
                self->pos        = 0;
                break;
        }
        previousRecordingStatus = self->recordingStatus;
    }
    if (self->playingEnabled && self->playEvents.used > 0) { //
        self->playing = true;
    }
    metronome(self);
    self->metroOut[pos] = 0.1 * *envelope(self, &self->amplitude) * (float)sinOsc(frequency, &self->sinePhase, self->rate);
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
            //TODO does this need an init value?
            LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, self->midiThroughInput[i], 0);
            lv2_atom_sequence_append_event(self->port_events_out1, outCapacity, (LV2_Atom_Event*)&offMsg);
        }
        self->inputIndex  = 0;
        self->prevThrough = 0;
    }
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
            //self->pos = reCalcPhase(self->bpm, self->beatInMeasure, self->rate, getDivisionFrames(self->division));
            self->previousSpeed = self->speed;
            debug_print("transport changed\n");
        }

        //reset phase when there is a new division
        if (*self->momentaryFx > 0 && *self->fxMode > 0) {
            self->division = *self->fxMode;
        } else if (self->division != *self->changeDiv) {
            self->notePlayed = calculateCurrentPlayHeadPos(self->bpm, self->beatInMeasure, 8, 32, 1); //TODO remove hard coded
            self->division   = *self->changeDiv;
            //self->pos = reCalcPos(self->bpm, self->beatInMeasure, self->rate, getDivisionFrames(self->division)); 
            debug_print("Division changed\n");
        }

        static bool posReset = false;

        if (self->pos < 10 && self->beatInMeasure < 0.01 && !posReset) {
            self->pos = 0;
            posReset  = true;
        } else if (self->beatInMeasure > 3.9) {
            posReset = false;
        }

        self->period = (uint32_t)(self->rate * (60.0f / (self->bpm * (getDivisionFrames(self->division) / 2.0f))));
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
        handleBarSyncRecording(self, i);
        static float factor = 1;
        static float posFactor = 1;
        if (self->playing) {
            factor                  = self->recordingBpm / self->bpm;
            posFactor               = self->previousFactorBpm / self->bpm;
            self->pos               = (uint32_t)(self->pos * posFactor);
            self->previousFactorBpm = self->bpm;
            self->previousBpm       = self->bpm;

            uint32_t nextNoteTriggerValue = (uint32_t)(self->playEvents.eventList[self->notePlayed][3] * factor);

            if(self->pos >= (uint32_t)(self->fullRecordingLength * factor)) { //TODO create function for this
                self->pos = 0; 
            } else {
                if (self->pos > nextNoteTriggerValue && self->playEvents.eventList[self->notePlayed][0] > 0 ) {
                    if (self-> pos < nextNoteTriggerValue + 300) {
                        handleNoteOn(self, outCapacity);
                        self->notePlayed = (self->notePlayed + 1) % self->playEvents.used;
                    } else {
                        self->notePlayed = (self->notePlayed + 1) % self->playEvents.used;
                    }
                }
            }
        }
        self->pos += 1;
        handleNoteOff(self, outCapacity, factor);
    }
    // Read incoming events
    LV2_ATOM_SEQUENCE_FOREACH(self->port_events_in, ev)
    {
        if (ev->body.type == self->urid_midiEvent)
        {
            const uint8_t* const msg = (const uint8_t*)(ev + 1);
            const uint8_t status = msg[0] & 0xF0;
            uint8_t midiNote   = msg[1];
            uint8_t velocity   = msg[2];
            uint8_t noteType   = msg[0];
            int modeHandle     = self->modeHandle;
            midiThrough(self, msg, status, modeHandle, outCapacity, ev);
            self->notesPressed = *countNotesPressed(status, &self->notesPressed);
            if (self->recording) {
                self->recordedEvents = recordNotes(self->recordedEvents, midiNote, velocity, noteType, self->recordedFrames);
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

