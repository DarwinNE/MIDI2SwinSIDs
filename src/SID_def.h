#ifndef __SID_DEF_H
#define __SID_DEF_H

#define BASE_MIDI_NOTE  24
#define SECONDVOICE     128
#define LFO_SIZE        256

// Composite instrument for drumkits on channel 10.
typedef struct SID_composite_tag {
    uint8_t  instrument;        // Number of the instrument
    uint8_t  note;              // Note to be played
} SID_composite;

// Definition of an instrument.
typedef struct SID_conf_tag {
    uint8_t  a;                 // Attack
    uint8_t  d;                 // Decay
    uint8_t  s;                 // Sustain
    uint8_t  r;                 // Release
    uint16_t duty_cycle;        // Duty cycle (for rectangular waveforms)
    uint8_t  filt_mode;         // Filter mode
    uint16_t filt_cutoff;       // Filter cutoff
    uint8_t  filt_resonance;    // Filter resonance
    uint8_t  filt_routing;      // Filter voice routing
    uint8_t  voice;             // Waveform voice 1
    uint16_t diff;              // Frequency difference (in cents) b/w voices
    uint8_t  a2;                // Attack voice 2
    uint8_t  d2;                // Decay voice 2
    uint8_t  s2;                // Sustain voice 2
    uint8_t  r2;                // Release voice 2
    uint16_t duty_cycle2;       // Duty cycle (for rectangular waveforms)
    uint8_t  voice2;            // Waveform voice 2
    uint8_t  lfo_rate;          // LFO rate
    uint8_t  lfo_depth;         // LFO depth -> pitch
    uint8_t  portamento;        // Portamento
    char    *name;              // Name of the instrument
} SID_conf;

#define NUM_VOICES 6

typedef struct VoiceDef_tag {
    int16_t  key;
    uint8_t  channel;
    uint32_t timestamp;
    uint8_t  voice;
    SID_conf inst;
    int16_t  freq;
    int16_t  oldfreq;
} VoiceDef;


// SID waveforms  (+ gate)
#define NOISE   129
#define PULSE   65
#define SAWTH   33
#define TRIAN   17
#define NONE    0

// SID filter modes
#define LO          1   // Low-pass filter
#define BP          2   // Band-pass filter
#define HI          4   // High-pass filter
#define M3          8   // Mute voice 3

// Routing
#define ALL         7   // Apply filter to all notes
#define NON         0   // No filter applied

#define MAXVOL      15


#define SID_WRITE 0
#define SID_READ  1

/* SID registers */

#define SID_VOICE_OFFSET    0x07

#define SID_V1_FREQ_LO  0x00
#define SID_V1_FREQ_HI  0x01
#define SID_V1_PW_LO    0x02
#define SID_V1_PW_HI    0x03
#define SID_V1_CONTROL  0x04
#define SID_V1_AD       0x05
#define SID_V1_SR       0x06

#define SID_V2_FREQ_LO  0x07
#define SID_V2_FREQ_HI  0x08
#define SID_V2_PW_LO    0x09
#define SID_V2_PW_HI    0x0A
#define SID_V2_CONTROL  0x0B
#define SID_V2_AD       0x0C
#define SID_V2_SR       0x0D

#define SID_V3_FREQ_LO  0x0E
#define SID_V3_FREQ_HI  0x0F
#define SID_V3_PW_LO    0x10
#define SID_V3_PW_HI    0x11
#define SID_V3_CONTROL  0x12
#define SID_V3_AD       0x13
#define SID_V3_SR       0x14

#define SID_FC_LO           0x15
#define SID_FC_HI           0x16
#define SID_RES_FILT        0x17
#define SID_MODE_VOL        0x18

#define SID_POTX            0x19
#define SID_POTY            0x1A
#define SID_OSC3_RANDOM     0x1B
#define SID_ENV3            0x1C

/* Exported functions (public). */
void SID_Set_Reg(int address, int data, int sid_num);
void SID_Note_On(uint8_t key, uint8_t velocity, SID_conf *instrument,
    uint8_t channel);
void SID_Note_Off(uint8_t key, uint8_t channel);
uint8_t GetFreeVoice(int key, uint8_t channel);
void SID_Stop_Voice(uint8_t voice);
void UpdateLFO(void);


#endif