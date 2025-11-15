/**
  ******************************************************************************
  * @file    main.c
  * @author  Davide Bucci
  * @version V0.3
  * @date    August 14, 2022 - November 2025
  * @brief   This file is the main file of the MIDI2SwinSID project.
  *
  ******************************************************************************
 */

/*
 Compile with "make" in the "manual_compile" directory.
 Flash command:  st-flash write Display.bin 0x8000000

*/
/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "main.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f4xx_hal.h"
#include "Utilities/Fonts/fonts.h"

#include "SID_def.h"
#include "MIDI_def.h"



void ShowInstrument(void);
void ShowVoices(void);

void harp_navigate(void);
void harp_reset(void);

/* UART handler declaration */
volatile UART_HandleTypeDef UartHandle;


/* Buffer used for reception */
uint8_t aRxBuffer[RXBUFFERSIZE];

int receive=0;

extern volatile uint8_t SustainPedal[];
extern volatile int8_t Master_Volume;  // 0 to 15

extern VoiceDef Voices[];
extern SID_conf GeneralMIDI[];
extern SID_composite DrumKit[];


#define MESSAGE_SIZE 256
char Message[MESSAGE_SIZE];

#define TRUE    1
#define FALSE   0

#define ANTIBOUNCE_VALUE 2

int  MessageCountdown;
int  AntiBounceHoldoff;
int8_t encoderAction;
int8_t encoderDirection;
int8_t changeField;
int8_t editorAction;
int8_t toggleEdit;
int32_t  LargeMovement=0;
int8_t somethingChanged;


int currentField;

#define NUM_FIELD_ARP       24
#define NUM_FIELD_OTHER     22
#define NUM_FIELD (currentMode==HARP?NUM_FIELD_ARP:NUM_FIELD_OTHER)

volatile int CurrInst;


int8_t currentWave1;
int8_t currentAttack1;
int8_t currentDecay1;
int8_t currentSustain1;
int8_t currentRelease1;
int16_t currentDutyCycle1;

int32_t currentV1toV2;

int8_t currentWave2;
int8_t currentAttack2;
int8_t currentDecay2;
int8_t currentSustain2;
int8_t currentRelease2;
int16_t currentDutyCycle2;

int16_t currentCutoff;
int8_t currentResonance;
int8_t currentFilterMode;
int8_t currentRouting;
int8_t currentPortamento;


int8_t currentChannel;
enum MIDI_mode currentMode;
int channelsInstr[16];          // Instruments associated to channels.


extern int8_t LFO_Table[];
extern uint16_t LFO_Pointer;
extern uint8_t LFO_Amount;
extern uint8_t LFO_Rate;

int division=3;
int bpm=120;
int division_table[7]={1,2,3,4,8,16,32};

TIM_HandleTypeDef TimHandle;

uint32_t uwPrescalerValue = 0;


/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* MIDI receive state machine states */
volatile enum MIDIState {idle, note_on_k, note_on_v, note_off_k,
    note_off_v, control_c, control_v, program_c} MIDI_ReceiveState;

void ConfigureGPIOPorts(void)
{
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();
    __GPIOF_CLK_ENABLE();
    __GPIOG_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_Init_A={0};
    GPIO_InitTypeDef GPIO_Init_B={0};
    GPIO_InitTypeDef GPIO_Init_C={0};
    GPIO_InitTypeDef GPIO_Init_D={0};
    GPIO_InitTypeDef GPIO_Init_E={0};
    GPIO_InitTypeDef GPIO_Init_F={0};
    GPIO_InitTypeDef GPIO_Init_G={0};

    // Setting for the button
    GPIO_Init_A.Pin =   GPIO_PIN_0;
    GPIO_Init_A.Mode =  GPIO_MODE_INPUT;
    GPIO_Init_A.Pull =  GPIO_NOPULL;
    GPIO_Init_A.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_A);

    /* Settings for the data bus */
    GPIO_Init_A.Pin =   GPIO_PIN_5;
    GPIO_Init_A.Mode =  GPIO_MODE_OUTPUT_PP;
    GPIO_Init_A.Pull =  GPIO_NOPULL;
    GPIO_Init_A.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_A);

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

    //   Encoder interrupt source (PF6)
    GPIO_Init_F.Pin =   GPIO_PIN_6;
    GPIO_Init_F.Mode =  GPIO_MODE_IT_FALLING;
    GPIO_Init_F.Pull =  GPIO_PULLUP;
    GPIO_Init_F.Speed = GPIO_SPEED_LOW;

    //   Encoder interrupt direction (PG2)
    GPIO_Init_G.Pin =   GPIO_PIN_2;
    GPIO_Init_G.Mode =  GPIO_MODE_INPUT;
    GPIO_Init_G.Pull =  GPIO_PULLUP;
    GPIO_Init_G.Speed = GPIO_SPEED_LOW;


    HAL_GPIO_Init(GPIOB, &GPIO_Init_B);
    HAL_GPIO_Init(GPIOC, &GPIO_Init_C);
    HAL_GPIO_Init(GPIOD, &GPIO_Init_D);
    HAL_GPIO_Init(GPIOE, &GPIO_Init_E);
    HAL_GPIO_Init(GPIOF, &GPIO_Init_F);
    HAL_GPIO_Init(GPIOG, &GPIO_Init_G);
        //   Encoder interrupt button (PG3)
    GPIO_Init_G.Pin =   GPIO_PIN_3;
    GPIO_Init_G.Mode =  GPIO_MODE_IT_FALLING;
    GPIO_Init_G.Pull =  GPIO_PULLUP;
    GPIO_Init_G.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOG, &GPIO_Init_G);

    /* Enable and set Encoder EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((IRQn_Type)(EXTI9_5_IRQn), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(EXTI9_5_IRQn));
    HAL_NVIC_SetPriority((IRQn_Type)(EXTI3_IRQn), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(EXTI3_IRQn));

//    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource0);

    /* PD0 is connected to EXTI_Line0 */
/*    EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_Line0;
    /* Enable interrupt *
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    /* Interrupt mode *
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    /* Triggers on rising and falling edge *
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    /* Add to EXTI *
    EXTI_Init(&EXTI_InitStruct);

    /* Add IRQ vector to NVIC */
    /* PD0 is connected to EXTI_Line0, which has EXTI0_IRQn vector *
    NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
    /* Set priority *
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
    /* Set sub priority *
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
    /* Enable interrupt *
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    /* Add to NVIC *
    NVIC_Init(&NVIC_InitStruct);*/

}


/* Private variables --------------------------------------------------------*/
TIM_HandleTypeDef    TimHandle;

/* Private function prototypes ----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions --------------------------------------------------------*/

/*
    Configure the timer
*/
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

  /* Compute the prescaler value to have TIM3 counter clock equal to 100 kHz */
  uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 100000) - 1;

  /* Set TIMx instance */
  TimHandle.Instance = TIMx;

  /* Initialize TIM3 peripheral as follows:
       + Period = 100 - 1
       + Prescaler = ((SystemCoreClock/2)/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  // The interrupt is called at 1 ms intervals
  TimHandle.Init.Period = 100 - 1;
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

void updateInstrument(int inst)
{
    channelsInstr[currentChannel]=inst;

    //currentWave1=GeneralMIDI[inst].voice;
    currentAttack1=GeneralMIDI[inst].a;
    currentDecay1=GeneralMIDI[inst].d;
    currentSustain1=GeneralMIDI[inst].s;
    currentRelease1=GeneralMIDI[inst].r;
    currentDutyCycle1=GeneralMIDI[inst].duty_cycle;

    currentV1toV2=GeneralMIDI[inst].diff;

    currentWave2=GeneralMIDI[inst].voice2;
    currentAttack2=GeneralMIDI[inst].a2;
    currentDecay2=GeneralMIDI[inst].d2;
    currentSustain2=GeneralMIDI[inst].s2;
    currentRelease2=GeneralMIDI[inst].r2;
    currentDutyCycle2=GeneralMIDI[inst].duty_cycle2;

    currentCutoff=GeneralMIDI[inst].filt_cutoff;
    currentResonance=GeneralMIDI[inst].filt_resonance;
    currentFilterMode=GeneralMIDI[inst].filt_mode;
    somethingChanged=TRUE;

}

#define NOTIFY_CHANGES()\
    MessageCountdown = 20;\
    somethingChanged=TRUE;

void updateWave1Message(int inst)
{
    char*   wav[] =
        {"None      ",
         "Triangular",
         "Sawtooth  ",
         "Pulse     ",
         "Noise     ",
         "FM        "};
    uint8_t choice[]={NONE, TRIAN, SAWTH, PULSE, NOISE, FM};

    GeneralMIDI[inst].voice = choice[currentWave1];
    sprintf(Message, "Wave 1: %s  ", wav[currentWave1]);
    NOTIFY_CHANGES();
}

void updateAttack1Message(int inst)
{
    GeneralMIDI[inst].a=currentAttack1;
    sprintf(Message, "Attack 1: %d  ", currentAttack1);
    NOTIFY_CHANGES();
}

void updateDecay1Message(int inst)
{
    GeneralMIDI[inst].d=currentDecay1;
    sprintf(Message, "Decay 1: %d  ", currentDecay1);
    NOTIFY_CHANGES();
}

void updateSustain1Message(int inst)
{
    GeneralMIDI[inst].s=currentSustain1;
    sprintf(Message, "Sustain 1: %d  ", currentSustain1);
    NOTIFY_CHANGES();
}

void updateRelease1Message(int inst)
{
    GeneralMIDI[inst].r=currentRelease1;
    sprintf(Message, "Release 1: %d  ", currentRelease1);
    NOTIFY_CHANGES();
}

void updateDutyCycle1Message(int inst)
{
    GeneralMIDI[inst].duty_cycle=currentDutyCycle1;
    sprintf(Message, "Duty 1: %d  ",currentDutyCycle1);
    NOTIFY_CHANGES();
}

void updateCurrentV1toV2Message(int inst)
{
    GeneralMIDI[inst].diff=currentV1toV2;
    sprintf(Message, "Rel v1 to v2: %4d", GeneralMIDI[inst].diff);

    NOTIFY_CHANGES();
}

void updateWave2Message(int inst)
{
    char*   wav[5] =
        {"None      ",
         "Triangular",
         "Sawtooth  ",
         "Pulse     ",
         "Noise     "};
    uint8_t choice[5]={NONE, TRIAN, SAWTH, PULSE, NOISE};

    GeneralMIDI[inst].voice2 = choice[currentWave2];
    sprintf(Message, "Wave 2: %s  ", wav[currentWave2]);
    NOTIFY_CHANGES();
}

void updateAttack2Message(int inst)
{
    GeneralMIDI[inst].a2=currentAttack2;
    sprintf(Message, "Attack 2: %d  ", currentAttack2);
    NOTIFY_CHANGES();
}

void updateDecay2Message(int inst)
{
    GeneralMIDI[inst].d2=currentDecay2;
    sprintf(Message, "Decay 2: %d  ", currentDecay2);
    NOTIFY_CHANGES();
}

void updateSustain2Message(int inst)
{
    GeneralMIDI[inst].s2=currentSustain2;
    sprintf(Message, "Sustain 2: %d  ", currentSustain2);
    NOTIFY_CHANGES();
}

void updateRelease2Message(int inst)
{
    GeneralMIDI[inst].r2=currentRelease2;
    sprintf(Message, "Release 2: %d  ", currentRelease2);
    NOTIFY_CHANGES();
}

void updateDutyCycle2Message(int inst)
{
    GeneralMIDI[inst].duty_cycle2=currentDutyCycle2;
    sprintf(Message, "Duty 2: %d  ",currentDutyCycle2);
    NOTIFY_CHANGES();
}

void updateFilterModeMessage(int inst)
{
    uint8_t choice[]={LO, HI, BP, M3};
    char*   wav[] = {"NONE", "Low pass", "Hi Pass", "Band pass", "Mute v3"};
    if(currentFilterMode > 0) {
        GeneralMIDI[inst].filt_mode = choice[currentFilterMode-1];
        GeneralMIDI[inst].filt_routing = ALL;
    } else {
        GeneralMIDI[inst].filt_routing = NON;
    }
    sprintf(Message, "Filter: %s    ", wav[currentFilterMode]);
    NOTIFY_CHANGES();
}

void updateFilterCutoffMessage(int inst)
{
    GeneralMIDI[inst].filt_cutoff=currentCutoff;
    sprintf(Message, "Cutoff: %d  ", GeneralMIDI[inst].filt_cutoff);
    NOTIFY_CHANGES();
}

void updateFilterResonanceMessage(int inst)
{
    GeneralMIDI[inst].filt_resonance=currentResonance;
    sprintf(Message, "Resonance: %d  ", GeneralMIDI[inst].filt_resonance);
    NOTIFY_CHANGES();
}

/*
void updateFilterRoutingMessage(int inst)
{
    GeneralMIDI[inst].filt_routing=currentRouting;
    if(GeneralMIDI[CurrInst].filt_routing==7) {
        sprintf(Message, "Filter routing: ALL ");
    } else if(GeneralMIDI[CurrInst].filt_routing==0) {
        sprintf(Message, "Filter routing: NONE ");
    } else {
        sprintf(Message, "Filter routing: %d  ",
            GeneralMIDI[CurrInst].filt_routing);
    }
}
*/

// Macro useful to easily update values.
#define UPDATE_PAR(P,MIN,MAX,INC) \
            if(direction) {\
                (P)+=(INC); if((P)>MAX) (P)=MAX;\
            } else {\
                (P)-=(INC); if((P)<MIN) (P)=MIN;\
            }

void updateCurrentValue(uint8_t direction)
{
    static float increment_f;
    int increment, increment_l;

    // Acceleration of the encoder for some parameters.
    // The LargeMovement is incremented at each cycle. Therefore, if it is not
    // put to zero, the value becomes large. When the user moves the encoder
    // fast, LargeMovement tends to remain low.
    if(LargeMovement<5) {
        increment_f*=1.6;
    } if(LargeMovement<10) {
        if(increment_f<2) {
            increment_f*=1.3;
        } else {
            increment_f*=1.1;
        }
    } else {
        increment_f=1;
    }
    // Avoid limit cases.
    if(increment_f>200) increment_f=200;
    LargeMovement=0;
    increment=(int)increment_f;
    increment_l=(int)(increment_f/10.0);
    if(increment_l<1) increment_l=1;

    switch(currentField) {
        case 0:     // Change instrument
            UPDATE_PAR(CurrInst,0,128,increment_l);
            updateInstrument(CurrInst);
            break;
        case 2:     // Wave 1
            UPDATE_PAR(currentWave1,0,5,1); // Allow the FM choice here
            updateWave1Message(CurrInst);
            break;
        case 3:     // Attack 1
            UPDATE_PAR(currentAttack1,0,15,1);
            updateAttack1Message(CurrInst);
            break;
        case 4:     // Decay 1
            UPDATE_PAR(currentDecay1,0,15,1);
            updateDecay1Message(CurrInst);
            break;
        case 5:     // Sustain 1
            UPDATE_PAR(currentSustain1,0,15,1);
            updateSustain1Message(CurrInst);
            break;
        case 6:     // Release 1
            UPDATE_PAR(currentRelease1,0,15,1);
            updateRelease1Message(CurrInst);
            break;
        case 7:     // Duty Cycle 1
            UPDATE_PAR(currentDutyCycle1,0,4095,increment);
            updateDutyCycle1Message(CurrInst);
            break;
        case 8:     // Rel v1 to v2
            UPDATE_PAR(currentV1toV2,0,9600,increment);
            updateCurrentV1toV2Message(CurrInst);
            break;
        case 9:     // Wave2
            UPDATE_PAR(currentWave2,0,4,1); // Do not allow FM
            updateWave2Message(CurrInst);
            break;
        case 10:     // Attack 2
            UPDATE_PAR(currentAttack2,0,15,1);
            updateAttack2Message(CurrInst);
            break;
        case 11:     // Decay 2
            UPDATE_PAR(currentDecay2,0,15,1);
            updateDecay2Message(CurrInst);
            break;
        case 12:     // Sustain 2
            UPDATE_PAR(currentSustain2,0,15,1);
            updateSustain2Message(CurrInst);
            break;
        case 13:     // Release 2
            UPDATE_PAR(currentRelease2,0,15,1);
            updateRelease2Message(CurrInst);
            break;
        case 14:     // Duty Cycle 2
            UPDATE_PAR(currentDutyCycle2,0,4095,increment);
            updateDutyCycle2Message(CurrInst);
            break;
        case 15:    // Filter mode
            UPDATE_PAR(currentFilterMode,0,4,1);
            updateFilterModeMessage(CurrInst);
            break;
        case 16:    // Filter cutoff
            UPDATE_PAR(currentCutoff,0,2047,increment);
            updateFilterCutoffMessage(CurrInst);
            break;
        case 17:    // Filter resonance
            UPDATE_PAR(currentResonance,0,15,1);
            updateFilterResonanceMessage(CurrInst);
            break;
        case 18:    // Portamento
            UPDATE_PAR(currentPortamento,0,100,1);
            GeneralMIDI[CurrInst].portamento=currentPortamento;
            break;
        case 19:    // Master volume
            UPDATE_PAR(Master_Volume,0,15,1);
            break;
        case 20:    // MIDI channel
            UPDATE_PAR(currentChannel,0,15,1);
            CurrInst=channelsInstr[currentChannel];
            updateInstrument(CurrInst);
            break;
        case 21:    // MIDI mode
            UPDATE_PAR(currentMode,0,4,1);
            harp_reset();
            break;
        case 22:    // BPM (for the arpeggiator)
            UPDATE_PAR(bpm,40,260,1);
            break;
        case 23:    // time division
            UPDATE_PAR(division,0,6,1);
            break;
        default:
    }
}



void UserAction(void)
{
    if(changeField) {
        somethingChanged=TRUE;
        if(encoderDirection) {
            ++currentField;
            if(currentField==1) currentField=2; // Avoid the separating line
        } else {
            --currentField;
            if(currentField==1) currentField=0; // Avoid the separating line
        }
        // Circular movement on the menus
        if(currentField<0) currentField=NUM_FIELD-1;
        if(currentField>=NUM_FIELD) currentField=0;
    } else {
        updateCurrentValue(encoderDirection);
    }
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
    BSP_LED_Off(LED3);
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
    somethingChanged=TRUE;
    currentMode=MULTI;

    Master_Volume = MAXVOL;
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
        (uint8_t *) "* MIDI2SwinSIDs *", CENTER_MODE);
    BSP_LCD_DisplayStringAtLineMode(5,
        (uint8_t *) "(C) 2022-2025", CENTER_MODE);
    BSP_LCD_DisplayStringAtLineMode(8,
        (uint8_t *) "Davide Bucci", CENTER_MODE);
    receive=0;
    /*##-2- Put UART peripheral in reception process #####################*/
    if(HAL_UART_Receive_IT(&UartHandle, aRxBuffer, 1) != HAL_OK) {
        Error_Handler();
    }
    
    receive=0;
    currentField=0;
    changeField=0;

    uint8_t old_instrument=255;
    MessageCountdown = 0;
    HAL_Delay(2000);
    BSP_LCD_Clear(LCD_COLOR_BLUE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

    uint32_t CounterRefreshInfos=0;
    
    uint8_t  ToErase=0;

    for(int i=0; i<LFO_SIZE; ++i)
        LFO_Table[i]=(int)(127.0*sin((float)i/LFO_SIZE*2.0*M_PI));

    while (1) {
        static int clearScreen;

        // Refresh the information every 10 repetitions of this loop.
        if(CounterRefreshInfos++>10) {
            CounterRefreshInfos=0;
            if(HAL_GPIO_ReadPin (GPIOA, GPIO_PIN_0)) {
                ShowVoices();
                clearScreen=TRUE;
            } else {
                if(clearScreen) {
                    clearScreen=FALSE;
                    BSP_LCD_Clear(LCD_COLOR_BLUE);
                    somethingChanged=TRUE;
                }
                ShowInstrument();
            }
            // Show that something is being changed in the last line.
            if(MessageCountdown) {
                --MessageCountdown;
                ToErase=TRUE;
                BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
                BSP_LCD_SetFont(&Font12);
                BSP_LCD_DisplayStringAt(0,230,
                    (uint8_t *) Message, CENTER_MODE);
            } else {
                if(ToErase) {
                    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
                    BSP_LCD_FillRect(0,230,240,16);         // Noisy!!!
                    ToErase=FALSE;
                }
            }
        }
        UpdateLFO();

        BSP_LED_Toggle(LED4);
        //BSP_LED_Off(LED3);
        HAL_Delay(10);
        ++LargeMovement;
        LFO_Pointer += GeneralMIDI[CurrInst].lfo_rate;
        if(encoderAction) {
            somethingChanged=TRUE;
            UserAction();
            encoderAction=FALSE;
            CounterRefreshInfos=20;
        }
        if(toggleEdit) {
            if(changeField) {
                changeField = FALSE;
            } else {
                changeField = TRUE;
            }
            somethingChanged=TRUE;
            toggleEdit=FALSE;
            CounterRefreshInfos=20;
        }
        if(AntiBounceHoldoff>0)
            --AntiBounceHoldoff;

        if(LFO_Pointer >= LFO_SIZE)
            LFO_Pointer = 0;
    }
}

void setEv(int l)
{
    if(l==currentField) {
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        if(changeField) {
            BSP_LCD_SetBackColor(LCD_COLOR_CYAN);
        } else {
            BSP_LCD_SetBackColor(LCD_COLOR_LIGHTRED);
        }
    } else {
        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    }
}

#define NUMSAMPLES 16

/** Show a small icon of the wave shape.
*/
void DrawWave(int x, int y, int wave, int duty)
{
    int height=8;
    int base=10;
    float dd=(float)duty/4096.0;
    float ll=4*base;
    float f1,f2;
    int valt[2*NUMSAMPLES];
    int valf[2*NUMSAMPLES];
    int val[]={3,-2,5,-1,-8,6,-2,2,
               3,-4,4,7,-6,5,2,-1};

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(x-2,y,4*base+4,20);
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    // Draw the baseline.
    BSP_LCD_DrawLine(x-2,y+height,             x+4*base+2,y+height);

    switch(wave) {
        default:
        case NONE:
            break;
        case TRIAN:
            BSP_LCD_DrawLine(x,y+height,             x+base,y);
            BSP_LCD_DrawLine(x+base,y,             x+3*base,y+2*height);
            BSP_LCD_DrawLine(x+3*base,y+2*height,  x+4*base,y+height);
            break;
        case SAWTH:
            BSP_LCD_DrawLine(x,y+height,             x+2*base,y);
            BSP_LCD_DrawLine(x+2*base,y,             x+2*base,y+2*height);
            BSP_LCD_DrawLine(x+2*base,y+2*height,    x+4*base,y+height);
            break;
        case NOISE:
            for(int i=1; i<NUMSAMPLES;++i) {
                BSP_LCD_DrawLine(
                    x+(float)(i-1)/(float)NUMSAMPLES*4*base,y+height+val[i-1],
                    x+(float)i/(float)NUMSAMPLES*4*base,y+height+val[i]);
            }
            break;
        case PULSE:
            BSP_LCD_DrawLine(x,y+height,            x,y+2*height);
            BSP_LCD_DrawLine(x,y+2*height,          x+ll*dd,y+2*height);
            BSP_LCD_DrawLine(x+ll*dd,y+2*height,    x+ll*dd,y);
            BSP_LCD_DrawLine(x+ll*dd,y,             x+ll,y);
            BSP_LCD_DrawLine(x+ll,y,                x+ll,y+height);
            break;
        case FM:
            for(int i=0; i<2*NUMSAMPLES;++i) {
                f1=8*sin((float)i/(2.0*NUMSAMPLES)*2.0*3.14);
                f2=sin(6*(float)i/(2.0*NUMSAMPLES)*2.0*3.14);
                valf[i]=((int)f1*f2);
                valt[i]=((int)f1);
            }

            for(int i=1; i<2*NUMSAMPLES;++i) {
                BSP_LCD_DrawLine(
                    x+(float)(i-1)/(float)NUMSAMPLES*2*base,y+height+valf[i-1],
                    x+(float)i/(float)NUMSAMPLES*2*base,y+height+valf[i]);
                BSP_LCD_DrawLine(
                    x+(float)(i-1)/(float)NUMSAMPLES*2*base,y+height+valt[i-1],
                    x+(float)i/(float)NUMSAMPLES*2*base,y+height+valt[i]);
            }
            break;
    }
}

/** Draw a small icon of the ADSR shape.
*/
void DrawADSR(int x, int y, int a, int d, int s, int r)
{
    int height=15;
    int base=10;
    int susd=10;

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(x,y,80,20);
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

    BSP_LCD_DrawLine(x,y+height,             base+x,y+height);  // base
    BSP_LCD_DrawLine(base+x,y+height,        base+x+a,y);       // A
    BSP_LCD_DrawLine(base+x+a,y,             base+x+a+d, y+height-s);   // D
    BSP_LCD_DrawLine(base+x+a+d, y+height-s, base+x+a+d+susd,y+height-s); // S
    BSP_LCD_DrawLine(base+x+a+d+susd, y+height-s,
        base+x+a+d+r+susd,y+height);    // S
    BSP_LCD_DrawLine(base+x+a+d+r+susd,y+height,
        2*base+x+a+d+r+susd,y+height);  // D
}

/** Draw the instrument number (MIDI code + 1) in a small rectangle.
*/
void DrawInstrNumber(int x, int y)
{
    char buffer[256];
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_FillRect(x,y,41,21);

    BSP_LCD_SetBackColor(LCD_COLOR_CYAN);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    sprintf(buffer, "%d",CurrInst+1);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(x+4,y+4,(uint8_t *)buffer, LEFT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DrawRect(x+1,y+1,38,18);
}

/** Show the current voice attributions and state in a window centered in the
    screen.
*/
void ShowVoices(void)
{
    char buffer[256];
    int x=28, y=100;
    int border=5;

    int xsize=240-2*x;
    int ysize=320-2*y;

    int i;
    int l=0;

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(x,y,xsize,ysize);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    BSP_LCD_DrawRect(x,y,xsize,ysize);

    BSP_LCD_SetFont(&Font12);
    y+=border;
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DisplayStringAt(x+border,y, "-----------SID0----------", LEFT_MODE);
    y+=12;
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    for(i=0; i<3; ++i) {
        sprintf(buffer, "Voice %d key,c:  %5d,%2d", i+1,
            Voices[i].key,Voices[i].channel);
        BSP_LCD_DisplayStringAt(x+border,y, (uint8_t *) buffer, LEFT_MODE);
        y+=12;
    }
    y+=12;
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DisplayStringAt(x+border,y, "-----------SID1----------", LEFT_MODE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    y+=12;
    for(i=3; i<6; ++i) {
        sprintf(buffer, "Voice %d key,c:  %5d,%2d", i-2,
            Voices[i].key,Voices[i].channel);
        BSP_LCD_DisplayStringAt(x+border,y, (uint8_t *) buffer, LEFT_MODE);
        y+=12;
    }
}

/** Show the current state of the selected channel: the instrument and its
    configuration (voices, filters, etc.)
    Show also the current MIDI channel and mode.
*/
void ShowInstrument(void)
{
    char buffer[256];
    int l=0;
    if(somethingChanged==FALSE)
        return;

    somethingChanged=FALSE;
    DrawADSR(150,50,
        GeneralMIDI[CurrInst].a, GeneralMIDI[CurrInst].d,
        GeneralMIDI[CurrInst].s, GeneralMIDI[CurrInst].r);
    DrawADSR(150,150-16,
        GeneralMIDI[CurrInst].a2, GeneralMIDI[CurrInst].d2,
        GeneralMIDI[CurrInst].s2, GeneralMIDI[CurrInst].r2);
    DrawWave(150,23,GeneralMIDI[CurrInst].voice,
         GeneralMIDI[CurrInst].duty_cycle);
    DrawWave(150,108,GeneralMIDI[CurrInst].voice2,
         GeneralMIDI[CurrInst].duty_cycle2);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);

    BSP_LCD_FillRect(45,0,240-45,18);
    setEv(l);
    BSP_LCD_SetFont(&Font16);
    sprintf(buffer, "%s",
        GeneralMIDI[CurrInst].name);
    BSP_LCD_DisplayStringAt(45,0,(uint8_t *)buffer, LEFT_MODE);
    ++l;
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

    ++l;
    BSP_LCD_DrawHLine(0,18,240);
    BSP_LCD_DrawHLine(0,17,240);

    DrawInstrNumber(0,0);

    BSP_LCD_SetFont(&Font12);

    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    setEv(l);
    switch(GeneralMIDI[CurrInst].voice) {
        case NONE:
            sprintf(buffer, "Wave 1: NONE      ");
            break;
        case TRIAN:
            sprintf(buffer, "Wave 1: TRIANGULAR");
            break;
        case PULSE:
            sprintf(buffer, "Wave 1: PULSE     ");
            break;
        case SAWTH:
            sprintf(buffer, "Wave 1: SAWTOOTH  ");
            break;
        case NOISE:
            sprintf(buffer, "Wave 1: NOISE     ");
            break;
        case FM:
            sprintf(buffer, "Wave 1: FM        ");
            break;
        default:    // We are not supposed to get there, but just in case...
            sprintf(buffer, "Wave 1: %2d        ",GeneralMIDI[CurrInst].voice);
            break;
    }
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *)buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Attack 1:  %2d", GeneralMIDI[CurrInst].a);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Decay 1:   %2d", GeneralMIDI[CurrInst].d);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Sustain 1: %2d", GeneralMIDI[CurrInst].s);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Release 1: %2d", GeneralMIDI[CurrInst].r);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Duty Cycle 1: %4d", GeneralMIDI[CurrInst].duty_cycle);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Rel v1 to v2: %4d", GeneralMIDI[CurrInst].diff);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);

    setEv(l);
    switch(GeneralMIDI[CurrInst].voice2) {
        case NONE:
            sprintf(buffer, "Wave 2: NONE      ");
            break;
        case TRIAN:
            sprintf(buffer, "Wave 2: TRIANGULAR");
            break;
        case PULSE:
            sprintf(buffer, "Wave 2: PULSE     ");
            break;
        case SAWTH:
            sprintf(buffer, "Wave 2: SAWTOOTH  ");
            break;
        case NOISE:
            sprintf(buffer, "Wave 2: NOISE     ");
            break;
    }
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Attack 2:  %2d", GeneralMIDI[CurrInst].a2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Decay 2:   %2d", GeneralMIDI[CurrInst].d2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Sustain 2: %2d", GeneralMIDI[CurrInst].s2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Release 2: %2d", GeneralMIDI[CurrInst].r2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Duty Cycle 2: %4d", GeneralMIDI[CurrInst].duty_cycle2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    if(GeneralMIDI[CurrInst].filt_routing == NON) {
        sprintf(buffer, "Filter mode:  NONE");
    } else {
        switch(GeneralMIDI[CurrInst].filt_mode) {
            case LO:
                sprintf(buffer, "Filter mode:   LOW");
                break;
            case BP:
                sprintf(buffer, "Filter mode:  BAND");
                break;
            case HI:
                sprintf(buffer, "Filter mode:    HI");
                break;
            case M3:
                sprintf(buffer, "Filter mode: MUTE3");
                break;

        }
    }

    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Filt. cutoff: %4d", GeneralMIDI[CurrInst].filt_cutoff);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Filt. reson.:   %2d",
        GeneralMIDI[CurrInst].filt_resonance);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    setEv(l);
    sprintf(buffer, "Portamento:    %3d", GeneralMIDI[CurrInst].portamento);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);

    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_DrawHLine(0,250,240);
    BSP_LCD_DrawHLine(0,251,240);
    BSP_LCD_SetFont(&Font16);
    setEv(l);
    sprintf(buffer, "Master Volume: %2d",Master_Volume);
    BSP_LCD_DisplayStringAt(0,255,(uint8_t *)buffer, LEFT_MODE);
    ++l;
    setEv(l);
    if (currentChannel == 9) {
        sprintf(buffer, "Channel: 10 (DrumKit)");
    } else {
        sprintf(buffer, "Channel: %2d           ", currentChannel+1);
    }
    BSP_LCD_DisplayStringAt(0,270,(uint8_t *)buffer, LEFT_MODE);
    ++l;
    setEv(l);
    char* m_mode[6]={"OMNI ", "POLY ", "MULTI", "MONO ", "ARP  "};

    sprintf(buffer, "MIDI Mode: %s", m_mode[currentMode]);
    BSP_LCD_DisplayStringAt(0,285,(uint8_t *)buffer, LEFT_MODE);
    ++l;

    if(currentMode==HARP) {    
        setEv(l);
        sprintf(buffer, "BPM: %3d", bpm);
        BSP_LCD_DisplayStringAt(0,300,(uint8_t *)buffer, LEFT_MODE);
        ++l;
        
        setEv(l);
        char* divisions[7]={"1", "1/2", "1/3", "1/4", "1/8","1/16", "1/32"};
        sprintf(buffer, "DIV: %4s", divisions[division]);
        BSP_LCD_DisplayStringAt(120,300,(uint8_t *)buffer, LEFT_MODE);
        ++l;
    } else {
        setEv(-1);      // With -1 this line is never active.
        sprintf(buffer, "                                   ");
        BSP_LCD_DisplayStringAt(0,300,(uint8_t *)buffer, LEFT_MODE);
        ++l;
    }
}


/*****************************************************************************
 **
 **     INTERFACE INTERRUPTS
 **
 *****************************************************************************/

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

    if(AntiBounceHoldoff>0) {
        return;
    }
    AntiBounceHoldoff=ANTIBOUNCE_VALUE;
    if(GPIO_Pin==GPIO_PIN_6 && encoderAction==FALSE) {
        // GP2 gives the direction of the encoder
        if(HAL_GPIO_ReadPin (GPIOG, GPIO_PIN_2) ) {
            encoderDirection = TRUE;
        } else {
            encoderDirection = FALSE;
        }

        encoderAction=TRUE;
    } else if(GPIO_Pin==GPIO_PIN_3 && toggleEdit==FALSE) {
        // GP3 is the button. Press to change field (the pin goes to zero)
        toggleEdit=TRUE;
        BSP_LED_On(LED3);

    }
}

/*
    Encoder movement
*/
void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);

}

/*
    Button press
*/
void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

/*****************************************************************************
 **
 **    MIDI RECEIVE STATE MACHINE
 **
 *****************************************************************************/

volatile int velocity;
volatile int key;
volatile int control;
volatile int value;
volatile int event_channel;

/*
    Check if we should process events on the given channel.
*/
int validateChannel(uint8_t ch)
{
    int ret=FALSE;
    switch(currentMode) {
        case OMNI:
            ret=TRUE;
            break;
        case POLY:
            if(ch==currentChannel)
                ret=TRUE;
            break;
        case MULTI:
            ret=TRUE;
            break;
        case MONO:
            ret=TRUE;
            break;
        case HARP:
            ret=TRUE;
            break;
    }
    return ret;
}

#define MAX_HARP_SIZE 16
uint8_t harp_stack[16][MAX_HARP_SIZE];
uint8_t harp_size[16];
uint8_t harp_iterator[16];

void harp_reset(void)
{
    for (int i=0; i<16; ++i)
        harp_size[i]=0;
}

void harp_push(int8_t ch, int8_t key)
{
    if(++(harp_size[ch])>=MAX_HARP_SIZE)
        harp_size[ch]=MAX_HARP_SIZE-1;
    harp_stack[ch][harp_size[ch]-1]=key;
}

void harp_remove(int8_t ch, int8_t key)
{
    for(int i=0; i<harp_size[ch];++i) {
        if(harp_stack[ch][i]==key) {
            --harp_size[ch];
            for (int j=i+1; j<=harp_size[ch];++j) {
                harp_stack[ch][j-1]=harp_stack[ch][j];
            }
            break;
        }
    }
}

void harp_navigate(void)
{
    int inst;
    if(currentMode!=HARP)
        return;

    for(int ch=0; ch<16; ++ch) {
        if(harp_size[ch]==0)
            continue;
    
        SID_Note_Off(harp_stack[ch][harp_iterator[ch]], ch);
    
        if(++harp_iterator[ch]>=harp_size[ch])
            harp_iterator[ch]=0;

        inst = channelsInstr[ch];
        SID_Note_On(harp_stack[ch][harp_iterator[ch]], 127, 
            &GeneralMIDI[inst],ch);
    }
}

#define ADJUST_NOTE_KEY(note, inst)\
    (note)=key; \
    (inst)=CurrInst;\
    if (currentMode==OMNI) {\
        event_channel=currentChannel;\
    } else if(currentMode==MULTI) {\
        (inst)=channelsInstr[event_channel];\
    }\
    if(event_channel==9) {\
        (inst)=DrumKit[key].instrument;\
        (note)=DrumKit[key].note;\
    }

void MIDIStateMachine(uint8_t rec, uint8_t channel, uint8_t event)
{
    // MIDI receive state machine.
    uint8_t note;
    uint8_t inst;

    switch (MIDI_ReceiveState) {
        case idle:
            // We are in this state most of the time, until an event is
            // received.

            // Check if we should respond to an event
            if(event>=0x80 && validateChannel(channel)==FALSE)
                break;

            event_channel=channel;
            if(event==NOTE_ON) {                  // NOTE_ON
                MIDI_ReceiveState = note_on_k;
            } else if(event==NOTE_OFF) {          // NOTE_OFF
                MIDI_ReceiveState = note_off_k;
            } else if(event==CONTROL_CHANGE) {    // CONTROL CHANGE
                MIDI_ReceiveState = control_c;
            } else if(event==PROGRAM_CHANGE) {    // PROGRAM CHANGE
                MIDI_ReceiveState = program_c;
            }
            break;
        case program_c:
            channelsInstr[event_channel] = rec;
            if(event_channel==currentChannel) {
                CurrInst=rec;
                updateInstrument(rec);
            }
            /*sprintf(Message, "Pr. ch.: ch %2d pr %3d  ", event_channel+1,
                channelsInstr[event_channel]);
            NOTIFY_CHANGES();*/
            somethingChanged=TRUE;
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
            ADJUST_NOTE_KEY(note, inst);
            /*sprintf(Message, "ch=%d int= %d, note=%d ",
                event_channel+1, inst+1, note);
            NOTIFY_CHANGES();*/
            if(currentMode==HARP && event_channel!=9) {
                harp_push(event_channel, note);
            } else {
                SID_Note_On(note, velocity, &GeneralMIDI[inst],event_channel);
            }
            break;
        case note_off_k:
            key = rec;
            MIDI_ReceiveState = note_off_v;
            break;
        case note_off_v:
            velocity = rec;
            MIDI_ReceiveState = idle;
            ADJUST_NOTE_KEY(note, inst);
            if(currentMode==HARP && event_channel!=9) {
                harp_remove(event_channel, note);
                SID_Note_Off(note,event_channel);
            } else {
                SID_Note_Off(note,event_channel);
            }
            break;
        case control_c:
            control = rec;
            MIDI_ReceiveState = control_v;
            break;
        case control_v:
            value = rec;
            MIDI_ReceiveState = idle;
            if(control == CTRL_PEDAL) {     // SUSTAIN PEDAL
                if (value>63) {     // SUSTAIN ON
                    SustainPedal[event_channel]=TRUE;
                } else {            // SUSTAIN OFF
                    SustainPedal[event_channel]=FALSE;
                    StopPedalVoices(event_channel);
                }
            } else if(control == CTRL_CH_WHEEL) {    // Arturia change wheel
                if(value == 0x41) {                 // Increase program change
                    somethingChanged=TRUE;
                    if(++CurrInst>128)
                        CurrInst = 128;
                } else if(value == 0x3F) {          // Decrease program change
                    somethingChanged=TRUE;
                    if(--CurrInst<0)
                        CurrInst = 0;
                }
            } else if(control == CTRL_DUTY1) { // Change DUTY 1  NOTE: FIND CC
                currentDutyCycle1 = value << 5;
                updateDutyCycle1Message(channelsInstr[event_channel]);
            } else if(control == CTRL_DUTY2) { // Change DUTY 2  NOTE: FIND CC
                GeneralMIDI[channelsInstr[event_channel]].duty_cycle2=value<<5;
                sprintf(Message, "Duty 2 : %d",
                    GeneralMIDI[channelsInstr[event_channel]].duty_cycle2);
                NOTIFY_CHANGES();
            } /*else if(control == 0x4c) {    // Change LFO depth/amount
                GeneralMIDI[CurrInst].lfo_rate = value;
                sprintf(Message, "LFO rate : %d",
                    GeneralMIDI[CurrInst].lfo_rate);
                NOTIFY_CHANGES();
            } else if(control == 0x4D) {    // Change LFO depth/amount
                GeneralMIDI[CurrInst].lfo_depth = value;
                sprintf(Message, "LFO amount : %d",
                    GeneralMIDI[CurrInst].lfo_depth);
                NOTIFY_CHANGES();
            }*/ else if(control == CTRL_ATTACK1) {    // Change attack 1
                currentAttack1 = value >> 3;
                updateAttack1Message(channelsInstr[event_channel]);
            } else if(control == CTRL_ATTACK2) {    // Change attack 2
                currentAttack2 = value >> 3;
                updateAttack2Message(channelsInstr[event_channel]);
            } else if(control == CTRL_DECAY1) {    // Change decay 1
                currentDecay1 = value >> 3;
                updateDecay1Message(channelsInstr[event_channel]);
            } else if(control == CTRL_DECAY2) {    // Change decay 2
                currentDecay2 = value >> 3;
                updateDecay2Message(channelsInstr[event_channel]);
            } else if(control == CTRL_SUSTAIN1) {    // Change sustain 1
                currentSustain1 = value >> 3;
                updateSustain1Message(channelsInstr[event_channel]);
            } else if(control == CTRL_SUSTAIN2) {    // Change sustain 2
                currentSustain2 = value >> 3;
                updateSustain2Message(channelsInstr[event_channel]);
                NOTIFY_CHANGES();
            } else if(control == CTRL_RELEASE1) {    // Change release 1
                currentRelease1 = value >> 3;
                updateRelease1Message(channelsInstr[event_channel]);
            } else if(control == CTRL_RELEASE2) {    // Change release 2
                currentRelease2 = value >> 3;
                updateRelease2Message(channelsInstr[event_channel]);
            } else if(control == CTRL_CUTOFF) {    // Change cutoff
                currentCutoff = value << 4;
                updateFilterCutoffMessage(channelsInstr[event_channel]);
            } else if(control == CTRL_RESONANCE) {    // Change resonance
                currentResonance = value >>3;
            } else if(control == CTRL_MASTERVOL) {    // Change master volume
                Master_Volume = value >>3;
                sprintf(Message, "Volume: %d", Master_Volume);
                NOTIFY_CHANGES();
            } else if(control == CTRL_WAVE1) {    // Change waveform 1
                currentWave1 = (int)(value)*5/127;
                updateWave1Message(channelsInstr[event_channel]);
            } else if(control == CTRL_WAVE2) {    // Change waveform 2
                currentWave2 = (int)(value)*5/127;
                updateWave2Message(channelsInstr[event_channel]);
            } else if(control == CTRL_FILTTYPE) {    // Filter type
                currentFilterMode = value>>5;
                updateFilterModeMessage(channelsInstr[event_channel]);
            } else if(control == CTRL_DIFFC) {  // Voice 2 to v. 1 diff COARSE
                GeneralMIDI[channelsInstr[event_channel]].diff=(value>>2)*100;
                sprintf(Message, "V2 to V1 : %d",
                    GeneralMIDI[channelsInstr[event_channel]].diff);
                NOTIFY_CHANGES();
            } else if(control == CTRL_DIFFF) {    // Voice 2 to v. 1 diff FINE
                GeneralMIDI[channelsInstr[event_channel]].diff =
                    ((int)(GeneralMIDI[channelsInstr[event_channel]].diff/100)
                    *100)+(value);
                sprintf(Message, "V2 to V1 : %d",
                    GeneralMIDI[channelsInstr[event_channel]].diff);
                NOTIFY_CHANGES();
            }

            break;
        default:
            MIDI_ReceiveState=idle;
    }
}


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
    uint8_t channel = rec & 0x0F;
    uint8_t event =   rec & 0xF0;

    MIDIStateMachine(rec, channel, event);

    HAL_UART_Receive_IT(&UartHandle, aRxBuffer, 1);
}


/**
  * @brief  Period elapsed callback in non blocking mode (see TB_init), called
        at 1ms intervals
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static int32_t counter;
    ++counter;
    float interval = 1000.0*60/bpm/division_table[division];
    if(counter>(int32_t) interval){
        counter=0;
        harp_navigate();
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    /* Turn LED3 on */
    //BSP_LED_On(LED3);
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
    //BSP_LED_On(LED3);
    if (huart->ErrorCode == HAL_UART_ERROR_ORE){
        // remove the error condition
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        // set the correct state, so that the UART_RX_IT works correctly
        huart->State = HAL_UART_STATE_BUSY_RX;
    }
}

