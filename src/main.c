/**
  ******************************************************************************
  * @file    main.c
  * @author  Pierpaolo Bagnasco, Davide Bucci
  * @version V0.1
  * @date    15-July-2014
  * @brief   This file provides an example of how to use the LCD of the
  *          STM32F429-DISCOVERY.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* Definition for TIMx clock resources */
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE                __HAL_RCC_TIM3_CLK_ENABLE

/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM3_IRQn
#define TIMx_IRQHandler                TIM3_IRQHandler

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
    
    GPIO_Init_E.Pin =   GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5
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

void SID_Set_RW(int rw)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6,
        rw ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/** SID chips latch data and address buses at the falling edge of their clock.
*/
void SID_Pulse_Clock(void)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_Delay(1);

}

void SID_Select(int id)
{
    if(id==1) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);

    } else {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
    }
}

void SID_Set_Register(int address, int data)
{
    SID_Set_Address(address);
    SID_Set_RW(SID_WRITE);
    SID_Set_Data(data);
    SID_Pulse_Clock();
}

/* End of SID interface functions */

/* Useful SID registers */

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

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef    TimHandle;
TIM_HandleTypeDef htim2;


uint16_t uwPrescalerValue = 0;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/


void TB_init(void)
{
  /*##-1- Configure the TIM peripheral #######################################*/ 
  /* -----------------------------------------------------------------------
    In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM3CLK = 2 * PCLK1  
      PCLK1 = HCLK / 4 
      => TIM3CLK = HCLK / 2 = SystemCoreClock /2
    To get TIM3 counter clock at 10 kHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = ((SystemCoreClock /2) /10 kHz) - 1
       
    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to update SystemCoreClock 
     variable value. Otherwise, any configuration based on this variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency  
  ----------------------------------------------------------------------- */  
  
  /* Compute the prescaler value to have TIM3 counter clock equal to 10 kHz */
  uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 10000) - 1;
  
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
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;

  if(HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if(HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }
    __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);

  BSP_LED_On(LED3);
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

    SID_Select(0);


    while (1) {
        BSP_LED_Toggle(LED4);
        SID_Set_Register(SID_MODE_VOL,  15);
        SID_Set_Register(SID_VOICE1_AD, 16+9);
        SID_Set_Register(SID_VOICE1_SR, 4*16+4);
        SID_Set_Register(SID_VOICE1_FREQ_HI, 29);
        SID_Set_Register(SID_VOICE1_FREQ_LO, 69);
        SID_Set_Register(SID_VOICE1_CONTROL, 17);
        HAL_Delay(50);
        SID_Set_Register(SID_VOICE1_CONTROL, 0);
        HAL_Delay(50);
    }
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    BSP_LED_Toggle(LED3);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
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
static void SystemClock_Config(void) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
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
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
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
