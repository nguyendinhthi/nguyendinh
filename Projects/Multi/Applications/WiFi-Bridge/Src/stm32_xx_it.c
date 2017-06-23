  /**
  ******************************************************************************
  * @file    stm32_xx_it.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    20-January-2016
  * @brief   Main Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "wifi_module.h"
#include "sensors.h"

/** @addtogroup FP-NET-6LPWIFI1_Applications
* @{
*/

/** @addtogroup WIFI_Bridge
* @{
*/


/** @defgroup 6LPWIFI1_IT
* @{
*/

extern UART_HandleTypeDef UartWiFiHandle, *UartMsgHandle;
extern TIM_HandleTypeDef    TimHandle, PushTimHandle;
extern TIM_HandleTypeDef htim5;
extern const struct sensors_sensor button_sensor;
extern volatile unsigned long seconds;
extern volatile clock_time_t ticks;
extern volatile uint32_t rtimer_clock;

/*-----------------------------------------------------*/

/*-----------------------------------------------------*/
/*contiki clock variables*/
extern volatile unsigned long seconds;
extern volatile clock_time_t ticks;
/*-----------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void USARTx_IRQHandler(void);    
void USARTx_PRINT_IRQHandler(void);
//void USARTx_EXTI_IRQHandler(void);
void TIMx_IRQHandler(void);
void TIMp_IRQHandler(void);

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
    BSP_LED_On(LED2); 
  }
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
    BSP_LED_On(LED2); 
  }
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
    BSP_LED_On(LED2); 
  }
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
    BSP_LED_On(LED2); 
  }
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
  BSP_LED_On(LED2); 
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
  BSP_LED_On(LED2); 
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}
/*---------------------------------------------------------------------------*/
/******************************************************************************/
/*                 STM32F1xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f1xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles TIM interrupt request.
  * @param  None
  * @retval None
  */
void TIMx_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TimHandle);
  
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles TIM interrupt request.
  * @param  None
  * @retval None
  */
void TIMp_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&PushTimHandle);
  
}
/*---------------------------------------------------------------------------*/
/**
* @brief  This function handles TIM5 interrupt request.
* @param  None
* @retval None
*/
void TIM5_IRQHandler(void)
{
  /* clear interrupt pending flag */
  __HAL_TIM_CLEAR_IT(&htim5, TIM_IT_UPDATE);

  rtimer_clock++;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
  Wifi_SysTick_Isr();
  Contiki_SysTick_Handler();
}
/*---------------------------------------------------------------------------*/
/**
* @brief  Period elapsed callback in non blocking mode
*         This timer is used for calling back User registered functions with information
* @param  htim : TIM handle
* @retval None
*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{ 
  Wifi_TIM_Handler(htim);
}
/*---------------------------------------------------------------------------*/
/**
* @brief  HAL_UART_RxCpltCallback
*         Rx Transfer completed callback
* @param  UsartHandle: UART handle
* @retval None
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandleArg)
{
  WiFi_HAL_UART_RxCpltCallback(UartHandleArg);
}
/*---------------------------------------------------------------------------*/
/**
* @brief  HAL_UART_TxCpltCallback
*         Tx Transfer completed callback
* @param  UsartHandle: UART handle
* @retval None
*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandleArg)
{
  WiFi_HAL_UART_TxCpltCallback(UartHandleArg);
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  UART error callbacks
  * @param  UsartHandle: UART handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  WiFi_HAL_UART_ErrorCallback(UartHandle);
}
/*---------------------------------------------------------------------------*/
/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles USARTx Handler.
  * @param  None
  * @retval None
  */
void USARTx_IRQHandler(void)
{
  HAL_UART_IRQHandler(&UartWiFiHandle);
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  This function handles USARTx vcom Handler.
  * @param  None
  * @retval None
  */
#ifdef USART_PRINT_MSG
void USARTx_PRINT_IRQHandler(void)
{
   HAL_UART_IRQHandler(UartMsgHandle);
}
#endif
/*---------------------------------------------------------------------------*/
void Contiki_SysTick_Handler(void)
{
  ticks++;
  if((ticks % CLOCK_SECOND) == 0) {
    seconds++;
    energest_flush();
  }

  if(etimer_pending()) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
void EXTI0_IRQHandler(void)
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
  }
  while(1);
}
/*---------------------------------------------------------------------------*/
void EXTI1_IRQHandler(void)
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
  }
  while(1);
}
/*---------------------------------------------------------------------------*/
void EXTI2_IRQHandler(void)
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
  }
  while(1);
}
/*---------------------------------------------------------------------------*/
void EXTI3_IRQHandler(void)
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_3) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
  }
  while(1);
}
/*---------------------------------------------------------------------------*/
/**
* @brief  This function handles External lines 15 to 4 interrupt request.
* @param  None
* @retval None
*/
void EXTI9_5_IRQHandler(void)
{
  /* EXTI line 7 interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(RADIO_GPIO_3_EXTI_LINE))
  {
    __HAL_GPIO_EXTI_CLEAR_IT(RADIO_GPIO_3_EXTI_LINE);

    HAL_GPIO_EXTI_Callback(RADIO_GPIO_3_EXTI_LINE);

    spirit1_interrupt_callback();
  }
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);

  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);

#ifndef  LPM_ENABLE

  //  if(__HAL_GPIO_EXTI_GET_IT(KEY_BUTTON_EXTI_LINE) != RESET)
  //  {
  //    __HAL_GPIO_EXTI_CLEAR_IT(KEY_BUTTON_EXTI_LINE);
  //
  // Set_KeyStatus(SET);
  //  }
  //
#else /*Low Power mode enabled*/

#if defined(RF_STANDBY)/*if spirit1 is in standby*/

  if(EXTI->PR & KEY_BUTTON_EXTI_LINE)
  {
    HAL_GPIO_EXTI_Callback(KEY_BUTTON_EXTI_LINE);
    /* EXTI line 13 interrupt detected */
    if(HAL_GPIO_ReadPin(KEY_BUTTON_GPIO_PORT, KEY_BUTTON_PIN) == 0x01) //0x00
    {
      HAL_GPIO_EXTI_Callback(KEY_BUTTON_EXTI_LINE);

      PushButtonStatusWakeup = SET;
      PushButtonStatusData = RESET;
      wakeupCounter = LPM_WAKEUP_TIME;
      dataSendCounter = DATA_SEND_TIME;
      dataSendCounter++;
    }
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_BUTTON_EXTI_LINE);
  }
#else /*if spirit1 is not in standby or sleep mode but MCU is in LPM*/

  if(__HAL_GPIO_EXTI_GET_IT(KEY_BUTTON_EXTI_LINE) != RESET)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_BUTTON_EXTI_LINE);

    HAL_GPIO_EXTI_Callback(KEY_BUTTON_EXTI_LINE);

    Set_KeyStatus(SET);
  }
#endif
#endif
}
/*---------------------------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if defined(MCU_STOP_MODE)/*if MCU is in stop mode*/

  /* Clear Wake Up Flag */
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  /* Configures system clock after wake-up from STOP: enable HSE, PLL and select
    PLL as system clock source (HSE and PLL are disabled in STOP mode) */
    SystemClockConfig_STOP();
#endif
#if defined(MCU_SLEEP_MODE)
    /* Resume Tick interrupt if disabled prior to sleep mode entry*/
    HAL_ResumeTick();
#endif

  if (GPIO_Pin == USER_BUTTON_PIN)
  {
    sensors_changed(&button_sensor);
  }
#ifdef SPWF04
  if(GPIO_Pin == WIFI_SPI_EXTI_PIN) {
    WIFI_SPI_IRQ_Callback();
  }
#endif
}
/*---------------------------------------------------------------------------*/
#ifdef SPWF04
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == WIFI_SPI_EXTI_PIN) {
    WIFI_SPI_IRQ_Callback();
  }
  else {
    //nothing to do
  }
}

/**
  * @brief  WIFI_SPI_EXTI_IRQHandler This function handles External line
  *         interrupt request for BlueNRG.
  * @param  None
  * @retval None
  */
void WIFI_SPI_EXTI_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(WIFI_SPI_EXTI_PIN);
}

#ifdef STM32L476xx
void DMA1_Channel2_IRQHandler(void)
{
  if(__HAL_DMA_GET_IT_SOURCE(SpiHandle.hdmarx, DMA_IT_TC)  && __HAL_DMA_GET_FLAG(SpiHandle.hdmarx, DMA_FLAG_TC2))
  {
    WiFi_DMA_RxCallback();
  }
}

void DMA1_Channel3_IRQHandler(void)
{
  if(__HAL_DMA_GET_IT_SOURCE(SpiHandle.hdmatx, DMA_IT_TC)  && __HAL_DMA_GET_FLAG(SpiHandle.hdmatx, DMA_FLAG_TC3))
  {
    WiFi_DMA_TxCallback();
  }
}
#endif /* STM32L476xx */

#ifdef STM32F401xE
void DMA2_Stream0_IRQHandler(void)
{
  if(__HAL_DMA_GET_IT_SOURCE(SpiHandle.hdmarx, DMA_IT_TC)  && __HAL_DMA_GET_FLAG(SpiHandle.hdmarx, DMA_FLAG_TCIF0_4))
  {
    WiFi_DMA_RxCallback();
  }
}

void DMA2_Stream3_IRQHandler(void)
{
  if(__HAL_DMA_GET_IT_SOURCE(SpiHandle.hdmatx, DMA_IT_TC)  && __HAL_DMA_GET_FLAG(SpiHandle.hdmatx, DMA_FLAG_TCIF3_7))
  {
    WiFi_DMA_TxCallback();
  }
}
#endif /* STM32F401xE */
#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
