/**
  ******************************************************************************
  * @file    main.c
  * @author  Davide Bucci
  * @version V0.2
  * @date    August 14, 2022 - December 2024
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
#include <math.h>
#include "SID_def.h"
#include "Utilities/Fonts/fonts.h"

void ShowInstrument(void);


/* UART handler declaration */
volatile UART_HandleTypeDef UartHandle;

/* Buffer used for reception */
uint8_t aRxBuffer[RXBUFFERSIZE];

int receive=0;

extern volatile int CurrInst;
extern volatile uint8_t SustainPedal;
extern volatile uint8_t Master_Volume;     // 0 to 15


extern SID_conf GeneralMIDI[];
extern VoiceDef Voices[]; // Shall we refactor code so we do not need it?

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


int currentField;

#define NUM_FIELD 20


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

int8_t currentChannel;
// The TT element is just there to force the enum to be signed, so that one
// can do tests such as if(currentMode>0) etc...
enum MIDI_mode {TT=-1,
    OMNI=0,                 // Respond to all events on all channels
    POLY,                   // Respond only to events on the current channel
    MULTI,                  // Respond to events on all channels separately
    MONO} currentMode;      // Respond to events on all channels, monophonic


extern int8_t LFO_Table[];
extern uint16_t LFO_Pointer;
extern uint8_t LFO_Amount;
extern uint8_t LFO_Rate;

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


    HAL_GPIO_Init(GPIOA, &GPIO_Init_A);
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




void updateInstrument(void)
{
    currentWave1=GeneralMIDI[CurrInst].voice;
    currentAttack1=GeneralMIDI[CurrInst].a;
    currentDecay1=GeneralMIDI[CurrInst].d;
    currentSustain1=GeneralMIDI[CurrInst].s;
    currentRelease1=GeneralMIDI[CurrInst].r;
    currentDutyCycle1=GeneralMIDI[CurrInst].duty_cycle;

    currentV1toV2=GeneralMIDI[CurrInst].diff;

    currentWave2=GeneralMIDI[CurrInst].voice2;
    currentAttack2=GeneralMIDI[CurrInst].a2;
    currentDecay2=GeneralMIDI[CurrInst].d2;
    currentSustain2=GeneralMIDI[CurrInst].s2;
    currentRelease2=GeneralMIDI[CurrInst].r2;
    currentDutyCycle2=GeneralMIDI[CurrInst].duty_cycle2;

    currentCutoff=GeneralMIDI[CurrInst].filt_cutoff;
    currentResonance=GeneralMIDI[CurrInst].filt_resonance;
    currentFilterMode=GeneralMIDI[CurrInst].filt_mode;
    //currentRouting=GeneralMIDI[CurrInst].filt_routing;

}

void updateWave1Message(void)
{
    char*   wav[5] =
        {"None      ",
         "Triangular",
         "Sawtooth  ",
         "Pulse     ",
         "Noise     "};
    uint8_t choice[5]={NONE, TRIAN, SAWTH, PULSE, NOISE};

    GeneralMIDI[CurrInst].voice = choice[currentWave1];
    sprintf(Message, "Wave 1: %s  ", wav[currentWave1]);
    MessageCountdown = 20;
}

void updateAttack1Message(void)
{
    GeneralMIDI[CurrInst].a=currentAttack1;
    sprintf(Message, "Attack 1: %d  ", currentAttack1);
    MessageCountdown = 20;
}

void updateDecay1Message(void)
{
    GeneralMIDI[CurrInst].d=currentDecay1;
    sprintf(Message, "Decay 1: %d  ", currentDecay1);
    MessageCountdown = 20;
}

void updateSustain1Message(void)
{
    GeneralMIDI[CurrInst].s=currentSustain1;
    sprintf(Message, "Sustain 1: %d  ", currentSustain1);
    MessageCountdown = 20;
}

void updateRelease1Message(void)
{
    GeneralMIDI[CurrInst].r=currentRelease1;
    sprintf(Message, "Release 1: %d  ", currentRelease1);
    MessageCountdown = 20;
}

void updateDutyCycle1Message(void)
{
    GeneralMIDI[CurrInst].duty_cycle=currentDutyCycle1;
    sprintf(Message, "Duty 1: %d  ",currentDutyCycle1);
    MessageCountdown = 20;
}

void updateCurrentV1toV2Message(void)
{
    GeneralMIDI[CurrInst].diff=currentV1toV2;
    sprintf(Message, "Rel v1 to v2: %4d", GeneralMIDI[CurrInst].diff);

    MessageCountdown = 20;
}

void updateWave2Message(void)
{
    char*   wav[5] =
        {"None      ",
         "Triangular",
         "Sawtooth  ",
         "Pulse     ",
         "Noise     "};
    uint8_t choice[5]={NONE, TRIAN, SAWTH, PULSE, NOISE};

    GeneralMIDI[CurrInst].voice2 = choice[currentWave2];
    sprintf(Message, "Wave 2: %s  ", wav[currentWave2]);
    MessageCountdown = 20;
}

void updateAttack2Message(void)
{
    GeneralMIDI[CurrInst].a2=currentAttack2;
    sprintf(Message, "Attack 2: %d  ", currentAttack2);
    MessageCountdown = 20;
}

void updateDecay2Message(void)
{
    GeneralMIDI[CurrInst].d2=currentDecay2;
    sprintf(Message, "Decay 2: %d  ", currentDecay2);
    MessageCountdown = 20;
}

void updateSustain2Message(void)
{
    GeneralMIDI[CurrInst].s2=currentSustain2;
    sprintf(Message, "Sustain 2: %d  ", currentSustain2);
    MessageCountdown = 20;
}

void updateRelease2Message(void)
{
    GeneralMIDI[CurrInst].r2=currentRelease2;
    sprintf(Message, "Release 2: %d  ", currentRelease2);
    MessageCountdown = 20;
}

void updateDutyCycle2Message(void)
{
    GeneralMIDI[CurrInst].duty_cycle2=currentDutyCycle2;
    sprintf(Message, "Duty 2: %d  ",currentDutyCycle2);
    MessageCountdown = 20;
}

void updateFilterModeMessage(void)
{
    uint8_t choice[3]={LO, HI, BP};
    char*   wav[4] = {"NONE", "Low pass", "Hi Pass", "Band pass"};
    if(currentFilterMode > 0) {
        GeneralMIDI[CurrInst].filt_mode = choice[currentFilterMode-1];
        GeneralMIDI[CurrInst].filt_routing = ALL;
    } else {
        GeneralMIDI[CurrInst].filt_routing = NON;
    }
    sprintf(Message, "Filter: %s    ", wav[currentFilterMode]);
    MessageCountdown = 20;
}

void updateFilterCutoffMessage(void)
{
    GeneralMIDI[CurrInst].filt_cutoff=currentCutoff;
    sprintf(Message, "Cutoff: %d  ", GeneralMIDI[CurrInst].filt_cutoff);
    MessageCountdown = 20;
}

void updateFilterResonanceMessage(void)
{
    GeneralMIDI[CurrInst].filt_resonance=currentResonance;
    sprintf(Message, "Resonance: %d  ", GeneralMIDI[CurrInst].filt_resonance);
    MessageCountdown = 20;
}

/*
void updateFilterRoutingMessage(void)
{
    GeneralMIDI[CurrInst].filt_routing=currentRouting;
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
            UPDATE_PAR(CurrInst,0,127,increment_l);
            updateInstrument();
            break;
        case 2:     // Wave 1
            UPDATE_PAR(currentWave1,0,4,1);
            updateWave1Message();
            break;
        case 3:     // Attack 1
            UPDATE_PAR(currentAttack1,0,15,1);
            updateAttack1Message();
            break;
        case 4:     // Decay 1
            UPDATE_PAR(currentDecay1,0,15,1);
            updateDecay1Message();
            break;
        case 5:     // Sustain 1
            UPDATE_PAR(currentSustain1,0,15,1);
            updateSustain1Message();
            break;
        case 6:     // Release 1
            UPDATE_PAR(currentRelease1,0,15,1);
            updateRelease1Message();
            break;
        case 7:     // Duty Cycle 1
            UPDATE_PAR(currentDutyCycle1,0,4095,increment);
            updateDutyCycle1Message();
            break;
        case 8:     // Rel v1 to v2
            UPDATE_PAR(currentV1toV2,0,9600,increment);
            updateCurrentV1toV2Message();
            break;
        case 9:     // Wave2
            UPDATE_PAR(currentWave2,0,4,1);
            updateWave2Message();
            break;
        case 10:     // Attack 2
            UPDATE_PAR(currentAttack2,0,15,1);
            updateAttack2Message();
            break;
        case 11:     // Decay 2
            UPDATE_PAR(currentDecay2,0,15,1);
            updateDecay2Message();
            break;
        case 12:     // Sustain 2
            UPDATE_PAR(currentSustain2,0,15,1);
            updateSustain2Message();
            break;
        case 13:     // Release 2
            UPDATE_PAR(currentRelease2,0,15,1);
            updateRelease2Message();
            break;
        case 14:     // Duty Cycle 2
            UPDATE_PAR(currentDutyCycle2,0,4095,increment);
            updateDutyCycle2Message();
            break;
        case 15:    // Filter mode
            UPDATE_PAR(currentFilterMode,0,3,1);
            updateFilterModeMessage();
            break;
        case 16:    // Filter cutoff
            UPDATE_PAR(currentCutoff,0,2047,increment);
            updateFilterCutoffMessage();
            break;
        case 17:    // Filter resonance
            UPDATE_PAR(currentResonance,0,15,1);
            updateFilterResonanceMessage();
            break;
        case 18:    // MIDI channel
            UPDATE_PAR(currentChannel,0,15,1);
            break;
        case 19:    // MIDI mode
            UPDATE_PAR(currentMode,0,3,1);
            break;
        default:
    }
}



void UserAction(void)
{

    if(changeField) {
        if(encoderDirection) {
            ++currentField;
            if(currentField==1) currentField=2; // Avoid the separating line
        } else {
            --currentField;
            if(currentField==1) currentField=0; // Avoid the separating line
        }
        if(currentField<0) currentField=0;
        if(currentField>=NUM_FIELD) currentField=NUM_FIELD-1;
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
        (uint8_t *) "(C) 2022-2024", CENTER_MODE);
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
        // Refresh the information every 10 repetitions of this loop.
        if(CounterRefreshInfos++>10) {
            CounterRefreshInfos=0;
            ShowInstrument();
            // Show that something is being changed in the last line.
            if(MessageCountdown) {
                --MessageCountdown;
                ToErase=1;
                BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
                BSP_LCD_SetFont(&Font16);
                BSP_LCD_DisplayStringAt(0,250,
                    (uint8_t *) Message, CENTER_MODE);
            } else {
                if(ToErase) {
                    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
                    BSP_LCD_FillRect(0,250,240,16);         // Noisy!!!
                    ToErase=0;
                }
            }
        }
        UpdateLFO();

        BSP_LED_Toggle(LED4);
        BSP_LED_Off(LED3);
        HAL_Delay(10);
        ++LargeMovement;
        LFO_Pointer += GeneralMIDI[CurrInst].lfo_rate;
        if(encoderAction) {
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
void DrawWave(int x, int y, int wave, int duty)
{
    int height=8;
    int base=10;
    float dd=(float)duty/4096.0;
    float ll=4*base;

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(x-2,y,4*base+4,20);
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    // Draw the baseline.
    BSP_LCD_DrawLine(x-2,y+height,             x+4*base+2,y+height);

    switch(wave) {
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
            int val[]={3,-2,5,-1,-8,6,-2,2,
                       3,-4,4,7,-6,5,2,-1};

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
    }
}

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

void ShowInstrument(void)
{
    char buffer[256];
    int l=0;


    DrawADSR(150,50,
        GeneralMIDI[CurrInst].a, GeneralMIDI[CurrInst].d,
        GeneralMIDI[CurrInst].s, GeneralMIDI[CurrInst].r);
    DrawADSR(150,150-16,
        GeneralMIDI[CurrInst].a2, GeneralMIDI[CurrInst].d2,
        GeneralMIDI[CurrInst].s2, GeneralMIDI[CurrInst].r2);
    DrawWave(150,20,GeneralMIDI[CurrInst].voice,
         GeneralMIDI[CurrInst].duty_cycle);
    DrawWave(150,100,GeneralMIDI[CurrInst].voice2,
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
        sprintf(buffer, "Filter mode: NONE ");
    } else {
        switch(GeneralMIDI[CurrInst].filt_mode) {
            case LO:
                sprintf(buffer, "Filter mode: LOW  ");
                break;
            case BP:
                sprintf(buffer, "Filter mode: BAND ");
                break;
            case HI:
                sprintf(buffer, "Filter mode: HI   ");
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

    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_DrawHLine(0,270,240);
    BSP_LCD_DrawHLine(0,271,240);

    setEv(l);
    BSP_LCD_SetFont(&Font16);
    sprintf(buffer, "Channel: %2d", currentChannel+1);
    BSP_LCD_DisplayStringAt(0,280,(uint8_t *)buffer, LEFT_MODE);
    ++l;
    setEv(l);
    char* m_mode[4]={"OMNI ", "POLY ", "MULTI", "MONO "};
    
    sprintf(buffer, "MIDI Mode: %s", m_mode[currentMode]);
    BSP_LCD_DisplayStringAt(0,300,(uint8_t *)buffer, LEFT_MODE);
    ++l;

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

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
}

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
            // TODO: recall here the channel configuration so that each one can
            // be played on a different instrument.
            ret=TRUE;
            break;
        case MONO:
            ret=TRUE;
            break;
    }
    return ret;
}


void MIDIStateMachine(uint8_t rec, uint8_t channel, uint8_t event)
{
// MIDI receive state machine.
    switch (MIDI_ReceiveState) {
        case idle:
            // We are in this state most of the time, until an event is
            // received.
            
            // Check if we should respond to an event
            if(event>=0x80 && validateChannel(channel)==FALSE)
                break;
            
            
            if(event==0x90) {           // NOTE_ON
                MIDI_ReceiveState = note_on_k;
            } else if(event==0x80) {    // NOTE_OFF
                MIDI_ReceiveState = note_off_k;
            } else if(event==0xB0) {    // CONTROL CHANGE
                MIDI_ReceiveState = control_c;
            } else if(event==0xC0) {    // PROGRAM CHANGE
                MIDI_ReceiveState = program_c;
            }
            break;
        case program_c:
            CurrInst = rec;
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
            SID_Note_On(key, velocity, &GeneralMIDI[CurrInst]);
            break;
        case note_off_k:
            key = rec;
            MIDI_ReceiveState = note_off_v;
            break;
        case note_off_v:
            velocity = rec;
            MIDI_ReceiveState = idle;
            // Stop the note here.
            SID_Note_Off(key);
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
                            SID_Stop_Voice(i);
                            Voices[i].key=0;
                        }
                    }
                }
            } else if(control == 0x72) {    // Arturia change wheel
                if(value == 0x41) {                 // Increase program change
                    if(++CurrInst>127)
                        CurrInst = 127;
                } else if(value == 0x3F) {          // Decrease program change
                    if(--CurrInst<0)
                        CurrInst = 0;
                }
            } else if(control == 0x4c) {    // Change DUTY 1  NOTE: FIND CC
                currentDutyCycle1 = value << 5;
                updateDutyCycle1Message();
            } else if(control == 0x4d) {    // Change DUTY 2  NOTE: FIND CC
                GeneralMIDI[CurrInst].duty_cycle2 = value << 5;
                sprintf(Message, "Duty 2 : %d",
                    GeneralMIDI[CurrInst].duty_cycle2);
                MessageCountdown = 20;
            } /*else if(control == 0x4c) {    // Change LFO depth/amount
                GeneralMIDI[CurrInst].lfo_rate = value;
                sprintf(Message, "LFO rate : %d",
                    GeneralMIDI[CurrInst].lfo_rate);
                MessageCountdown = 20;
            } else if(control == 0x4D) {    // Change LFO depth/amount
                GeneralMIDI[CurrInst].lfo_depth = value;
                sprintf(Message, "LFO amount : %d",
                    GeneralMIDI[CurrInst].lfo_depth);
                MessageCountdown = 20;
            }*/ else if(control == 0x49) {    // Change attack 1
                currentAttack1 = value >> 3;
                updateAttack1Message();
            } else if(control == 0x50) {    // Change attack 2
                currentAttack2 = value >> 3;
                updateAttack2Message();
            } else if(control == 0x4B) {    // Change decay 1
                currentDecay1 = value >> 3;
                updateDecay1Message();
            } else if(control == 0x51) {    // Change decay 2
                currentDecay2 = value >> 3;
                updateDecay2Message();
            } else if(control == 0x4F) {    // Change sustain 1
                currentSustain1 = value >> 3;
                updateSustain1Message();
            } else if(control == 0x52) {    // Change sustain 2
                currentSustain2 = value >> 3;
                updateSustain2Message();
                MessageCountdown = 20;
            } else if(control == 0x48) {    // Change release 1
                currentRelease1 = value >> 3;
                updateRelease1Message();
            } else if(control == 0x53) {    // Change release 2
                currentRelease2 = value >> 3;
                updateRelease2Message();
            } else if(control == 0x4A) {    // Change cutoff
                currentCutoff = value << 4;
                updateFilterCutoffMessage();
            } else if(control == 0x47) {    // Change resonance
                currentResonance = value >>3;

            } else if(control == 0x55) {    // Change master volume
                Master_Volume = value >>3;
                sprintf(Message, "Volume: %d", Master_Volume);
                MessageCountdown = 20;
            } else if(control == 0x5D) {    // Change waveform 1
                currentWave1 = (int)(value)*5/127;
                updateWave1Message();
            } else if(control == 0x12) {    // Change waveform 2
                currentWave2 = (int)(value)*5/127;
                updateWave2Message();
            } else if(control == 0x13) {    // Filter type
                currentFilterMode = value>>5;
                updateFilterModeMessage();
            } else if(control == 0x10) {    // Voice 2 to voice 1 diff COARSE
                GeneralMIDI[CurrInst].diff = (value>>2) * 100;
                sprintf(Message, "V2 to V1 : %d",
                    GeneralMIDI[CurrInst].diff);
                MessageCountdown = 20;
            } else if(control == 0x11) {    // Voice 2 to voice 1 diff FINE
                GeneralMIDI[CurrInst].diff =
                    ((int)(GeneralMIDI[CurrInst].diff/100)*100)
                    +(value);
                sprintf(Message, "V2 to V1 : %d",
                    GeneralMIDI[CurrInst].diff);
                MessageCountdown = 20;
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
    /* Turn LED3 on: Transfer error in reception/transmission process */
    //BSP_LED_On(LED3);
    if (huart->ErrorCode == HAL_UART_ERROR_ORE){
        // remove the error condition
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        // set the correct state, so that the UART_RX_IT works correctly
        huart->State = HAL_UART_STATE_BUSY_RX;
    }
}

