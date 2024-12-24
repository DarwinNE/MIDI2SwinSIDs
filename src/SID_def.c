#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "SID_def.h"


VoiceDef Voices[NUM_VOICES];

int8_t  LFO_Table[LFO_SIZE];
uint16_t LFO_Pointer=0;
uint8_t LFO_Amount = 0;
uint8_t LFO_Rate = 0;
uint32_t counter = 0;


uint16_t C64_freq_table[]={
    278, 295, 313, 331, 351, 372, 394, 417, 442, 468, 496, 526, 557, 590, 625,
    662, 702, 743, 788, 834, 884, 937, 992, 1051, 1114, 1180, 1250, 1325, 1403,
    1487, 1575, 1669, 1768, 1873, 1985, 2103, 2228, 2360, 2500, 2649, 2807,
    2973, 3150, 3338, 3536, 3746, 3969, 4205, 4455, 4720, 5001, 5298, 5613,
    5947, 6300, 6675, 7072, 7493, 7938, 8410, 8910, 9440, 10001, 10596, 11226,
    11894, 12601, 13350, 14144, 14985, 15876, 16820, 17820, 18880, 20003, 21192,
    22452, 23787, 25202, 26700, 28288, 29970, 31752, 33640, 35641, 37760, 40005,
    42384, 44904, 47574, 50403, 53401, 56576, 59940, 63504, 67280}; // PAL

#define FREQ_CORRECTION 98/100


/******************************************************************************
    General MIDI patches
******************************************************************************/

/* A, D, S and R range from 0 to 15.
   Duty cycle applies for PULSE waveform and range from 0 to 4095. Value 2048
   represents a square wave.
   CTFF (cutoff frequency) varies between 0 and 2048.
   Res(onance) varies between 0 (no resonance) and 15 (max resonance)
   We measure instrument number starting from 1.
   Asterisk "*" means NOT TWEAKED YET.
*/

SID_conf GeneralMIDI[256] = {
//   FIRST VOICE------------------------------|-------SECOND VOICE -----------|
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE  LFR LFD PORT  NAME                 NUMBER (idx+1)
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |     |   |   |     |                          |
    { 2,11,0 ,2 ,1632,LO, 624, 0,ALL,PULSE,1200, 2, 8, 0, 1,   0, TRIAN,  0,  0,  0,"Ac. Grand Piano"},         // 1
    { 2,11,0 ,2 ,1184,LO, 416, 0,ALL,PULSE,1200, 1, 8, 0, 2,   0, SAWTH,  0,  0,  0,"Bright Ac. Piano"},        // 2
    { 2,10, 0,1 ,2048,LO, 512, 4,ALL,PULSE,1200, 1,10, 0, 1, 512, PULSE,  0,  0,  0,"El. Grand Piano"},         // 3
    { 1,8 ,0 ,1 , 0  ,LO, 600,10,ALL,SAWTH, 610, 1,10, 0, 1,2048, PULSE,  0,  0,  0,"Honky-tonk Piano"},        // 4
    { 1,10, 0,1 , 512,LO, 400,10,ALL,PULSE, 600, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"El. Piano 1"},             // 5
    { 1,11,0 ,2 , 512,LO, 300, 4,ALL,PULSE,1200, 1, 6, 0, 1,   0, SAWTH,  0,  0,  0,"El. Piano 2"},             // 6
    { 1,10, 0,1 , 0  ,LO, 245, 0,NON,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Harpsicord"},              // 7
    { 0,10, 0,6 , 0  ,LO, 864,15,ALL,SAWTH,1800, 0, 10,0, 6,   0, SAWTH,  0,  0,  0,"Clavi"},                   // 8
    { 0,10, 0,3 ,2048,LO, 128, 0,NON,TRIAN,2400, 0, 8, 0, 2,   0, TRIAN,  0,  0,  0,"Celesta"},                 // 9
    { 1,9 ,0 , 4, 0  ,LO,1024, 0,NON,TRIAN,2400, 0, 8, 0, 3,2048, PULSE,  0,  0,  0,"Glockenspiel"},            // 10
    { 0,8 ,0 , 8, 544,LO, 576, 5,ALL,PULSE,1200, 0, 9, 0, 9, 448, PULSE,  0,  0,  0,"Music box"},               // 11
    { 0,12,0 ,3 , 0  ,LO, 256, 7,ALL,TRIAN,1204, 0,12, 0, 3,2240, PULSE,  3,  1,  0,"Vibraphone"},              // 12
    { 0,9 ,0 ,5 , 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Marimba"},                 // 13
    { 0,6 ,0 ,8 , 0  ,LO,1024, 0,NON,TRIAN,2400, 0, 4, 0, 4,   0, TRIAN,  0,  0,  0,"Xylophone"},               // 14
    { 0,10, 0,6 , 0  ,LO, 700,15,ALL,TRIAN,1805, 0, 10,0, 6,   0, SAWTH,  2,  7,  0,"Tubular Bells"},           // 15
    { 0,6 ,0 ,2 , 0  ,LO,1024, 0,NON,SAWTH, 600, 0, 6, 0, 2,   0, NONE ,  0,  0,  0,"Dulcimer"},                // 16
    { 2,1 ,15,1 , 0  ,LO, 512, 4,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Drawbar Organ"},           // 17
    { 1,1 ,13,1 , 0  ,LO,1024, 4,ALL,SAWTH,1200, 0, 6, 0, 1,2048, PULSE,  0,  0,  0,"Percussive Organ"},        // 18
    { 1,2 ,13,1 , 0  ,LO, 624, 4,ALL,TRIAN,1200, 1, 6, 0, 4,   0, TRIAN,  0,  0,  0,"Rock Organ"},              // 19
    { 1,2 ,15, 1, 0  ,LO, 512, 4,NON,TRIAN,2400, 1, 2,15, 1,   0, TRIAN,  0,  0,  0,"Church Organ"},            // 20
    { 1,1 ,15, 2,1024,LO, 128, 4,ALL,PULSE, 600, 1, 1,15, 1,   0, TRIAN,  0,  0,  0,"Reed Organ"},              // 21
    { 1,1 ,15, 2, 512,LO, 320, 0,ALL,PULSE,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Accordion"},               // 22
    { 7,2 ,13, 2, 512,LO, 336, 0,ALL,PULSE, 601, 7,13,13, 2,3360, PULSE,  0,  0,  0,"Harmonica"},               // 23
//   FIRST VOICE------------------------------|-------SECOND VOICE -----------|
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE  LFR LFD PORT  NAME                     NUMBER
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |     |   |   |     |                          |
    { 1,2 ,12,2 ,1024,LO, 512, 0,NON,PULSE,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Tango Accordion"},         // 24
    { 0,10, 0, 4, 256,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Ac. Guitar (nylon)"},      // 25
    { 0,10, 0, 4, 256,LO,1024, 0,NON,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Ac. Guitar (steel)"},      // 26
    { 0,10, 0,5 , 256,LO, 256, 0,ALL,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"El. Guitar (jazz)"},       // 27
    { 0,10, 0,5 , 256,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"El. Guitar (clean)"},      // 28
    { 0,11,0 ,2 , 256,LO, 160, 0,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"El. Guitar (muted)"},      // 29
    { 0,12,0 ,3 , 512,LO, 432, 9,ALL,PULSE,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Overdriven Guitar"},       // 30
    { 0,12,0 ,3 , 256,LO, 512, 8,ALL,PULSE, 600, 0,12, 0, 3, 512, PULSE,  0,  0,  0,"Distortion Guitar"},       // 31
    { 0,12,0 ,3 , 0  ,HI, 256, 7,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Guitar Harmonics"},        // 32
    { 1,12,0 ,3 , 0  ,LO, 200, 0,ALL,TRIAN, 600, 0,12, 0, 3,   0, TRIAN,  0,  0,  0,"Ac. Bass"},                // 33
    { 1,12,0 , 4, 0  ,LO, 250, 8,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"El. Bass (finger)"},       // 34
    { 1,12,0 , 4, 0  ,LO, 792, 8,NON,TRIAN, 600, 0, 0, 0, 0,2112, PULSE,  0,  0,  0,"El. Bass (pick)"},         // 35
    { 2,11,0 , 3, 0  ,LO, 688,15,ALL,SAWTH, 600, 2,10, 0, 3,   0, NONE ,  2,  2,  0,"Fretless Bass"},           // 36
    { 5,11,0 , 3, 0  ,LO, 320, 0,ALL,SAWTH, 600, 0, 4, 0, 3,2112, PULSE,  0,  0,  0,"Slap Bass 1"},             // 37
    { 5,11,0 , 3, 0  ,LO, 544, 0,ALL,SAWTH, 600, 0, 4, 0, 3,2112, PULSE,  0,  0,  0,"Slap Bass 2"},             // 38
    { 5,11,0 , 3, 0  ,LO, 320, 0,ALL,SAWTH, 600, 4,11, 0, 1,2112, PULSE,  0,  0,  0,"Synth Bass 1"},            // 39
    { 5,11,0 , 3, 0  ,LO, 544, 0,ALL,SAWTH, 600, 4,11, 0, 1,2112, PULSE,  0,  0,  0,"Synth Bass 2"},            // 40
    { 5,10,13, 3, 704,LO, 752, 5,ALL,PULSE, 600, 6, 8,11, 5,   0, SAWTH,  0,  0,  0,"Violin"},                  // 41
    { 6,10,13, 3, 730,LO, 600, 5,ALL,PULSE, 600, 6, 8,11, 5,   0, SAWTH,  0,  0,  0,"Viola"},                   // 42
    { 7,10,13, 3, 768,LO, 550, 5,ALL,PULSE, 600, 6, 8,11, 9,   0, SAWTH,  0,  0,  0,"Cello"},                   // 43
    { 8,10,13, 3, 900,LO, 480, 5,ALL,PULSE, 600, 6, 8,11, 9,   0, SAWTH,  0,  0,  0,"Contrabass"},              // 44
    { 7, 7,13, 3, 0  ,LO, 800, 6,ALL,SAWTH, 598, 6, 8,11, 5,   0, SAWTH, 20,  8,  0,"Tremolo Strings"},         // 45
    { 0, 9, 0, 2, 0  ,LO, 800, 6,ALL,SAWTH, 598, 1, 9 ,0, 1,1824, PULSE,  0,  0,  0,"Pizzicato Strings"},       // 46
//   FIRST VOICE------------------------------|-------SECOND VOICE -----------|
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE  LFR LFD PORT  NAME                     NUMBER
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |     |   |   |     |                          |
    { 1,10, 0,1 , 0  ,LO, 245, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Orchestral Harp"},         // 47
    { 0,10, 0,10, 0  ,LO, 512, 0,ALL,NOISE, 600, 0,10, 0,10,   0, NOISE,  0,  0,  0,"Timpani"},                 // 48
    { 7, 7,13, 3, 0  ,LO, 496, 2,ALL,SAWTH, 602, 6, 8,11, 5,   0, SAWTH,  0,  0,  0,"String Ensemble 1"},       // 49
    { 7, 7,13, 3, 0  ,LO, 800, 6,ALL,SAWTH, 598, 6, 8,11, 5,   0, SAWTH,  0,  0,  0,"String Ensemble 2"},       // 50
    { 7, 7,13, 3, 0  ,LO, 496, 2,ALL,SAWTH, 598, 6, 8,11, 5,1824, PULSE,  0,  0,  0,"Synth Strings 1"},         // 51
    { 7, 7,13, 3, 0  ,LO, 800, 6,ALL,SAWTH, 598, 6, 8,11, 5,1824, PULSE,  0,  0,  0,"Synth Strings 2"},         // 52
    { 7, 7,13, 3, 0  ,BP, 336,15,ALL,PULSE,1203, 6, 8,11, 5,1728, PULSE,  1,  1,  3,"Choir Aahs"},              // 53
    { 7, 7,13, 3, 0  ,BP, 224, 6,ALL,SAWTH,1203, 6, 8,11, 5, 672, NONE ,  0,  0,  3,"Voice Oohs"},              // 54
    { 7, 7,13, 3, 0  ,LO, 336,15,ALL,PULSE,1203, 6, 8,11, 5,1728, PULSE,  0,  0,  0,"Synth Voice"},             // 55
    { 2, 7, 0, 3, 0  ,LO, 800, 6,ALL,SAWTH, 598, 2, 8, 0, 5,   0, SAWTH,  0,  0,  0,"Orchestra Hit"},           // 56
    { 4,7 ,14, 2, 640,LO, 560,14,ALL,PULSE, 598, 7, 4, 0, 4,1344, PULSE,  0,  0,  0,"Trumpet"},                 // 57
    { 5,7 ,14, 2, 896,LO, 480,14,ALL,PULSE, 598, 8, 4, 0, 4,1344, PULSE,  0,  0,  0,"Trombone"},                // 58
    { 7,7 ,14, 2,1152,LO, 400,14,ALL,PULSE, 599, 8, 4, 0, 4,1344, PULSE,  0,  0,  0,"Tuba"},                    // 59
    { 4,7 ,14, 2, 640,LO, 560,14,ALL,TRIAN, 598, 7, 4, 0, 4,1344, PULSE,  0,  0,  0,"Muted Trumpet"},           // 60
    { 6, 0,15, 0, 0  ,LO, 320, 0,ALL,TRIAN,1200, 8,10,12, 0,   0, SAWTH,  0,  0,  0,"French Horn"},             // 61
    { 4,7 ,14, 2,3648,LO, 672,14,ALL,PULSE, 604, 7, 4, 0, 4,1344, PULSE,  0,  0,  0,"Brass Section"},           // 62
    { 4,7 ,14, 2,2944,LO, 720,14,ALL,PULSE, 606, 7, 4, 0, 4,2016, PULSE,  0,  0,  0,"Synth Brass 1"},           // 63
    { 4,7 ,14, 2,2592,LO, 832,14,ALL,PULSE, 606, 7, 4, 0, 4,2016, PULSE,  0,  0,  0,"Synth Brass 2"},           // 64
    { 5,10,13, 3, 768,LO, 600, 7,ALL,PULSE, 600, 6, 8,11, 9,   0, SAWTH,  0,  0,  0,"Soprano Sax"},             // 65
    { 7,10,13, 3, 768,LO, 550,10,ALL,PULSE, 600, 7, 8,11, 9,   0, SAWTH,  0,  0,  0,"Alto Sax"},                // 66
    {8 ,10,13, 3, 768,LO, 500,12,ALL,PULSE, 600, 8, 8,11, 9,   0, SAWTH,  0,  0,  0,"Tenor Sax"},               // 67
    {8 ,10,13, 3, 768,LO, 450,15,ALL,PULSE, 600, 8, 8,11, 9,   0, SAWTH,  0,  0,  0,"Baritone Sax"},            // 68
    { 3,7 ,14, 2,1888,LO, 560,10,ALL,PULSE, 598, 7, 4,10, 4,2624, PULSE,  0,  0,  0,"Oboe"},                    // 69
//   FIRST VOICE------------------------------|-------SECOND VOICE -----------|
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE  LFR LFD PORT  NAME                     NUMBER
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |     |   |   |     |                          |
    { 3,7 ,14, 2,1888,LO, 144, 2,ALL,PULSE, 598, 7, 4,10, 4,2624, PULSE,  0,  0,  0,"English Horn"},            // 70
    { 3,7 ,14, 2, 352,LO, 288, 4,ALL,PULSE, 602, 3, 8,10, 4, 960, PULSE,  0,  0,  0,"Bassoon"},                 // 71
    { 4,7 ,14, 2, 480,LO, 480, 4,ALL,PULSE, 602, 4, 4,10, 4, 480, PULSE,  0,  0,  0,"Clarinet"},                // 72
    { 3,7 ,14, 2,2496,LO, 600,12,ALL,PULSE,1202, 8, 1,12, 4,1600, PULSE,  0,  0,  0,"Piccolo"},                 // 73
    { 3,7 ,14, 2,2496,LO, 272, 1,ALL,PULSE,1202, 8, 1,12, 4,1600, PULSE,  0,  0,  0,"Flute"},                   // 74
    { 3,7 ,14, 2,1152,LO, 240, 2,ALL,PULSE,1202, 3, 1,12, 4, 960, PULSE,  0,  0,  0,"Recorder"},                // 75
    { 3,7 ,14, 2,1152,LO, 240, 0,ALL,TRIAN, 300, 6, 3, 0, 0, 960, NOISE,  0,  0,  0,"Pan Flute"},               // 76
    { 3,7 ,14, 2,1152,LO, 400, 0,ALL,SAWTH, 300, 6, 3, 0, 0, 960, NOISE,  0,  0,  0,"Blown Bottle"},            // 77
    { 3,7 ,14, 2,1152,LO, 240, 2,ALL,TRIAN, 600, 7, 1, 2, 4, 960, NOISE,  0,  0,  0,"Shakuhachi"},              // 78
    { 5,7 ,14, 2,1152,LO, 608,13,ALL,PULSE,2402, 7, 8,10, 4, 960, PULSE,  1,  1, 10,"Wistle"},                  // 79
    { 5,7 ,14, 2,1536,LO, 368,12,ALL,PULSE,1204, 7, 1,12, 4, 480, PULSE,  1,  1,  1,"Ocarina"},                 // 80
    { 2,0 ,15, 0,2048,LO,1024, 0,NON,PULSE,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Lead 1 (square)"},         // 81
    { 2,0 ,15, 0, 0  ,LO,1024, 0,NON,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Lead 2 (SAWTH)"},          // 82
    { 3,7 ,14, 2,1888,LO, 368, 7,ALL,PULSE, 598, 7, 4,10, 4,2624, PULSE,  0,  0,  0,"Lead 3 (calliope)"},       // 83
    { 2, 9, 0, 0, 704,LO, 592, 5,ALL,PULSE, 600, 0, 9, 0, 0, 640, SAWTH,  0,  0,  0,"Lead 4 (chiff)"},          // 84
    { 0,12,0 ,3 , 256,LO, 512, 8,NON,PULSE, 600, 0,12, 0, 3, 512, SAWTH,  0,  0,  0,"Lead 5 (charang)"},        // 85
    { 7, 7,13, 3, 0  ,BP, 336,15,ALL,PULSE,1203, 6, 8,11, 5,1728, PULSE,  1,  1,  3,"Lead 6 (voice)*"},         // 86
    { 0, 0,15, 0, 0  ,LO, 384, 0,ALL,TRIAN,1800, 2, 4,10, 6,2336, PULSE,  0,  0,  0,"Lead 7 (fifths)"},         // 87
    { 0, 0,15, 0, 0  ,LO, 384, 0,ALL,SAWTH,2415, 2, 4,10, 6,2336, PULSE,  1,  1,  0,"Lead 8 (bass+lead)"},      // 88
    { 7, 7,13, 3, 0  ,LO,1000, 8,ALL,SAWTH, 598, 6, 8,11, 5,   0, SAWTH,  0,  0, 10,"Pad 1 (new age)"},         // 89
    {13, 9,13, 5, 0  ,LO, 432, 4,ALL,SAWTH,1200,12,10,11, 5,   0, SAWTH,  0,  0, 20,"Pad 2 (warm)"},            // 90
    { 5,7 ,14, 2,1152,LO, 608,13,ALL,PULSE,2402, 7, 8,10, 4, 960, PULSE,  1,  1,  0,"Pad 3 (polysynth)"},       // 91
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Pad 4 (choir)*"},          // 92
//   FIRST VOICE------------------------------|-------SECOND VOICE -----------|
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE  LFR LFD PORT  NAME                     NUMBER
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |     |   |   |     |                          |
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Pad 5 (bowed)*"},          // 93
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Pad 6 (metallic)*"},       // 94
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Pad 7 (halo)*"},           // 95
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Pad 8 (sweep)*"},          // 96
    { 5, 0,11, 4, 0  ,BP, 352, 0,ALL,NONE , 200,15,11,13, 6,   0, NOISE,  0,  0, 20,"FX 1 (rain)"},             // 97
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"FX 2 (soundtrack)*"},      // 98
    { 5,12, 5, 1,1213,HI, 480, 4,ALL,TRIAN,1213, 9,14, 9, 8,1408, PULSE,  1,  1,  2,"FX 3 (crystal)"},          // 99
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"FX 4 (athmosphere)*"},     // 100
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"FX 5 (brightness)*"},      // 101
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"FX 6 (goblins)*"},         // 102
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"FX 7 (echoes)*"},          // 103
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"FX 8 (sci-fi)*"},          // 104
    { 1, 8, 0, 1, 0  ,LO, 400, 0,NON,   FM, 400, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Sitar FM"},                // 105
    { 1, 7, 0, 1, 0  ,LO, 245, 0,NON,   FM, 600, 0, 6, 0, 1,   0, SAWTH,  0,  0,  0,"Banjo FM"},                // 106
    { 0,12, 0, 4, 0  ,LO,1024, 0,NON,   FM, 360, 0, 0, 4, 0,   0, NONE ,  0,  0,  1,"Shamisen FM"},             // 107
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Koto*"},                   // 108
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Kalimba*"},                // 109
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Bag Pipe*"},               // 110
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Fiddle*"},                 // 111
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Shanai*"},                 // 112
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Tinkle Bell*"},            // 113
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Agogo*"},                  // 114
    { 0, 5, 0, 3, 0  ,HI, 530,11,ALL,NOISE,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Steel Drums"},             // 115
    { 1, 3, 0, 2, 0  ,LO, 507,15,ALL,NOISE,2400, 1, 4, 0, 2,   0, TRIAN,  0,  0,  0,"Woodblock"},               // 116
//   FIRST VOICE------------------------------|-------SECOND VOICE -----------|
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE  LFR LFD PORT  NAME                     NUMBER
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |     |   |   |     |                          |
    { 2, 6,0 , 3, 0  ,LO, 225,10,ALL,NOISE,3850, 1, 6, 0, 3,   0, TRIAN,  0,  0,  0,"Taiko Drum"},              // 117
    { 2, 6,0 , 3, 0  ,LO, 225,10,ALL,NOISE,3850, 1, 6, 0, 3,   0, NOISE,  0,  0,  0,"Melodic Tom"},             // 118
    { 2, 6,0 , 3, 0  ,LO, 124,15,ALL,NOISE, 203, 1, 6, 0, 3,   0, NONE ,  0,  0,  0,"Synth Drum"},              // 119
    {10, 1,0 , 0, 0  ,LO,1024, 0,NON,NOISE,2861,10, 1, 0, 0,   0, NOISE,  0,  0,  0,"Reverse Cymbal"},          // 120
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Guitar Fret Noise*"},      // 121
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Breath Noise*"},           // 122
    {12,11, 2,10, 0  ,LO,1024, 0,NON,NOISE, 800,11,12, 2,11,   0, NOISE,  0,  0,  0,"Seashore"},                // 123
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Bird Tweet*"},             // 124
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Telephone Ring*"},         // 125
    { 5, 0,15, 2, 0  ,LO,1024, 0,NON,NOISE, 800, 5, 0,15, 2,   0, NONE , 30,127,  0,"Helicopter"},              // 126
    { 0, 0,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,  0,  0,  0,"Applause*"},               // 127
    { 0,9 , 0, 0, 0  ,LO,1024, 0,NON,NOISE, 800, 0, 9, 0, 0,   0, NOISE,  0,  0,  0,"Gunshot"},                 // 128
    { 0,9 , 0, 0, 0  ,LO,1024, 0,NON,NONE ,   0, 0, 9, 0, 0,   0, NONE ,  0,  0,  0,"None"}                     // 129
};

// Channel 10 drum kit
SID_composite DrumKit[128] = {
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},

// INSTR NOTE
//   |    |
    {119-1, 45},    // 35 B0  Acoustic Bass Drum
    {119-1, 55},    // 36 C1  Bass Drum 1
    {116-1, 52},    // 37 C#1 Side Stick
    {117-1, 43},    // 38 D1  Acoustic Snare
    {119-1, 55},    // 39 D#1 Hand Clap           *
    {119-1, 55},    // 40 E1  Electric Snare      *
    {118-1, 48},    // 41 F1  Low Floor Tom
    {115-1, 79},    // 42 F#1 Closed Hi Hat
    {118-1, 55},    // 43 G1  High Floor Tom      *
    {119-1, 55},    // 44 G#1 Pedal Hi Hat        *
    {119-1, 55},    // 45 A1  Low Tom
    {119-1, 55},    // 46 A#1 Open Hi Hat         *
    {118-1, 52},    // 47 Low Mid Tom
    {118-1, 53},    // 48 Hi Mid Tom
    {48 -1, 96},    // 49 Crash Cymbal 1
    {118-1, 60},    // 50 High Tom
    {48 -1, 90},    // 51 Ride Cymbal 1
    {48 -1, 103},   // 52 Chinese Cymbal
    {119-1, 55},    // 53 Ride Bell               *
    {119-1, 55},    // 54 Tambourine              *
    {47 -1, 85},    // 55 Splash Cymbal
    {119-1, 55},    // 56 Cowbell                 *
    {48 -1, 90},    // 57 Crash Cymbal
    {119-1, 55},    // 58 Vibrasplash             *
    {119-1, 55},    // 59 Ride Cymbal 2           *
    {119-1, 55},    // 60 Hi Bongo                *
    {119-1, 55},    // 61 Low Bongo               *
    {119-1, 55},    // 62 Mute Hi Conga           *
    {119-1, 55},    // 63 Open Hi Conga           *
    {119-1, 55},    // 64 Low Conga               *
    {119-1, 55},    // 65 High Timbale            *
    {119-1, 55},    // 66 Low Timbale             *
    {119-1, 55},    // 67 High Agogo              *
    {119-1, 55},    // 68 Low Agogo               *
    {119-1, 55},    // 69 Cbasa                   *
    {119-1, 55},    // 70 Maracas                 *
    {119-1, 55},    // 71 Short Whistle           *
    {119-1, 55},    // 72 Long Whistle            *
    {119-1, 55},    // 73 Short Guiro             *
    {119-1, 55},    // 74 Long Guiro              *
    {119-1, 67},    // 75 Claves                  *
    {116-1, 60},    // 76 E4  Hi Wood Block
    {116-1, 55},    // 77 F4  Low Wood Block
    {119-1, 55},    // 78 F#4 Mute Cuica          *
    {119-1, 55},    // 79 G4  Open Cuica          *
    {119-1, 55},    // 80 G4# Mute Triangle       *
    {119-1, 55}     // 81 A4  Open Triangle       *
};

/* SID interface functions */

volatile uint8_t SustainPedal[16]; // Pedal state for each channel
volatile int8_t Master_Volume;     // 0 to 15


#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))



/**
    Put a certain value on the data pins of the SID's
*/
void SID_Set_Data(int data)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
        (data & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,
        (data & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,
        (data & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3,
        (data & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,
        (data & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11,
        (data & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12,
        (data & 0x40) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
        (data & 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void SID_Set_Address(int address)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2,
        (address & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4,
        (address & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5,
        (address & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7,
        (address & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2,
        (address & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void SID_Set_RW(int rw)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6,
        rw ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/** SID chips latch data and address buses at the falling edge of their clock.
    Here, we wait for a rising state of the clock.
*/
/*void SID_Sync_Clock(void)
{
    // Wait for a RESET state
    while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3)!=GPIO_PIN_RESET) ;
    // Here we surely are in a RESET state (or very close to a transition).
    // Wait for a SET state.
    while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3)!=GPIO_PIN_SET) ;
    // A rising transition just happened.
}*/

/** Select the given SID (or none of them id is different from 0 or 1)
*/
void SID_Select(int id)
{
    // CS SID active low.
    if(id==1) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
    } else if (id==0) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
    }
}

void SID_Set_Reg(int address, int data, int sid_num)
{
    SID_Set_Address(address);
    SID_Set_RW(SID_WRITE);
    SID_Set_Data(data);
    // Pulse the CS line to strobe data.
    SID_Select(sid_num);
    SID_Select(-1);
}
/** Switch on one of the SID oscillators, configure the instrument.
*/
void SID_Note_On(uint8_t key_m, uint8_t velocity, SID_conf *inst,
    uint8_t channel)
{
    // BASE_MIDI_NOTE represents the offset between note 0 in the GeneralMIDI
    // standard and the frequency table for the SID.
    // Use key_m every time you need to use the GeneralMIDI event.
    uint8_t key_fr=key_m-BASE_MIDI_NOTE;
    uint8_t voice1, voice2;

    // If the first oscillator is active, play it.
    if(inst->voice != NONE) {
        // The FM modulator is a little particular, as voice2 must be the third
        // oscillator in one of the SID's
        if(inst->voice==FM) {
            voice1=GetFMVoices(key_m, channel);
            // Choose SID0 or SID1 for voice2.
            if(voice1<2)
                voice2=2;
            else
                voice2=5;
        } else {
            voice1=GetFreeVoice(key_m, channel);
        }

        Voices[voice1].key=key_m,
        Voices[voice1].channel=channel;
        Voices[voice1].timestamp=counter;
        Voices[voice1].voice=inst->voice;
        Voices[voice1].inst=*inst;
        if (key_fr >= COUNTOF(C64_freq_table))
            return;
        uint16_t freq= C64_freq_table[key_fr]*FREQ_CORRECTION;
        Voices[voice1].freq=freq;

        // Select the correct SID, depending on the voice number.
        int sid_num = 0;
        if(voice1 > 2) {
            voice1 -= 3;
            sid_num = 1;
        }
        uint8_t offset = SID_VOICE_OFFSET*voice1;

        SID_Set_Reg(SID_MODE_VOL, (Master_Volume & 0x0F) |
            (inst->filt_mode & 0xF)<<4, sid_num);
        SID_Set_Reg(SID_FC_LO,inst->filt_cutoff & 0x0007, sid_num);
        SID_Set_Reg(SID_FC_HI, (inst->filt_cutoff & 0x07F8)>>3, sid_num);
        SID_Set_Reg(SID_RES_FILT, (inst->filt_resonance & 0xF)<<4 |
            (inst->filt_routing&0x0F), sid_num);
        if(!inst->portamento) {
            SID_Set_Reg(SID_V1_FREQ_HI+offset, (uint8_t)((freq & 0xFF00)>>8),
                sid_num);
            SID_Set_Reg(SID_V1_FREQ_LO+offset, (uint8_t)(freq & 0x00FF),
                sid_num);
        }
        SID_Set_Reg(SID_V1_AD+offset, inst->a*16+inst->d, sid_num);
        SID_Set_Reg(SID_V1_SR+offset, inst->s*16+inst->r, sid_num);
        SID_Set_Reg(SID_V1_PW_LO+offset, (uint8_t)(inst->duty_cycle & 0x00FF),
            sid_num);
        SID_Set_Reg(SID_V1_PW_HI+offset,
            (uint8_t)((inst->duty_cycle & 0xFF00)>>8), sid_num);

        SID_Set_Reg(SID_V1_CONTROL+offset, inst->voice, sid_num);
    }

    // Check if a second voice is present and, if yes, play it.
    if(inst->voice2 != NONE || inst->voice==FM) {
        if(inst->voice==FM) {
            // voice2 is already correct.
        } else {
            voice2 = GetFreeVoice(key_m*SECONDVOICE, channel);
        }

        Voices[voice2].key=((int16_t)key_m)*SECONDVOICE;
        Voices[voice2].channel=channel;
        Voices[voice2].timestamp=counter;
        Voices[voice2].inst=*inst;
        Voices[voice2].voice=inst->voice2;

        if (key_fr >= COUNTOF(C64_freq_table))
            return;
        uint32_t v2freq_l = C64_freq_table[key_fr]*FREQ_CORRECTION;


        if(v2freq_l*inst->diff>0)
            v2freq_l = (v2freq_l*inst->diff*2)/1200;
        else
            v2freq_l = (v2freq_l*1200)/inst->diff/2;

        if(v2freq_l>65536)
            return;

        int16_t v2freq = (int16_t) v2freq_l;
        Voices[voice2].freq = v2freq;

        // Select the correct SID, depending on the voice number.
        int sid_num = 0;
        if(voice2 > 2) {
            voice2 -= 3;
            sid_num = 1;
        }

        uint8_t offset = SID_VOICE_OFFSET*voice2;

        SID_Set_Reg(SID_MODE_VOL, (Master_Volume & 0x0F) |
            (inst->filt_mode & 0xF)<<4, sid_num);
        SID_Set_Reg(SID_FC_LO,inst->filt_cutoff & 0x0007, sid_num);
        SID_Set_Reg(SID_FC_HI, (inst->filt_cutoff & 0x07F8)>>3, sid_num);
        SID_Set_Reg(SID_RES_FILT, (inst->filt_resonance & 0xF)<<4 |
            (inst->filt_routing&0x0F), sid_num);
        if(!inst->portamento) {
            SID_Set_Reg(SID_V1_FREQ_HI+offset, (uint8_t)((v2freq & 0xFF00)>>8),
                sid_num);
            SID_Set_Reg(SID_V1_FREQ_LO+offset, (uint8_t)(v2freq & 0x00FF),
                sid_num);
        }
        SID_Set_Reg(SID_V1_AD+offset, inst->a2*16+inst->d2, sid_num);
        SID_Set_Reg(SID_V1_SR+offset, inst->s2*16+inst->r2, sid_num);
        SID_Set_Reg(SID_V1_PW_LO+offset, (uint8_t)(inst->duty_cycle2 & 0x00FF),
            sid_num);
        SID_Set_Reg(SID_V1_PW_HI+offset,
            (uint8_t)((inst->duty_cycle2 & 0xFF00)>>8), sid_num);
        SID_Set_Reg(SID_V1_CONTROL+offset, inst->voice2, sid_num);
    }
}

/** Shuts off a note being played.
*/
void SID_Note_Off(uint8_t key, uint8_t channel)
{
    // Search for the note being played.
    int16_t key_V2 = ((int16_t)key)*SECONDVOICE; // Code for the second voice
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(    (Voices[i].channel==channel) &&
              ((Voices[i].key == key)
            || (Voices[i].key == -key)
            || (Voices[i].key == key_V2)
            || (Voices[i].key == -key_V2)))
        {
            // TODO: pedal should be associated to a channel.
            if(SustainPedal[channel] && Voices[i].key>0) {
                // If the pedal is depressed, mark note as sustained.
                Voices[i].key=-Voices[i].key;
             } else if(!SustainPedal[channel]) {
                // If not, switch off the note.
                SID_Stop_Voice(i);
                Voices[i].key=0;
                Voices[i].oldfreq=Voices[i].freq;
            }
        }
    }
}

/** Send a stop command to a SID oscillator.
    Frees the voice.
*/
void SID_Stop_Voice(uint8_t v_t)
{
    uint8_t sid_num = 0;
    uint8_t voice = v_t;
    Voices[v_t].key=0;
    Voices[v_t].oldfreq=Voices[v_t].freq;
    // Voices 0,1,2 are attributed to SID0, voices 3,4,5 to SID1
    if(voice > 2) {
        voice -= 3;
        sid_num = 1;
    }
    uint8_t offset = SID_VOICE_OFFSET*voice;
    // Here offset contains the offset for the voice and sid_num the SID #.
    SID_Set_Reg(SID_V1_CONTROL+SID_VOICE_OFFSET*voice,
        Voices[v_t].voice & 0xFE, sid_num);
}

/* End of SID interface functions */

void UpdateLFO(void)
{
    uint16_t freq;
    uint8_t sid_num=0;
    uint8_t vv;

    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].key!=0) {
            if(i > 2) {
                vv = i- 3;
                sid_num = 1;
            } else {
                vv=i;
            }
            uint8_t offset = SID_VOICE_OFFSET*vv;

            float alpha = 1;
            if(Voices[i].inst.portamento)
                alpha = (float)(counter-Voices[i].timestamp)/
                    Voices[i].inst.portamento;
            if(alpha>1)
                alpha=1;
            if(alpha<0)
                alpha=0;

            freq = (uint16_t)(Voices[i].freq*alpha+ Voices[i].oldfreq*
                (1-alpha));
            freq += (uint16_t)(Voices[i].inst.lfo_depth/127.0*
                LFO_Table[LFO_Pointer]*(Voices[i].freq>>8));
            SID_Set_Reg(SID_V1_FREQ_HI+offset, (uint8_t)((freq & 0xFF00)>>8),
                sid_num);
            SID_Set_Reg(SID_V1_FREQ_LO+offset, (uint8_t)(freq & 0x00FF),
                sid_num);

        }
    }
    ++counter;
}

/** Gets the voice for the first FM operator.
    If the result is 0 or 1, this means that the voice 3 of SID0 has to be used.
    If the result is 3 or 4, this means that the voice 3 of SID1 has to be used.
*/
uint8_t GetFMVoices(int key, uint8_t channel)
{
    // Try to see if SID0, voice 3 is free
    if(Voices[2].key==0) {
        // try to see if SID0, voice 1 is free
        if(Voices[0].key==0)
            return 0;
        // try to see if SID0, voice 2 is free
        if(Voices[1].key==0)
            return 1;
    }

    // Try to see if SID1, voice 3 is free
    if(Voices[5].key==0) {
        // try to see if SID1, voice 1 is free
        if(Voices[3].key==0)
            return 3;
        // try to see if SID1, voice 2 is free
        if(Voices[4].key==0)
            return 4;
    }
    // If no voice 3 is available, use the SID that has been used first
    if(Voices[2].timestamp<Voices[5].timestamp) {
        // Stop voices being played.
        SID_Stop_Voice(0);
        SID_Stop_Voice(2);
        return 0;
    } else {
        // Stop voices being played.
        SID_Stop_Voice(3);
        SID_Stop_Voice(5);
        return 3;
    }

    return 0;
}

/** Get the first available voice to play the given key.
    Return NUM_VOICES if no voice is available
*/
uint8_t GetFreeVoice(int key, uint8_t channel)
{
    // Order in which the voices are attributed. Try to share them between the
    // two SIDs in the best possible way. The main problem is that the filter
    // setting of the last played note will interact with the filter settings
    // of all the SID. If only two notes are played, that at least is OK.
    uint8_t or[]={0,3,1,4,2,5};
    uint8_t i;
    // Search if the same note has already have been played on the same
    // channel. In this case, we are going to reuse the same voice.
    // Sustained notes have key<0;
    for(uint8_t k=0; k<NUM_VOICES; ++k) {
        i=or[k];
        if(Voices[i].key==key && Voices[i].channel==channel) {
            return i;
        }
    }    // Search if there is a voice not being played.
    for(uint8_t k=0; k<NUM_VOICES; ++k) {
        i=or[k];
        if(Voices[i].key==0) {
            // If it is found, use it.
            return i;
        }
    }
    // If pedal is depressed, select the oldest voice currently sustained.
    uint32_t oldest = Voices[0].timestamp;
    uint8_t  pos = 0;       // Position being tested.
    uint8_t  susnotes = 0;  // Flag: if it is != 0, there are sustained notes.
    // Search for the oldest sustained note.
    for(uint8_t k=0; k<NUM_VOICES; ++k) {
        i=or[k];
        if(Voices[i].key<0 && Voices[i].timestamp<=oldest) {
            pos = i;
            susnotes = 1;
        }
    }
    // Now, pos contains the position of the oldest sustained note.
    if(susnotes) {
        // Be positively sure that the note is stopped. It seems that sometimes
        // it does not work reliably.
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        // Free the place

        return pos;
    }
    // If there are no sustained notes, pick up the oldest note currently on.
    for(uint8_t k=0; k<NUM_VOICES; ++k) {
        i=or[k];
        if(Voices[i].timestamp<oldest) {
            pos = i;
        }
    }
    // Be positively sure that the note is stopped. It seems that sometimes
    // it does not work reliably.
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    Voices[pos].key=0;
    Voices[pos].oldfreq=Voices[pos].freq;
    return pos;
}