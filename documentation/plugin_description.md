# PLUGIN DESCRIPTION

This plugin is MIDI step sequencer. Here is a brief description of all the controls:

#### MODE
* Clear All: deletes the previous recording
* Record: records the MIDI input and puts it in a sequence.
* play: This will play the recorded sequence. It will start playing in the start of the next bar.
* Record Append: If there is new MIDI input it gets appended to the current sequence.
* Record Overwrite: In this mode the current sequence gets overwritten in the order of the sequence. If the plugin gets new MIDI input it will first overwrite the first note, then the second etc. The sequence will stay the same length. With this function the Sequencer can be used as a "arpeggiator".
* Undo last: undo a change that is made to the sequence.

#### DIVISION
 This control is made to adjust the speed of the loop. The available divisions are:
 * whole note
 * whole note triplets
 * half note
 * half note triplets
 * quarter note
 * quarter note triplets
 * eight note
 * eight note triplets
 * sixteenth note
 * sixteenth note triplets
 * thirty-second note
 * thirty-second triplets

#### RECORDBARS
With this plugin you can alter your loop while playing. For instance you can transpose the loop or overwrite the notes to make variations. For some variations you would have to keep on altering it, for instance if you use transpose to play your loop in multiple keys, if you would stop playing the transpose knob it will just stay in one key again. As a solution for this you can record you altered loop with the RECORDBARS function.
If you have a loop with a certain transpose pattern or some other variation you can hit the RECORDBARS knob and it will record the loop with the changes and then this become your new loop.

#### RECORDINGLENGTH
This control is linked to the RECORDBARS control and with this control you set the length of bars you want to record.
* In the first part of the list you can set an exact amount of bars you want to record. When it is used in this setting there is a pre-count of one bar that will start as soon is there is a new beginning of a bar.
* The last option is called "free". With this function you can hit record and then play, when you are done recording you can hit stop and then it will estimate the length of the recorded bars(it always gets rounded off two whole bars).

####  NOTELENGTH
This will set the length of the notes in the sequence. the minimum length are very short notes if the length is on max the notes will be as long as when the next note starts.

#### TRANSPOSE
This knob will take effect when the plugin MODE control is on play. With the transpose ON it will transpose the loop when you hit a key. When it is off the loop will just keep on playing and you can play something else over the loop.


#### BUGS AND THINGS TO IMPLEMENT
* The plugin can only start playing when the PLAYING function is on in the MOD-UI. This needs to be adjusted.
* When it plays the first time it starts at the beginning of a new bar. After that it starts right away when the MODE control is to "play", this needs to fixed. 
* Clear All in MODE is not implemented yet.
* "free" mode in RECORDINGLENGTH is almost done but for the moment not compiled because it still needs a few fixes.
* It needs to extra controls:
1. A control to put in a rest or a tie
2. A "SWING" control which makes the loop less static.
* Visual feedback for the RECORDBARS pre-count.Maybe also a control to set the amount of bars for the pre-count.
* The RECORDBARS function works but at the moment only for the first time. Still need to look into the way the variables get reset.
* Implement a way to input MIDI notes without a MIDI device.
* Implement a way to save the recorded loops.
