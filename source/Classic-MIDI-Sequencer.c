/*
 */

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"


#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


typedef struct {
  LV2_URID atom_Blank;
  LV2_URID atom_Float;
  LV2_URID atom_Object;
  LV2_URID atom_Path;
  LV2_URID atom_Resource;
  LV2_URID atom_Sequence;
  LV2_URID time_Position;
  LV2_URID time_barBeat;
  LV2_URID time_beatsPerMinute;
  LV2_URID time_speed;
} MetroURIs;


typedef enum {
  PORT_ATOM_IN = 0,
  PORT_ATOM_OUT1,
  METRO_CONTROL,
  MODE
} PortEnum;


typedef struct {

  //syncVars=================================================
  LV2_URID_Map*  map;     // URID map feature
  MetroURIs      uris;    // Cache of mapped URIDs
  LV2_Atom_Sequence* control;

  double rate;   // Sample rate
  float bpm;
  float speed; // Transport speed (usually 0=stop, 1=play)
  float phase;
  //==========================================================
  
  // was last run on the 2nd output?
  bool was_second;
  bool playing;

  // URIDs
  LV2_URID urid_midiEvent;

  // aray to save midi events
  //uint8_t midiEventsOn[64];
  
	uint8_t *midiEventsOn;
  size_t used;
  size_t size; 
  
	const int* mode;
  // control ports
  const float* port_target;

  // atom ports
  const LV2_Atom_Sequence* port_events_in;
  LV2_Atom_Sequence* port_events_out1;
} Data;


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
    //LV2_URID_Map* const map   = self->map;
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
   // Initialise instance fields
    self->rate       = rate;
    self->bpm        = 120.0f;
     
    //init midi event list
    self->midiEventsOn = (uint8_t *)malloc(1 * sizeof(uint8_t));
    self->used = 0;
    self->size = 1;


    return self;
}



static void connect_port(LV2_Handle instance, uint32_t port, void* data)
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
      self->mode = (const int*)data;
  }
}



static void activate(LV2_Handle instance)
{
   // Data* self = (Data*)instance;
}


//phase oscillator to use for timing of the beatsync 
static float* phaseOsc(float frequency, float* phase, float rate)
{
  *phase += frequency / rate;
  //wrap phase 
  if(*phase >= 1) *phase = *phase - 1; 
  
  return phase;
}


//create a midi message 
static LV2_Atom_MIDI createMidiEvent(Data* self, uint8_t status, uint8_t note, uint8_t velocity)
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


static void insertNote(Data* self, uint8_t note) {
  if (self->used == self->size) {
    self->size *= 2;
    self->midiEventsOn = (uint8_t *)realloc(self->midiEventsOn, self->size * sizeof(uint8_t));
  }
  self->midiEventsOn[self->used++] = note;
}

static void clearSequence(Data* self)
{
  free(self->midiEventsOn);
  self->midiEventsOn = NULL;
  self->used = self->size = 0;
}
//sequence the MIDI notes that are written into an array
static void sequence(Data* self)
{
  static bool first = false;
  static bool trigger = false;
  static int i = 0;

  // Get the capacity
  const uint32_t out_capacity_1 = self->port_events_out1->atom.size;

  // Write an empty Sequence header to the outputs
  lv2_atom_sequence_clear(self->port_events_out1);
  
  // LV2 is so nice...
  self->port_events_out1->atom.type = self->port_events_in->atom.type;
  

  if (self->phase < 0.2 && !trigger && self->playing == true) {
    //create note on message
    LV2_Atom_MIDI msg = createMidiEvent(self, 144, self->midiEventsOn[i % 4], 127);
    
    lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);

    //send note off
    if ( first == true ) {

      LV2_Atom_MIDI msg = createMidiEvent(self, 128, self->midiEventsOn[(i + 3) % 4 ], 0);

      lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);

    }
    trigger = true;
    first = true;
    i++;
  } else {
    if (self->phase > 0.2 ) {
      trigger = false;    
    }   
  }
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
    // Received a beat position, synchronise
    // This hard sync may cause clicks, a real plugin would be more graceful
    const float frames_per_beat = 60.0f / self->bpm * self->rate;
    const float bar_beats       = ((LV2_Atom_Float*)beat)->body;
    const float beat_beats      = bar_beats - floorf(bar_beats);  
   
    if (self->speed != previousSpeed) {

      self->phase = beat_beats;
      previousSpeed = self->speed;
     // printf("beat_beats = %f\n", beat_beats);
     // printf("phase = %f\n", self->phase);  
    }
  }
}


static void run(LV2_Handle instance, uint32_t n_samples)
{
    Data* self = (Data*)instance;
    static bool init = 0;
    
    //set playing to false when opening the plugin to prevent it from looping from the start
    if ( init == 0 ) {
      self->playing = false;
      init = 1;
    }
    
    float frequency = self->bpm / 60;

    //a phase Oscillator that we use for the tempo of the midi-sequencer 
    for (uint32_t pos = 0; pos < n_samples; pos++) {
      self->phase = *phaseOsc(frequency, &self->phase, self->rate);
    }
  
    //const bool target_second = (*self->port_target) > 0.5f;

    // Read incoming events
    LV2_ATOM_SEQUENCE_FOREACH(self->port_events_in, ev)
    {

      if (ev->body.type == self->urid_midiEvent)
      {
        const uint8_t* const msg = (const uint8_t*)(ev + 1);
        const uint8_t status  = msg[0] & 0xF0;

        static int count = 0;

        switch (status)
        {
          case LV2_MIDI_MSG_NOTE_ON:
            insertNote(self, msg[1]);
            //self->midiEventsOn[count % 4] = msg[1];
            count++;
            if (count >= 4) {
              self->playing = true;
            }
            break;
          default:
            break;
        }
      }
    }

		//=========================================================================

		 //Metro*           self = (Metro*)instance;
		 const MetroURIs* uris = &self->uris;

		 // Work forwards in time frame by frame, handling events as we go
		 const LV2_Atom_Sequence* in     = self->control;
		 uint32_t                 last_t = 0;
		 for (const LV2_Atom_Event* ev = lv2_atom_sequence_begin(&in->body);
				 !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
				 ev = lv2_atom_sequence_next(ev)) {

			 // Play the click for the time slice from last_t until now
			 //play(self, last_t, ev->time.frames);


			 // Check if this event is an Object
			 // (or deprecated Blank to tolerate old hosts)
			 if (ev->body.type == uris->atom_Object ||
					 ev->body.type == uris->atom_Blank) {
				 const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
				 if (obj->body.otype == uris->time_Position) {
					 // Received position information, update
					 update_position(self, obj);
				 }
			 }

    // Update time for next iteration and move to next event
   // last_t = ev->time.frames;
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


