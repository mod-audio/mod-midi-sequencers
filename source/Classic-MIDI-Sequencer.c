/*
 * =====================================================================================
 *
 *       Filename:  classic-midi-sequencer.c
 *
 *    Description: 
 *       Compiler:  gcc
 *
 *         Author:  Bram Giesen (), bram@moddevices.com
 *   Organization:  MOD Devices
 *
 * =====================================================================================
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

#define PLUGIN_URI "http://moddevices.com/plugins/mod-devel/mod-classic-midi-sequencer"

typedef enum {
  MIDI_IN      = 0,
  MIDI_OUT     = 1,
  CONTROL      = 2,
  MODE         = 3, 
  DIVISIONS    = 4,
  INPUTMODE    = 5, 
  NOTELENGTH   = 6,
  OCTAVESPREAD = 7,
  LATCH        = 8
} PortIndex;

typedef struct {
  LV2_URID atom_Blank;
  LV2_URID atom_Float;
  LV2_URID atom_Object;
  LV2_URID atom_Path;
  LV2_URID atom_Resource;
  LV2_URID atom_Sequence;
  LV2_URID atom_Position;
  LV2_URID atom_barBeat;
  LV2_URID atom_beatsPerMinute;
  LV2_URID time_speed;
} ClockURIs;

typedef struct {
  LV2_URID_Map*  map;
  LV2_Log_Log*   log;  
  LV2_Log_Logger logger;
  ClockURIs      uris;

  //midi-clock vars bpm etc..


} Clock;


static void 
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
  Clock* self = (Clock*)instance;

  //switch ((PortIndex)Port) {
  //  case //PORT
  //    self->var = (float*)data;
  //    break;
  //} 
}

//init 
static void 
activate(LV2_Handle instance)
{
  Clock* self = (Clock*)instance;
  


}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
  Clock* self = (Clock*)calloc(1, sizeof(Clock));
  if(!self)
  {
    return NULL;
  }

  for (uint32_t i=0; features[i]; ++i) {
    if (!strcmp (features[i]->URI, LV2_URID_map)) 
    {
      self->map = (LV2_URID_Map*)features[i]->data;
    } 
    else if (!strcmp (features[i]->URI, LV2_LOG_log)) 
    {
      self->log = (LV2_Log_Log*)features[i]->data;
    }
  }

  lv2_log_logger_init (&self->logger, self->map, self->log);

  if (!self->map) {
    lv2_log_error (&self->logger, "error: Host doesn't support urid:map\n");
    free (self);
    return NULL;
  }

  //Map Uris
  ClockURIs* const    uris = &self->uris;
  LV2_URID_Map* const map  = self->map;
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

  return (LV2_Handle)self;
}

static void 
run(LV2_Handle instace, uint32_t n_samples)
{
  Clock* self = (Clock*)instance;
  const ClockURIs* uris = &self->uris;

}

static void 
deactivate(LV2_Handle instance)
{
}

static void
cleanup(LV2_Handle instance)
{
  free(instance)
}

static const void*
extension_data(const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  PLUGIN_URI,
  instantiate,
  connect_port,
  activate,
  run, 
  deactivate,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  switch (index) {
    case 0:
      return &descriptor;
    default:
      return NULL;
  }
}
