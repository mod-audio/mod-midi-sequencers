#!/bin/bash

jack_connect jack-keyboard:midi_out effect_1:in
jack_connect effect_1:out1 effect_0:lv2_events_in
