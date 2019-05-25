#!/bin/bash

jack_connect jack-keyboard:midi_out effect_1:in
jack_connect effect_1:out1 effect_0:lv2_events_in
jack_connect effect_1:metronome_out system:playback_1
jack_connect effect_1:metronome_out system:playback_2
jack_connect effect_0:lv2_audio_out_1 system:playback_1
jack_connect effect_0:lv2_audio_out_2 system:playback_2
