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
  MODE,
  DIVISION
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
  float beatInMeasure;
  const float* division;
  //==========================================================
  
  bool playing;
  int latchTranspose;
  int transpose;
  // URIDs
  LV2_URID urid_midiEvent;

  
	uint8_t *midiEventsOn;
  uint8_t *copiedEvents;
	uint8_t *recordEvents;
  size_t used;
  size_t size; 
  
	const float* mode;

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
   
    self->rate       = rate;
    self->bpm        = 120.0f;
    self->beatInMeasure = 0;
     
    //init midi event list
    self->midiEventsOn = (uint8_t *)malloc(1 * sizeof(uint8_t));
    self->copiedEvents = (uint8_t *)malloc(1 * sizeof(uint8_t));  
    self->used = 0;
    self->size = 1;

    self->transpose = 0;


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
  }
}



static void 
activate(LV2_Handle instance)
{

}


//phase oscillator to use for timing of the beatsync 
static float* 
phaseOsc(float frequency, float* phase, float rate)
{
  *phase += frequency / rate;
  
  if(*phase >= 1) *phase = *phase - 1; 
  
  return phase;
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



//static void 
//insertNote(Data* self, uint8_t note) 
//{ 
//  if (self->used == self->size) {
//    self->size *= 2;
//    self->midiEventsOn = (uint8_t *)realloc(self->midiEventsOn, self->size * sizeof(uint8_t));
//  }
//  self->midiEventsOn[self->used++] = note;
//}

static void 
insertNote(uint8_t* array, uint8_t note, size_t used, size_t size) 
{ 
  if (used == size) {
    size *= 2;
    array = (uint8_t *)realloc(array, size * sizeof(uint8_t));
  }
  array[used++] = note;
}


static void 
clearSequence(Data* self)
{
  free(self->midiEventsOn);
  self->midiEventsOn = NULL;
  self->used = self->size = 0;
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



static float 
calculateFrequency(uint8_t bpm, float division)
{
  float rateValues[11] = {15,20,30,40,60,80,120,160.0000000001,240,320.0000000002,480};
  float frequency = bpm / rateValues[(int)division];
 
  return frequency;
}



static bool 
checkDifference(uint8_t* arrayA, uint8_t* arrayB, size_t length)
{
  if (sizeof(arrayA) != sizeof(arrayB)) {
    return true;
  } else {
    for (size_t index = 0; index < length; index++) {
      if (arrayA[index] != arrayB[index]) { 
        return true;
      }
    }
  }
  return false;
}

//static void
//recordMIDI(Data* self, uint8_t notes)
//{ 
//  int recording = param;
//  
//  if (!paramchanged && beatpos == 0 ) 
//  {
//    self->midiEventsOn = self->recordedEvents;       
//  }
//  
//  switch (recording) 
//  {
//    case playing:
//      break;
//    case recording:
//      midiArray->insertNotes(self->recordedEvents, note);
//  }
//
//
//}

//sequence the MIDI notes that are written into an array
static void 
sequence(Data* self)
{
  static uint8_t midiNote = 0;
  static uint8_t prevNote = 0;
  static bool different;
  static bool first = false;
  static bool trigger = false;
  static bool cleared = true;
  static size_t notePlayed = 0;
  
  // Get the capacity
  const uint32_t out_capacity_1 = self->port_events_out1->atom.size;

  // Write an empty Sequence header to the outputs
  lv2_atom_sequence_clear(self->port_events_out1);

  self->port_events_out1->atom.type = self->port_events_in->atom.type;

  if (self->playing) 
  {
    
    different = checkDifference(self->copiedEvents, self->midiEventsOn, self->used);
   
    if (different)
    {
      self->copiedEvents = (uint8_t *)realloc(self->copiedEvents, self->used * sizeof(uint8_t));
 
      for (size_t noteIndex = 0; noteIndex < self->used; noteIndex++) {
        self->copiedEvents[noteIndex] = self->midiEventsOn[noteIndex];
      }   
      different = false;
    }

    if (self->phase < 0.2 && !trigger && self->used > 0) 
    {
      //create note on message
      midiNote = self->copiedEvents[notePlayed] + self->transpose;
      LV2_Atom_MIDI msg = createMidiEvent(self, 144, midiNote, 127);

      lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);
      
      //send note off
      if (first) {

        LV2_Atom_MIDI msg = createMidiEvent(self, 128, prevNote, 0);

        lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);

      }
      
      prevNote = midiNote;
      cleared = false;
      trigger = true;
      first = true;
      
      notePlayed++;
      notePlayed = (notePlayed > (self->used - 1)) ? 0 : notePlayed;
    
    } else {
      if (self->phase > 0.2 ) {
        trigger = false;    
      }   
    }
  } else { // self->playing = false, send note offs of current notes.
    
    if ( !cleared && *self->mode < 2 ) {
      for (size_t noteOffIndex = 0; noteOffIndex < 127; noteOffIndex++) {
        LV2_Atom_MIDI msg = createMidiEvent(self, 128, noteOffIndex, 0);
        lv2_atom_sequence_append_event(self->port_events_out1, out_capacity_1, (LV2_Atom_Event*)&msg);
      }

      clearSequence(self);    
      cleared = true;

    }
  }
}



static void 
run(LV2_Handle instance, uint32_t n_samples)
{
    Data* self = (Data*)instance;
    static float prevMod = 1;

    self->latchTranspose = 1;

    //TODO create seperate function for the switching
    //switch between the play/recording modes =========
  
    int modeStatus = (int)*self->mode;
    static int modeHandle = 0;

    if (*self->mode != prevMod) {
      switch (modeStatus)
      {
        case 0:
          self->playing = false;
          modeHandle = 0;
          break;
        case 1:
          self->playing = false;
          modeHandle = 1;  
          break;
        case 2:
          if (self->used > 0)
            self->playing = true;
          
          if (self->latchTranspose == 1 && self->playing == true) {
            modeHandle = 3;
          } else {
            modeHandle = 0;
          }
          break;
        case 3: 
          modeHandle = 1;
          self->playing = true;
          break;
        case 4:
          modeHandle = 2;
          self->playing = true;
          break;
        case 5:
          break;
      }
      prevMod = *self->mode;
    }
    
    //==============================================
    
    // Read incoming events
    LV2_ATOM_SEQUENCE_FOREACH(self->port_events_in, ev)
    {

      if (ev->body.type == self->urid_midiEvent)
      {
        const uint8_t* const msg = (const uint8_t*)(ev + 1);
        const uint8_t status  = msg[0] & 0xF0;

        static size_t count = 0;
        
        //TODO add record current loop
      
        switch (status)
        {
          case LV2_MIDI_MSG_NOTE_ON:
            
            switch (modeHandle)
            {
              case 0:
                break;
              case 1:
                insertNote(self->midiEventsOn, msg[1], self->used, self->size);
                break;
              case 2:
                self->midiEventsOn[count++ % self->used] = msg[1];
                break;
              case 3:
                self->transpose = msg[1] - self->midiEventsOn[0];
                break;
            }
            
            break;
          default:
            break;
        }
      }
    }

		//=========================================================================

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
    
    static float previousDevision;
    static float frequency; 
    static float divisionRate = 4;
    static bool resetPhase = true;

    if (self->beatInMeasure < 0.5 && resetPhase) {
      
      if (*self->division != previousDevision) {
        divisionRate = *self->division;  
        previousDevision = *self->division; 
      } 
      
      self->phase = 0.0;
      resetPhase = false;
    
    } else {
      if (self->beatInMeasure > 0.5) {
        resetPhase = true;
      } 
    }


    frequency = calculateFrequency(self->bpm, divisionRate);
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


