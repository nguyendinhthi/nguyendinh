  /**
  ******************************************************************************
  * @file    main.c
  * @author  Central LAB
  * @version V2.0.0
  * @date    18-April-2017
  * @brief   Main program body
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

/**
 * @mainpage FP-NET-6LPWIFI1 6LoWPAN and Wi-Fi software
 *
 * @image html st_logo.png
 *
 * <b>Introduction</b>
 *
 * This firmware package includes Components Device Drivers, Board Support Package
 * and example application for the following STMicroelectronics elements:
 * - X-NUCLEO-IDW01M1 Wi-Fi expansion board
 * - X-NUCLEO-IDS01A4/X-NUCLEO-IDS01A5 SPIRIT1 Sub-1GHz radio expansion boards
 * - NUCLEO-F401RE Nucleo board
 *
 * @attention
 * <b>Important Hardware Additional Information</b><br>
 * \image html X-NUCLEO-IDW01M1-mods.jpg "Figure 1: X-NUCLEO-IDW01M1 expansion board"
 * <br>Before connecting X-NUCLEO-IDW01M1 with X-NUCLEO-IDS01A4 (or X-NUCLEO-IDS01A5) expansion board through the Arduino UNO R3 extension connector,
 * move this 0-ohm resistor from R4 to R34, as the above figure 1 shows.<br>
 *
 * <b>Example Application: Wi-Fi Bridge</b>
 *
 * This application allows to create a bridge between a 6LoWPAN network and the Internet using Contiki OS and the Wi-Fi module (SPWF01SA).
 * The application supports the UDP protocol and takes advantage of the NAT64 technology in order to translate IPv6 packets into IPv4 packets.
 * The function of the Wi-Fi bridge is to seamlessly forward every packet that comes from the 6LoWPAN network to the IPv4 network.
 * The nodes connected to the Wi-Fi bridge are not aware of this implementation since they make a request directly to a specific server, and not to the Wi-Fi bridge which transparently connects the nodes to the requested server.
 * The application can support up to 8 nodes simultaneously (this is due to a limitation of the Wi-Fi module).
 *
 * <b>Example Application: LWM2M to IBM</b>
 *
 * This application allows to create a bridge between a WSM of nodes running the OMA LWM2M protocol and the cloud services offered by IMB, using Contiki OS and the Wi-Fi module (SPWF01SA).
 * This sample application implements an OMA LWM2M server that manages the OMA LWM2M clients inside the WSN network. Differently from the Wi-Fi bridge sample
 * application scenario, the LWM2M protocol is used only inside the 6LoWPAN network. Through the Wi-Fi module, it then connects to IBM Watson cloud, using the
 * MQTT protocol. This application is configured by default to send data via Wi-Fi to the IBM Watson IoT cloud in quickstart mode for data visualization only.
 * However, it can be quickly modified to use the device in register mode. The latter requires an account on the IBM Watson IoT cloud (if the user wants to
 * command the device; e.g., turn LED LD2 on or off). Further details are provided in the FP-CLD-WATSON1 function pack documentation available at www.st.com.
 *
 */


/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include "main.h"
#include "stm32f4xx_nucleo.h"
#include "radio_shield_config.h"
#include "wifi_interface.h"
#include "wifi_support.h"
#include "wifi_globals.h"
#include "my_wifi_types.h"
#include "cube_hal.h"
#include "process.h"
#include "contiki-spirit1-main.h"

/** @defgroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @defgroup WIFI_Bridge
  * @{
  */


/* Private typedef -----------------------------------------------------------*/

/* Configuration  ------------------------------------------------------------*/
#define UART_BAUD_RATE          115200

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FlagStatus TamperStatus = RESET;
volatile uint8_t scheduler_started=0;
int dec_precision = 2;
volatile float UVI_Value;

/* UART ---------------------------------------------------------------------*/
UART_HandleTypeDef UART_MsgHandle;
__IO wifi_state_t wifi_state,wifi_next_state;
wifi_config config;
wifi_scan net_scan[WIFI_SCAN_BUFFER_LIST];

/* Default parameters */
#if 0
  /* Default configuration SSID/PWD */
  char * ssid = "STM";
  char * seckey = "STMdemoPWD";
#else
  char * ssid = NULL;
  char * seckey = NULL;
#endif
  extern WiFi_Priv_Mode mode;
  extern uWifiTokenInfo UnionWifiToken;

/* Private function prototypes -----------------------------------------------*/
void UART_Msg_Gpio_Init(void);
void USART_PRINT_MSG_Configuration(UART_HandleTypeDef *UART_MsgHandle, uint32_t baud_rate);

/* Private functions ---------------------------------------------------------*/

 /**
  * @brief  Main program
  * 		Initializes HAL structures, the SPWF01SA (wifi) and calls the contiki main
  * @param  None
  * @retval int: 1 success, 0 failure
  */
int main(void)
{
  uint32_t board_uid[3] = {0, 0, 0};
  uint8_t ready = 0;
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  int i=0;
  wifi_bool SSID_found = WIFI_FALSE;

  HAL_Init();
  
  /* Configure the system clock to 64 MHz */
  SystemClock_Config();
  
  HAL_EnableDBGStopMode();

  MX_GPIO_Init();

  /* configure the timers  */
  Timer_Config( );
  BSP_LED_Init(LED2);
  RadioShieldLedInit(RADIO_SHIELD_LED);
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);

  UART_Configuration(UART_BAUD_RATE);
#ifdef USART_PRINT_MSG
  UART_Msg_Gpio_Init();
  USART_PRINT_MSG_Configuration(&UART_MsgHandle,UART_BAUD_RATE);
#endif

  Set_UartMsgHandle(&UART_MsgHandle);

  printf("\r\n\n/********************************************************\n");
  printf("\r *                                                      *\n");
  printf("\r * FP-NET-6LPWIFI1 Function Pack v2.0.0                 *\n");
  printf("\r * X-NUCLEO-IDW01M1 Wi-Fi Configuration.                *\n");
  printf("\r * WiFi Bridge Application                              *\n");
  printf("\r *                                                      *\n");
  printf("\r *******************************************************/\n");

  /*Compiler, HAL and firmware info:*/
printf("\t(HAL %ld.%ld.%ld_%ld)\r\n"
    "\tCompiled %s %s"
#if defined (__IAR_SYSTEMS_ICC__)
    " (IAR)\r\n\n",
#elif defined (__CC_ARM)
    " (KEIL)\r\n\n",
#elif defined (__GNUC__)
    " (openstm32)\r\n\n",
#endif
       HAL_GetHalVersion() >>24,
      (HAL_GetHalVersion() >>16)&0xFF,
      (HAL_GetHalVersion() >> 8)&0xFF,
       HAL_GetHalVersion()      &0xFF,
     __DATE__,__TIME__);

  HAL_GetUID(board_uid);
  printf("\r\nBoard UID: %08lx%08lx%08lx\r\n", board_uid[0], board_uid[1], board_uid[2]);

  config.power=wifi_active;
  config.power_level=high;
  config.dhcp=on;//use DHCP IP address
  config.ht_mode = WIFI_FALSE;
  config.http_timeout = 3; //Longer HTTP Timeout, no expected effect for this FP

  wifi_state = wifi_state_idle;
  
  /* Init the wi-fi module */
  printf("\r\n\nInitializing the wifi module...");
  status = wifi_init(&config);
  if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("Error in Config");
    return 0;
  }

  /* After WiFi initialization (mandatory) but before looking for SSID and Password
   * (we give more time to the user to see the waring, in case), the WiFi firmware
   * version is checked. */
  check_wifi_firmware_version();

  if ((ssid!=NULL) && (seckey!=NULL)) {
    printf("\r\nUsing SSID and PWD written in the code. ");

    memcpy(WIFITOKEN.NetworkSSID, ssid, strlen(ssid));
    memcpy(WIFITOKEN.NetworkKey, seckey, strlen(seckey));
    memcpy(WIFITOKEN.AuthenticationType, "WPA2", strlen("WPA2"));
  } else {
    printf("\r\nStarting configure procedure for SSID and PWD....  ");
    HAL_Delay(3000);

    if (ConfigAPSettings() < 0) {
      printf("\n\rFailed to set AP settings.");
      return 0;
    } else {
      printf("\n\rAP settings set.");
    }
  }

  while (!ready)
  {
    switch (wifi_state) 
    {
      case wifi_state_reset:
      break;
    
      case wifi_state_ready:
    	 // printf("\r\n >>running WiFi Scan...\r\n");
		  status = wifi_network_scan(net_scan, WIFI_SCAN_BUFFER_LIST);
		  if(status==WiFi_MODULE_SUCCESS)
		  {
			for (i=0; i<WIFI_SCAN_BUFFER_LIST; i++)
			{
				  //printf(net_scan[i].ssid);
				  //printf("\r\n");
		          if(( (char *) strstr((const char *)net_scan[i].ssid,(const char *)WIFITOKEN.NetworkSSID)) !=NULL)
				  {
					  printf("\r\n >>network present...connecting to AP...\r\n");
                      SSID_found = WIFI_TRUE;
					  if(!((status =  wifi_connect(WIFITOKEN.NetworkSSID, WIFITOKEN.NetworkKey, mode))== WiFi_MODULE_SUCCESS))
					  {
						printf("\r\n >>Wrong SSID-KEY Detected.Please try again...\r\n");
						//HAL_Delay(500);//Let module go to sleep
					  }
					  break;
				  }
			  }
              if(!SSID_found)
              {
                  printf("\r\nGiven SSID not found!\r\n");
              }
			  memset(net_scan, 0x00, sizeof(net_scan));
		  } else {
			printf("\r\n >>Scan failure...\r\n");
		  }
		  wifi_state = wifi_state_idle;
		  break;

      case wifi_state_connected:
        printf("\r\n >>connected...\r\n");
        ready = 1;
        break;
      
      case wifi_state_disconnected:
        wifi_state = wifi_state_reset;
        break;

    case wifi_state_idle:        
        printf(".");
        fflush(stdout);
        HAL_Delay(500);
        break;
        
    default:
      break;
    }    
  }
  
  printf("\n\nSuccessfully configured WiFi module and connected to the selected AP... Starting Contiki and SPIRIT1 configuration\n\n");

  /* Initialize RTC */
  RTC_Config();
  RTC_TimeStampConfig();

  Stack_6LoWPAN_Init();

  while(1) {
    int r = 0;
    do {
      r = process_run();
    } while(r > 0);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/******** Wi-Fi Indication User Callback *********/
void ind_wifi_on()
{
  printf("\r\nWiFi Initialized and Ready..\r\n");
  wifi_state = wifi_state_ready;
}
/*---------------------------------------------------------------------------*/
void ind_wifi_connected()
{
  printf("\r\nWiFi connected...\r\n");
  wifi_state = wifi_state_connected;
}
/*---------------------------------------------------------------------------*/
#ifdef USART_PRINT_MSG
void USART_PRINT_MSG_Configuration(UART_HandleTypeDef *UART_MsgHandle, uint32_t baud_rate)
{
  UART_MsgHandle->Instance             = WIFI_UART_MSG;
  UART_MsgHandle->Init.BaudRate        = baud_rate;
  UART_MsgHandle->Init.WordLength      = UART_WORDLENGTH_8B;
  UART_MsgHandle->Init.StopBits        = UART_STOPBITS_1;
  UART_MsgHandle->Init.Parity          = UART_PARITY_NONE ;
  UART_MsgHandle->Init.HwFlowCtl       = UART_HWCONTROL_NONE;// USART_HardwareFlowControl_RTS_CTS;
  UART_MsgHandle->Init.Mode            = UART_MODE_TX_RX;

  if(HAL_UART_DeInit(UART_MsgHandle) != HAL_OK)
  {
    Error_Handler();
  }
  if(HAL_UART_Init(UART_MsgHandle) != HAL_OK)
  {
    Error_Handler();
  }
}
/*---------------------------------------------------------------------------*/
void UART_Msg_Gpio_Init()
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  USARTx_PRINT_TX_GPIO_CLK_ENABLE();
  USARTx_PRINT_RX_GPIO_CLK_ENABLE();


  /* Enable USARTx clock */
  USARTx_PRINT_CLK_ENABLE();
    __SYSCFG_CLK_ENABLE();
  /*##-2- Configure peripheral GPIO ##########################################*/
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = WiFi_USART_PRINT_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
#if defined(USE_STM32L0XX_NUCLEO) || defined(USE_STM32F4XX_NUCLEO) || defined(USE_STM32L4XX_NUCLEO)
  GPIO_InitStruct.Alternate = PRINTMSG_USARTx_TX_AF;
#endif
  HAL_GPIO_Init(WiFi_USART_PRINT_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = WiFi_USART_PRINT_RX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
#if defined(USE_STM32L0XX_NUCLEO) || defined(USE_STM32F4XX_NUCLEO) || defined(USE_STM32L4XX_NUCLEO)
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Alternate = PRINTMSG_USARTx_RX_AF;
#endif

  HAL_GPIO_Init(WiFi_USART_PRINT_RX_GPIO_PORT, &GPIO_InitStruct);

#ifdef WIFI_USE_VCOM
  /*##-3- Configure the NVIC for UART ########################################*/
  /* NVIC for USART */
  HAL_NVIC_SetPriority(USARTx_PRINT_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(USARTx_PRINT_IRQn);
#endif
}
#endif  // end of USART_PRINT_MSG

/**
  * @}
  */
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
