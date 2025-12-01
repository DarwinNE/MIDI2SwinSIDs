#ifndef __MIDI_DEF_H
#define __MIDI_DEF_H

// The TT element is just there to force the enum to be signed, so that one
// can do tests such as if(currentMode>0) etc...
enum MIDI_mode {TT=-1,
    OMNI=0,                 // Respond to all events on all channels
    POLY,                   // Respond only to events on the current channel
    MULTI,                  // Respond to events on all channels separately
    MONO,                   // Respond to events on all channels, monophonic
    HARP};                  // Arpeggiator

#define NOTE_ON         0x90
#define NOTE_OFF        0x80
#define POLY_AFTERTOUCH 0xA0
#define CONTROL_CHANGE  0xB0
#define PROGRAM_CHANGE  0xC0
#define PITCH_BEND      0xE0

#define CTRL_MOD_WHEEL  0x01
#define CTRL_PEDAL      0x40
#define CTRL_CH_WHEEL   0x72
#define CTRL_DUTY1      0x4C
#define CTRL_DUTY2      0x4D
#define CTRL_ATTACK1    0x49
#define CTRL_ATTACK2    0x50
#define CTRL_DECAY1     0x4B
#define CTRL_DECAY2     0x51
#define CTRL_SUSTAIN1   0x4F
#define CTRL_SUSTAIN2   0x52
#define CTRL_RELEASE1   0x48
#define CTRL_RELEASE2   0x53
#define CTRL_CUTOFF     0x4A
#define CTRL_RESONANCE  0x47
#define CTRL_MASTERVOL  0x55
#define CTRL_WAVE1      0x5D
#define CTRL_WAVE2      0x12
#define CTRL_FILTTYPE   0x13
#define CTRL_DIFFC      0x10
#define CTRL_DIFFF      0x11

#endif