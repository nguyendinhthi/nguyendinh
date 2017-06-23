  /**
  ******************************************************************************
  * @file    stm32_spwf_wifi.h
  * @author  Central LAB
  * @version V2.0.1
  * @date    17-May-2016
  * @brief   Header file for HAL related functionality of X-CUBE-WIFI1
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WIFI_MODULE_CONF_H
#define __WIFI_MODULE_CONF_H

/* Includes ------------------------------------------------------------------*/
#include "wifi_conf.h"
    
#ifdef USE_STM32F4XX_NUCLEO  
#include "stm32f4xx_hal.h"
#include "stm32f4xx_nucleo.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rcc_ex.h"
#endif

#ifdef USE_STM32L4XX_NUCLEO
#include "stm32l4xx_hal.h"
#include "stm32l4xx_nucleo.h"
#include "stm32l4xx_hal_rcc.h"
#include "stm32l4xx_hal_rcc_ex.h"
#endif

#ifdef USE_STM32L0XX_NUCLEO
#include "stm32l0xx_hal.h"
#include "stm32l0xx_nucleo.h"
#include "stm32l0xx_hal_rcc.h"
#include "stm32l0xx_hal_rcc_ex.h"
#endif

#ifdef USE_STM32F1xx_NUCLEO
#include "stm32f1xx_hal.h"
#include "stm32f1xx_nucleo.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#define WiFi_ENABLE
#define FW_UPDATE_PACKET_SIZE   252

/********** TimeOUT *******************/
#define WIFI_TIME_OUT           40000   //4 sec
#define WIFI_HTTP_REQUEST_INTERVAL      60 //6sec

#if defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)
void WiFi_SPI_Init(SPI_HandleTypeDef * hspi);
void set_irq_as_output(void);
void set_irq_as_input(void);
void Enable_SPI_Receiving_Path(void);
void Disable_SPI_Receiving_Path(void);
void Enable_SPI_CS(void);
void Disable_SPI_CS(void);
void DisableEnable_SPI_CS(void);
#endif

/********** SPI Pins and Peripherals *******************/
#if defined (USE_STM32L4XX_NUCLEO) 
#define __HAL_WIFI_SPI_ENABLE_DMAREQ(__HANDLE__, __DMAREQ__)   ((__HANDLE__)->Instance->CR2 |= (__DMAREQ__)) 
#define __HAL_WIFI_SPI_DISABLE_DMAREQ(__HANDLE__, __DMAREQ__)   ((__HANDLE__)->Instance->CR2 &= (~(__DMAREQ__)))     
#define __HAL_WIFI_SPI_GET_RX_DATA_REGISTER_ADDRESS(__HANDLE__)   ((uint32_t)&(__HANDLE__)->Instance->DR) 
#define __HAL_WIFI_SPI_GET_TX_DATA_REGISTER_ADDRESS(__HANDLE__)   ((uint32_t)&(__HANDLE__)->Instance->DR)
#define __HAL_WIFI_DMA_SET_PERIPHERAL_ADDRESS(__HANDLE__, __ADDRESS__)   ((__HANDLE__)->Instance->CPAR = (__ADDRESS__)) 
#define __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(__HANDLE__, __ADDRESS__)   ((__HANDLE__)->Instance->CMAR = (__ADDRESS__)) 
#define __HAL_WIFI_DMA_SET_COUNTER(__HANDLE__, __COUNTER__)   ((__HANDLE__)->Instance->CNDTR = (__COUNTER__)) 
#define __HAL_WIFI_DMA_SET_MINC(__HANDLE__)   ((__HANDLE__)->Instance->CCR |= (DMA_CCR_MINC))
#define __HAL_WIFI_DMA_CLEAR_MINC(__HANDLE__)   ((__HANDLE__)->Instance->CCR &= ~(DMA_CCR_MINC))
#endif
#if defined (USE_STM32F4XX_NUCLEO)
#define __HAL_WIFI_SPI_ENABLE_DMAREQ(__HANDLE__, __DMAREQ__)   ((__HANDLE__)->Instance->CR2 |= (__DMAREQ__))
#define __HAL_WIFI_SPI_DISABLE_DMAREQ(__HANDLE__, __DMAREQ__)   ((__HANDLE__)->Instance->CR2 &= (~(__DMAREQ__)))
#define __HAL_WIFI_SPI_GET_RX_DATA_REGISTER_ADDRESS(__HANDLE__)   ((uint32_t)&(__HANDLE__)->Instance->DR)
#define __HAL_WIFI_SPI_GET_TX_DATA_REGISTER_ADDRESS(__HANDLE__)   ((uint32_t)&(__HANDLE__)->Instance->DR)
#define __HAL_WIFI_DMA_SET_PERIPHERAL_ADDRESS(__HANDLE__, __ADDRESS__)   ((__HANDLE__)->Instance->PAR = (__ADDRESS__))
#define __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(__HANDLE__, __ADDRESS__)   ((__HANDLE__)->Instance->M0AR = (__ADDRESS__))
#define __HAL_WIFI_DMA_SET_MEMORY1_ADDRESS(__HANDLE__, __ADDRESS__)   ((__HANDLE__)->Instance->M1AR = (__ADDRESS__))
#define __HAL_WIFI_DMA_SET_COUNTER(__HANDLE__, __COUNTER__)   ((__HANDLE__)->Instance->NDTR = (__COUNTER__))
#define __HAL_WIFI_DMA_SET_MINC(__HANDLE__)   ((__HANDLE__)->Instance->CR |= (DMA_SxCR_MINC))
#define __HAL_WIFI_DMA_CLEAR_MINC(__HANDLE__)   ((__HANDLE__)->Instance->CR &= ~(DMA_SxCR_MINC))
#endif

/*SPI pins and needles*/
// SPI Instance
#define WIFI_SPI_INSTANCE           SPI1
#define WIFI_SPI_CLK_ENABLE()       __SPI1_CLK_ENABLE()

// SPI Configuration
#define WIFI_SPI_MODE               SPI_MODE_MASTER
#define WIFI_SPI_DIRECTION          SPI_DIRECTION_2LINES
#define WIFI_SPI_DATASIZE           SPI_DATASIZE_8BIT
#define WIFI_SPI_CLKPOLARITY        SPI_POLARITY_LOW
#define WIFI_SPI_CLKPHASE           SPI_PHASE_1EDGE
#define WIFI_SPI_NSS                SPI_NSS_SOFT
#define WIFI_SPI_FIRSTBIT           SPI_FIRSTBIT_MSB
#define WIFI_SPI_TIMODE             SPI_TIMODE_DISABLED
#define WIFI_SPI_CRCPOLYNOMIAL      7
#define WIFI_SPI_BAUDRATEPRESCALER  SPI_BAUDRATEPRESCALER_16
#define WIFI_SPI_CRCCALCULATION     SPI_CRCCALCULATION_DISABLED

// SPI Reset Pin: PA.8 or same as PC12?
#define WIFI_SPI_RESET_PIN          GPIO_PIN_8
#define WIFI_SPI_RESET_MODE         GPIO_MODE_OUTPUT_PP
#define WIFI_SPI_RESET_PULL         GPIO_PULLUP
#define WIFI_SPI_RESET_SPEED        GPIO_SPEED_LOW
#define WIFI_SPI_RESET_ALTERNATE    0
#define WIFI_SPI_RESET_PORT         GPIOA
#define WIFI_SPI_RESET_CLK_ENABLE() __GPIOA_CLK_ENABLE()

// SCLK: PA.5
#define WIFI_SPI_SCLK_PIN           GPIO_PIN_5
#define WIFI_SPI_SCLK_MODE          GPIO_MODE_AF_PP
#define WIFI_SPI_SCLK_PULL          GPIO_PULLDOWN
#define WIFI_SPI_SCLK_SPEED         GPIO_SPEED_FAST
#define WIFI_SPI_SCLK_ALTERNATE     GPIO_AF5_SPI1
#define WIFI_SPI_SCLK_PORT          GPIOA
#define WIFI_SPI_SCLK_CLK_ENABLE()  __GPIOA_CLK_ENABLE()

// MISO (Master Input Slave Output): PA.6
#define WIFI_SPI_MISO_PIN           GPIO_PIN_6
#define WIFI_SPI_MISO_MODE          GPIO_MODE_AF_PP
#define WIFI_SPI_MISO_PULL          GPIO_NOPULL
#define WIFI_SPI_MISO_SPEED         GPIO_SPEED_FAST
#define WIFI_SPI_MISO_ALTERNATE     GPIO_AF5_SPI1
#define WIFI_SPI_MISO_PORT          GPIOA
#define WIFI_SPI_MISO_CLK_ENABLE()  __GPIOA_CLK_ENABLE()

// MOSI (Master Output Slave Input): PA.7
#define WIFI_SPI_MOSI_PIN           GPIO_PIN_7
#define WIFI_SPI_MOSI_MODE          GPIO_MODE_AF_PP
#define WIFI_SPI_MOSI_PULL          GPIO_NOPULL
#define WIFI_SPI_MOSI_SPEED         GPIO_SPEED_FAST
#define WIFI_SPI_MOSI_ALTERNATE     GPIO_AF5_SPI1
#define WIFI_SPI_MOSI_PORT          GPIOA
#define WIFI_SPI_MOSI_CLK_ENABLE()  __GPIOA_CLK_ENABLE()

// NSS/CSN/CS: PC.1 (new board)
#define WIFI_SPI_CS_PIN             GPIO_PIN_1
#define WIFI_SPI_CS_MODE            GPIO_MODE_OUTPUT_PP
#define WIFI_SPI_CS_PULL            GPIO_PULLUP
#define WIFI_SPI_CS_SPEED           GPIO_SPEED_FAST
#define WIFI_SPI_CS_ALTERNATE       0
#define WIFI_SPI_CS_PORT            GPIOC
#define WIFI_SPI_CS_CLK_ENABLE()    __GPIOC_CLK_ENABLE()

// IRQ: PC.7 (new board)
#define WIFI_SPI_IRQ_PIN            GPIO_PIN_7
#define WIFI_SPI_IRQ_MODE           GPIO_MODE_IT_FALLING
#define WIFI_SPI_IRQ_PULL           GPIO_PULLUP
#define WIFI_SPI_IRQ_SPEED          GPIO_SPEED_FAST
#define WIFI_SPI_IRQ_ALTERNATE      0
#define WIFI_SPI_IRQ_PORT           GPIOC
#define WIFI_SPI_IRQ_CLK_ENABLE()   __GPIOC_CLK_ENABLE()

// EXTI External Interrupt for SPI
#define WIFI_SPI_EXTI_IRQn          EXTI9_5_IRQn//EXTI4_IRQn //EXTI9_5_IRQn //EXTI0_IRQn
#define WIFI_SPI_EXTI_IRQHandler    EXTI9_5_IRQHandler//EXTI4_IRQHandler //EXTI9_5_IRQHandler //EXTI0_IRQHandler
#define WIFI_SPI_EXTI_PIN           WIFI_SPI_IRQ_PIN
#define WIFI_SPI_EXTI_PORT          WIFI_SPI_IRQ_PORT
#define RTC_WAKEUP_IRQHandler       RTC_WKUP_IRQHandler

#define WIFI_SPI_FORCE_RESET()          __SPI1_FORCE_RESET()
#define WIFI_SPI_RELEASE_RESET()        __SPI1_RELEASE_RESET()

#if defined (USE_STM32L4XX_NUCLEO) 
#define WIFI_DMA_CLK_ENABLE()           __DMA1_CLK_ENABLE()

/* Definition for SPIx's DMA */
#define WIFI_SPI_TX_DMA_CHANNEL         DMA1_Channel3
#define WIFI_SPI_TX_DMA_REQUEST         DMA_REQUEST_1
#define WIFI_SPI_TX_DMA_TC_FLAG         DMA_FLAG_TC3
   
#define WIFI_SPI_RX_DMA_CHANNEL         DMA1_Channel2
#define WIFI_SPI_RX_DMA_REQUEST         DMA_REQUEST_1
#define WIFI_SPI_RX_DMA_TC_FLAG         DMA_FLAG_TC2
/* Definition for SPIx's NVIC */
#define WIFI_SPI_DMA_TX_IRQn            DMA1_Channel3_IRQn
#define WIFI_SPI_DMA_RX_IRQn            DMA1_Channel2_IRQn
#endif

#if defined (USE_STM32F4XX_NUCLEO) 
#define WIFI_DMA_CLK_ENABLE()           __DMA2_CLK_ENABLE()
/* Definition for SPIx's DMA */
#define WIFI_SPI_TX_DMA_CHANNEL         DMA_CHANNEL_3
#define WIFI_SPI_TX_DMA_STREAM          DMA2_Stream3
#define WIFI_SPI_TX_DMA_TC_FLAG         DMA_FLAG_TCIF3_7
   
#define WIFI_SPI_RX_DMA_CHANNEL         DMA_CHANNEL_3
#define WIFI_SPI_RX_DMA_STREAM          DMA2_Stream0
#define WIFI_SPI_RX_DMA_TC_FLAG         DMA_FLAG_TCIF0_4
/* Definition for SPIx's NVIC */
#define WIFI_SPI_DMA_TX_IRQn            DMA2_Stream3_IRQn
#define WIFI_SPI_DMA_RX_IRQn            DMA2_Stream0_IRQn
#endif


#define SPIx_IRQn                       SPI1_IRQn
#define SPIx_IRQHandler                 SPI1_IRQHandler

/********** Wi-Fi *******************/
#ifdef USART_PRINT_MSG

#ifdef USE_STM32L0XX_NUCLEO  

#define WIFI_UART_MSG                           USART2
#define USARTx_PRINT_CLK_ENABLE()              __USART2_CLK_ENABLE()
#define USARTx_PRINT_RX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()
#define USARTx_PRINT_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define USARTx_PRINT_FORCE_RESET()             __USART2_FORCE_RESET()
#define USARTx_PRINT_RELEASE_RESET()           __USART2_RELEASE_RESET()

#define PRINTMSG_USARTx_TX_AF                       GPIO_AF4_USART2
#define PRINTMSG_USARTx_RX_AF                       GPIO_AF4_USART2

#endif //USE_STM32L0XX_NUCLEO

#if defined (USE_STM32F1xx_NUCLEO) || defined (USE_STM32F4XX_NUCLEO) || defined (USE_STM32L4XX_NUCLEO)

#define WIFI_UART_MSG                           USART2
#define USARTx_PRINT_CLK_ENABLE()              __HAL_RCC_USART2_CLK_ENABLE()
#define USARTx_PRINT_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_PRINT_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_PRINT_FORCE_RESET()             __HAL_RCC_USART2_FORCE_RESET()
#define USARTx_PRINT_RELEASE_RESET()           __HAL_RCC_USART2_RELEASE_RESET()

#define PRINTMSG_USARTx_TX_AF                       GPIO_AF7_USART2
#define PRINTMSG_USARTx_RX_AF                       GPIO_AF7_USART2

#endif //(USE_STM32F1xx_NUCLEO) || (USE_STM32F4XX_NUCLEO)

#define WiFi_USART_PRINT_TX_PIN                    GPIO_PIN_2
#define WiFi_USART_PRINT_TX_GPIO_PORT              GPIOA
#define WiFi_USART_PRINT_RX_PIN                    GPIO_PIN_3
#define WiFi_USART_PRINT_RX_GPIO_PORT              GPIOA


/* Definition for USARTx's NVIC */
#define USARTx_PRINT_IRQn                      USART2_IRQn
#define USARTx_PRINT_IRQHandler                USART2_IRQHandler

#endif //USART_PRINT_MSG


/* User can use this section to tailor USARTx/UARTx instance used and associated 
resources */
/* Definition for USARTx clock resources */
#ifdef USE_STM32L0XX_NUCLEO
#if 0
#define WB_WIFI_UART                     LPUART1 
#define USARTx_CLK_ENABLE()              __LPUART1_CLK_ENABLE() 
#define USARTx_RX_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __LPUART1_FORCE_RESET() 
#define USARTx_RELEASE_RESET()           __LPUART1_RELEASE_RESET() 
#endif
#if 1 /*For exp hardware revision*/
#define WB_WIFI_UART                     USART1 
#define USARTx_CLK_ENABLE()              __USART1_CLK_ENABLE() 
#define USARTx_RX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __USART1_FORCE_RESET() 
#define USARTx_RELEASE_RESET()           __USART1_RELEASE_RESET() 
#endif
#endif //USE_STM32L0XX_NUCLEO  

#ifdef USE_STM32F1xx_NUCLEO

#define WB_WIFI_UART                     USART1
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()

#endif //USE_STM32F1xx_NUCLEO

#ifdef USE_STM32F4XX_NUCLEO

#define WB_WIFI_UART                     USART1
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE() //__HAL_RCC_GPIOA_CLK_ENABLE() //Modified for WiFi Bridge
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()

#endif //USE_STM32F4XX_NUCLEO  

#ifdef USE_STM32L4XX_NUCLEO
#define WB_WIFI_UART                     USART1
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()

#endif //USE_STM32L4XX_NUCLEO 

/* Definition for USARTx Pins */
#define WiFi_USART_TX_PIN                    GPIO_PIN_9
#define WiFi_USART_TX_GPIO_PORT              GPIOA
#define WiFi_USART_RX_PIN                    GPIO_PIN_7 //GPIO_PIN_10 //Modified for WiFi Bridge
#define WiFi_USART_RX_GPIO_PORT              GPIOB //GPIOA //Modified for WiFi Bridge

#define WiFi_USART_RTS_PIN                    GPIO_PIN_12
#define WiFi_USART_RTS_GPIO_PORT              GPIOA
#define WiFi_USART_CTS_PIN                    GPIO_PIN_11
#define WiFi_USART_CTS_GPIO_PORT              GPIOA

#ifdef USE_STM32L0XX_NUCLEO
#if 0
#define WiFi_USARTx_TX_AF                    GPIO_AF0_LPUART1
#define WiFi_USARTx_RX_AF                    GPIO_AF0_LPUART1
#endif
#if 1 /*For exp hardware revision*/
#define WiFi_USARTx_TX_AF                    GPIO_AF4_USART1
#define WiFi_USARTx_RX_AF                    GPIO_AF4_USART1
#endif

#endif    

#if defined (USE_STM32L4XX_NUCLEO) || defined (USE_STM32F4XX_NUCLEO)
#define WiFi_USARTx_TX_AF                    GPIO_AF7_USART1
#define WiFi_USARTx_RX_AF                    GPIO_AF7_USART1
#endif    

#ifdef SPWF04  //PA8 for SPWF04
#if 1
#define  WiFi_RESET_GPIO_PIN              GPIO_PIN_8 /*PC12/8*/
#else
#define  WiFi_RESET_GPIO_PIN              GPIO_PIN_0
#endif

#define  WiFi_RESET_GPIO_PORT              GPIOA /*PC12 or PA8 for the new X-nucleo-idw04m1 boards?*/

#else          // PC12 for SPWF01
#if 1
#define  WiFi_RESET_GPIO_PIN              GPIO_PIN_12
#else
#define  WiFi_RESET_GPIO_PIN              GPIO_PIN_0
#endif

#define  WiFi_RESET_GPIO_PORT              GPIOC /*PC12*/

#endif         //end 

#define  WiFi_WAKEUP_GPIO_PIN              GPIO_PIN_8
#define  WiFi_WAKEUP_GPIO_PORT             GPIOC /*PC8*/

#if defined (USE_STM32L4XX_NUCLEO) || defined (USE_STM32F4XX_NUCLEO)
#define RESET_WAKEUP_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#endif

#ifdef USE_STM32F1xx_NUCLEO
#define RESET_WAKEUP_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#endif

#ifdef USE_STM32L0XX_NUCLEO
#define RESET_WAKEUP_GPIO_CLK_ENABLE()      __GPIOC_CLK_ENABLE()
#endif

/* Definition for USARTx's NVIC */
#ifdef USE_STM32L0XX_NUCLEO
#if 0
#define USARTx_IRQn                     LPUART1_IRQn 
#define USARTx_IRQHandler               LPUART1_IRQHandler
#endif
#if 1 /*For exp hardware revision*/
#define USARTx_IRQn                     USART1_IRQn 
#define USARTx_IRQHandler               USART1_IRQHandler
#endif
#define USARTx_EXTI_IRQn                EXTI4_15_IRQn
#define USARTx_EXTI_IRQHandler          EXTI4_15_IRQHandler

#endif //USE_STM32L0XX_NUCLEO
#ifdef USE_STM32F1xx_NUCLEO
#define USARTx_IRQn                     USART1_IRQn
#define USARTx_IRQHandler               USART1_IRQHandler

//#define USARTx_EXTI_IRQn                EXTI15_10_IRQn
//#define USARTx_EXTI_IRQHandler          EXTI15_10_IRQHandler

#endif //USE_STM32L0XX_NUCLEO
#if defined (USE_STM32L4XX_NUCLEO) || defined (USE_STM32F4XX_NUCLEO)
#define USARTx_IRQn                     USART1_IRQn
#define USARTx_IRQHandler               USART1_IRQHandler

#define USARTx_EXTI_IRQn                EXTI15_10_IRQn
#define USARTx_EXTI_IRQHandler          EXTI15_10_IRQHandler

#endif //USE_STM32F4XX_NUCLEO

#if defined (USE_STM32F1xx_NUCLEO)
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()
#define TIMp                           TIM2
#define TIMp_CLK_ENABLE()              __HAL_RCC_TIM2_CLK_ENABLE()

/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM3_IRQn
#define TIMx_IRQHandler                TIM3_IRQHandler
#define TIMp_IRQn                      TIM2_IRQn
#define TIMp_IRQHandler                TIM2_IRQHandler
#endif

#if defined (USE_STM32L4XX_NUCLEO) || defined (USE_STM32F4XX_NUCLEO)
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()
#define TIMp                           TIM2
#define TIMp_CLK_ENABLE()              __HAL_RCC_TIM2_CLK_ENABLE()

/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM3_IRQn
#define TIMx_IRQHandler                TIM3_IRQHandler
#define TIMp_IRQn                      TIM2_IRQn
#define TIMp_IRQHandler                TIM2_IRQHandler
#endif

#ifdef USE_STM32L0XX_NUCLEO
#define TIMx                           TIM2
#define TIMx_CLK_ENABLE()              __TIM2_CLK_ENABLE()
#define TIMp                           TIM21
#define TIMp_CLK_ENABLE()              __TIM21_CLK_ENABLE()

/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM2_IRQn
#define TIMx_IRQHandler                TIM2_IRQHandler
#define TIMp_IRQn                      TIM21_IRQn
#define TIMp_IRQHandler                TIM21_IRQHandler
#endif

/* Exported functions ------------------------------------------------------- */
void Push_Timer_Config(void);

/* Size of Trasmission buffer */
#define TXBUFFERSIZE                    (COUNTOF(aTxBuffer) - 1)
/* Size of Reception buffer */
#define RXBUFFERSIZE                    TXBUFFERSIZE

#define  WiFi_USART_BAUD_RATE                115200

#endif //__WIFI_MODULE_CONF_H
