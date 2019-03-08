/*
 * =====================================================================================
 *
 *       Filename:  structs.h
 *
 *    Description:  file containing structs
 *
 *        Version:  1.0
 *        Created:  12/25/2018 02:22:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Bram Giesen (), bram@moddevices.com
 *   Organization:  MOD Devices
 *
 * =====================================================================================
 */

#ifndef _H_STRUCTS_
#define _H_STRUCTS_

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
#include <stdint.h>

#ifndef DEBUG
#define DEBUG 0
#endif
#define debug_print(...) \
((void)((DEBUG) ? fprintf(stderr, __VA_ARGS__) : 0))

typedef enum PortEnum {
  PORT_ATOM_IN = 0,
  PORT_ATOM_OUT1,
  METRO_CONTROL,
	NOTEMODE,
  MODE,
  DIVISION,
  NOTELENGTH,
  TRANSPOSE,
  SWING,
  RANDOMIZETIMMING,
  VELOCITYCURVE,
  CURVEDEPTH,
  CURVELENGTH
} PortEnum;

typedef enum ModeEnum {
  CLEAR_ALL = 0,
  RECORD,
  PLAY,
  RECORD_APPEND,
  RECORD_OVERWRITE,
  UNDO_LAST
} ModeEnum;

typedef struct MetroURIs {
  LV2_URID atom_Blank;
  LV2_URID atom_Float;
  LV2_URID atom_Object;
  LV2_URID atom_Path;
  LV2_URID atom_Resource;
  LV2_URID atom_Sequence;
  LV2_URID time_Position;
  LV2_URID time_barBeat;
  LV2_URID time_beatsPerMinute;
  LV2_URID time_beatsPerBar;
  LV2_URID time_speed;
} MetroURIs;

typedef struct Array {
  uint8_t *eventList;
  size_t used;
  size_t size;
} Array;

typedef struct Data {

  double rate;   // Sample rate
  float  bpm;
  float  barsize;
  float  speed; // Transport speed (usually 0=stop, 1=play)
  float  phase;
  float  velocityLFO;
  float  noteLengthTime[2];
  int    activeNotes;
  int    noteStarted[2];
  float  beatInMeasure;
  float  divisionRate;
  size_t notePlayed;
  bool   through;
  bool   firstBar;
  bool   playing;
  int    transpose;

  Array *writeEvents;
  Array *playEvents;

  const float* noteMode;
  const float* mode;
  const float* division;
  const float* noteLengthParam;
  const float* latchTranspose;
  const float* swing;
  const float* randomizeTimming;
  const float* velocityCurve;
  const float* curveDepth;
  const float* curveLength;
  const LV2_Atom_Sequence* port_events_in;
  LV2_Atom_Sequence*       port_events_out1;

  LV2_URID           urid_midiEvent;
  LV2_URID_Map*      map;     // URID map feature
  LV2_Atom_Sequence* control;
  MetroURIs          uris;    // Cache of mapped URIDs

} Data;
#endif //_H_STRUCTS_
