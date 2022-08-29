#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "SID_def.h"

// Track how many notes we have played since the last reset.
uint32_t now;

VoiceDef Voices[NUM_VOICES];

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

/*  268,284,301,318,337,358,379,401,425,451,477,506,536,
    568,602,637,675,716,758,803,851,902,955,1012,1072,1136,1204,1275,1351,1432,
    1517,1607,1703,1804,1911,2025,2145,2273,2408,2551,2703,2864,3034,3215,
    3406,3608,3823,4050,4291,4547,4817,5103,5407,5728,6069,6430,6812,7217,
    7647,8101,8583,9094,9634,10207,10814,11457,12139,12860,13625,14435,15294,
    16203,17167,18188,19269,20415,21629,22915,24278,25721,27251,28871,30588,
    32407,34334,36376,38539,40830,43258,45830,48556,51443,54502,57743,61176,
    64814};  // NTSC */

/******************************************************************************
    General MIDI patches
******************************************************************************/

/* A, D, S and R range from 0 to 15.
   Duty cycle applies for PULSE waveform and range from 0 to 4095. Value 2048
   represents a square wave.
   CTFF (cutoff frequency) varies between 0 and 2048.
   Res(onance) varies between 0 (no resonance) and 15 (max resonance)
   We measure instrument number starting from 0.
   Asterisk "*" means NOT TESTED YET.
*/

SID_conf GeneralMIDI[256] = {
//   FIRST VOICE-----------------------------     SECOND VOICE ----------
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE DIFF A  D  S  R  Duty WAVE   NAME               NUMBER
//   |  |  |  |    |   |   |    |  |    |     |   |  |  |  |   |    |      |
    {2 ,11,0 ,2 ,1632,LO,624 , 0,ALL,PULSE,1200, 2, 8, 0, 1,   0, TRIAN,"Acoustic Grand Piano"},    // 0
    {2 ,11,0 ,2 ,1184,LO,416 , 0,ALL,PULSE,1200, 1, 8, 0, 2,   0, SAWTH,"Bright Acoustic Piano"},   // 1
    {2 ,10,0 ,1 ,2048,LO,512 , 4,ALL,PULSE,1200, 1,10, 0, 1, 512, PULSE,"Electric Grand Piano"},    // 2
    {1 ,8 ,0 ,1 , 0  ,LO,600 ,10,ALL,SAWTH, 610, 1,10, 0, 1,2048, PULSE,"Honky-tonk Piano"},        // 3   Would add a "chorus" effect here!
    {1 ,10,0 ,1 , 512,LO,400 ,10,ALL,PULSE, 600, 0, 0, 0, 0,   0, NONE ,"Electric Piano 1"},        // 4
    {1 ,11,0 ,2 , 512,LO,300 , 4,ALL,PULSE,1200, 1, 6, 0, 1,   0, SAWTH,"Electric Piano 2"},        // 5
    {1 ,10,0 ,1 , 0  ,LO,245 , 0,NON,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,"Harpsicord"},              // 6
    {0 ,10,0 ,6 , 0  ,LO,864 ,15,ALL,SAWTH,1800, 0, 10,0, 6,   0, SAWTH,"Clavi"},                   // 7
    {0 ,10,0 ,3 ,2048,LO,128 , 0,NON,TRIAN,2400, 0, 8, 0, 2,   0, TRIAN,"Celesta"},                 // 8
    {1 ,9 ,0 ,4 , 0  ,LO,1024, 0,NON,TRIAN,2400, 0, 8, 0, 3,2048, PULSE,"Glockenspiel"},            // 9
    {0 ,6 ,0 ,6 , 0  ,LO,300 , 0,NON,TRIAN,2400, 0, 5, 0, 5,   0, SAWTH,"Music box"},               // 10
    {0 ,11,0 ,3 , 0  ,LO,512 , 0,NON,TRIAN,1200, 1,10, 0, 3,   0, TRIAN,"Vibraphone"},              // 11
    {0 ,6 ,0 ,5 , 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Marimba"},                 // 12
    {0 ,6 ,0 ,8 , 0  ,LO,1024, 0,NON,TRIAN,2400, 0, 4, 0, 4,   0, TRIAN,"Xylophone"},               // 13
    {0 ,10,0 ,6 , 0  ,LO,700 ,15,ALL,TRIAN,1805, 0, 10,0, 6,   0, SAWTH,"Tubular Bells"},           // 14
    {0 ,6 ,0 ,2 , 0  ,LO,1024, 0,NON,SAWTH, 600, 0, 6, 0, 2,   0, NONE ,"Dulcimer"},                // 15
    {2 ,1 ,15,1 , 0  ,LO,512 , 4,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Drawbar Organ"},           // 16
    {1 ,1 ,13,1 , 0  ,LO,1024, 4,ALL,SAWTH,   0, 0, 6, 0, 1,2048, PULSE,"Percussive Organ"},        // 17
    {1 ,2 ,13,1 , 0  ,LO,624 , 4,ALL,TRIAN,1200, 1, 6, 0, 4,   0, TRIAN,"Rock Organ"},              // 18
    {1 ,2 ,15,1 , 0  ,LO,512 , 4,NON,TRIAN,2400, 1, 2,15, 1,   0, TRIAN,"Church Organ"},            // 19
    {1 ,1 ,15,2 ,1024,LO,128 , 4,ALL,PULSE, 600, 1, 1,15, 1,   0, TRIAN,"Reed Organ"},              // 20
    {1 ,1 ,15,2 , 512,LO,320 , 0,ALL,PULSE,   0, 0, 0, 0, 0,   0, NONE ,"Accordion"},               // 21
    {1 ,2 ,13,2 , 512,LO,512 , 0,ALL,PULSE,1200, 1, 2,13, 2,   0, TRIAN,"Harmonica"},               // 22
    {1 ,2 ,12,2 ,1024,LO,512 , 0,NON,PULSE,   0, 0, 0, 0, 0,   0, NONE ,"Tango Accordion"},         // 23
    {0 ,10,0 ,4 , 256,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Acoustic Guitar (nylon)"}, // 24
    {0 ,10,0 ,4 , 256,LO,256 , 0,ALL,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,"Acoustic Guitar (steel)"}, // 25
    {0 ,10,0 ,5 , 256,LO,1024, 0,NON,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,"Electric Guitar (jazz)"},  // 26
    {0 ,10,0 ,5 , 256,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Electric Guitar (clean)"}, // 27
    {0 ,11,0 ,2 , 256,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Electric Guitar (muted)"}, // 28
    {0 ,12,0 ,3 , 512,LO, 432, 9,ALL,PULSE,   0, 0, 0, 0, 0,   0, NONE ,"Overdriven Guitar"},       // 29
    {0 ,12,0 ,3 , 256,LO, 512, 8,ALL,PULSE, 600, 0,12, 0, 3, 512, PULSE,"Distortion Guitar"},       // 30
    {0 ,12,0 ,3 , 0  ,HI, 256, 7,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Guitar Harmonics"},        // 31
    {1 ,12,0 ,3 , 0  ,LO, 200, 0,ALL,TRIAN, 600, 0,12, 0, 3,   0, TRIAN,"Acoustic Bass"},           // 32
    {1 ,12,0 ,4 , 0  ,LO, 250, 8,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Electric Bass (finger)"},  // 33
    {1 ,12,0 ,4 , 0  ,LO, 792, 8,ALL,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Electric Bass (pick)"},    // 34
    {2 ,11,0 , 3, 0  ,LO, 688,15,ALL,SAWTH, 600, 2,10, 0, 3,   0, NONE ,"Fretless Bass"},           // 35
    {2 ,11,0 , 3, 0  ,LO, 688,15,ALL,SAWTH, 600, 2,10, 0, 3,   0, NONE ,"Slap Bass 1*"},            // 36
    {2 ,11,0 , 3, 0  ,LO, 688,15,ALL,SAWTH, 600, 2,10, 0, 3,   0, NONE ,"Slap Bass 2*"},            // 37
    {2 ,11,0 , 3, 0  ,LO, 688,15,ALL,SAWTH, 600, 2,10, 0, 3,   0, NONE ,"Synth Bass 1*"},           // 38
    {2 ,11,0 , 3, 0  ,LO, 688,15,ALL,SAWTH, 600, 2,10, 0, 3,   0, NONE ,"Synth Bass 2*"},           // 39
    {5 ,10,13, 3, 704,LO, 752, 5,ALL,PULSE, 600, 6, 8,11, 5,   0, SAWTH,"Violin"},                 // 40
    {6 ,10,13, 3, 730,LO, 600, 5,ALL,PULSE, 600, 6, 8,11, 5,   0, SAWTH,"Viola"},                  // 41
    {7 ,10,13, 3, 768,LO, 550, 5,ALL,PULSE, 600, 6, 8,11, 9,   0, SAWTH,"Cello"},                  // 42
    {8 ,10,13, 3, 900,LO, 480, 5,ALL,PULSE, 600, 6, 8,11, 9,   0, SAWTH,"Contrabass"},             // 43
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Tremolo Strings*"},        // 44
    {0 , 9, 0, 2, 0  ,LO, 800, 6,ALL,SAWTH, 598, 1, 9,0, 1,1824, PULSE,"Pizzicato Strings"},      // 45
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Orchestral Harp*"},        // 46
    {0 ,10,0 ,10, 0  ,LO, 512, 0,ALL,NOISE, 600, 0,10, 0,10,   0, NOISE,"Timpani"},                 // 47
    {7 , 7,13, 3, 0  ,LO, 496, 2,ALL,SAWTH, 602, 6, 8,11, 5,   0, SAWTH,"String Ensemble 1"},      // 48
    {7 , 7,13, 3, 0  ,LO, 800, 6,ALL,SAWTH, 598, 6, 8,11, 5,   0, SAWTH,"String Ensemble 2"},      // 49
    {7 , 7,13, 3, 0  ,LO, 496, 2,ALL,SAWTH, 598, 6, 8,11, 5,1824, PULSE,"Synth Strings 1"},        // 50
    {7 , 7,13, 3, 0  ,LO, 800, 6,ALL,SAWTH, 598, 6, 8,11, 5,1824, PULSE,"Synth Strings 2"},        // 51
    {7 , 7,13, 3, 0  ,BP, 608, 6,ALL,SAWTH,1203, 6, 8,11, 5, 672, PULSE,"Choir Aahs*"},             // 52
    {7 , 7,13, 3, 0  ,BP, 224, 6,ALL,SAWTH,1203, 6, 8,11, 5, 672, NONE ,"Voice Oohs*"},             // 53
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Synth Voice*"},            // 54
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Orchestra Hit*"},          // 55
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Trumpet*"},                // 56
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Trombone*"},               // 57
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Tuba*"},                   // 58
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Muted Trumpet*"},          // 59
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"French Horn*"},            // 60
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Brass Section*"},          // 61
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Synth Brass 1*"},          // 62
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Synth Brass 2*"},          // 63
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Soprano Sax*"},            // 64
    {5 ,10,13, 3, 768,LO, 550,15,ALL,PULSE, 600, 5, 8,11, 9,   0, SAWTH,"Alto Sax"},               // 65
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Tenor Sax*"},              // 66
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Baritone Sax*"},           // 67
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Oboe*"},                   // 68
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"English Horn*"},           // 69
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Bassoon*"},                // 70
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Clarinet*"},               // 71
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Piccolo*"},                // 72
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Flute*"},                  // 73
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Recorder*"},               // 74
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pan Flute*"},              // 75
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Blown Bottle*"},           // 76
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Shakuhachi*"},             // 77
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Wistle*"},                 // 78
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Ocarina*"},                // 79
    {0 ,0 ,15, 0,2048,LO,1024, 0,NON,PULSE,   0, 0, 0, 0, 0,   0, NONE ,"Lead 1 (square)"},         // 80
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,SAWTH,   0, 0, 0, 0, 0,   0, NONE ,"Lead 2 (SAWTH)"},          // 81
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Lead 3 (calliope)*"},      // 82
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Lead 4 (chiff)*"},         // 83
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Lead 5 (charang)*"},       // 84
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Lead 6 (voice)*"},         // 85
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Lead 7 (fifths)*"},        // 86
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Lead 8 (bass+lead)*"},     // 87
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 1 (new age)*"},        // 88
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 2 (warm)*"},           // 89
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 3 (polysynth)*"},      // 90
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 4 (choir)*"},          // 91
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 5 (bowed)*"},          // 92
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 6 (metallic)*"},       // 93
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Pad 7 (halo)*"},           // 94
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 1 (rain)*"},            // 95
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 2 (soundtrack)*"},      // 96
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 3 (crystal)*"},         // 97
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 4 (athmosphere)*"},     // 98
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 5 (brightness)*"},      // 99
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 6 (goblins)*"},         // 100
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 7 (echoes)*"},          // 101
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"FX 8 (sci-fi)*"},          // 102
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Sitar*"},                  // 103
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Banjo*"},                  // 104
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Shamisen*"},               // 105
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Koto*"},                   // 106
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Kalimba*"},                // 107
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Bag Pipe*"},               // 108
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Fiddle*"},                 // 109
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Shanai*"},                 // 110
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Tinkle Bell*"},            // 111
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Agogo*"},                  // 112
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Steel Drums*"},            // 113
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Woodblock*"},              // 114
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Taiko Drum*"},             // 115
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Melodic Tom*"},            // 116
    {2 , 9,0 ,10, 0  ,BP, 112, 0,ALL,NOISE,1200, 1,10, 0,10,   0, NOISE,"Synth Drum"},             // 117
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Reverse Cymbal*"},         // 118
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Guitar Fret Noise*"},      // 119
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Breath Noise*"},           // 120
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Seashore*"},               // 121
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Bird Tweet*"},             // 122
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Telephone Ring*"},         // 123
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Helicopter*"},             // 124
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Applause*"},               // 125
    {0 ,0 ,15, 0, 0  ,LO,1024, 0,NON,TRIAN,   0, 0, 0, 0, 0,   0, NONE ,"Gunshot*"}                 // 126
}; 

/* SID interface functions */

volatile int CurrInst;
volatile uint8_t SustainPedal;
volatile uint8_t Master_Volume;     // 0 to 15


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

void SID_Select(int id)
{
    if(id==1) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
    } else if (id==0) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
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

void SID_Note_On(uint8_t key_m, uint8_t velocity, SID_conf *inst)
{
    uint8_t key=key_m-BASE_MIDI_NOTE;

    if(inst->voice != NONE) {
        uint8_t voice = GetFreeVoice(key_m);
        Voices[voice].key=key_m;
        Voices[voice].timestamp=now++;

        int sid_num = 0;

        if(voice > 2) {
            voice -= 3;
            sid_num = 1;
        }
        uint8_t offset = SID_VOICE_OFFSET*voice;

        if (key >= COUNTOF(C64_freq_table))
            return;
        SID_Select(-1);

        SID_Set_Reg(SID_MODE_VOL, (Master_Volume & 0x0F) | (inst->filt_mode & 0xF)<<4, sid_num);
        SID_Set_Reg(SID_FC_LO,inst->filt_cutoff & 0x0007, sid_num);
        SID_Set_Reg(SID_FC_HI, (inst->filt_cutoff & 0x07F8)>>3, sid_num);
        SID_Set_Reg(SID_RES_FILT, (inst->filt_resonance & 0xF)<<4 | (inst->filt_routing&0x0F), sid_num);
        SID_Set_Reg(SID_V1_FREQ_HI+offset, (uint8_t)(((C64_freq_table[key]*FREQ_CORRECTION) & 0xFF00)>>8), sid_num);
        SID_Set_Reg(SID_V1_FREQ_LO+offset, (uint8_t)((C64_freq_table[key]*FREQ_CORRECTION) & 0x00FF), sid_num);
        SID_Set_Reg(SID_V1_AD+offset, inst->a*16+inst->d, sid_num);
        SID_Set_Reg(SID_V1_SR+offset, inst->s*16+inst->r, sid_num);
        SID_Set_Reg(SID_V1_PW_LO+offset, (uint8_t)(inst->duty_cycle & 0x00FF), sid_num);
        SID_Set_Reg(SID_V1_PW_HI+offset, (uint8_t)((inst->duty_cycle & 0xFF00)>>8), sid_num);
        SID_Set_Reg(SID_V1_CONTROL+offset, inst->voice, sid_num);
    }

    // Check if a second voice is present and if yes play it.
    if(inst->voice2 != NONE) {
        uint8_t voice = GetFreeVoice(key_m*SECONDVOICE);
        Voices[voice].key=((int16_t)key_m)*SECONDVOICE;
        Voices[voice].timestamp=now++;
        int sid_num = 0;
        if(voice > 2) {
            voice -= 3;
            sid_num = 1;
        }
        uint32_t v2freq_l = C64_freq_table[key]*FREQ_CORRECTION;
        if(v2freq_l*inst->diff>0)
            v2freq_l = (v2freq_l*inst->diff*2)/1200;
        else
            v2freq_l = (v2freq_l*1200)/inst->diff/2;
    
        if(v2freq_l>65536)
            return;
        
        int16_t v2freq = (int16_t) v2freq_l;
        uint8_t offset = SID_VOICE_OFFSET*voice;
        SID_Select(-1);
        SID_Set_Reg(SID_MODE_VOL, (Master_Volume & 0x0F) | (inst->filt_mode & 0xF)<<4, sid_num);
        SID_Set_Reg(SID_FC_LO,inst->filt_cutoff & 0x0007, sid_num);
        SID_Set_Reg(SID_FC_HI, (inst->filt_cutoff & 0x07F8)>>3, sid_num);
        SID_Set_Reg(SID_RES_FILT, (inst->filt_resonance & 0xF)<<4 | (inst->filt_routing&0x0F), sid_num);
        SID_Set_Reg(SID_V1_FREQ_HI+offset, (uint8_t)((v2freq & 0xFF00)>>8), sid_num);
        SID_Set_Reg(SID_V1_FREQ_LO+offset, (uint8_t)(v2freq & 0x00FF), sid_num);
        SID_Set_Reg(SID_V1_AD+offset, inst->a2*16+inst->d2, sid_num);
        SID_Set_Reg(SID_V1_SR+offset, inst->s2*16+inst->r2, sid_num);
        SID_Set_Reg(SID_V1_PW_LO+offset, (uint8_t)(inst->duty_cycle2 & 0x00FF), sid_num);
        SID_Set_Reg(SID_V1_PW_HI+offset, (uint8_t)((inst->duty_cycle2 & 0xFF00)>>8), sid_num);
        SID_Set_Reg(SID_V1_CONTROL+offset, inst->voice2, sid_num);
    }
}

void SID_Note_Off(uint8_t key)
{
    int16_t key_V2 = ((int16_t)key)*SECONDVOICE;
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(    (Voices[i].key == key)
            || (Voices[i].key == -key)
            || (Voices[i].key == key_V2)
            || (Voices[i].key == -key_V2))
        {
            if(SustainPedal && Voices[i].key>0) {
                // If the pedal is depressed, mark note as sustained.
                Voices[i].key=-Voices[i].key;
             } else if(!SustainPedal) {
                SID_Stop_Voice(i);
                Voices[i].key=0;
            }
        }
    }
}

void SID_Stop_Voice(uint8_t voice)
{
    uint8_t sid_num = 0;
    if(voice > 2) {
        voice -= 3;
        sid_num = 1;
    }
    uint8_t offset = SID_VOICE_OFFSET*voice;

    SID_Set_Reg(SID_V1_CONTROL+SID_VOICE_OFFSET*voice,
        GeneralMIDI[CurrInst].voice & 0xFE, sid_num);
}

/* End of SID interface functions */



/** Return NUM_VOICES if no voice is available */
uint8_t GetFreeVoice(int key)
{
    // Search if the same note has already have been played.
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].key==key) {
            return i;
        }
    }    // Search if there is a voice not playing.
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].key==0) {
            return i;
        }
    }
    // If pedal is depressed, select the oldest voice currently sustained.
    
    uint32_t oldest = Voices[0].timestamp;
    uint8_t  pos = 0;
    uint8_t  susnotes = 0;  // Flag: if != 0, there are sustained notes.
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].key<0 && Voices[i].timestamp<=oldest) {
            pos = i;
            susnotes = 1;
        }
    }
    if(susnotes) {
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        SID_Stop_Voice(pos);
        Voices[pos].key=0;
        return pos;
    }
    // If there are no sustained notes, pick up the oldest note currently on.
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].timestamp<oldest) {
            pos = i;
        }
    }
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    SID_Stop_Voice(pos);
    Voices[pos].key=0;
    return pos;
}