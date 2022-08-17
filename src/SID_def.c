#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "SID_def.h"


uint16_t C64_freq_table[]={268,284,301,318,337,358,379,401,425,451,477,506,536,
    568,602,637,675,716,758,803,851,902,955,1012,1072,1136,1204,1275,1351,1432,
    1517,1607,1703,1804,1911,2025,2145,2273,2408,2551,2703,2864,3034,3215,
    3406,3608,3823,4050,4291,4547,4817,5103,5407,5728,6069,6430,6812,7217,
    7647,8101,8583,9094,9634,10207,10814,11457,12139,12860,13625,14435,15294,
    16203,17167,18188,19269,20415,21629,22915,24278,25721,27251,28871,30588,
    32407,34334,36376,38539,40830,43258,45830,48556,51443,54502,57743,61176,
    64814};

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
//   A  D  S  R   Duty FM CTFF RES ROUT WAVE         NAME               NUMBER
//   |  |  |  |    |   |   |    |  |    |            |                     |
    {1 ,11,0 ,2 ,2048,LO,300 ,  0,ALL,PULSE,"Acoustic Grand Piano"},    // 0
    {1 ,11,0 ,2 ,1024,LO,392 ,  0,ALL,PULSE,"Bright Acoustic Piano"},   // 1
    {1 ,10,0 ,1 ,2048,LO,512 ,  4,ALL,PULSE,"Electric Grand Piano"},    // 2
    {1 ,10,0 ,1 , 0  ,LO,128 ,  8,ALL,SAWTH,"Honky-tonk Piano"},        // 3   Would add a "chorus" effect here!
    {1 ,10,0 ,1 , 512,LO,400 ,  4,ALL,PULSE,"Electric Piano 1"},        // 4
    {1 ,11,0 ,2 , 512,LO,300 ,  4,ALL,PULSE,"Electric Piano 2"},        // 5
    {1 ,10,0 ,1 , 0  ,LO,245 ,  0,NON,SAWTH,"Harpsicord"},              // 6
    {1 ,10,0 ,1 , 512,LO,1800,  0,ALL,PULSE,"Clavi"},                   // 7
    {0 ,10,0 ,3 ,2048,LO,128 ,  0,NON,TRIAN,"Celesta"},                 // 8
    {1 ,5 ,0 ,4 , 0  ,LO,1024,  0,NON,SAWTH,"Glockenspiel"},            // 9
    {0 ,6 ,0 ,6 , 0  ,LO,300 ,  0,ALL,TRIAN,"Music box"},               // 10
    {0 ,6 ,0 ,6 , 0  ,LO,512 ,  0,ALL,TRIAN,"Vibraphone"},              // 11
    {0 ,6 ,0 ,5 , 0  ,LO,1024,  0,NON,TRIAN,"Marimba"},                 // 12
    {0 ,6 ,0 ,8 , 0  ,LO,1024,  0,NON,TRIAN,"Xylophone"},               // 13
    {0 ,6 ,0 ,6 , 0  ,LO,128 ,  0,ALL,TRIAN,"Tubular Bells"},           // 14
    {0 ,6 ,0 ,6 , 0  ,LO,1024,  0,NON,TRIAN,"Dulcimer"},                // 15
    {2 ,1 ,15,1 , 0  ,LO,512 ,  4,ALL,TRIAN,"Drawbar Organ"},           // 16
    {1 ,1 ,13,1 , 0  ,LO,512 ,  4,NON,SAWTH,"Percussive Organ"},        // 17
    {1 ,2 ,13,1 , 0  ,LO,300 ,  4,ALL,TRIAN,"Rock Organ"},              // 18
    {1 ,2 ,15,1 , 0  ,LO,512 ,  4,NON,TRIAN,"Church Organ"},            // 19
    {1 ,1 ,15,2 ,1024,LO,128 ,  4,ALL,PULSE,"Reed Organ"},              // 20
    {1 ,1 ,15,2 , 512,LO,256 ,  0,ALL,PULSE,"Accordion"},               // 21
    {1 ,2 ,13,2 , 512,LO,512 ,  0,NON,PULSE,"Harmonica"},               // 22
    {1 ,2 ,12,2 ,1024,LO,512 ,  0,NON,PULSE,"Tango Accordion"},         // 23
    {0 ,10,0 ,4 , 256,LO,1024,  0,NON,TRIAN,"Acoustic Guitar (nylon)"}, // 24
    {0 ,10,0 ,4 , 256,LO,256 ,  0,ALL,SAWTH,"Acoustic Guitar (steel)"}, // 25
    {0 ,10,0 ,5 , 256,LO,1024,  0,NON,SAWTH,"Electric Guitar (jazz)"},  // 26
    {0 ,10,0 ,5 , 256,LO,1024,  0,NON,TRIAN,"Electric Guitar (clean)"}, // 27
    {0 ,11,0 ,2 , 256,LO,1024,  0,NON,TRIAN,"Electric Guitar (muted)"}, // 28
    {0 ,12,0 ,3 , 512,LO,128 ,  9,ALL,PULSE,"Overdriven Guitar"},       // 29
    {0 ,12,0 ,3 , 256,LO,512 ,  8,ALL,PULSE,"Distortion Guitar"},       // 30
    {0 ,12,0 ,3 , 0  ,HI,256 ,  7,ALL,TRIAN,"Guitar Harmonics"},        // 31
    {1 ,12,0 ,3 , 0  ,LO,200 ,  0,ALL,TRIAN,"Acoustic Bass"},           // 32
    {1 ,12,0 ,4 , 0  ,LO,250 ,  8,ALL,TRIAN,"Electric Bass (finger)"},  // 33
    {1 ,12,0 ,4 , 0  ,LO,792 ,  8,ALL,TRIAN,"Electric Bass (pick)"},    // 34
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Fretless Bass*"},          // 35
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Slap Bass 1*"},            // 36
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Slap Bass 2*"},            // 37
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Bass 1*"},           // 38
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Bass 2*"},           // 39
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Violin*"},                 // 40
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Viola*"},                  // 41
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Cello*"},                  // 42
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Contrabass*"},             // 43
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Tremolo Strings*"},        // 44
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pizzicato Strings*"},      // 45
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Orchestral Harp*"},        // 46
    {0 ,10,0 ,10, 0  ,LO,512 ,  0,ALL,NOISE,"Timpani"},                 // 47
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"String Ensemble 1*"},      // 48
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"String Ensemble 2*"},      // 49
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Strings 1*"},        // 50
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Strings 2*"},        // 51
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Choir Aahs*"},             // 52
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Voice Oohs*"},             // 53
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Voice*"},            // 54
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Orchestra Hit*"},          // 55
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Trumpet*"},                // 56
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Trombone*"},               // 57
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Tuba*"},                   // 58
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Muted Trumpet*"},          // 59
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"French Horn*"},            // 60
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Brass Section*"},          // 61
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Brass 1*"},          // 62
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Brass 2*"},          // 63
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Soprano Sax*"},            // 64
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Alto Sax*"},               // 65
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Tenor Sax*"},              // 66
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Baritone Sax*"},           // 67
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Oboe*"},                   // 68
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"English Horn*"},           // 69
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Bassoon*"},                // 70
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Clarinet*"},               // 71
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Piccolo*"},                // 72
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Flute*"},                  // 73
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Recorder*"},               // 74
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pan Flute*"},              // 75
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Blown Bottle*"},           // 76
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Shakuhachi*"},             // 77
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Wistle*"},                 // 78
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Ocarina*"},                // 79
    {0 ,0 ,15, 0,2048,LO,1024,  0,NON,PULSE,"Lead 1 (square)"},         // 80
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,SAWTH,"Lead 2 (SAWTH)"},          // 81
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Lead 3 (calliope)*"},      // 82
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Lead 4 (chiff)*"},         // 83
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Lead 5 (charang)*"},       // 84
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Lead 6 (voice)*"},         // 85
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Lead 7 (fifths)*"},        // 86
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Lead 8 (bass+lead)*"},     // 87
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 1 (new age)*"},        // 88
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 2 (warm)*"},           // 89
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 3 (polysynth)*"},      // 90
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 4 (choir)*"},          // 91
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 5 (bowed)*"},          // 92
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 6 (metallic)*"},       // 93
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Pad 7 (halo)*"},           // 94
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 1 (rain)*"},            // 95
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 2 (soundtrack)*"},      // 96
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 3 (crystal)*"},         // 97
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 4 (athmosphere)*"},     // 98
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 5 (brightness)*"},      // 99
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 6 (goblins)*"},         // 100
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 7 (echoes)*"},          // 101
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"FX 8 (sci-fi)*"},          // 102
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Sitar*"},                  // 103
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Banjo*"},                  // 104
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Shamisen*"},               // 105
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Koto*"},                   // 106
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Kalimba*"},                // 107
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Bag Pipe*"},               // 108
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Fiddle*"},                 // 109
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Shanai*"},                 // 110
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Tinkle Bell*"},            // 111
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Agogo*"},                  // 112
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Steel Drums*"},            // 113
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Woodblock*"},              // 114
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Taiko Drum*"},             // 115
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Melodic Tom*"},            // 116
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Synth Drum*"},             // 117
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Reverse Cymbal*"},         // 118
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Guitar Fret Noise*"},      // 119
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Breath Noise*"},           // 120
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Seashore*"},               // 121
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Bird Tweet*"},             // 122
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Telephone Ring*"},         // 123
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Helicopter*"},             // 124
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Applause*"},               // 125
    {0 ,0 ,15, 0, 0  ,LO,1024,  0,NON,TRIAN,"Gunshot*"}                 // 126
}; 

/* SID interface functions */

volatile uint8_t Current_Instrument;
volatile uint8_t SustainPedal;
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

void SID_Note_On(uint8_t key, uint8_t velocity, uint8_t voice,
    SID_conf *inst)
{
    int sid_num = 0;

    if(voice > 2) {
        voice -= 3;
        sid_num = 1;
    }
    uint8_t offset = SID_VOICE_OFFSET*voice;

    key-=BASE_MIDI_NOTE;
    if (key >= COUNTOF(C64_freq_table))
        return;
    SID_Select(-1);

    SID_Set_Reg(SID_MODE_VOL, MAXVOL | (inst->filt_mode & 0xF)<<4, sid_num);
    SID_Set_Reg(SID_FC_LO,inst->filt_cutoff & 0x0007, sid_num);
    SID_Set_Reg(SID_FC_HI, (inst->filt_cutoff & 0x07F8)>>3, sid_num);
    SID_Set_Reg(SID_RES_FILT, (inst->filt_resonance & 0xF)<<4 | (inst->filt_routing&0x0F), sid_num);
    SID_Set_Reg(SID_V1_FREQ_HI+offset, (uint8_t)((C64_freq_table[key] & 0xFF00)>>8), sid_num);
    SID_Set_Reg(SID_V1_FREQ_LO+offset, (uint8_t)(C64_freq_table[key] & 0x00FF), sid_num);
    SID_Set_Reg(SID_V1_AD+offset, inst->a*16+inst->d, sid_num);
    SID_Set_Reg(SID_V1_SR+offset, inst->s*16+inst->r, sid_num);
    SID_Set_Reg(SID_V1_PW_LO+offset, (uint8_t)(inst->duty_cycle & 0x00FF), sid_num);
    SID_Set_Reg(SID_V1_PW_HI+offset, (uint8_t)((inst->duty_cycle & 0xFF00)>>8), sid_num);
    SID_Set_Reg(SID_V1_CONTROL+offset, inst->voice, sid_num);
}

void SID_Note_Off(uint8_t voice)
{
    int sid_num = 0;

    if(voice > 2) {
        voice -= 3;
        sid_num = 1;
    }
    uint8_t offset = SID_VOICE_OFFSET*voice;

    SID_Set_Reg(SID_V1_AD+offset, 0, sid_num);
    SID_Set_Reg(SID_V1_SR+offset, 0, sid_num);
    SID_Set_Reg(SID_V1_CONTROL+SID_VOICE_OFFSET*voice,
        GeneralMIDI[Current_Instrument].voice & 0xFE, sid_num);
}

/* End of SID interface functions */

VoiceDef Voices[NUM_VOICES];

/** Return NUM_VOICES if no voice is available */
uint8_t GetFreeVoice(int key)
{
    // Search if the same note has already have been played or sustained.
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
        if(Voices[i].key<0 && Voices[i].timestamp<oldest) {
            pos = i;
            susnotes = 1;
        }
    }
    if(susnotes) {
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        SID_Note_Off(pos);
        Voices[pos].key=0;
        return pos;
    }
    // If there are no sustained notes, pick up the oldest note currently on.
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].timestamp<oldest) {
            pos = i;
        }
    }
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    SID_Note_Off(pos);
    Voices[pos].key=0;
    return pos;
}