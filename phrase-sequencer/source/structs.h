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
  PORT_METROME_OUT,
  METRO_CONTROL,
	NOTEMODE,
  MODE,
  DIVISION,
  NOTELENGTH,
  OCTAVESPREAD,
  TRANSPOSE,
  SWING,
  RANDOMIZETIMMING,
	VELOCITYMODE,
  VELOCITYCURVE,
  CURVEDEPTH,
  CURVECLIP,
  CURVELENGTH,
	VELOCITYPATTERNLENGTH,
	PATTERNVEL1,
	PATTERNVEL2,
	PATTERNVEL3,
	PATTERNVEL4,
	PATTERNVEL5,
	PATTERNVEL6,
	PATTERNVEL7,
	PATTERNVEL8
} PortEnum;

typedef enum ModeEnum {
  CLEAR_ALL = 0,
  STOP,
  RECORD,
  PLAY,
  RECORD_OVERWRITE,
  RECORD_APPEND,
  UNDO_LAST
} ModeEnum;

typedef enum AttackReleaseEnum {
  IDLE = 0,
  ATTACK,
  RELEASE
} AttackReleaseEnum;

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
  uint8_t eventList[248][2];
  size_t used;
} Array;

typedef struct Data {

  double  rate;   // Sample rate
  double  frequency;
  double  nyquist;
  double  velPhase;
  double  x1;
  double  phase;
  double  sinePhase;
  double  phaseRecord;
  double  velocityLFO;
  float   *metroOut;
  float   amplitude;
  float   bpm;
  float   barsize;
  float   beat;
  float   speed; // Transport speed (usually 0=stop, 1=play)
  float   noteLengthTime[2];
  int     activeNotes;

  int     modeHandle;
  int     prevMod;
  int     prevLatch;
  
  int     placementIndex;
  float   notePlacement[2];
  
  uint8_t noteTie;
  uint8_t velocity;
  int     noteStarted[2];
  uint8_t noteOffArr[4];
  float   noteOffTimer[4][2];
  float   beatInMeasure;
  float   divisionRate;

  size_t  count;
  size_t  inputIndex;
  size_t  notesPressed;
  uint8_t prevThrough;
  uint8_t midiThroughInput[16];

  uint8_t ARstate;
  //resetPhase vars:
  float previousDevision;
  bool  startPreCount;
  bool  recording;
  bool  previousPlaying;
  bool  resetPhase;

  size_t  patternIndex;
  size_t  notePlayed;
  size_t  octaveIndex;
  size_t  noteOffIndex;
  size_t  noteOffSendIndex;
  bool    firstRecordedNote;
  bool    through;
  bool    firstBar;
  bool    playing;
  bool    clip;
  bool    trigger;
  bool    triggerSet;
  bool    preCountTrigger;
  bool    cleared;
  int     transpose;
  int     countTicks;
	
	const float** pattern[8];
  Array* writeEvents;
  Array* playEvents;
  AttackReleaseEnum  ARStatus;

  const float* noteMode;
  const float* mode;
  const float* division;
  const float* noteLengthParam;
  const float* latchTranspose;
  const float* swing;
  const float* randomizeTimming;
	const float* velocityMode;
  const float* velocityCurve;
  const float* curveDepth;
  const float* curveLength;
  const float* curveClip;
  const float* octaveSpread;
	const float* velocityPatternLength;
	const float*   patternVel1;
	const float*   patternVel2;
	const float*   patternVel3;
	const float*   patternVel4;
	const float*   patternVel5;
	const float*   patternVel6;
	const float*   patternVel7;
	const float*   patternVel8;

  const LV2_Atom_Sequence* port_events_in;
  LV2_Atom_Sequence*       port_events_out1;

  LV2_URID           urid_midiEvent;
  LV2_URID_Map*      map;     // URID map feature
  LV2_Atom_Sequence* control;
  MetroURIs          uris;    // Cache of mapped URIDs

} Data;
#endif //_H_STRUCTS_
