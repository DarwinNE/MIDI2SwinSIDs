#ifndef __MIDI_DEF_H
#define __MIDI_DEF_H

// The TT element is just there to force the enum to be signed, so that one
// can do tests such as if(currentMode>0) etc...
enum MIDI_mode {TT=-1,
    OMNI=0,                 // Respond to all events on all channels
    POLY,                   // Respond only to events on the current channel
    MULTI,                  // Respond to events on all channels separately
    MONO} ;                 // Respond to events on all channels, monophonic

#define NOTE_ON         0x90
#define NOTE_OFF        0x80
#define CONTROL_CHANGE  0xB0
#define PROGRAM_CHANGE  0xC0

#define CTRL_PEDAL      64

#endif