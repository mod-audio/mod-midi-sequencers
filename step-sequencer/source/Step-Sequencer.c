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

  self->rate           = rate;
  self->bpm            = 120.0f;
  self->beatInMeasure  = 0;
  self->divisionRate   = 4;
	self->phase          = 0;
  self->velPhase       = 0.998;
  self->x1             = 0.00000001; 
  self->velocityLFO    = 0;
  self->octaveIndex    = 0;  
	debug_print("test debug\n");
  //init objects
  self->writeEvents  = (Array* )malloc(sizeof(Array));
  self->playEvents   = (Array* )malloc(sizeof(Array));
  
  //init vars
  self->writeEvents->used  = 0;
  self->playEvents->used   = 0;

  self->notePlayed  = 0;
  self->transpose   = 0;
  
  self->through    = true;
  self->firstBar   = false;
  self->playing    = false;
  self->clip       = false;

	//init pointer for velocity pattern
	self->pattern[0] = &self->patternVel1;
	self->pattern[1] = &self->patternVel2;
	self->pattern[2] = &self->patternVel3;
	self->pattern[3] = &self->patternVel4;
	self->pattern[4] = &self->patternVel5;
	self->pattern[5] = &self->patternVel6;
	self->pattern[6] = &self->patternVel7;
	self->pattern[7] = &self->patternVel8;

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
		case METRO_CONTROL:
			self->control = (LV2_Atom_Sequence*)data;
			break;
		case NOTEMODE:
			self->noteMode = (const float*)data;
			break;
		case MODE:
			self->mode = (const float*)data;
			break;
		case DIVISION:
			self->division = (const float*)data;
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
		case VELOCITYMODE:
			self->velocityMode = (const float*)data;
			break;
		case VELOCITYCURVE:
			self->velocityCurve = (const float*)data;
			break;
		case CURVEDEPTH:
			self->curveDepth = (const float*)data;
			break;
		case CURVECLIP:
			self->curveClip = (const float*)data;
			break;
		case CURVELENGTH:
			self->curveLength = (const float*)data;
			break;
		case VELOCITYPATTERNLENGTH:
			self->velocityPatternLength = (const float*)data;
			break;
		case PATTERNVEL1:
			self->patternVel1 = (const float*)data;
			break;
		case PATTERNVEL2:
			self->patternVel2 = (const float*)data;
			break;
		case PATTERNVEL3:
			self->patternVel3 = (const float*)data;
			break;
		case PATTERNVEL4:
			self->patternVel4 = (const float*)data;
			break;
		case PATTERNVEL5:
			self->patternVel5 = (const float*)data;
			break;
		case PATTERNVEL6:
			self->patternVel6 = (const float*)data;
			break;
		case PATTERNVEL7:
			self->patternVel7 = (const float*)data;
			break;
		case PATTERNVEL8:
			self->patternVel8 = (const float*)data;
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


static int
switchMode(Data* self)
{
  static int modeHandle  = 0;
  ModeEnum modeStatus    = (int)*self->mode;  
  
  //if (*self->mode != prevMod || *self->latchTranspose == prevLatch) 
 // {
    switch (modeStatus)
    {
      case CLEAR_ALL:
        self->playing    = false;
        self->through    = true; 
        self->notePlayed = 0;
        modeHandle       = 0;
        break;
      case RECORD:
        self->playing    = false;
        self->through    = true; 
        self->notePlayed = 0;
        modeHandle       = 1;  
        break;
      case PLAY:
        if (self->writeEvents->used > 0)
          self->playing = true;
        
        //set MIDI input through 
        if (*self->latchTranspose == 1 && self->playing == true) {
          modeHandle = 3;
          self->through = false;
        } else {
          self->through = true;
          modeHandle = 0;
        }
        break;
      case RECORD_APPEND: 
        modeHandle    = 1;
        self->through = false;
        self->playing = true;
        break;
      case RECORD_OVERWRITE:
        modeHandle    = 2;
        self->through = false;
        self->playing = true;
        break;
      case UNDO_LAST:
        break;
    }
  //  prevMod = *self->mode;
 //   prevLatch = *self->latchTranspose;
 //
 // }
  if (*self->noteMode == 0) {
    modeHandle = 4;
  }

  return modeHandle;
}


static void 
handleNotes(Data* self, const uint8_t* const msg, uint8_t status, int modeHandle, uint32_t out_capacity_1, void* ev)
{
  static size_t count = 0;
  
	//MIDI through   
	if (self->through) {
    lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, ev);
  }
  
  switch (status)
  {
    case LV2_MIDI_MSG_NOTE_ON:

      switch (modeHandle)
      {
        case 0:
          break;
        case 1:
          insertNote(self->writeEvents, msg[1]);
          break;
        case 2:
          self->writeEvents->eventList[count++ % self->writeEvents->used] = msg[1];
          break;
        case 3:
          self->transpose = msg[1] - self->writeEvents->eventList[0];
          break;
        case 4:
          insertNote(self->writeEvents, 0);
          break;
      }
      break;
    case LV2_MIDI_MSG_NOTE_OFF:
      break;
    default:
      break;
  }
}

//sequence the MIDI notes that are written into an array
static void 
sequence(Data* self)
{
  static bool    different;
  static uint8_t noteOffArr[4] = {0, 0, 0, 0};
  static int  noteOffIndex  = 0; 
  static uint8_t midiNote      = 0;
  static bool    trigger       = false;
  static bool    cleared       = true;
  
  int modeHandle = switchMode(self);
  // Get the capacity
  const uint32_t out_capacity_1 = self->port_events_out1->atom.size;
  // Write an empty Sequence header to the outputs
  lv2_atom_sequence_clear(self->port_events_out1);
  self->port_events_out1->atom.type = self->port_events_in->atom.type;
  
  // Read incoming events
  LV2_ATOM_SEQUENCE_FOREACH(self->port_events_in, ev)
  {
    if (ev->body.type == self->urid_midiEvent)
    {
      const uint8_t* const msg = (const uint8_t*)(ev + 1);
      const uint8_t status = msg[0] & 0xF0;
      handleNotes(self, msg, status, modeHandle, out_capacity_1, ev);
 
    }
  }

  if (self->playing && self->firstBar) 
  {
    //makes a copy of the event list if the there are new events
    different = checkDifference(self->playEvents->eventList, self->writeEvents->eventList, self->playEvents->used, self->writeEvents->used);

    if (different)
    {
      copyEvents(self->writeEvents, self->playEvents);  
      different = false;
    }
    
    float offset = *self->randomizeTimming * ((rand() % 100) * 0.003);
    

    if (self->phase < 0.2 && self->phase > offset && !trigger && self->playEvents->used > 0) 
    {

      //TODO look for a cleaner way to filter out the rests 
      if ( self->playEvents->eventList[self->notePlayed] > 0)
      {
        int octave = 12 * self->octaveIndex - 12; 
        self->octaveIndex = (self->octaveIndex + 1) % (int)*self->octaveSpread;
        //create note on message
        midiNote = self->playEvents->eventList[self->notePlayed] + self->transpose + octave;
				
				static int patternIndex = 0;
				static int velocity;
       
        //TODO create function for velocity handling
        if (*self->velocityMode == 0) {
          velocity = 80;
        } else if (*self->velocityMode == 1) { 
          velocity = 127 + (int)floor(((self->velocityLFO) - 127) * *self->curveDepth);
				} else if (*self->velocityMode == 2) {
          velocity = (int)floor(**self->pattern[patternIndex]);
          patternIndex = (patternIndex + 1) % (int)floor(*self->velocityPatternLength); 
				}
         
        if (self->clip)
          self->clip = false;

        LV2_Atom_MIDI onMsg = createMidiEvent(self, 144, midiNote, velocity);
        lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&onMsg);

      }
      //prevNote = midiNote;
      noteOffArr[noteOffIndex] = midiNote;
      self->noteStarted[noteOffIndex] = 1;

      noteOffIndex ^= 1;
      self->activeNotes = self->activeNotes + 1;
      
      cleared = false;
      trigger = true;
      //increment sequence index 
      self->notePlayed++;
      self->notePlayed = (self->notePlayed > (self->playEvents->used - 1)) ? 0 : self->notePlayed;

    } else 
    { //if this is false: (self->phase < 0.2 && !trigger && self->writeEvents->used > 0)
      
      if (self->phase > 0.5) 
      {
        trigger = false;    
      }

    }
    
    static int noteOffSendIndex = 0;
    //send Note Off
    //for (int i = self->activeNotes - 1; i > - 1; i--) {
      
      if (self->noteLengthTime[noteOffSendIndex] > *self->noteLengthParam)
      { 
        LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, noteOffArr[noteOffSendIndex], 0);
        lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&offMsg);
        self->noteStarted[noteOffSendIndex] = 0;
        self->noteLengthTime[noteOffSendIndex] = 0.0; 
        self->activeNotes--;
        noteOffSendIndex ^= 1;
      }
   // }   
  } else 
  { // self->playing = false, send note offs of current notes.

    if ( !cleared && *self->mode < 2 ) {
      for (size_t mNotes = 0; mNotes < 127; mNotes++) {
        LV2_Atom_MIDI msg = createMidiEvent(self, 128, mNotes, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);
      }
      clearSequence(self->writeEvents);
      clearSequence(self->playEvents);
      
      //reset vars
      self->writeEvents->used  = 0;
      self->playEvents->used   = 0;
      self->activeNotes        = 0;
      self->transpose          = 0;
      self->firstBar           = false;
      self->octaveIndex        = 0;
     
      for (int i = self->activeNotes - 1; i > - 1; i--) {
        self->noteLengthTime[i] = 0.0;
      }

      noteOffIndex = 0;
      self->activeNotes = 0;
      cleared = true;
    }
  }
}



static void 
run(LV2_Handle instance, uint32_t n_samples)
{
  Data* self = (Data*)instance;

  static float frequency; 

	// LV2 is so nice...
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
  
  resetPhase(self);
  
  frequency = calculateFrequency(self->bpm, self->divisionRate);
  //a phase Oscillator that we use for the tempo of the midi-sequencer
  for (uint32_t pos = 0; pos < n_samples; pos++) {
    self->phase = *phaseOsc(frequency, &self->phase, self->rate, *self->swing);
    self->velocityLFO = *velOsc(frequency, &self->velocityLFO, self->rate, self->velocityCurve, self->curveDepth,
        self->curveLength, self->curveClip, self);
    for (int i = 0; i < 2; i++) {
      if (self->noteStarted[i] > 0)
        self->noteLengthTime[i] += frequency / self->rate;
    }
  }
  sequence(self);
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


