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

#define NUM_NOTE_PROPS 2

typedef enum PortEnum {
    PORT_ATOM_IN = 0,
    PORT_ATOM_OUT1,
    METRO_CONTROL,
    CVLFO1,
    CVLFO2,
    CVLFO3,
    CVLFO4,
    NOTEMODE,
    MODE,
    DIVISION,
    NOTELENGTH,
    OCTAVESPREAD,
    OCTAVEMODE,
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
    PATTERNVEL8,
    LFO1CONNECT,
    LFO1DEPTH,
    LFO2CONNECT,
    LFO2DEPTH,
    LFO3CONNECT,
    LFO3DEPTH,
    LFO4CONNECT,
    LFO4DEPTH,
    METARECORDING,
    METAMODE
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
    uint8_t eventList[248][NUM_NOTE_PROPS];
    size_t used;
} Array;

typedef struct Data {

    double  rate;   // Sample rate
    double  frequency;
    double  nyquist;
    double  velPhase;
    double  x1;
    double  phase;
    double  velocityLFO;
    float   bpm;
    float   barsize;
    float   speed; // Transport speed (usually 0=stop, 1=play)
    float   lfo1;
    float   lfo2;
    float   noteLengthTime[2];
    int     activeNotes;

    int     modeHandle;
    int     prevMod;
    int     prevLatch;
    int     previousOctaveMode;

    int     placementIndex;
    float   notePlacement[2];

    uint8_t noteTie;
    uint8_t velocity;
    uint8_t transpose;
    uint8_t metaNote;
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

    //resetPhase vars:
    float previousDevision;
    float previousFrequency;
    bool  previousPlaying;
    bool  resetPhase;

    size_t  patternIndex;
    size_t  notePlayed;
    size_t  playHead;
    size_t  numNotesInBar;
    size_t  metaBegin;
    int     octaveIndex;
    size_t  octaveMode;
    size_t  noteOffIndex;
    size_t  noteOffSendIndex;
    bool    octaveUp;
    bool    setMetaBegin;
    bool    firstRecordedNote;
    bool    through;
    bool    firstBar;
    bool    playing;
    bool    clip;
    bool    trigger;
    bool    triggerSet;
    bool    cleared;
    bool    metaRecording;
    bool    renderMeta;
    int     countTicks;

    const float** parameters[19];
    uint8_t velocityPattern[8];
    Array* metaEvents;
    Array* writeEvents;
    Array* playEvents;

    float variables[19];
    float division;         
    float noteLength;  
    float octaveSpread;     
    float swing;            
    float randomizeTimming; 
    float velocityMode;     
    float velocityCurve;    
    float curveDepth;       
    float curveClip;        
    float patternVel1;      
    float patternVel2;      
    float patternVel3;      
    float patternVel4;      
    float patternVel5;      
    float patternVel6;      
    float patternVel7;      
    float patternVel8;      
    const float* lfo1PortParam;
    const float* lfo2PortParam;
    const float* lfo3PortParam;
    const float* lfo4PortParam;
    const float* noteModeParam;
    const float* modeParam;
    const float* divisionParam;
    const float* noteLengthParam;
    const float* latchTransposeParam;
    const float* swingParam;
    const float* randomizeTimmingParam;
    const float* velocityModeParam;
    const float* velocityCurveParam;
    const float* curveDepthParam;
    const float* curveLengthParam;
    const float* curveClipParam;
    const float* octaveSpreadParam;
    const float* octaveModeParam;
    const float* velocityPatternLengthParam;
    const float* patternVel1Param;
    const float* patternVel2Param;
    const float* patternVel3Param;
    const float* patternVel4Param;
    const float* patternVel5Param;
    const float* patternVel6Param;
    const float* patternVel7Param;
    const float* patternVel8Param;
    const float* lfo1ConnectParam;
    const float* lfo1DepthParam;
    const float* lfo2ConnectParam;
    const float* lfo2DepthParam;
    const float* lfo3ConnectParam;
    const float* lfo3DepthParam;
    const float* lfo4ConnectParam;
    const float* lfo4DepthParam;
    const float* metaRecordingParam;
    const float* metaModeParam;
    const LV2_Atom_Sequence* port_events_in;
    LV2_Atom_Sequence*       port_events_out1;

    LV2_URID           urid_midiEvent;
    LV2_URID_Map*      map;     // URID map feature
    LV2_Atom_Sequence* control;
    MetroURIs          uris;    // Cache of mapped URIDs

} Data;
#endif //_H_STRUCTS_
