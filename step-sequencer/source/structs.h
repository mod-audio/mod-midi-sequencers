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
    CVLFO1,
    CVLFO2,
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
    PATTERNVEL8,
    LFO1CONNECT,
    LFO1DEPTH,
    LFO2CONNECT,
    LFO2DEPTH,
    RECORDMETADATA,
    METADATAMODE
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


typedef struct EventList {
    uint8_t eventList[248][2];
    size_t  amountOfProps;
    size_t  used;
} EventList;


typedef struct StepSeq {

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

    //resetPhase vars:
    float previousDevision;
    float previousFrequency;
    bool  previousPlaying;
    bool  resetPhase;

    size_t  patternIndex;
    size_t  notePlayed;
    size_t  barLimit;
    size_t  playHead;
    size_t  octaveIndex;
    size_t  noteOffIndex;
    size_t  noteOffSendIndex;
    bool    metaDataRendered;
    bool    recordingMetaData;
    bool    firstRecordedNote;
    bool    through;
    bool    firstBar;
    bool    playing;
    bool    clip;
    bool    trigger;
    bool    triggerSet;
    bool    cleared;
    int     transpose;
    int     countTicks;

    const float** parameters[18];
    uint8_t   velocityPattern[8];
    size_t    metaBegin;
    EventList metaEvents;
    EventList writeEvents;
    EventList playEvents;

    float variables[18];
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
    const float* enableMetaRecordingParam;
    const float* metaModeParam;
    const LV2_Atom_Sequence* port_events_in;
    LV2_Atom_Sequence*       port_events_out1;

    LV2_URID           urid_midiEvent;
    LV2_URID_Map*      map;     // URID map feature
    LV2_Atom_Sequence* control;
    MetroURIs          uris;    // Cache of mapped URIDs

} StepSeq;


#endif //_H_STRUCTS_
