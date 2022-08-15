/**
  ******************************************************************************
  * @file    main.c
  * @author  Davide Bucci
  * @version V0.1
  * @date    August 14, 2022
  * @brief   This file is the main file of the MIDI2SwinSID project.
  *          
  ******************************************************************************
 */

/*
 Compile with "make" in the "manual_compile" directory.
 Flash command:  st-flash write Display.bin 0x8000000

*/
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <stdio.h>

/* UART handler declaration */
volatile UART_HandleTypeDef UartHandle;

/* Buffer used for reception */
uint8_t aRxBuffer[RXBUFFERSIZE];

int receive=0;

#define BASE_MIDI_NOTE 24
uint16_t C64_freq_table[]={268,284,301,318,337,358,379,401,425,451,477,506,536,
    568,602,637,675,716,758,803,851,902,955,1012,1072,1136,1204,1275,1351,1432,
    1517,1607,1703,1804,1911,2025,2145,2273,2408,2551,2703,2864,3034,3215,
    3406,3608,3823,4050,4291,4547,4817,5103,5407,5728,6069,6430,6812,7217,
    7647,8101,8583,9094,9634,10207,10814,11457,12139,12860,13625,14435,15294,
    16203,17167,18188,19269,20415,21629,22915,24278,25721,27251,28871,30588,
    32407,34334,36376,38539,40830,43258,45830,48556,51443,54502,57743,61176,
    64814};
    
typedef struct SID_conf_tag {
    uint8_t  ad;
    uint8_t  sr;
    uint16_t duty_cycle;
    uint8_t  voice;
    char    *name;
} SID_conf;

volatile uint8_t Current_Instrument;
volatile uint8_t SustainPedal;

// SID waveforms  (+ gate)
#define NOISE       129
#define PULSE       65
#define SAWTOOTH    33
#define TRIANGLE    17

// We measure instruments starting from 0.
// Asterisk "*" means NOT TESTED YET.
SID_conf GeneralMIDI[256] = {
//   A     D   S     R   Duty   WAVE          NAME
//   |     |   |     |    |       |            |
    {0 *16+11, 0 *16+2 , 0  ,  TRIANGLE, "Acoustic Grand Piano"},   // 0
    {1 *16+10, 0 *16+1 , 0  ,  SAWTOOTH, "Bright Acoustic Piano"},  // 1
    {1 *16+11, 0 *16+2 , 256,  PULSE,    "Electric Grand Piano"},   // 2
    {0 *16+10, 0 *16+1 , 0  ,  SAWTOOTH, "Honky-tonk Piano"},       // 3
    {1 *16+10, 0 *16+1 , 1024, PULSE,    "Electric Piano 1"},       // 4
    {1 *16+11, 0 *16+2 , 512,  PULSE,    "Electric Piano 2"},       // 5
    {1 *16+10, 0 *16+1 , 128,  PULSE,    "Harpsicord"},             // 6
    {1 *16+10, 0 *16+1 , 192,  PULSE,    "Clavi"},                  // 7
    {0 *16+3 , 0 *16+3 , 0  ,  TRIANGLE, "Celesta"},                // 8
    {0 *16+4 , 0 *16+4 , 0  ,  SAWTOOTH, "Glockenspiel"},           // 9
    {0 *16+6 , 0 *16+6 , 0  ,  TRIANGLE, "Music box"},              // 10
    {0 *16+6 , 0 *16+6 , 0  ,  TRIANGLE, "Vibraphone*"},            // 11
    {0 *16+6 , 0 *16+6 , 0  ,  TRIANGLE, "Marimba*"},               // 12
    {0 *16+6 , 0 *16+6 , 0  ,  TRIANGLE, "Xylophone*"},             // 13
    {0 *16+6 , 0 *16+6 , 0  ,  TRIANGLE, "Tubular Bells*"},         // 14
    {0 *16+6 , 0 *16+6 , 0  ,  TRIANGLE, "Dulcimer*"},              // 15
    {1 *16+1 , 15*16+1 , 0  ,  TRIANGLE, "Drawbar Organ"},          // 16
    {0 *16+1 , 13*16+1 , 0  ,  SAWTOOTH, "Percussive Organ"},       // 17
    {0 *16+2 , 13*16+1 , 0  ,  TRIANGLE, "Rock Organ"},             // 18
    {1 *16+2 , 15*16+1 , 0  ,  TRIANGLE, "Church Organ"},           // 19
    {1 *16+0 , 15*16+1 , 1024, PULSE   , "Reed Organ"},             // 20
    {0+0 *16, 0+15*16, 0 , 17, "Accordion*"},
    {0+0 *16, 0+15*16, 0 , 17, "Harmonica*"},
    {0+0 *16, 0+15*16, 0 , 17, "Tango Accordion*"},
    {0+0 *16, 0+15*16, 0 , 17, "Acoustic Guitar (nylon)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Acoustic Guitar (steel)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Electric Guitar (jazz)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Electric Guitar (clean)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Electric Guitar (muted)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Overdriven Guitar*"},
    {0+0 *16, 0+15*16, 0 , 17, "Distortion Guitar*"},
    {0+0 *16, 0+15*16, 0 , 17, "Guitar Harmonics*"},
    {0+0 *16, 0+15*16, 0 , 17, "Acoustic Bass*"},
    {0+0 *16, 0+15*16, 0 , 17, "Electric Bass (finger)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Electric Bass (pick)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Fretless Bass*"},
    {0+0 *16, 0+15*16, 0 , 17, "Slap Bass 1*"},
    {0+0 *16, 0+15*16, 0 , 17, "Slap Bass 2*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Bass 1*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Bass 2*"},
    {0+0 *16, 0+15*16, 0 , 17, "Violin*"},
    {0+0 *16, 0+15*16, 0 , 17, "Viola*"},
    {0+0 *16, 0+15*16, 0 , 17, "Cello*"},
    {0+0 *16, 0+15*16, 0 , 17, "Contrabass*"},
    {0+0 *16, 0+15*16, 0 , 17, "Tremolo Strings*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pizzicato Strings*"},
    {0+0 *16, 0+15*16, 0 , 17, "Orchestral Harp*"},
    {0+0 *16, 0+15*16, 0 , 17, "Timpani*"},
    {0+0 *16, 0+15*16, 0 , 17, "String Ensemble 1*"},
    {0+0 *16, 0+15*16, 0 , 17, "String Ensemble 2*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Strings 1*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Strings 2*"},
    {0+0 *16, 0+15*16, 0 , 17, "Choir Aahs*"},
    {0+0 *16, 0+15*16, 0 , 17, "Voice Oohs*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Voice*"},
    {0+0 *16, 0+15*16, 0 , 17, "Orchestra Hit*"},
    {0+0 *16, 0+15*16, 0 , 17, "Trumpet*"},
    {0+0 *16, 0+15*16, 0 , 17, "Trombone*"},
    {0+0 *16, 0+15*16, 0 , 17, "Tuba*"},
    {0+0 *16, 0+15*16, 0 , 17, "Muted Trumpet*"},
    {0+0 *16, 0+15*16, 0 , 17, "French Horn*"},
    {0+0 *16, 0+15*16, 0 , 17, "Brass Section*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Brass 1*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Brass 2*"},
    {0+0 *16, 0+15*16, 0 , 17, "Soprano Sax*"},
    {0+0 *16, 0+15*16, 0 , 17, "Alto Sax*"},
    {0+0 *16, 0+15*16, 0 , 17, "Tenor Sax*"},
    {0+0 *16, 0+15*16, 0 , 17, "Baritone Sax*"},
    {0+0 *16, 0+15*16, 0 , 17, "Oboe*"},
    {0+0 *16, 0+15*16, 0 , 17, "English Horn*"},
    {0+0 *16, 0+15*16, 0 , 17, "Bassoon*"},
    {0+0 *16, 0+15*16, 0 , 17, "Clarinet*"},
    {0+0 *16, 0+15*16, 0 , 17, "Piccolo*"},
    {0+0 *16, 0+15*16, 0 , 17, "Flute*"},
    {0+0 *16, 0+15*16, 0 , 17, "Recorder*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pan Flute*"},
    {0+0 *16, 0+15*16, 0 , 17, "Blown Bottle*"},
    {0+0 *16, 0+15*16, 0 , 17, "Shakuhachi*"},
    {0+0 *16, 0+15*16, 0 , 17, "Wistle*"},
    {0+0 *16, 0+15*16, 0 , 17, "Ocarina*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 1 (square)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 2 (sawtooth)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 3 (calliope)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 4 (chiff)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 5 (charang)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 6 (voice)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 7 (fifths)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Lead 8 (bass+lead)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 1 (new age)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 2 (warm)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 3 (polysynth)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 4 (choir)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 5 (bowed)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 6 (metallic)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Pad 7 (halo)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 1 (rain)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 2 (soundtrack)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 3 (crystal)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 4 (athmosphere)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 5 (brightness)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 6 (goblins)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 7 (echoes)*"},
    {0+0 *16, 0+15*16, 0 , 17, "FX 8 (sci-fi)*"},
    {0+0 *16, 0+15*16, 0 , 17, "Sitar*"},
    {0+0 *16, 0+15*16, 0 , 17, "Banjo*"},
    {0+0 *16, 0+15*16, 0 , 17, "Shamisen*"},
    {0+0 *16, 0+15*16, 0 , 17, "Koto*"},
    {0+0 *16, 0+15*16, 0 , 17, "Kalimba*"},
    {0+0 *16, 0+15*16, 0 , 17, "Bag Pipe*"},
    {0+0 *16, 0+15*16, 0 , 17, "Fiddle*"},
    {0+0 *16, 0+15*16, 0 , 17, "Shanai*"},
    {0+0 *16, 0+15*16, 0 , 17, "Tinkle Bell*"},
    {0+0 *16, 0+15*16, 0 , 17, "Agogo*"},
    {0+0 *16, 0+15*16, 0 , 17, "Steel Drums*"},
    {0+0 *16, 0+15*16, 0 , 17, "Woodblock*"},
    {0+0 *16, 0+15*16, 0 , 17, "Taiko Drum*"},
    {0+0 *16, 0+15*16, 0 , 17, "Melodic Tom*"},
    {0+0 *16, 0+15*16, 0 , 17, "Synth Drum*"},
    {0+0 *16, 0+15*16, 0 , 17, "Reverse Cymbal*"},
    {0+0 *16, 0+15*16, 0 , 17, "Guitar Fret Noise*"},
    {0+0 *16, 0+15*16, 0 , 17, "Breath Noise*"},
    {0+0 *16, 0+15*16, 0 , 17, "Seashore*"},
    {0+0 *16, 0+15*16, 0 , 17, "Bird Tweet*"},
    {0+0 *16, 0+15*16, 0 , 17, "Telephone Ring*"},
    {0+0 *16, 0+15*16, 0 , 17, "Helicopter*"},
    {0+0 *16, 0+15*16, 0 , 17, "Applause*"},
    {0+0 *16, 0+15*16, 0 , 17, "Gunshot*"}
};


/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* MIDI receive state machine states */
volatile enum MIDIState {idle, note_on_k, note_on_v, note_off_k,
    note_off_v, control_c, control_v, program_c} MIDI_ReceiveState;

/* SID interface functions */
void ConfigureGPIOPorts(void)
{
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_Init_A={0};
    GPIO_InitTypeDef GPIO_Init_B={0};
    GPIO_InitTypeDef GPIO_Init_C={0};
    GPIO_InitTypeDef GPIO_Init_D={0};
    GPIO_InitTypeDef GPIO_Init_E={0};


    /* Settings for the data bus */
    GPIO_Init_A.Pin =   GPIO_PIN_5;
    GPIO_Init_A.Mode =  GPIO_MODE_OUTPUT_PP;
    GPIO_Init_A.Pull =  GPIO_NOPULL;
    GPIO_Init_A.Speed = GPIO_SPEED_LOW;

    GPIO_Init_B.Pin =   GPIO_PIN_4 | GPIO_PIN_7;
    GPIO_Init_B.Mode =  GPIO_MODE_OUTPUT_PP;
    GPIO_Init_B.Pull =  GPIO_NOPULL;
    GPIO_Init_B.Speed = GPIO_SPEED_LOW;

    GPIO_Init_C.Pin =   GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_11
            | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_Init_C.Mode =  GPIO_MODE_OUTPUT_PP;
    GPIO_Init_C.Pull =  GPIO_NOPULL;
    GPIO_Init_C.Speed = GPIO_SPEED_LOW;

    /* Settings for the address bus and other signals */
    GPIO_Init_D.Pin =   GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_Init_D.Mode =  GPIO_MODE_OUTPUT_PP;
    GPIO_Init_D.Pull =  GPIO_NOPULL;
    GPIO_Init_D.Speed = GPIO_SPEED_LOW;

    GPIO_Init_E.Pin =   GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
            | GPIO_PIN_6;
    GPIO_Init_E.Mode =  GPIO_MODE_OUTPUT_PP;
    GPIO_Init_E.Pull =  GPIO_NOPULL;
    GPIO_Init_E.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_Init_A);
    HAL_GPIO_Init(GPIOB, &GPIO_Init_B);
    HAL_GPIO_Init(GPIOC, &GPIO_Init_C);
    HAL_GPIO_Init(GPIOD, &GPIO_Init_D);
    HAL_GPIO_Init(GPIOE, &GPIO_Init_E);
}

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

#define SID_WRITE 0
#define SID_READ  1

/* Useful SID registers */

#define SID_VOICE_OFFSET    0x07

#define SID_VOICE1_FREQ_LO  0x00
#define SID_VOICE1_FREQ_HI  0x01
#define SID_VOICE1_PW_LO    0x02
#define SID_VOICE1_PW_HI    0x03
#define SID_VOICE1_CONTROL  0x04
#define SID_VOICE1_AD       0x05
#define SID_VOICE1_SR       0x06

#define SID_VOICE2_FREQ_LO  0x07
#define SID_VOICE2_FREQ_HI  0x08
#define SID_VOICE2_PW_LO    0x09
#define SID_VOICE2_PW_HI    0x0A
#define SID_VOICE2_CONTROL  0x0B
#define SID_VOICE2_AD       0x0C
#define SID_VOICE2_SR       0x0D

#define SID_VOICE3_FREQ_LO  0x0E
#define SID_VOICE3_FREQ_HI  0x0F
#define SID_VOICE3_PW_LO    0x10
#define SID_VOICE3_PW_HI    0x11
#define SID_VOICE3_CONTROL  0x12
#define SID_VOICE3_AD       0x13
#define SID_VOICE3_SR       0x14

#define SID_FC_LO           0x15
#define SID_FC_HI           0x16
#define SID_RES_FILT        0x17
#define SID_MODE_VOL        0x18

#define SID_POTX            0x19
#define SID_POTY            0x1A
#define SID_OSC3_RANDOM     0x1B
#define SID_ENV3            0x1C


void SID_Set_RW(int rw)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6,
        rw ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/** SID chips latch data and address buses at the falling edge of their clock.
    Here, we wait for a rising state of the clock.
*/
void SID_Sync_Clock(void)
{
    // Wait for a RESET state
    while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3)!=GPIO_PIN_RESET) ;
    // Here we surely are in a RESET state (or very close to a transition).
    // Wait for a SET state.
    while (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3)!=GPIO_PIN_SET) ;
    // A rising transition just happened.
}

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

void SID_Set_Register(int address, int data, int sid_num)
{
    SID_Set_Address(address);
    SID_Set_RW(SID_WRITE);
    SID_Set_Data(data);
    // Pulse the CS line to strobe data.
    SID_Select(sid_num);
    SID_Select(-1);


}

void SID_Note_On(uint8_t key, uint8_t velocity, uint8_t voice,
    SID_conf *instrument)
{
    int sid_num = 0;
    if(voice > 2) {
        voice -= 3;
        sid_num = 1;
    }
    
    key-=BASE_MIDI_NOTE;
    if (key<0 || key >= COUNTOF(C64_freq_table))
        return;
    BSP_LED_On(LED3);
    SID_Select(-1);

    SID_Set_Register(SID_MODE_VOL,  15, sid_num);

    uint8_t offset = SID_VOICE_OFFSET*voice;

    SID_Set_Register(SID_VOICE1_FREQ_HI+offset,
        (uint8_t)((C64_freq_table[key] & 0xFF00)>>8), sid_num);
    SID_Set_Register(SID_VOICE1_FREQ_LO+offset,
        (uint8_t)((C64_freq_table[key] & 0x00FF)), sid_num);
    SID_Set_Register(SID_VOICE1_AD+offset, instrument->ad, sid_num);
    SID_Set_Register(SID_VOICE1_SR+offset, instrument->sr, sid_num);
    SID_Set_Register(SID_VOICE1_PW_LO+offset, 
        (uint8_t)(instrument->duty_cycle & 0x00FF), sid_num);
    SID_Set_Register(SID_VOICE1_PW_HI+offset, 
        (uint8_t)((instrument->duty_cycle & 0xFF00)>>8), sid_num);
    SID_Set_Register(SID_VOICE1_CONTROL+offset, instrument->voice, sid_num);
}

void SID_Note_Off(uint8_t voice)
{
    int sid_num = 0;
    if(voice > 2) {
        voice -= 3;
        sid_num = 1;
    }
    BSP_LED_Off(LED3);
    SID_Set_Register(SID_VOICE1_CONTROL+SID_VOICE_OFFSET*voice,
        GeneralMIDI[Current_Instrument].voice & 0xFE, sid_num);
}

/* End of SID interface functions */

#define NUM_VOICES 6

typedef struct VoiceDef_tag {
    int      key;
    uint32_t timestamp;
} VoiceDef;

// Track how many notes we have played since the last reset.
uint32_t now;

VoiceDef Voices[NUM_VOICES];

/** Return NUM_VOICES if no voice is available */
uint8_t GetFreeVoice(int key)
{
    // Search if the same note has already have been played or sustained.
    for(uint8_t i=0; i<NUM_VOICES; ++i) {
        if(Voices[i].key==key || Voices[i].key==-key) {
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

/* Private variables --------------------------------------------------------*/
TIM_HandleTypeDef    TimHandle;


uint16_t uwPrescalerValue = 0;

/* Private function prototypes ----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions --------------------------------------------------------*/

void TB_init(void)
{
  /*##-1- Configure the TIM peripheral ######################################*/
  /* -----------------------------------------------------------------------
    In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1),
    since APB1 prescaler is different from 1.
      TIM3CLK = 2 * PCLK1
      PCLK1 = HCLK / 4
      => TIM3CLK = HCLK / 2 = SystemCoreClock /2
    To get TIM3 counter clock at 1 MHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = ((SystemCoreClock /2) /1 MHz) - 1

    Note:
     SystemCoreClock variable holds HCLK frequency and is defined in 
     system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to update 
     SystemCoreClock variable value. Otherwise, any configuration based on this
     variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system 
         clock frequency
  ----------------------------------------------------------------------- */

  /* Compute the prescaler value to have TIM3 counter clock equal to 30 MHz */
  uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 30000000) - 1;

  /* Set TIMx instance */
  TimHandle.Instance = TIMx;

  /* Initialize TIM3 peripheral as follows:
       + Period = 10000 - 1
       + Prescaler = ((SystemCoreClock/2)/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  TimHandle.Init.Period = 10000 - 1;
  TimHandle.Init.Prescaler = uwPrescalerValue;
  TimHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

    /*##-2- Start the TIM Base generation in interrupt mode #################*/
    /* Start Channel1 */
    if(HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
    {
        /* Starting Error */
        Error_Handler();
    }
    __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}

/**
 * @brief   Main program
 * @param  None
 * @retval None
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    BSP_LED_Init(LED3);
    BSP_LED_Init(LED4);
    TB_init();

    ConfigureGPIOPorts();
    /*##-1- Configure the UART peripheral ###################################*/
    /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
    /* UART1 configured as follow:
      - Word Length = 8 Bits
      - Stop Bit = One Stop bit
      - Parity = None
      - BaudRate = 31250 baud (as per MIDI standard)
      - Hardware flow control disabled (RTS and CTS signals) */
    UartHandle.Instance          = USARTx;
    UartHandle.Init.BaudRate     = 31250;
    UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits     = UART_STOPBITS_1;
    UartHandle.Init.Parity       = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode         = UART_MODE_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if(HAL_UART_Init(&UartHandle) != HAL_OK) {
        Error_Handler();
    }
    MIDI_ReceiveState = idle;
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, (uint32_t) LCD_FRAME_BUFFER);
    BSP_LCD_SetLayerVisible(0, ENABLE);

    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_CYAN);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(20,20,200,280);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_DisplayOn();

    BSP_LCD_SetFont(&Font16);

    BSP_LCD_DisplayStringAtLineMode(2,
        (uint8_t *) "MIDI2SwinSIDs", CENTER_MODE);
    BSP_LCD_DisplayStringAtLineMode(5,
        (uint8_t *) "Copyright 2022", CENTER_MODE);
    BSP_LCD_DisplayStringAtLineMode(8,
        (uint8_t *) "Davide Bucci", CENTER_MODE);
    receive=0;
    /*##-2- Put UART peripheral in reception process #####################*/  
    if(HAL_UART_Receive_IT(&UartHandle, aRxBuffer, 1) != HAL_OK) {
        Error_Handler();
    }
    receive=0;
    uint8_t old_instrument=255;
    char buffer[256];

    while (1) {
        BSP_LED_Toggle(LED4);
        HAL_Delay(100);
        if(old_instrument!=Current_Instrument) {
            BSP_LCD_DisplayStringAtLineMode(10,
                (uint8_t *) "                   ", CENTER_MODE);
            BSP_LCD_DisplayStringAtLineMode(10,
                (uint8_t *) GeneralMIDI[Current_Instrument].name, CENTER_MODE);
            old_instrument=Current_Instrument;
        }
        BSP_LCD_DisplayStringAtLineMode(14,
        "                                           ", CENTER_MODE);
        sprintf(buffer, "%d %d %d, %d %d %d",
            Voices[0].key,Voices[1].key,Voices[2].key,
            Voices[3].key,Voices[4].key,Voices[5].key);
        BSP_LCD_DisplayStringAtLineMode(14, (uint8_t *) buffer, CENTER_MODE);
    }
}

    
volatile int velocity;
volatile int key;
volatile int control;
volatile int value;

/**
  * @brief  Rx Transfer completed callback
  * @param  handle: UART handle
  * @note
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
    if(handle->Instance !=USARTx)
        return;

    uint8_t rec = aRxBuffer[0];
    int channel = rec & 0x0F;
    int event =   rec & 0xF0;

    // MIDI receive state machine.
    switch (MIDI_ReceiveState) {
        case idle:


            // We are in this state most of the time, until an event is
            // received.
            if(event==0x90) {           // NOTE ON
                MIDI_ReceiveState = note_on_k;
            } else if(event==0x80) {    // NOTE OFF
                MIDI_ReceiveState = note_off_k;
            } else if(event==0xB0) {    // CONTROL CHANGE
                MIDI_ReceiveState = control_c;
            } else if(event==0xC0) {    // PROGRAM CHANGE
                MIDI_ReceiveState = program_c;
            }
            break;
        case program_c:
            Current_Instrument = rec;
            MIDI_ReceiveState = idle;
            break;
        case note_on_k:
            key = rec;
            MIDI_ReceiveState = note_on_v;
            break;
        case note_on_v:
            velocity = rec;
            MIDI_ReceiveState = idle;
            // Play the note here!
            uint8_t v = GetFreeVoice(key);
            SID_Note_On(key, velocity, v, &GeneralMIDI[Current_Instrument]);
            Voices[v].key=key;
            Voices[v].timestamp=now;
            ++now;
            break;
        case note_off_k:
            key = rec;
            MIDI_ReceiveState = note_off_v;
            break;
        case note_off_v:
            velocity = rec;
            MIDI_ReceiveState = idle;
            // Stop the note here.
            for(uint8_t i=0; i<NUM_VOICES; ++i) {
                if(Voices[i].key==key || Voices[i].key==-key) {
                    if(SustainPedal) {
                        // If the pedal is depressed, mark note as sustained.
                        Voices[i].key=-key;
                     } else {
                        SID_Note_Off(i);
                        Voices[i].key=0;
                        break;
                    }
                }
            }
            break;
        case control_c:
            control = rec;
            MIDI_ReceiveState = control_v;
            break;
        case control_v:
            value = rec;
            MIDI_ReceiveState = idle;
            if(control == 64) {     // SUSTAIN PEDAL
                if (value>63) {     // SUSTAIN ON
                    SustainPedal=1;
                } else {            // SUSTAIN OFF
                    SustainPedal=0;
                    for(uint8_t i=0; i<NUM_VOICES; ++i) {
                        if(Voices[i].key<0) {
                            SID_Note_Off(i);
                            Voices[i].key=0;
                        }
                    }
                }
            }
            break;
        default:
            MIDI_ReceiveState=idle;
    }
    HAL_UART_Receive_IT(&UartHandle, aRxBuffer, 1);
}



/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    //HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_3);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    /* Turn LED3 on */
    BSP_LED_On(LED3);
    while(1) {  }
}


/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 180000000
 *            HCLK(Hz)                       = 180000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 4
 *            APB2 Prescaler                 = 2
 *            HSE Frequency(Hz)              = 8000000
 *            PLL_M                          = 8
 *            PLL_N                          = 360
 *            PLL_P                          = 2
 *            PLL_Q                          = 7
 *            VDD(V)                         = 3.3
 *            Main regulator output voltage  = Scale1 mode
 *            Flash Latency(WS)              = 5
 *         The LTDC Clock is configured as follow :
 *            PLLSAIN                        = 192
 *            PLLSAIR                        = 4
 *            PLLSAIDivR                     = 8
 * @param  None
 * @retval None
 *
 * COPYRIGHT(c) 2014 STMicroelectronics
 */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct={0};
    RCC_OscInitTypeDef RCC_OscInitStruct={0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /* Enable Power Control clock */
    __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /*##-1- System Clock Configuration #########################################*/
    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    HAL_PWREx_ActivateOverDrive();

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

    /*##-2- LTDC Clock Configuration ###########################################*/
    /* LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/4 = 48 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / RCC_PLLSAIDIVR_8 = 48/8 = 6 Mhz */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/**
  * @brief  UART error callbacks
  * @param  huart: UART handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
 void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    /* Turn LED3 on: Transfer error in reception/transmission process */
    BSP_LED_On(LED3);
    if (huart->ErrorCode == HAL_UART_ERROR_ORE){
        // remove the error condition
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        // set the correct state, so that the UART_RX_IT works correctly
        huart->State = HAL_UART_STATE_BUSY_RX;
    }
}

