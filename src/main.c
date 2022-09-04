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
#include <math.h>
#include "SID_def.h"

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
int  MessageCountdown;

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
        if(CounterRefreshInfos++>10) {
            CounterRefreshInfos=0;
            ShowInstrument();
            if(MessageCountdown) {
                --MessageCountdown;
                ToErase=1;
                BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
                BSP_LCD_DisplayStringAtLineMode(19, 
                    (uint8_t *) Message, CENTER_MODE);
            } else {
                if(ToErase) {
                    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
                    BSP_LCD_FillRect(0,320-16,240,16);         // Noisy!!!
                    ToErase=0;
                }
            }
        }
        UpdateLFO();

        BSP_LED_Toggle(LED4);
        HAL_Delay(10);
        LFO_Pointer += GeneralMIDI[CurrInst].lfo_rate;
        if(LFO_Pointer >= LFO_SIZE)
            LFO_Pointer = 0;

        /*
            char buffer[256];

        BSP_LCD_DisplayStringAtLineMode(18,
            "[                           ]", CENTER_MODE);
        sprintf(buffer, "%d %d %d, %d %d %d",
            Voices[0].key,Voices[1].key,Voices[2].key,
            Voices[3].key,Voices[4].key,Voices[5].key);
        BSP_LCD_DisplayStringAtLineMode(14, (uint8_t *) buffer, CENTER_MODE);*/
    }
}

void ShowInstrument(void)
{
    char buffer[256];
    int l=0;
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    sprintf(buffer, "%s                               ",
        GeneralMIDI[CurrInst].name);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    ++l;
    BSP_LCD_DrawHLine(0,18,240);
    BSP_LCD_DrawHLine(0,17,240);
    
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
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
    sprintf(buffer, "Attack 1: %d   ", GeneralMIDI[CurrInst].a);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Decay 1: %d   ", GeneralMIDI[CurrInst].d);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Sustain 1: %d   ", GeneralMIDI[CurrInst].s);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Release 1: %d   ", GeneralMIDI[CurrInst].r);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Duty Cycle 1: %d   ", GeneralMIDI[CurrInst].duty_cycle);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Diff v1 to v2: %d cents   ", GeneralMIDI[CurrInst].diff);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    
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
    sprintf(buffer, "Attack 2: %d   ", GeneralMIDI[CurrInst].a2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Decay 2: %d   ", GeneralMIDI[CurrInst].d2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Sustain 2: %d   ", GeneralMIDI[CurrInst].s2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Release 2: %d   ", GeneralMIDI[CurrInst].r2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Duty Cycle 2: %d   ", GeneralMIDI[CurrInst].duty_cycle2);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Filter mode: %d   ", GeneralMIDI[CurrInst].filt_mode);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Filter cutoff: %d   ", GeneralMIDI[CurrInst].filt_cutoff);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Filter resonance: %d   ", 
        GeneralMIDI[CurrInst].filt_resonance);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
    sprintf(buffer, "Filter routing: %d   ", 
        GeneralMIDI[CurrInst].filt_routing);
    BSP_LCD_DisplayStringAtLineMode(l++, (uint8_t *) buffer, LEFT_MODE);
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
                GeneralMIDI[CurrInst].duty_cycle = value << 5;
                sprintf(Message, "Duty 1 : %d",
                    GeneralMIDI[CurrInst].duty_cycle);
                MessageCountdown = 20;
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
                GeneralMIDI[CurrInst].a = value >> 3;
                sprintf(Message, "Attack 1: %d",
                    GeneralMIDI[CurrInst].a);
                MessageCountdown = 20;
            } else if(control == 0x50) {    // Change attack 2
                GeneralMIDI[CurrInst].a2 = value >> 3;
                sprintf(Message, "Attack 2: %d",
                    GeneralMIDI[CurrInst].a2);
                MessageCountdown = 20;
            } else if(control == 0x4B) {    // Change decay 1
                GeneralMIDI[CurrInst].d = value >> 3;
                sprintf(Message, "Decay 1: %d",
                    GeneralMIDI[CurrInst].d);
                MessageCountdown = 20;
            } else if(control == 0x51) {    // Change decay 2
                GeneralMIDI[CurrInst].d2 = value >> 3;
                sprintf(Message, "Decay 2: %d",
                    GeneralMIDI[CurrInst].d2);
                MessageCountdown = 20;
            } else if(control == 0x4F) {    // Change sustain 1
                GeneralMIDI[CurrInst].s = value >> 3;
                sprintf(Message, "Sustain 1: %d",
                    GeneralMIDI[CurrInst].s);
                MessageCountdown = 20;
            } else if(control == 0x52) {    // Change sustain 2
                GeneralMIDI[CurrInst].s2 = value >> 3;
                sprintf(Message, "Sustain 2: %d",
                    GeneralMIDI[CurrInst].s2);
                MessageCountdown = 20;
            } else if(control == 0x48) {    // Change release
                GeneralMIDI[CurrInst].r = value >> 3;
                sprintf(Message, "Release 1: %d",
                    GeneralMIDI[CurrInst].r);
                MessageCountdown = 20;
            } else if(control == 0x53) {    // Change release
                GeneralMIDI[CurrInst].r2 = value >> 3;
                sprintf(Message, "Release 2: %d",
                    GeneralMIDI[CurrInst].r2);
                MessageCountdown = 20;
            } else if(control == 0x4A) {    // Change cutoff
                GeneralMIDI[CurrInst].filt_cutoff = value << 4;
                sprintf(Message, "Cutoff: %d",
                    GeneralMIDI[CurrInst].filt_cutoff);
                MessageCountdown = 20;
            } else if(control == 0x47) {    // Change resonance
                GeneralMIDI[CurrInst].filt_resonance = value >>3;
                sprintf(Message, "Resonance: %d",
                    GeneralMIDI[CurrInst].filt_resonance);
                MessageCountdown = 20;
            } else if(control == 0x55) {    // Change master volume
                Master_Volume = value >>3;
                sprintf(Message, "Volume: %d", Master_Volume);
                MessageCountdown = 20;
            } else if(control == 0x5D) {    // Change waveform 1
                uint8_t choice[5]={NONE, TRIAN, SAWTH, PULSE, NOISE};
                int ch = (int)(value)*5/127;
                char*   wav[5] = 
                    {"None      ", 
                     "Triangular", 
                     "Sawtooth  ", 
                     "Pulse     ",
                     "Noise     "};
                GeneralMIDI[CurrInst].voice = choice[ch];
                sprintf(Message, "Wave 1: %s", wav[ch]);
                MessageCountdown = 20;
            } else if(control == 0x12) {    // Change waveform 2
                uint8_t choice[5]={NONE, TRIAN, SAWTH, PULSE, NOISE};
                int ch = (int)(value)*5/127;
                char*   wav[5] = 
                    {"None      ", 
                     "Triangular", 
                     "Sawtooth  ", 
                     "Pulse     ",
                     "Noise     "};
                GeneralMIDI[CurrInst].voice2 = choice[ch];
                sprintf(Message, "Wave 2: %s", wav[ch]);
                MessageCountdown = 20;
            } else if(control == 0x13) {    // Filter type
                uint8_t choice[3]={LO, HI, BP};
                char*   wav[4] = {"NONE", "Low pass", "Hi Pass", "Band pass"};
                uint8_t ch = value>>5;
                if(ch > 0) {
                    GeneralMIDI[CurrInst].filt_mode = choice[ch-1];
                    GeneralMIDI[CurrInst].filt_routing = ALL;
                } else {
                    GeneralMIDI[CurrInst].filt_routing = NON;
                }
                sprintf(Message, "Filter: %s", wav[ch]);
                MessageCountdown = 20;
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

