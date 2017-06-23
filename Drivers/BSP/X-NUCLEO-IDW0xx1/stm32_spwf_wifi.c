/**
 ******************************************************************************
 * @file    stm32_spwf_wifi.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    17-May-2016
 * @brief   HAL related functionality of X-CUBE-WIFI1
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
#include "wifi_module.h"
#include "stm32_spwf_wifi.h"
#include "wifi_globals.h"

/** @addtogroup BSP
* @{
*/ 


/** @defgroup  NUCLEO_WIFI_DRIVER
  * @brief Wi-Fi_driver modules
  * @{
  */


/** @defgroup NUCLEO_WIFI_DRIVER_Private_Defines
  * @{
  */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Prescaler declaration */
uint32_t uwPrescalerValue = 0;  

/* TIM handle declaration */

/**
  * @}
  */

/** @addtogroup NUCLEO_WIFI_DRIVER_Private_Variables
  * @{
  */
/* Private variables ---------------------------------------------------------*/

/**
  * @}
  */

  
/** @defgroup NUCLEO_WIFI_DRIVER_Private_Functions
  * @{
  */

  /*##-1- Configure the TIM peripheral #######################################*/
  /* -----------------------------------------------------------------------
    In this example TIM3 input clock (TIM3CLK)  is set to APB1 clock (PCLK1) x2,
    since APB1 prescaler is set to 4 (0x100).
       TIM3CLK = PCLK1*2
       PCLK1   = HCLK/2
    => TIM3CLK = PCLK1*2 = (HCLK/2)*2 = HCLK = SystemCoreClock
    To get TIM3 counter clock at 10 KHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = (SystemCoreClock /10 KHz) - 1

    Note:
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f1xx.c file.
     Each time the core clock (HCLK) changes, user had to update SystemCoreClock
     variable value. Otherwise, any configuration based on this variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
  ----------------------------------------------------------------------- */
void Timer_Config()
{
  /* Compute the prescaler value to have TIMx counter clock equal to 10000 Hz */
  uwPrescalerValue = (uint32_t)(SystemCoreClock / 10000) - 1;
  
  /* Set TIMx instance */
  TimHandle.Instance = TIMx;

  /* Initialize TIMx peripheral as follows:
       + Period = 10000 - 1
       + Prescaler = (SystemCoreClock/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
#if defined (USE_STM32L0XX_NUCLEO) || defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
  TimHandle.Init.Period            = 100 - 1;
#endif
#if defined (USE_STM32F1xx_NUCLEO) 
  TimHandle.Init.Period            = 100 - 1;
#endif  
  TimHandle.Init.Prescaler         = uwPrescalerValue;
  TimHandle.Init.ClockDivision     = 0;
  TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
#ifdef USE_STM32F1xx_NUCLEO
  TimHandle.Init.RepetitionCounter = 0;
#endif 

  if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }

}

/**
  * @brief Push_Timer_Config
  *        This function configures the Push Timer
  * @param None
  * @retval None
  */
void Push_Timer_Config()
{
  /* Compute the prescaler value to have TIMx counter clock equal to 10000 Hz */
  uwPrescalerValue = (uint32_t)(SystemCoreClock / 10000) - 1;
  
  /* Set TIMx instance */
  PushTimHandle.Instance = TIMp;

  /* Initialize TIMx peripheral as follows:
       + Period = 10000 - 1
       + Prescaler = (SystemCoreClock/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  PushTimHandle.Init.Period            = 10 - 1;//10000
  PushTimHandle.Init.Prescaler         = uwPrescalerValue;
  PushTimHandle.Init.ClockDivision     = 0;
  PushTimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
#ifdef USE_STM32F1xx_NUCLEO
  PushTimHandle.Init.RepetitionCounter = 0;
#endif 

  if (HAL_TIM_Base_Init(&PushTimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }

}

/**
* @brief  USART_Configuration
* WB_WIFI_UART configured as follow:
*      - BaudRate = 115200 baud  
*      - Word Length = 8 Bits
*      - One Stop Bit
*      - No parity
*      - Hardware flow control enabled (RTS and CTS signals)
*      - Receive and transmit enabled
*
* @param  None
* @retval None
*/
void UART_Configuration(uint32_t baud_rate)
{
  UartWiFiHandle.Instance             = WB_WIFI_UART;
  UartWiFiHandle.Init.BaudRate        = baud_rate;
  UartWiFiHandle.Init.WordLength      = UART_WORDLENGTH_8B;
  UartWiFiHandle.Init.StopBits        = UART_STOPBITS_1;
  UartWiFiHandle.Init.Parity          = UART_PARITY_NONE ;
#ifdef SPWF04
  UartWiFiHandle.Init.HwFlowCtl       = UART_HWCONTROL_NONE;
#else
  UartWiFiHandle.Init.HwFlowCtl       = UART_HWCONTROL_RTS;
#endif
  UartWiFiHandle.Init.Mode            = UART_MODE_TX_RX;
  UartWiFiHandle.Init.OverSampling    = UART_OVERSAMPLING_16;
  //UartWiFiHandle.Init.OneBitSampling  = UART_ONEBIT_SAMPLING_ENABLED;
  
  if(HAL_UART_DeInit(&UartWiFiHandle) != HAL_OK)
  {
    Error_Handler();
  }  
  if(HAL_UART_Init(&UartWiFiHandle) != HAL_OK)
  {
    Error_Handler();
  }  
 }


//#ifdef USART_PRINT_MSG
//void USART_PRINT_MSG_Configuration(uint32_t baud_rate)
//{
//  UartMsgHandle.Instance             = WIFI_UART_MSG;
//  UartMsgHandle.Init.BaudRate        = baud_rate;
//  UartMsgHandle.Init.WordLength      = UART_WORDLENGTH_8B;
//  UartMsgHandle.Init.StopBits        = UART_STOPBITS_1;
//  UartMsgHandle.Init.Parity          = UART_PARITY_NONE ;
//  UartMsgHandle.Init.HwFlowCtl       = UART_HWCONTROL_NONE;// USART_HardwareFlowControl_RTS_CTS;
//  UartMsgHandle.Init.Mode            = UART_MODE_TX_RX;
//
//  if(HAL_UART_DeInit(&UartMsgHandle) != HAL_OK)
//  {
//    Error_Handler();
//  }  
//  if(HAL_UART_Init(&UartMsgHandle) != HAL_OK)
//  {
//    Error_Handler();
//  }
//      
//}


//void UART_Msg_Gpio_Init()
//{ 
//  GPIO_InitTypeDef  GPIO_InitStruct;
//
//  /*##-1- Enable peripherals and GPIO Clocks #################################*/
//  /* Enable GPIO TX/RX clock */
//  USARTx_PRINT_TX_GPIO_CLK_ENABLE();
//  USARTx_PRINT_RX_GPIO_CLK_ENABLE();
//
//
//  /* Enable USARTx clock */
//  USARTx_PRINT_CLK_ENABLE(); 
//    __SYSCFG_CLK_ENABLE();
//  /*##-2- Configure peripheral GPIO ##########################################*/  
//  /* UART TX GPIO pin configuration  */
//  GPIO_InitStruct.Pin       = WiFi_USART_PRINT_TX_PIN;
//  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
//  GPIO_InitStruct.Pull      = GPIO_PULLUP;
//  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
//#if defined (USE_STM32L0XX_NUCLEO) || (USE_STM32F4XX_NUCLEO) || (USE_STM32L4XX_NUCLEO)
//  GPIO_InitStruct.Alternate = PRINTMSG_USARTx_TX_AF;
//#endif  
//  HAL_GPIO_Init(WiFi_USART_PRINT_TX_GPIO_PORT, &GPIO_InitStruct);
//
//  /* UART RX GPIO pin configuration  */
//  GPIO_InitStruct.Pin = WiFi_USART_PRINT_RX_PIN;
//  GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
//#if defined (USE_STM32L0XX_NUCLEO) || (USE_STM32F4XX_NUCLEO) || (USE_STM32L4XX_NUCLEO)
//  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
//  GPIO_InitStruct.Alternate = PRINTMSG_USARTx_RX_AF;
//#endif 
//  
//  HAL_GPIO_Init(WiFi_USART_PRINT_RX_GPIO_PORT, &GPIO_InitStruct);
//  
//#ifdef WIFI_USE_VCOM
//  /*##-3- Configure the NVIC for UART ########################################*/
//  /* NVIC for USART */
//  HAL_NVIC_SetPriority(USARTx_PRINT_IRQn, 0, 1);
//  HAL_NVIC_EnableIRQ(USARTx_PRINT_IRQn);
//#endif
//}
//#endif

#if defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
 
void WiFi_SPI_Init(SPI_HandleTypeDef * hspi)
{
  hspi->Instance = WIFI_SPI_INSTANCE;
  hspi->Init.Mode = WIFI_SPI_MODE;
  hspi->Init.Direction = WIFI_SPI_DIRECTION;
  hspi->Init.DataSize = WIFI_SPI_DATASIZE;
  hspi->Init.CLKPolarity = WIFI_SPI_CLKPOLARITY;
  hspi->Init.CLKPhase = WIFI_SPI_CLKPHASE;
  hspi->Init.NSS = WIFI_SPI_NSS;
  hspi->Init.FirstBit = WIFI_SPI_FIRSTBIT;
  hspi->Init.TIMode = WIFI_SPI_TIMODE;
  hspi->Init.CRCPolynomial = WIFI_SPI_CRCPOLYNOMIAL;
  hspi->Init.BaudRatePrescaler = WIFI_SPI_BAUDRATEPRESCALER;
  hspi->Init.CRCCalculation = WIFI_SPI_CRCCALCULATION;

  HAL_SPI_Init(hspi);

  __HAL_WIFI_SPI_ENABLE_DMAREQ(hspi, SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
  
  __HAL_SPI_ENABLE(hspi);
  
  return;
}

void SPI_Gpio_Init(SPI_HandleTypeDef* hspi)
{ 
  GPIO_InitTypeDef GPIO_InitStruct;
  if(hspi->Instance==WIFI_SPI_INSTANCE)
  {
    /* Enable peripherals clock */
    
    /* Enable GPIO Ports Clock */ 
    //WIFI_SPI_RESET_CLK_ENABLE();
    WIFI_SPI_SCLK_CLK_ENABLE();
    WIFI_SPI_MISO_CLK_ENABLE();
    WIFI_SPI_MOSI_CLK_ENABLE();
    WIFI_SPI_CS_CLK_ENABLE();
    WIFI_SPI_IRQ_CLK_ENABLE();
    
    /* Enable SPI clock */
    WIFI_SPI_CLK_ENABLE();    
    
    /* RESET */
//    GPIO_InitStruct.Pin = WIFI_SPI_RESET_PIN;
//    GPIO_InitStruct.Mode = WIFI_SPI_RESET_MODE;
//    GPIO_InitStruct.Pull = WIFI_SPI_RESET_PULL;
//    GPIO_InitStruct.Speed = WIFI_SPI_RESET_SPEED;
//    GPIO_InitStruct.Alternate = WIFI_SPI_RESET_ALTERNATE;
//    HAL_GPIO_Init(WIFI_SPI_SCLK_PORT, &GPIO_InitStruct); 
    
    /* SCLK */
    GPIO_InitStruct.Pin = WIFI_SPI_SCLK_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_SCLK_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_SCLK_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_SCLK_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_SCLK_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_SCLK_PORT, &GPIO_InitStruct); 
    
    /* MISO */
    GPIO_InitStruct.Pin = WIFI_SPI_MISO_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_MISO_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_MISO_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_MISO_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_MISO_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_MISO_PORT, &GPIO_InitStruct);
    
    /* MOSI */
    GPIO_InitStruct.Pin = WIFI_SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_MOSI_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_MOSI_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_MOSI_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_MOSI_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_MOSI_PORT, &GPIO_InitStruct);
    
    /* NSS/CSN/CS */
    GPIO_InitStruct.Pin = WIFI_SPI_CS_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_CS_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_CS_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_CS_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_CS_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_SET);
    
    /* IRQ -- INPUT */
    GPIO_InitStruct.Pin = WIFI_SPI_IRQ_PIN;
    GPIO_InitStruct.Mode = WIFI_SPI_IRQ_MODE;
    GPIO_InitStruct.Pull = WIFI_SPI_IRQ_PULL;
    GPIO_InitStruct.Speed = WIFI_SPI_IRQ_SPEED;
    GPIO_InitStruct.Alternate = WIFI_SPI_IRQ_ALTERNATE;
    HAL_GPIO_Init(WIFI_SPI_IRQ_PORT, &GPIO_InitStruct);
    
    /* Configure the NVIC for EXTI */  
    HAL_NVIC_SetPriority(WIFI_SPI_EXTI_IRQn, 2, 0);    
    //HAL_NVIC_EnableIRQ(WIFI_SPI_EXTI_IRQn);
  }
}

/**
 * @brief  Set in Output mode the IRQ.
 * @param  None
 * @retval None
 */
void set_irq_as_output(void)
{
  HAL_GPIO_WritePin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN, GPIO_PIN_SET);
  //HAL_LPPUART_GPIO_Set_Mode(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN_POSITION, GPIO_MODE_OUTPUT_PP);
  __HAL_GPIO_EXTI_CLEAR_IT(WIFI_SPI_IRQ_PIN);
}

/**
 * @brief  Set the IRQ in input mode.
 * @param  None
 * @retval None
 */
void set_irq_as_input(void)
{
  HAL_GPIO_WritePin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN, GPIO_PIN_RESET); // WARNING: it may conflict with BlueNRG driving High
  //HAL_LPPUART_GPIO_Set_Mode(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN_POSITION, GPIO_MODE_INPUT);
}

/**
 * @brief  Enable SPI IRQ.
 * @param  None
 * @retval None
 */
void Enable_SPI_Receiving_Path(void)
{  
  __HAL_GPIO_EXTI_CLEAR_IT(WIFI_SPI_EXTI_PIN);
  HAL_NVIC_ClearPendingIRQ(WIFI_SPI_EXTI_IRQn);
  HAL_NVIC_EnableIRQ(WIFI_SPI_EXTI_IRQn);
  
//  if (HAL_GPIO_ReadPin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN) == GPIO_PIN_RESET)
//  {
//    __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
//  }
}

/**
 * @brief  Disable SPI IRQ.
 * @param  None
 * @retval None
 */
void Disable_SPI_Receiving_Path(void)
{  
  HAL_NVIC_DisableIRQ(WIFI_SPI_EXTI_IRQn);
}

/**
 * @brief  Enable SPI CS.
 * @param  None
 * @retval None
 */
void Enable_SPI_CS(void)
{
  /* CS reset */
  HAL_GPIO_WritePin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_RESET);
#if defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
  while( HAL_GPIO_ReadPin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN) != GPIO_PIN_RESET );
#endif
}

/**
 * @brief  Disable SPI CS.
 * @param  None
 * @retval None
 */
void Disable_SPI_CS(void)
{
  while (__HAL_SPI_GET_FLAG(&SpiHandle,SPI_FLAG_BSY) == SET);
  
  /* CS set */
  HAL_GPIO_WritePin(WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_SET);
}

#define CS_PULSE_700NS_NBR_CYCLES_REQ  352
#define CS_PULSE_LENGTH (CS_PULSE_700NS_NBR_CYCLES_REQ/4)
#define DUMMY_RAM_ADDRESS_TO_READ (0x20000000)

/**
 * @brief  Disable and Enable SPI CS.
 * @param  None
 * @retval None
 */
void DisableEnable_SPI_CS(void)
{
  volatile uint8_t localloop;
  
  Disable_SPI_CS(); /**< CS Set */
  
  /**
   *  The CS shall be kept high for at least 625ns
   */
  for (localloop = 0 ; localloop < CS_PULSE_LENGTH ; localloop++)
  {
    //*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ;
  }
  
  Enable_SPI_CS(); /**< CS Reset */

  return;
}

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

/******************* (C) COPYRIGHT 2015 STMicroelectronics *****END OF FILE****/

