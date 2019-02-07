/*
 * =====================================================================================
 *
 *       Filename:  Classic-midi-sequencer.c
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
  uris->time_speed          = map->map(map->handle, LV2_TIME__speed);

  self->rate          = rate;
  self->bpm           = 120.0f;
  self->beatInMeasure = 0;
  self->divisionRate  = 4;	
	debug_print("test debug\n");
  //init objects
  self->recordEvents = (Array *)malloc(sizeof(Array));
  self->writeEvents  = (Array* )malloc(sizeof(Array));
  self->playEvents   = (Array* )malloc(sizeof(Array));
  //init arrays
  self->recordEvents->eventList = (uint8_t *)malloc(sizeof(uint8_t));
  self->writeEvents->eventList  = (uint8_t *)malloc(sizeof(uint8_t));
  self->playEvents->eventList   = (uint8_t *)malloc(sizeof(uint8_t));  
  //init vars
  self->writeEvents->used  = 0;
  self->writeEvents->size  = 1;
  self->recordEvents->used = 0;
  self->recordEvents->size = 1;
  self->playEvents->used   = 0;
  self->playEvents->size   = 1;

  self->notePlayed = 0;
  self->transpose  = 0;
  
  self->through    = true;
  self->firstBar   = false;
  self->playing    = false;
  self->recording  = false;
  
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
    case MODE:
      self->mode = (const float*)data;
      break;
    case DIVISION:
      self->division = (const float*)data;
      break;
    case RECORDBARS:
      self->recordBars = (const float*)data;
      break;
    case NOTELENGTH:
      self->noteLengthParam = (const float*)data;
      break;
    case TRANSPOSE:
      self->latchTranspose = (const float*)data;
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
  LV2_Atom *beat = NULL, *bpm = NULL, *speed = NULL;
  lv2_atom_object_get(obj,
      uris->time_barBeat, &beat,
      uris->time_beatsPerMinute, &bpm,
      uris->time_speed, &speed,
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
    self->beatInMeasure = ((LV2_Atom_Float*)beat)->body; 
    if (self->speed != previousSpeed) {

      self->phase = beat_beats;
      previousSpeed = self->speed;
    }
  }
}


static int
switchMode(Data* self)
{
  static float prevMod   = 1;
  static float prevLatch = 1;
  static int modeHandle  = 0;
  ModeEnum modeStatus    = (int)*self->mode;  
  
  //if (*self->mode != prevMod || *self->latchTranspose == prevLatch) 
 // {
    switch (modeStatus)
    {
      case CLEAR_ALL:
        self->playing    = false;
        self->recording  = false;
        self->through    = true; 
        self->notePlayed = 0;
        modeHandle       = 0;
        break;
      case RECORD:
        self->playing    = false;
        self->recording  = false;
        self->through    = true; 
        self->notePlayed = 0;
        modeHandle       = 1;  
        break;
      case PLAY:
        if (self->writeEvents->used > 0)
          self->playing = true;
        
        //set MIDI input through 
        if (*self->latchTranspose == 1 && self->playing == true) {
          self->through = false;
        } else {
          self->through = true;
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
 // }
  return modeHandle;
}


static void 
handleNotes(Data* self, const uint8_t* const msg, uint8_t status, int modeHandle, uint32_t out_capacity_1, void* ev)
{
  static size_t count = 0;

  //TODO add record current loop
  
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
          debug_print("note on = %i\n", msg[1]);
          break;
        case 2:
          self->writeEvents->eventList[count++ % self->writeEvents->used] = msg[1];
          break;
        case 3:
          self->transpose = msg[1] - self->writeEvents->eventList[0];
          debug_print("note on = %i\n", msg[1]);
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
  static size_t  noteOffIndex  = 0; 
  static float   noteLength    = 1;
  static uint8_t midiNote      = 0;
  static uint8_t prevNote      = 0;
  static bool    trigger       = false;
  static bool    cleared       = true;
  
  int modeHandle = switchMode(self);

  // Get the capacity
  const uint32_t out_capacity_1 = self->port_events_out1->atom.size;
  // Write an empty Sequence header to the outputs
  lv2_atom_sequence_clear(self->port_events_out1);
  self->port_events_out1->atom.type = self->port_events_in->atom.type;

  static float prevFloatLength = 0;
  
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

  if (*self->noteLengthParam != prevFloatLength){ 
    noteLength = 0.1 + (*self->noteLengthParam * 0.9);
    prevFloatLength = *self->noteLengthParam;
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
  
    if (self->phase >= noteLength || (self->phase < 0.2 && noteLength == 1 && !trigger))
    {
      //send note off
      if ( noteOffIndex > 0) 
      { 
        for (size_t i = 0; i < noteOffIndex; i++) 
        {
          LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, noteOffArr[i], 0);
          lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&offMsg);
        }
        noteOffIndex = 0;
      }
    }   
    
    if (self->phase < 0.2 && !trigger && self->playEvents->used > 0) 
    {
      //TODO this is a extra note off check, needs to be removed later...=============================== 
      if ( noteOffIndex > 0) 
      { 
        for (size_t i = 0; i < noteOffIndex; i++) 
        {
          LV2_Atom_MIDI offMsg = createMidiEvent(self, 128, noteOffArr[i], 0);
          lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&offMsg);
        }
        noteOffIndex = 0;
      }

      //=================================================================================================
      
      //create note on message
      midiNote = self->playEvents->eventList[self->notePlayed] + self->transpose;
      recordNotes(self, midiNote);
      LV2_Atom_MIDI onMsg = createMidiEvent(self, 144, midiNote, 127);
      lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&onMsg);

      //prevNote = midiNote;
      noteOffArr[noteOffIndex++ % 4] = midiNote;
      cleared = false;
      trigger = true;
      
      //increment sequence index 
      self->notePlayed++;
      self->notePlayed = (self->notePlayed > (self->playEvents->used - 1)) ? 0 : self->notePlayed;

    } else 
    { //if this is false: (self->phase < 0.2 && !trigger && self->writeEvents->used > 0)
      
      if (self->phase > 0.2 ) 
      {
        trigger = false;    
      }

    }
  } else 
  { // self->playing = false, send note offs of current notes.

    if ( !cleared && *self->mode < 2 ) {
      for (size_t mNotes = 0; mNotes < 127; mNotes++) {
        LV2_Atom_MIDI msg = createMidiEvent(self, 128, mNotes, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);
      }
      clearSequence(self->writeEvents);
      clearSequence(self->recordEvents);
      clearSequence(self->playEvents);
      

      //TODO create a function for this
      //init objects
      self->recordEvents = (Array *)malloc(sizeof(Array));
      self->writeEvents  = (Array* )malloc(sizeof(Array));
      self->playEvents   = (Array* )malloc(sizeof(Array));
      //init arrays
      self->recordEvents->eventList = (uint8_t *)malloc(sizeof(uint8_t));
      self->writeEvents->eventList  = (uint8_t *)malloc(sizeof(uint8_t));
      self->playEvents->eventList   = (uint8_t *)malloc(sizeof(uint8_t));  
      //init vars
      self->writeEvents->used  = 0;
      self->writeEvents->size  = 1;
      self->recordEvents->used = 0;
      self->recordEvents->size = 1;
      self->playEvents->used   = 0;
      self->playEvents->size   = 1;
      self->firstBar = false;
      cleared = true;
    }
  }
}



static void 
run(LV2_Handle instance, uint32_t n_samples)
{
  Data* self = (Data*)instance;

  static float frequency; 
 
	// Get the capacity
	const uint32_t out_capacity_1 = self->port_events_out1->atom.size;

	// Write an empty Sequence header to the outputs
	//lv2_atom_sequence_clear(self->port_events_out1);

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
    self->phase = *phaseOsc(frequency, &self->phase, self->rate);
  }
  sequence(self);
}



static void cleanup(LV2_Handle instance)
{
  free(instance);
}



static const LV2_Descriptor descriptor = {
  .URI = "http://moddevices.com/plugins/mod-devel/Classic-MIDI-Sequencer",
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


