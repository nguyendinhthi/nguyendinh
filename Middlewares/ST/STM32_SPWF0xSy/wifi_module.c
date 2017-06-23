/**
 ******************************************************************************
 * @file    wifi_module.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    17-May-2016
 * @brief   Enable Wi-Fi functionality using AT cmd set
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
#include "wifi_globals.h"

/** @addtogroup MIDDLEWARES
* @{
*/ 

/** @defgroup  NUCLEO_WIFI_MODULE
  * @brief Wi-Fi driver modules
  * @{
  */

/** @defgroup NUCLEO_WIFI_MODULE_Private_Defines
  * @{
  */

/**
  * @}
  */

/** @addtogroup NUCLEO_WIFI_MODULE_Private_Variables
  * @{
  */
wifi_instances_t wifi_instances;
/*
#ifdef SPWF04
extern wifi_bool wind_55_in_Q;
#endif
*/
/***********All Buffers**************/

#if defined (__CC_ARM)
size_t strnlen (const char* s, size_t maxlen);

size_t strnlen (const char* s, size_t maxlen)
  {
    size_t len = 0;
    while ((len <= maxlen) && (*s))
      {
          s++;
          len++;
      }
    return len;
  }
#endif

// [DLI]
#ifdef WIFI_USE_VCOM

uint8_t console_listen_char[1];
uint8_t console_input_char[1];
uint8_t console_send_char[1];
uint8_t console_echo_char[1];
__IO ITStatus console_send_ready = RESET;
__IO ITStatus console_echo_ready = SET;
__IO ITStatus console_push_ready = RESET;

// Virtual-COM UART
void console_input() 
{
  HAL_UART_Receive_IT(UartMsgHandle, (uint8_t *)console_input_char, 1);
}

void wifi_vcom() 
{
  if (console_push_ready == SET) 
    {
       push_buffer_queue(&wifi_instances.big_buff, WiFi_Counter_Variables.uart_byte);
       console_push_ready = RESET;
       HAL_UART_Receive_IT(&UartWiFiHandle, (uint8_t *)WiFi_Counter_Variables.uart_byte, 1);
    }
 if(console_echo_ready == SET) 
   {
      WiFi_Counter_Variables.temp = pop_buffer_queue(&wifi_instances.big_buff);
      if(WiFi_Counter_Variables.temp != NULL) 
        {
          console_echo_ready = RESET;
          HAL_UART_Transmit_IT(UartMsgHandle, WiFi_Counter_Variables.temp, 1);
        }
   }
}

#endif

/**
  * @}
  */
  
/** @defgroup NUCLEO_WIFI_MODULE_Private_Functions
  * @{
  */

/**
  * @brief  WiFi_Module_Init
  *         Initialize wifi module
  * @param  None
  * @retval None
  */
void WiFi_Module_Init(void)
{
#ifdef WIFI_USE_VCOM
  console_input();
#endif  
  init(&wifi_instances.big_buff, 1024);//Init the ring buffer  
  IO_status_flag.wifi_ready = 0; //reset to get user callback on HW started
  wifi_connected = 0; //reset to get user callback on WiFi UP

#if defined(CONSOLE_UART_ENABLED)
  Receive_Data();
#else  
  #if defined(SPWF04) //SPWF01 does not have SPI
    WiFi_SPI_Init(&SpiHandle); 
    Enable_SPI_Receiving_Path();  
  #endif
#endif
  
  Set_WiFi_Counter_Variables( );
  Set_WiFi_Control_Variables( );
  
#ifdef USE_STM32L0XX_NUCLEO
  event_init(&wifi_instances.event_buff, 10); //max 15 events can be Q'ed (Event Buffer is of size 15)
#else
  event_init(&wifi_instances.event_buff, 50); //max 50 events can be Q'ed (Event Buffer is of size 50)
#endif
  
#ifndef WIFI_USE_VCOM
  Start_Timer();  
  memset(open_sockets,0x00, 8); //init the open socket array
  
  if(HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)//Start the TIM timer
    {
      #if DEBUG_PRINT
      printf("Error");
      #endif
      Error_Handler();
    }
#endif
}

/**
* @brief  Set_WiFi_Control_Variables
*         Sets the default value of some control Variables
* @param  None
* @retval None
*/
void Set_WiFi_Control_Variables(void)
{
  IO_status_flag.enable_dequeue                      = WIFI_TRUE;
  IO_status_flag.command_mode                        = WIFI_TRUE;
  IO_status_flag.radio_off                           = WIFI_FALSE;
  IO_status_flag.wifi_ready                          = WIFI_FALSE;
  IO_status_flag.UartReady                           = RESET;
  IO_status_flag.TxUartReady                         = RESET;
  IO_status_flag.Uart2Ready                          = RESET;
  IO_status_flag.WIND64_count                        = 0;
  IO_status_flag.AT_Response_Received                = WIFI_FALSE;
  IO_status_flag.WiFi_WIND_State                     = Undefine_state;
  IO_status_flag.AT_event_processing                 = WIFI_NO_EVENT;
  IO_status_flag.AT_CMD_count                        = 0;
  
  WiFi_Control_Variables.switch_by_default_to_command_mode    = WIFI_TRUE;
  WiFi_Control_Variables.queue_wifi_wind_message              = WIFI_TRUE;
  WiFi_Control_Variables.enable_timeout_timer                 = WIFI_FALSE;
  //enable SockOn_Server_Closed_Callback when wind:58 received, not when user requests for socket close.
  //WiFi_Control_Variables.packet_payload_data_available = WIFI_FALSE;
  WiFi_Control_Variables.enable_SockON_Server_Closed_Callback = WIFI_TRUE;
}

/**
* @brief  Set_WiFi_Counter_Variables
*         Sets the default value of some counter Variables
* @param  None
* @retval None
*/
void Set_WiFi_Counter_Variables(void)
{
  WiFi_Counter_Variables.no_of_open_client_sockets = 0;
  WiFi_Counter_Variables.wind64_DQ_wait            = 0;
  WiFi_Counter_Variables.Socket_Data_Length        = 0;
  WiFi_Counter_Variables.number_of_bytes           = 0;
  WiFi_Counter_Variables.interim_number_of_bytes   = 0;
  WiFi_Counter_Variables.sock_total_count          = 0;
  WiFi_Counter_Variables.pop_buffer_size           = 0;
  WiFi_Counter_Variables.epoch_time                = 0;
  WiFi_Counter_Variables.sleep_count               = 0;
  WiFi_Counter_Variables.standby_time              = 0;
  WiFi_Counter_Variables.scanned_ssids             = 0;
  WiFi_Counter_Variables.timeout_tick              = 0;
#ifdef SPWF01
  WiFi_Counter_Variables.last_process_buffer_index = 5;
#else
  WiFi_Counter_Variables.last_process_buffer_index = 10;
#endif
}

/**
* @brief  WiFi_Configuration
*         Default Wifi configuration parameters
* @param  None
* @retval None
*/
void WiFi_Configuration()
{

  /* Set the network privacy mode 
    (0=none, 1=WEP, 2=WPA-Personal (TKIP/AES) or WPA2-Personal (TKIP/AES)) */  
  WiFi_Config_Variables.wifi_mode               =  WiFi_STA_MODE;
  WiFi_Config_Variables.wifi_priv_mode          =  WPA_Personal;
  WiFi_Config_Variables.wifi_ssid               =  "NETGEAR54" ;
  WiFi_Config_Variables.Wifi_Sec_key            =  "12341234";
  
   /*Power Management Settings*/
  WiFi_Config_Variables.sleep_enabled           = 0;//0=disabled, 1=enabled
  WiFi_Config_Variables.standby_enabled         = 1;
  WiFi_Config_Variables.standby_time            = 10;//in seconds
  WiFi_Config_Variables.wifi_powersave          = 1;//0=Active, 1=PS Mode, 2=Fast-PS Mode
  WiFi_Config_Variables.wifi_operational_mode   = 11;//11= Doze mode, 12= Quiescent mode
  WiFi_Config_Variables.wifi_listen_interval    = 0; //Wakeup every n beacon
  WiFi_Config_Variables.wifi_beacon_wakeup      = 1;         
}

/**
* @brief  wifi_reset
*         Reset WiFi module using PC12 gpio pin
* @param  None
* @retval None
*/
void wifi_reset(void)
{
  RESET_WAKEUP_GPIO_CLK_ENABLE();
  __GPIOA_CLK_ENABLE();

  wifi_instances.GPIO_InitStruct.Pin       = WiFi_RESET_GPIO_PIN;
  wifi_instances.GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
  wifi_instances.GPIO_InitStruct.Pull      = GPIO_PULLUP;
  wifi_instances.GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;

  HAL_GPIO_Init(WiFi_RESET_GPIO_PORT, &wifi_instances.GPIO_InitStruct);

//  IO_status_flag.WiFi_WIND_State.WiFiHWStarted = WIFI_FALSE;
  wifi_connected = 0; //reset wifi_connected to get user callback
//  memset((void*)&IO_status_flag.WiFi_WIND_State,0x00,sizeof(IO_status_flag.WiFi_WIND_State)); /*reset the WIND State?*/
  IO_status_flag.WiFi_WIND_State = Undefine_state;

  /* ===   RESET PIN - PC12   ===*/
  HAL_GPIO_WritePin(WiFi_RESET_GPIO_PORT, WiFi_RESET_GPIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(200);
  /* ===   SET PIN - PC12   ===*/
  HAL_GPIO_WritePin(WiFi_RESET_GPIO_PORT, WiFi_RESET_GPIO_PIN, GPIO_PIN_SET);
  //  HAL_Delay(50);
    HAL_Delay(200); //MGR
  HAL_GPIO_DeInit(WiFi_RESET_GPIO_PORT, WiFi_RESET_GPIO_PIN);

#ifndef WIFI_USE_VCOM
  while(IO_status_flag.WiFi_WIND_State != WiFiHWStarted)
    {
      __NOP(); //nothing to do
    }
#endif
}

/**
* @brief  PowerUp_WiFi_Module
*         Power up Wi-Fi module,SET GPIO PA0 pin 
* @param  None
* @retval None
*/
void PowerUp_WiFi_Module(void)
{
  /* ===   SET PIN - PC12   ===*/
  HAL_GPIO_WritePin(WiFi_RESET_GPIO_PORT, WiFi_RESET_GPIO_PIN, GPIO_PIN_SET);
}

/**
* @brief  Receive_Data
*         Receive data from UART port
* @param  uint8_t number of bytes to be received
* @retval None
*/
void Receive_Data(void)
{
  HAL_GPIO_WritePin(WiFi_USART_RTS_GPIO_PORT, WiFi_USART_RTS_PIN, GPIO_PIN_RESET);//Assert RTS
  if(HAL_UART_Receive_IT(&UartWiFiHandle, (uint8_t *)WiFi_Counter_Variables.uart_byte, 1) !=HAL_OK)
    {
      #if DEBUG_PRINT
      printf("HAL_UARTx_Receive_IT Error");
      #endif
    }
  else 
    {
      WiFi_Control_Variables.Uartx_Rx_Processing = WIFI_TRUE;
    }
}

/**
* @brief  Period elapsed callback in non blocking mode
*         This timer is used for calling back User registered functions with information
* @param  htim : TIM handle
* @retval None
*/
void Wifi_TIM_Handler(TIM_HandleTypeDef *htim)
{
  /**********************************************************************
  *                                                                     *
  *       Be careful not to make a blocking                             *
  *       call from this function, see                                  *
  *       example Socket_Read() and Socket_Close()                      *
  *                                                                     *
  **********************************************************************/
    WiFi_Status_t status = WiFi_MODULE_SUCCESS;

    if(WiFi_Control_Variables.stop_event_dequeue == WIFI_FALSE)
      {
          __disable_irq();
          wifi_instances.DeQed_wifi_event = pop_eventbuffer_queue(&wifi_instances.event_buff);
          __enable_irq();

          if(wifi_instances.DeQed_wifi_event!=NULL)
            {              
                            
              switch(wifi_instances.DeQed_wifi_event->event)
                {
                  case WIFI_WIND_EVENT:
                          Process_DeQed_Wind_Indication(wifi_instances.DeQed_wifi_event);
                          break;

                  case WIFI_SOCK_ID_EVENT:
                          /*check ID and update SocketID array*/
                          WiFi_Counter_Variables.no_of_open_client_sockets++;
                          if(WiFi_Counter_Variables.no_of_open_client_sockets > 8)  //Max number of clients is 8
                            {
                        	  WiFi_Counter_Variables.no_of_open_client_sockets--;//MGR
                              IO_status_flag.AT_Response_Received = WIFI_TRUE;
                              WiFi_Counter_Variables.AT_RESPONSE = WiFi_NOT_SUPPORTED;     
                              break;
                            }
//check						  WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;//MGR
//check						  WiFi_Counter_Variables.timeout_tick = 0;//MGR
                          open_sockets[wifi_instances.DeQed_wifi_event->socket_id]  = WIFI_TRUE;
                          WiFi_Counter_Variables.Socket_Open_ID = wifi_instances.DeQed_wifi_event->socket_id;
                          IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                          IO_status_flag.AT_Response_Received = WIFI_TRUE;
                          WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                          break;
                          
                  case WIFI_SOCK_SERVER_ID_EVENT:
                          /*check ID and update ServerSocketID array*/
                          WiFi_Counter_Variables.no_of_open_server_sockets++;
                          WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                          WiFi_Counter_Variables.timeout_tick = 0;
                          if(WiFi_Counter_Variables.no_of_open_server_sockets > 4)  //Max number of server socket open is 4
                            {
                              IO_status_flag.AT_Response_Received = WIFI_TRUE;
                              WiFi_Counter_Variables.AT_RESPONSE = WiFi_NOT_SUPPORTED;     
                              break;
                            }
                          open_server_sockets[wifi_instances.DeQed_wifi_event->socket_id]  = WIFI_TRUE;
                          WiFi_Counter_Variables.Server_Socket_Open_ID = wifi_instances.DeQed_wifi_event->socket_id;
                          IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                          IO_status_flag.AT_Response_Received = WIFI_TRUE;
                          WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;

                          break;
                          
                  case WIFI_HTTP_EVENT:
                          Reset_AT_CMD_Buffer();  
                          #if defined(CONSOLE_UART_ENABLED)
                            if(WiFi_Counter_Variables.http_ind==1) 
                              sprintf((char*)WiFi_AT_Cmd_Buff,AT_HTTPPOST_REQUEST,WiFi_Counter_Variables.curr_hostname, WiFi_Counter_Variables.curr_path, (int)WiFi_Counter_Variables.curr_port_number);
                            else if(WiFi_Counter_Variables.http_ind==0) 
                              sprintf((char*)WiFi_AT_Cmd_Buff,AT_HTTPGET_REQUEST,WiFi_Counter_Variables.curr_hostname, WiFi_Counter_Variables.curr_path, (int)WiFi_Counter_Variables.curr_port_number);

                            status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          #else /*SPI Tx*/
                            if(WiFi_Counter_Variables.http_ind==1) 
                              {
                                  if(WiFi_Counter_Variables.curr_port_number!=0)
                                    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.HTTPPOST=%s,%s,%d,0,NULL,NULL,NULL,NULL",WiFi_Counter_Variables.curr_hostname, WiFi_Counter_Variables.curr_path, (int)WiFi_Counter_Variables.curr_port_number);
                                  else 
                                    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.HTTPPOST=%s,%s",WiFi_Counter_Variables.curr_hostname, WiFi_Counter_Variables.curr_path);//@TBD:Error!
                              }
                            else if(WiFi_Counter_Variables.http_ind==0) 
                              {
                                  if(WiFi_Counter_Variables.curr_port_number!=0)
                                    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.HTTPGET=%s,%s,%d,0,NULL,NULL,NULL,NULL",WiFi_Counter_Variables.curr_hostname, WiFi_Counter_Variables.curr_path, (int)WiFi_Counter_Variables.curr_port_number);
                                  else 
                                    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.HTTPGET=%s,%s,80,0,NULL,NULL,NULL,NULL",WiFi_Counter_Variables.curr_hostname, WiFi_Counter_Variables.curr_path);
                              }
                            
                            run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                            status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code
                          #endif
                          if(status == WiFi_MODULE_SUCCESS) 
                            {                                
                                WiFi_Counter_Variables.timeout_tick = 0;
                                WiFi_Control_Variables.enable_timeout_timer = WIFI_TRUE;
                                WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
                                WiFi_Control_Variables.http_req_pending   = WIFI_TRUE;
                                IO_status_flag.AT_event_processing = WIFI_HTTP_EVENT;
                            }
                          else
                            {
                                #if DEBUG_PRINT
                                  printf("\r\n ERROR DURING HTTP COMMAND TRANSFER \r\n");
                                #endif
                                WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                            }
                          break;

                  case WIFI_CLIENT_SOCKET_WRITE_EVENT:
                          Reset_AT_CMD_Buffer();
                          /* AT+S.SOCKW=00,11<cr> */
                          #if defined(CONSOLE_UART_ENABLED)
                            sprintf((char*)WiFi_AT_Cmd_Buff,AT_SOCKET_WRITE, WiFi_Counter_Variables.curr_sockID, WiFi_Counter_Variables.curr_DataLength);
                            status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          #else
                            sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKW=%d,%d %s", WiFi_Counter_Variables.curr_sockID, WiFi_Counter_Variables.curr_DataLength, WiFi_Counter_Variables.curr_data);
                            run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                            status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code
                          #endif
                          if(status == WiFi_MODULE_SUCCESS) 
                            {
                                Reset_AT_CMD_Buffer();                                
                                #if defined(CONSOLE_UART_ENABLED)
                                memcpy((char*)WiFi_AT_Cmd_Buff, (char*)WiFi_Counter_Variables.curr_data, WiFi_Counter_Variables.curr_DataLength);
                                status = USART_Transmit_AT_Cmd(WiFi_Counter_Variables.curr_DataLength);
                                #else
                                //SPI_Transmit_Manager_Poll(WiFi_AT_Cmd_Buff, WiFi_Counter_Variables.curr_DataLength);
                                status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code
                                IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_WRITE_EVENT;
                                IO_status_flag.send_data = WIFI_TRUE;
                                WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
                                #endif
                            }
                          if(status != WiFi_MODULE_SUCCESS)
                            {
                                #if DEBUG_PRINT
                                  printf("\r\n ERROR In Socket Write\r\n");
                                #endif
                                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                                WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                            }
                          break;

                  #ifdef SPWF04                          
                  case WIFI_SERVER_SOCKET_WRITE_EVENT:
                          Reset_AT_CMD_Buffer();
                          
                          /* AT+S.SOCKDW=<id>,<clientID>,len<cr> */
                          #if defined(CONSOLE_UART_ENABLED)
                            sprintf((char*)WiFi_AT_Cmd_Buff,AT_SERVER_SOCKET_WRITE, WiFi_Counter_Variables.curr_serverID, WiFi_Counter_Variables.curr_sockID, WiFi_Counter_Variables.curr_DataLength);
                            status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          #else
                            sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKDW=%d,%d,%d %s", WiFi_Counter_Variables.curr_serverID, WiFi_Counter_Variables.curr_sockID, WiFi_Counter_Variables.curr_DataLength, WiFi_Counter_Variables.curr_data);
                            run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                            status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code
                          #endif
                          if(status == WiFi_MODULE_SUCCESS) 
                            {
                                Reset_AT_CMD_Buffer();
                                #if defined(CONSOLE_UART_ENABLED)
                                  memcpy((char*)WiFi_AT_Cmd_Buff, (char*)WiFi_Counter_Variables.curr_data, WiFi_Counter_Variables.curr_DataLength);
                                  status = USART_Transmit_AT_Cmd(WiFi_Counter_Variables.curr_DataLength);
                                #else
                                  //SPI_Transmit_Manager_Poll(WiFi_AT_Cmd_Buff, WiFi_Counter_Variables.curr_DataLength);
                                  status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code
                                  IO_status_flag.AT_event_processing = WIFI_SERVER_SOCKET_WRITE_EVENT;
                                  IO_status_flag.send_data = WIFI_TRUE;
                                  WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
                                #endif
                            }
                          if(status != WiFi_MODULE_SUCCESS)
                            {
                                #if DEBUG_PRINT
                                  printf("\r\n ERROR In Server Socket Write\r\n");
                                #endif
                                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                                WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                            }
                          break;
                  #else
                  case WIFI_SERVER_SOCKET_WRITE_EVENT:
                        break;
                  #endif
                  case WIFI_CLIENT_SOCKET_OPEN_EVENT:
                          Reset_AT_CMD_Buffer();
                          #if defined(CONSOLE_UART_ENABLED)
                              /* AT+S.SOCKON = myserver,1234,t <cr> */  
                              sprintf((char*)WiFi_AT_Cmd_Buff,AT_SOCKET_OPEN,WiFi_Counter_Variables.curr_hostname,(int)WiFi_Counter_Variables.curr_port_number,WiFi_Counter_Variables.curr_protocol);        
                              status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          #else
                              /* AT+S.SOCKON = myserver,1234,t <cr> */  
                              //@TBD: Set "is_secure_socket" only when the socket being opened is TLS/"s"/"domain.txt"
                              if((strcmp((char const *)WiFi_Counter_Variables.curr_protocol,"t")!=0) && (strcmp((char const *)WiFi_Counter_Variables.curr_protocol,"u")!=0))
                                WiFi_Control_Variables.is_secure_socket = WIFI_TRUE;
                              sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKON=%s,%d,NULL,%s",WiFi_Counter_Variables.curr_hostname,(int)WiFi_Counter_Variables.curr_port_number,WiFi_Counter_Variables.curr_protocol);
                              run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                              status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
                          #endif
                          
                          if(status != WiFi_MODULE_SUCCESS)
                            {
                                #if DEBUG_PRINT
                                  printf("\r\n ERROR During Socket Open \r\n");
                                #endif
                                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                                WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                            }
                          else
                            {
                                IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_OPEN_EVENT;
                                WiFi_Counter_Variables.timeout_tick = 0;
                                WiFi_Control_Variables.enable_timeout_timer = WIFI_TRUE;                                
                            }
                          break;
                          
                  case WIFI_SERVER_SOCKET_OPEN_EVENT:  
                          Reset_AT_CMD_Buffer();

                          /* AT+S.SOCKDON=portNo,t<cr> */
                          #if defined(CONSOLE_UART_ENABLED)
                            sprintf((char*)WiFi_AT_Cmd_Buff,AT_SERVER_SOCKET_OPEN,(int)WiFi_Counter_Variables.curr_port_number,WiFi_Counter_Variables.curr_protocol);        
                            status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          #else
                            sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKDON=%d,%s",(int)WiFi_Counter_Variables.curr_port_number,WiFi_Counter_Variables.curr_protocol);
                            run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                            status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
                          #endif
                          
                          if(status != WiFi_MODULE_SUCCESS)
                            {
                                #if DEBUG_PRINT
                                  printf("\r\n ERROR During Server Socket Open \r\n");
                                #endif
                                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                                WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                            }
                          else
                            {
                                IO_status_flag.AT_event_processing = WIFI_SERVER_SOCKET_OPEN_EVENT;
                                WiFi_Counter_Variables.timeout_tick = 0;
                                WiFi_Control_Variables.enable_timeout_timer = WIFI_TRUE;
                            }
                          break;

                  case WIFI_CLIENT_SOCKET_CLOSE_EVENT:
                          if(open_sockets[wifi_instances.DeQed_wifi_event->socket_id])
                            {
                                Reset_AT_CMD_Buffer();
                                //@TBD: Do a SOCK.R (Read) before closing the socket
                                 #if defined(CONSOLE_UART_ENABLED)
                                 /* AT+S.SOCKC=00<cr> */                                
                                sprintf((char*)WiFi_AT_Cmd_Buff,AT_SOCKET_CLOSE,wifi_instances.DeQed_wifi_event->socket_id);
                                status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                                #else
                                
                                sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKC=%d",wifi_instances.DeQed_wifi_event->socket_id);
                                run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                                status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
                                #endif
                                if(status == WiFi_MODULE_SUCCESS)
                                  {
                                    WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                                    WiFi_Control_Variables.stop_event_dequeue          = WIFI_TRUE;
                                    WiFi_Counter_Variables.remote_socket_closed_id     = wifi_instances.DeQed_wifi_event->socket_id;
                                    
                                    //for making changes in the value of open_sockets[sock_id] if no error is returned
                                    IO_status_flag.client_socket_close_ongoing = WIFI_TRUE;
                                    
                                    //prevent the OK received after socket close command to be Q'ed
                                    IO_status_flag.prevent_push_OK_event       = WIFI_TRUE;
                                  }
                                else
                                  {
                                    #if DEBUG_PRINT
                                      printf("\r\n ERROR During Socket Close \r\n");
                                    #endif
                                  }
                            }
                          else
                            printf("\r\n Socket already close");
                          break; 

                  case WIFI_SERVER_SOCKET_CLOSE_EVENT:
                        Reset_AT_CMD_Buffer();
                        #ifdef SPWF04
                          if(open_server_sockets[wifi_instances.DeQed_wifi_event->server_id])
                          {
                              #if defined(CONSOLE_UART_ENABLED)
                              if(wifi_instances.DeQed_wifi_event->socket_id != 9)
                              {
                                  sprintf((char *)WiFi_AT_Cmd_Buff,AT_SPECIFIC_CLIENT_ON_SERVER_CLOSE,wifi_instances.DeQed_wifi_event->server_id,wifi_instances.DeQed_wifi_event->socket_id);
                                  WiFi_Control_Variables.close_specific_client = WIFI_TRUE;
                              }
                              else
                              {
//                                  sprintf((char *)WiFi_AT_Cmd_Buff,AT_SERVER_SOCKET_CLOSE,wifi_instances.DeQed_wifi_event->server_id);
                              }
                              status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                              #else
                              //sprintf((char *)WiFi_AT_Cmd_Buff,AT_SPECIFIC_CLIENT_ON_SERVER_CLOSE,wifi_instances.DeQed_wifi_event->server_id,wifi_instances.DeQed_wifi_event->socket_id);
                              sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKDC=%d,%d",wifi_instances.DeQed_wifi_event->server_id,wifi_instances.DeQed_wifi_event->socket_id);
                              run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                              status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
                              #endif
                              if(status == WiFi_MODULE_SUCCESS)
                                {
                                  WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                                  WiFi_Control_Variables.stop_event_dequeue      = WIFI_TRUE;
                                  WiFi_Counter_Variables.remote_server_closed_id = wifi_instances.DeQed_wifi_event->server_id;
                                  WiFi_Counter_Variables.remote_socket_closed_id = wifi_instances.DeQed_wifi_event->socket_id;

                                  //for making changes in the value of open_sockets[sock_id] if no error is returned
                                  IO_status_flag.server_socket_close_ongoing = WIFI_TRUE;

                                  //prevent the OK received after socket close command to be Q'ed
                                  IO_status_flag.prevent_push_OK_event       = WIFI_TRUE;
                                }
                              else
                                {
                                  #if DEBUG_PRINT
                                    printf("\r\n ERROR During Socket Close \r\n");
                                  #endif
                                }
                          }
                        #else
                          sprintf((char *)WiFi_AT_Cmd_Buff,AT_SERVER_SOCKET_CLOSE);
                          status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          if(status == WiFi_MODULE_SUCCESS)
                          {
                            WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
                          }
                        #endif
                          break;
                          
                  case WIFI_FILE_EVENT:
                          Reset_AT_CMD_Buffer();

                          if(WiFi_Counter_Variables.curr_filename == NULL)
                            {
                                /* AT+S.FSL */
                                sprintf((char*)WiFi_AT_Cmd_Buff,AT_DISPLAY_FILENAME_LIST);
                            }
                          else if(WiFi_Counter_Variables.curr_hostname == NULL)
                            {
                                #ifdef SPWF01
                                   /* AT+S.FSP=/index.html */
                                  sprintf((char*)WiFi_AT_Cmd_Buff,AT_DISPLAY_FILE_CONTENT,WiFi_Counter_Variables.curr_filename);
                                #else
                                  /* @TODO: the default second and third parameter of the command */
                                  sprintf((char*)WiFi_AT_Cmd_Buff,AT_DISPLAY_FILE_CONTENT,WiFi_Counter_Variables.curr_filename);
                                #endif
                            }
                          else
                            {
                                /* AT+S.HTTPDFSUPDATE=%s,/outfile.img  */
                                sprintf((char*)WiFi_AT_Cmd_Buff,AT_DOWNLOAD_IMAGE_FILE,WiFi_Counter_Variables.curr_hostname,WiFi_Counter_Variables.curr_filename,(int)WiFi_Counter_Variables.curr_port_number);
                            }
                          status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          if(status == WiFi_MODULE_SUCCESS)
                            {
                                WiFi_Control_Variables.enable_receive_http_response = WIFI_TRUE;
                                WiFi_Control_Variables.enable_receive_data_chunk    = WIFI_TRUE;
                                WiFi_Control_Variables.enable_receive_file_response = WIFI_TRUE;
                            }
                          else
                            {
                              #if DEBUG_PRINT
                                printf("\r\n ERROR DURING FILE OPERATION \r\n");
                              #endif
                              IO_status_flag.AT_Response_Received = WIFI_TRUE;
                              WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                            }
                          break;

                    case WIFI_FILE1_EVENT:
                          Reset_AT_CMD_Buffer();
                          
                          if( WiFi_Counter_Variables.volume == -1 && WiFi_Counter_Variables.erase == -1) {
                            if(WiFi_Counter_Variables.curr_filename !=NULL && WiFi_Counter_Variables.mod_filename!=NULL) {
                              #ifdef SPWF04
                                /* Rename a file.... AT+S.FSR=<old_filename>,<new_filename> */
                                sprintf((char*)WiFi_AT_Cmd_Buff,AT_RENAME_FILE, WiFi_Counter_Variables.curr_filename,WiFi_Counter_Variables.mod_filename);
                              #endif
                            }
                            else if(WiFi_Counter_Variables.curr_filename !=NULL && WiFi_Counter_Variables.mod_filename==NULL) {
                                /* Delete a file...AT+S.FSD=<filename> */ 
                                sprintf((char*)WiFi_AT_Cmd_Buff,AT_DELETE_FILE, WiFi_Counter_Variables.curr_filename);
                            }
                          } else {
                            #ifdef SPWF04
                                /* erase user memory volume...AT+S.FSU=[<volume>],[<erase>] */
                                sprintf((char*)WiFi_AT_Cmd_Buff,AT_UNMOUNT_USER_MEMORY, WiFi_Counter_Variables.volume,WiFi_Counter_Variables.erase);
                            #endif
                          }
                          status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          /* Wait till OK of this command is not received */
                          if(status != WiFi_MODULE_SUCCESS)
                           {
                               #if DEBUG_PRINT
                                 printf("\r\n ERROR During File Event\r\n");
                               #endif
                               IO_status_flag.AT_Response_Received = WIFI_TRUE;
                               WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                           }
                          break;
                          
                  case WIFI_FW_UPDATE_EVENT:
                          
                          Reset_AT_CMD_Buffer();
                          #if defined(CONSOLE_UART_ENABLED)
                            /*AT+S.FWUPDATE=e,<hostname>,[<path&query>,<port>,<TLS>,<username>,<passwd>]\r*/
                            sprintf((char*)WiFi_AT_Cmd_Buff,AT_FWUPDATE,WiFi_Counter_Variables.curr_hostname,WiFi_Counter_Variables.curr_filename,(int)WiFi_Counter_Variables.curr_port_number);
                            status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
                          #else
                            sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.FWUPDATE=e,%s,/%s,%d,0,NULL,NULL",WiFi_Counter_Variables.curr_hostname,WiFi_Counter_Variables.curr_filename,(int)WiFi_Counter_Variables.curr_port_number);
                            run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                            status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
                          #endif
                          
                          if(status == WiFi_MODULE_SUCCESS)
                            {
                              WiFi_Control_Variables.enable_fw_update_read      = WIFI_TRUE;
                              WiFi_Control_Variables.enable_receive_data_chunk  = WIFI_TRUE;
                              IO_status_flag.AT_event_processing = WIFI_FW_UPDATE_EVENT;
                            }
                          else 
                            {
                              #if DEBUG_PRINT
                                printf("\r\n ERROR DURING FIRMWARE UPDATE \r\n");
                              #endif
                              IO_status_flag.AT_Response_Received = WIFI_TRUE;
                              WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                            }
                          break;

                  case WIFI_ERROR_EVENT:
                          #if DEBUG_PRINT
                            //printf("\r\n ERROR!\r\n");
                          #endif
                          IO_status_flag.AT_Response_Received = WIFI_TRUE;
                          WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                          IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                          break;

                  case WIFI_OK_EVENT:
                          #if DEBUG_PRINT
                          printf("\r\n<<OK\r\n");
                          #endif                          
                          // no break
                  case WIFI_GCFG_EVENT:
                  case WIFI_GPIO_EVENT:
                          IO_status_flag.AT_Response_Received = WIFI_TRUE;
                          WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                          IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                          break;

                  case WIFI_STANDBY_CONFIG_EVENT:
                          #if DEBUG_PRINT
                              printf("\r\nGoing into standby..\r\n");  
                          #endif
                          break;

                  case WIFI_RESUME_CONFIG_EVENT:
                          #if DEBUG_PRINT
                              printf("\r\nResuming from standby..\r\n");
                          #endif
                          IO_status_flag.AT_Response_Received = WIFI_TRUE;//let main run-on
                          break;
                          
                  case WIFI_NO_EVENT:
                  case WIFI_SCAN_EVENT:
                  case WIFI_SERVER_SOCKET_QUERY_EVENT:
                  case WIFI_SERVER_SOCKET_READ_DATA:
                  case WIFI_SERVER_SOCKET_READ_EVENT:
                  case WIFI_CLIENT_SOCKET_QUERY_EVENT:
                  case WIFI_CLIENT_SOCKET_READ_DATA:
                  case WIFI_CLIENT_SOCKET_READ_EVENT:
                  case WIFI_CERT_WRITE_EVENT:
                          break;
               }
            }
      }
      /* If data is pending on client socket SOCKON, make read requests*/
      if(WiFi_Control_Variables.start_sock_read == WIFI_TRUE)
              {
                  WiFi_Control_Variables.start_sock_read = WIFI_FALSE;
                  Socket_Read(WiFi_Counter_Variables.Socket_Data_Length);                  
              }  
#ifdef SPWF04    
      /* If data is pending on server socket SOCKDON, make read requests*/
      else if(WiFi_Control_Variables.start_sockd_read == WIFI_TRUE)
              {
                  WiFi_Control_Variables.start_sockd_read = WIFI_FALSE;
                  Socket_Server_Read(WiFi_Counter_Variables.Socket_Data_Length);                  
              }  
#endif
      /* Call Query, after notification for TLS is received */
      else if(WiFi_Control_Variables.enable_query == WIFI_TRUE && IO_status_flag.enable_dequeue == WIFI_TRUE)
              {
                  WiFi_Control_Variables.enable_query = WIFI_FALSE;
                  //@TBD: Flushing the buffer may be detrimental if we have genuine follow on WIND55?
                  Socket_Pending_Data();                  
              }      
#ifdef SPWF04
      else if(WiFi_Control_Variables.enable_server_query == WIFI_TRUE && IO_status_flag.enable_dequeue == WIFI_TRUE)
              {
                  WiFi_Control_Variables.enable_server_query = WIFI_FALSE;
                  //@TBD: Flushing the buffer may be detrimental if we have genuine follow on WIND55?
                  Server_Pending_Data();                  
              }  
#endif
      else if(WiFi_Control_Variables.Pending_SockON_Callback==WIFI_TRUE)//for client socket
              {
                  WiFi_Control_Variables.Pending_SockON_Callback=WIFI_FALSE;
                  //Now callback to user with user_data pointer <UserDataBuff>              
                  ind_wifi_socket_data_received(-1, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.message_size, WiFi_Counter_Variables.chunk_size);
                  memset(UserDataBuff, 0x00, MAX_BUFFER_GLOBAL);//Flush the buffer
                  Resume_Dequeue();
#ifndef CONSOLE_UART_ENABLED //Only in case of SPI					
                  WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                  if(packet_payload_data_available)
                  {
                      packet_payload_data_available = WIFI_FALSE;
                      ResumePayloadReception();//only part of payload has been read and sent to user
                  } else if(spi_callback_in_progress)
                  {
                      spi_callback_in_progress = WIFI_FALSE;
                      //WiFi_Control_Variables.afterOK_enable_query = WIFI_TRUE;
                      ResumePacketReception();//full payload read, now get next header
                  }
#endif                  
              }

      else if(WiFi_Control_Variables.Pending_SockD_Callback == WIFI_TRUE)//for server socket
              {
                  WiFi_Control_Variables.Pending_SockD_Callback=WIFI_FALSE;      
                  //Now callback to user with user_data pointer <UserDataBuff>
                  ind_wifi_socket_data_received(WiFi_Counter_Variables.sockdon_id_user, WiFi_Counter_Variables.sockon_id_user, (uint8_t *)UserDataBuff, WiFi_Counter_Variables.message_size, WiFi_Counter_Variables.chunk_size);
                  memset(UserDataBuff, 0x00, MAX_BUFFER_GLOBAL); //Flush the buffer
                  Resume_Dequeue();
#ifndef CONSOLE_UART_ENABLED //Only in case of SPI                  
                  if(packet_payload_data_available)
                  {                    
                      packet_payload_data_available = WIFI_FALSE;
                      ResumePayloadReception();//only part of payload has been read and sent to user
                  } else if(spi_callback_in_progress)
                  {
                      WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                      spi_callback_in_progress = WIFI_FALSE;
                      ResumePacketReception();//full payload read, now get next header
                  }
#endif                  
              }

      else if(WiFi_Control_Variables.Client_Socket_Close_Cmd == WIFI_TRUE)//for client socket
              {
                  // Q the close socket event
                  if(open_sockets[WiFi_Counter_Variables.client_socket_close_id])
                    {
                      Queue_Client_Close_Event(WiFi_Counter_Variables.client_socket_close_id);
                    }
                  WiFi_Control_Variables.Client_Socket_Close_Cmd = WIFI_FALSE;
              }

      else if(WiFi_Control_Variables.SockON_Server_Closed_Callback==WIFI_TRUE)//for client socket
              {
                  //callback the user
                  ind_wifi_socket_client_remote_server_closed(&WiFi_Counter_Variables.closed_socket_id);
                  WiFi_Control_Variables.SockON_Server_Closed_Callback = WIFI_FALSE;
              }

      else if(WiFi_Control_Variables.HTTP_Data_available == WIFI_TRUE)
              {
                  WiFi_Control_Variables.HTTP_Data_available=WIFI_FALSE;
                  ind_wifi_http_data_available((uint8_t *)UserDataBuff,WiFi_Counter_Variables.UserDataBuff_index);
                  memset(UserDataBuff, 0x00, MAX_BUFFER_GLOBAL);//Flush the buffer
                  Resume_Dequeue();
#ifndef CONSOLE_UART_ENABLED //Only in case of SPI		                  
                  if(packet_payload_data_available)
                  {                    
                      packet_payload_data_available = WIFI_FALSE;
                      ResumePayloadReception();//only part of payload has been read and sent to user
                  } else if(spi_callback_in_progress)
                  {
                      WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                      spi_callback_in_progress = WIFI_FALSE;
                      ResumePacketReception();//full payload read, now get next header
                  }
#endif                  
              }

      else if (WiFi_Control_Variables.FILE_Data_available == WIFI_TRUE)
              {
                  ind_wifi_file_data_available((uint8_t *) UserDataBuff);
                  memset(UserDataBuff, 0x00, MAX_BUFFER_GLOBAL);//Flush the buffer
                  Resume_Dequeue();
                  WiFi_Control_Variables.FILE_Data_available = WIFI_FALSE;
              }
      else if(WiFi_Control_Variables.Client_Connected == WIFI_TRUE)
              {
                  ind_socket_server_client_joined();
                  WiFi_Control_Variables.Client_Connected = WIFI_FALSE;
              }

      else if(WiFi_Control_Variables.Client_Disconnected == WIFI_TRUE)
              {
                  ind_socket_server_client_left();
                  WiFi_Control_Variables.Client_Disconnected = WIFI_FALSE;
              }

    //Make callbacks from here to user for pending events
    switch(IO_status_flag.WiFi_WIND_State)
    {
      //Make callbacks from here to user for pending events
      case WiFiHWStarted: 
        if(IO_status_flag.wifi_ready == 2)//Twice reset for User Callback
         {
            IO_status_flag.wifi_ready++;
            ind_wifi_on();//Call this once only...This if for wifi_on (instead of console active
         }
        break;

      case WiFiUp:
        if(wifi_connected == 0)
         {
            wifi_connected = 1;
            ind_wifi_connected();  //wifi connected
         }
//        IO_status_flag.WiFi_WIND_State = Undefine_state;
        break;

      case WiFiStarted_MiniAPMode:
         ind_wifi_ap_ready();
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         break;

      case WiFiAPClientJoined: 
         ind_wifi_ap_client_joined(WiFi_Counter_Variables.client_MAC_address);
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         break;

      case WiFiAPClientLeft:
         ind_wifi_ap_client_left(WiFi_Counter_Variables.client_MAC_address);
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         break;

      case Deep_Sleep_Callback:
         ind_wifi_resuming();
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         break;

      case standby_resume_callback:
         ind_wifi_resuming();
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         break;

      case WiFiHWFailure:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_HW_FAILURE_ERROR);     //call with error number      
         break;

      case HardFault:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_HARD_FAULT_ERROR);     //call with error number      
         break;

      case StackOverFlow:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_STACK_OVERFLOW_ERROR); //call with error number      
         break;

      case Mallocfailed:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_MALLOC_FAILED_ERROR);  //call with error number      
         break;

      case InitFailure:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_INIT_ERROR);   //call with error number      
         break;

      case StartFailed:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_START_FAILED_ERROR);   //call with error number      
         break;
      
      case WiFiException:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_error(WiFi_EXCEPTION_ERROR);      //call with error number      
         break;

      case PS_Mode_Failure:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_warning(WiFi_POWER_SAVE_WARNING); //call with error number      
         break;

      case HeapTooSmall:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_warning(WiFi_HEAP_TOO_SMALL_WARNING);     //call with error number      
         break;
      
      case WiFiSignalLOW:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_warning(WiFi_SIGNAL_LOW_WARNING); //call with error number      
         break;  

      case WiFiDeauthentication:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_connection_error(WiFi_DE_AUTH);
         break;

      case WiFiDisAssociation:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_connection_error(WiFi_DISASSOCIATION);
         break;

      case WiFiJoinFailed:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_connection_error(WiFi_JOIN_FAILED);
         break;
      
      case WiFiScanBlewUp:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_connection_error(WiFi_SCAN_BLEWUP);    //@TBD to check if user made call, so not call callback if true
         break;
      
      case WiFiScanFailed:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_connection_error(WiFi_SCAN_FAILED);    //@TBD to check if user made call, so not call callback if true
         break;
      
      case WiFiUnHandledInd:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_packet_lost(WiFi_UNHANDLED_IND_ERROR); //@TBD to check if user made call, so not call callback if true
         break;
    
      case WiFiRXMgmt:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_packet_lost(WiFi_RX_MGMT);  //@TBD to check if user made call, so not call callback if true
         break;
      
      case WiFiRXData:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_packet_lost(WiFi_RX_DATA);  //@TBD to check if user made call, so not call callback if true
         break;  
      
      case WiFiRxUnk:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_packet_lost(WiFi_RX_UNK);  //@TBD to check if user made call, so not call callback if true
         break;  
      
      case WiFiSockdDataLost:
         IO_status_flag.WiFi_WIND_State = Undefine_state;
         ind_wifi_socket_server_data_lost();  //@TBD to check if user made call, so not call callback if true
         break;

      case WiFiDeAuth:
      case WiFiPowerDown:
      case Undefine_state:
    	 break;
    }
}

/**
* @brief  Start_Timer
*         Start Timer 
* @param  None
* @retval None
*/
void Start_Timer()
{
  IO_status_flag.tickcount = WIFI_FALSE;
  IO_status_flag.Timer_Running = WIFI_TRUE;
}

/**
* @brief  Stop_Timer
*         Stop Timer request
* @param  None
* @retval None
*/
void Stop_Timer()
{  
  IO_status_flag.tickcount      = WIFI_FALSE;  
  IO_status_flag.Timer_Running  = WIFI_FALSE;    
  IO_status_flag.UartReady      = SET;
}

/**
* @brief  Stop_Dequeue
*         Stop dequeuing data from the ring buffer
* @param  None
* @retval None
*/
void Stop_Dequeue()
{
  IO_status_flag.enable_dequeue = WIFI_FALSE;
}

/**
* @brief  Resume_Dequeue
*         Resume dequeuing data from the ring buffer
* @param  None
* @retval None
*/
void Resume_Dequeue()
{
  IO_status_flag.enable_dequeue = WIFI_TRUE;
}

/**
* @brief  Wifi_SysTick_Isr
*         Function called every SysTick to process buffer
* @param  None
* @retval None
*/
void Wifi_SysTick_Isr()
{
    //Check if Data is Paused
    if((IO_status_flag.Timer_Running) && (IO_status_flag.enable_dequeue==WIFI_TRUE) /*&& ((tickcount++) >= PROCESS_WIFI_TIMER)*/)
      {
          Process_WiFi();
      }

    if(WiFi_Control_Variables.resume_receive_data == WIFI_TRUE)
      {
          if(is_half_empty(&wifi_instances.big_buff))
            {
                WiFi_Control_Variables.resume_receive_data = WIFI_FALSE;
                Receive_Data();
            }
      }

    if(WiFi_Control_Variables.Standby_Timer_Running) // module is in sleep and after expiry RX will be conf as EXTI
      {
          if((WiFi_Counter_Variables.standby_time++) >= EXTI_CONF_TIMER)
            {
                WiFi_Control_Variables.Standby_Timer_Running=WIFI_FALSE;
                WiFi_Counter_Variables.standby_time = 0;
            }
      }
#if 0
    if(WiFi_Control_Variables.enable_timeout_timer)     // module will timeout when no response from server received
      {
          WiFi_Counter_Variables.timeout_tick++;
          //wait for 20 seconds before timeout
          if(WiFi_Counter_Variables.timeout_tick > 20000)       //wait for 20s before timeout
            {
                #if DEBUG_PRINT
                  printf("\r\n Timeout! No response received.\r\n");
                #endif
                WiFi_Counter_Variables.timeout_tick         = 0;
                WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                WiFi_Counter_Variables.AT_RESPONSE          = WiFi_AT_CMD_RESP_ERROR; // Timeout if no response received.
                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                 //re-enable event Q after 200ms
                WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
            }
      }
#endif

    /*A Resume WIND:70 has come and triggered this
    So checking here if after that resume we fall back to sleep (another WIND69) within SLEEP_RESUME_PREVENT time.
    If yes, we assume it is a false resume and hence do nothing and go back to sleep
    If no WIND69 (going into sleep) has come, we can assume the resume was genuine and then enable the callback
    */
    if((WiFi_Control_Variables.Deep_Sleep_Timer) && (WiFi_Counter_Variables.sleep_count++) >= SLEEP_RESUME_PREVENT)
      {
          if(WiFi_Control_Variables.Deep_Sleep_Enabled == WIFI_TRUE)//which means we have received another WIND69 in the 2 seconds
            {
                //do nothing, go back to sleep
                WiFi_Control_Variables.Deep_Sleep_Enabled = WIFI_TRUE;
                IO_status_flag.WiFi_WIND_State = Deep_Sleep_Callback;
            }
          else if (WiFi_Control_Variables.Deep_Sleep_Enabled == WIFI_FALSE) //which means we have not received any WIND69 during the last 2 seconds
            {
                //enable the user callback as it is a genuine WIND70
               IO_status_flag.WiFi_WIND_State = Deep_Sleep_Callback;
            }
          Stop_DeepSleep_Timer();
      }
}

/**
* @brief  WiFi_HAL_UART_TxCpltCallback
*         Tx Transfer completed callback
* @param  UsartHandle: UART handle 
* @retval None
*/
void WiFi_HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandleArg)
{
#ifdef WIFI_USE_VCOM
  if (UartHandleArg == UartMsgHandle)
    console_echo_ready = SET;
#else
  /* Set transmission flag: transfer complete */
  IO_status_flag.TxUartReady = SET; 
#endif
}

/**
* @brief  WiFi_HAL_UART_RxCpltCallback
*         Rx Transfer completed callback
* @param  UsartHandle: UART handle 
* @retval None
*/
void WiFi_HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandleArg)
{
#ifdef WIFI_USE_VCOM
  if (UartHandleArg==&UartWiFiHandle)
#endif  
  {
    #ifndef WIFI_USE_VCOM
      WiFi_Control_Variables.Uartx_Rx_Processing = WIFI_FALSE;
      Stop_Timer();
      __disable_irq();
      push_buffer_queue(&wifi_instances.big_buff, WiFi_Counter_Variables.uart_byte);
      __enable_irq();
      Start_Timer();
    #else
        console_push_ready = SET;
    #endif

    #ifndef WIFI_USE_VCOM
      #ifdef SPWF01
          if(is_half_full(&wifi_instances.big_buff))
            {
              WiFi_Control_Variables.resume_receive_data = WIFI_TRUE;
              HAL_GPIO_WritePin(WiFi_USART_RTS_GPIO_PORT, WiFi_USART_RTS_PIN, GPIO_PIN_SET);//De-assert RTS
            } 
          else
            {
              if(WiFi_Control_Variables.AT_Cmd_Processing == WIFI_FALSE)
                {
                  //call Rx only if TX is not under processing (AT command)
                  if(HAL_UART_Receive_IT(&UartWiFiHandle, (uint8_t *)WiFi_Counter_Variables.uart_byte, 1) !=HAL_OK)
                    {
                      #if DEBUG_PRINT 
                      printf("HAL_UARTx_Receive_IT Error");
                      #endif
                    }
                  else 
                    {
                      WiFi_Control_Variables.Uartx_Rx_Processing = WIFI_TRUE;
                    }
                }
            }
      #elif defined(SPWF04) && defined(CONSOLE_UART_ENABLED)
          if(WiFi_Control_Variables.AT_Cmd_Processing == WIFI_FALSE)
            {
              //call Rx only if TX is not under processing (AT command)
              if(HAL_UART_Receive_IT(&UartWiFiHandle, (uint8_t *)WiFi_Counter_Variables.uart_byte, 1) !=HAL_OK)
                {
                  #if DEBUG_PRINT 
                  printf("HAL_UARTx_Receive_IT Error");
                  #endif
                }
              else 
                {
                  WiFi_Control_Variables.Uartx_Rx_Processing = WIFI_TRUE;
                }
            }
      #endif //SPWF01
    #endif  // WIFI_USE_VCOM
  }
#ifdef WIFI_USE_VCOM
  else
    {
      console_send_char[0] = console_input_char[0];
      console_input();
      console_send_ready = SET;
      HAL_UART_Transmit_IT(&UartWiFiHandle, (uint8_t*)console_send_char, 1);
    }
#endif
}  
  
/**
* @brief  USART_Receive_AT_Resp
*         Receive and check AT cmd response
* @param  None
* @retval WiFi_Status_t : Response of AT cmd  
*/

WiFi_Status_t USART_Receive_AT_Resp( )
{
  while(IO_status_flag.AT_Response_Received != WIFI_TRUE) {
		__NOP(); //nothing to do
	}
  IO_status_flag.AT_Response_Received = WIFI_FALSE;
  return WiFi_Counter_Variables.AT_RESPONSE;
}

/**
  * @brief  UART error callbacks
  * @param  UsartHandle: UART handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void WiFi_HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
    Error_Handler();
}

/**
* @brief  Process_WiFi
*         Pop a byte from the circular buffer and send the byte for processing
*         This function should be called from main or should be run with a periodic timer
* @param  None
* @retval None
*/
void Process_WiFi(void)
{
#if defined(CONSOLE_UART_ENABLED)
    __disable_irq();
    WiFi_Counter_Variables.temp = pop_buffer_queue(&wifi_instances.big_buff);   //contents of temp(pop_buffer) will not change till another de-queue is made
    __enable_irq();

    if(WiFi_Counter_Variables.temp!=NULL) 
      {
        Process_Buffer(WiFi_Counter_Variables.temp);
      }

   if(WiFi_Control_Variables.event_deQ_x_wind64)//if de-Q is stopped due to WIND64 wait
     {
       WiFi_Counter_Variables.wind64_DQ_wait++;//1ms for each count
       if(WiFi_Counter_Variables.wind64_DQ_wait>50)//wait for 50ms for example
         {
           WiFi_Counter_Variables.wind64_DQ_wait=0;
           WiFi_Control_Variables.event_deQ_x_wind64 = WIFI_FALSE;
           //re-enable event Q after 50ms
           WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
         }
     }
#endif
}


/**
* @brief  Process_Dequeued_Wind_Indication
*         Process Wind Indication after popping from Queue
* @param  L_DeQued_wifi_event popped event contents
* @retval None
*/

void Process_DeQed_Wind_Indication(wifi_event_TypeDef * L_DeQued_wifi_event)
{
  switch(L_DeQued_wifi_event->wind)
  {
    case Heap_Too_Small:
      IO_status_flag.WiFi_WIND_State = HeapTooSmall;
      break;
    case WiFi_Hardware_Dead:
      IO_status_flag.WiFi_WIND_State = WiFiHWFailure;
      break;
    case Hard_Fault:
      IO_status_flag.WiFi_WIND_State = HardFault;
      break;   
    case StackOverflow:
      IO_status_flag.WiFi_WIND_State = StackOverFlow;
      break;
    case MallocFailed:
      IO_status_flag.WiFi_WIND_State = Mallocfailed;
      break;
    case Error:
      IO_status_flag.WiFi_WIND_State = InitFailure;
      break;
    case WiFi_PS_Mode_Failure:
      IO_status_flag.WiFi_WIND_State = PS_Mode_Failure;
      break;
    case WiFi_Signal_LOW:
      IO_status_flag.WiFi_WIND_State = WiFiSignalLOW;
      break;
    case JOINFAILED :
      IO_status_flag.WiFi_WIND_State = WiFiJoinFailed;
      break;
    case SCANBLEWUP:
      IO_status_flag.WiFi_WIND_State = WiFiScanBlewUp;
      break;
    case SCANFAILED:
      IO_status_flag.WiFi_WIND_State = WiFiScanFailed;
      break;
    case WiFi_Up:
#ifdef SPWF04
      WiFi_Counter_Variables.wifi_up ++;
      if(WiFi_Counter_Variables.wifi_up == 2) {
        IO_status_flag.WiFi_WIND_State = WiFiUp;
        WiFi_Counter_Variables.wifi_up = 0; 
      }
#else
      IO_status_flag.WiFi_WIND_State = WiFiUp;
#endif
      break;
    case WiFi_Started_MiniAP_Mode:
      IO_status_flag.WiFi_WIND_State = WiFiStarted_MiniAPMode;
      break;
    case Start_Failed :
      IO_status_flag.WiFi_WIND_State = StartFailed;  
      break;
    case WiFi_EXCEPTION :
      IO_status_flag.WiFi_WIND_State = WiFiException;
      break;
    case WiFi_Hardware_Started :
      IO_status_flag.wifi_ready++;
      IO_status_flag.WiFi_Enabled = WIFI_TRUE;
      IO_status_flag.WiFi_WIND_State = WiFiHWStarted;
      /*If this is a start-up after standby*/
      if(WiFi_Control_Variables.trigger_wakeup_callback == WIFI_TRUE)
        {
          WiFi_Control_Variables.trigger_wakeup_callback = WIFI_FALSE;
          WiFi_Control_Variables.Standby_Enabled = WIFI_FALSE;
          IO_status_flag.WiFi_WIND_State = standby_resume_callback;
        }      
      break;
    case Scan_Complete:
      WiFi_Control_Variables.Scan_Ongoing = WIFI_FALSE;
      break;
    case WiFi_UNHANDLED_IND:
      IO_status_flag.WiFi_WIND_State = WiFiUnHandledInd;
      break;
    case WiFi_Powered_Down:
      IO_status_flag.WiFi_Enabled = WIFI_FALSE;      
      //wifi_ready = 0;
//      IO_status_flag.WiFi_WIND_State.WiFiHWStarted = WIFI_FALSE;
      IO_status_flag.WiFi_WIND_State = WiFiPowerDown;
      break;
    case WiFi_Deauthentication:
      IO_status_flag.WiFi_WIND_State = WiFiDeauthentication;
      break;
    case WiFi_Disassociation:
      wifi_connected = 0; 
      IO_status_flag.WiFi_WIND_State = WiFiDisAssociation;
      break;  
    case RX_MGMT:
      IO_status_flag.WiFi_WIND_State = WiFiRXMgmt;
      break;
    case RX_DATA:
      IO_status_flag.WiFi_WIND_State = WiFiRXData;
      break;
    case RX_UNK:
      IO_status_flag.WiFi_WIND_State = WiFiRxUnk;
      break;
    case SockON_Data_Pending:
      /* +WIND:55:Pending Data:%d:%d */
      if (WiFi_Control_Variables.enable_sock_read == WIFI_TRUE)
        {       
            #ifdef SPWF04
              //printf ("\nAlert!\r\n");
              break;//in case of SPWF4 if read is ongoing no need to queue additional WIND55
            #else
              #if DEBUG_PRINT
                printf ("\nAlert!\r\n");
              #endif
              WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
              WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
              //break;
            #endif
        }
#ifdef SPWF04
      if(L_DeQued_wifi_event->server_id==9)//this is a client data pending event
      {
          WiFi_Counter_Variables.sockon_query_id = L_DeQued_wifi_event->socket_id;
          if(open_sockets[WiFi_Counter_Variables.sockon_query_id])
            {
                WiFi_Control_Variables.enable_query = WIFI_TRUE;
                WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
            }
      }
      else //this is a server data pending event
      {
          WiFi_Counter_Variables.sockdon_query_id = L_DeQued_wifi_event->server_id;
          WiFi_Counter_Variables.sockon_query_id = L_DeQued_wifi_event->socket_id;
          if(open_server_sockets[WiFi_Counter_Variables.sockdon_query_id])
            {
                //WiFi_Control_Variables.data_pending_sockD = WIFI_TRUE;//switch on this only when we have something to read on a query
                WiFi_Control_Variables.enable_server_query = WIFI_TRUE;
                WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
            }
      }  
      //wind_55_in_Q = WIFI_FALSE;
      break;
#else
      WiFi_Counter_Variables.sockon_query_id = L_DeQued_wifi_event->socket_id;
      if(open_sockets[WiFi_Counter_Variables.sockon_query_id])
        {
            WiFi_Control_Variables.enable_query = WIFI_TRUE;
            WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
        }
      break;
#endif
    case SockON_Server_Socket_Closed:
      WiFi_Counter_Variables.client_socket_close_id = L_DeQued_wifi_event->socket_id;
      WiFi_Control_Variables.Client_Socket_Close_Cmd = WIFI_TRUE;
      break;
#ifdef SPWF01 
    case SockD_Pending_Data:
      WiFi_Counter_Variables.number_of_bytes = L_DeQued_wifi_event->data_length;
      WiFi_Control_Variables.data_pending_sockD = WIFI_TRUE;
      WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
      //WiFi_Control_Variables.enable_receive_data_chunk = WIFI_TRUE;       // read data in chunk now from ring buffer

      if(!IO_status_flag.data_mode)
        {
          if(L_DeQued_wifi_event->wind64_pending_packet_no == 1)
            {   //If this is the first WIND64 pending event de-Q'ed
                WiFi_Control_Variables.switch_by_default_to_command_mode = WIFI_FALSE; //we don't want to switch back to command mode after changing to data mode here
                WiFi_switch_to_data_mode();     //switch by default
            }
          else
            {
                WiFi_Control_Variables.data_pending_sockD = WIFI_FALSE;
                WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE; 
            }
        }
      else //already data is coming from previous WIND:64
        {
            WiFi_Control_Variables.enable_receive_data_chunk = WIFI_TRUE;
            WiFi_Counter_Variables.last_process_buffer_index =5;
            WiFi_Control_Variables.enable_sock_read = WIFI_TRUE;//n
            WiFi_Counter_Variables.SockON_Data_Length = WiFi_Counter_Variables.number_of_bytes;
        }
      break;
#else
  case Remote_Configuration:
      break;
#endif
    case Incoming_socket_client:
      WiFi_Control_Variables.Client_Connected = WIFI_TRUE;
      wifi_client_connected=1;  //Set this so that the callback can be made to the user
      break;
    case Outgoing_socket_client:
#if defined(SPWF04)
      // for every outgoing client, close the client on server socket(AT+S.SOCKDC=<sid>,<cid>)
      Queue_Server_Close_Event(L_DeQued_wifi_event->server_id,L_DeQued_wifi_event->socket_id);
#endif
      WiFi_Control_Variables.Client_Disconnected = WIFI_TRUE;
      wifi_client_disconnected=0;//Set this so that the callback can be made to the user
      wifi_client_connected = 0;
      break;
    case SockD_Dropping_Data:
      IO_status_flag.WiFi_WIND_State = WiFiSockdDataLost;
      break;
    case Going_Into_Standby:
      WiFi_Control_Variables.Standby_Enabled = WIFI_TRUE;
      break;
    case Resuming_From_Standby:
      WiFi_Control_Variables.Standby_Enabled = WIFI_FALSE;
       IO_status_flag.WiFi_WIND_State = standby_resume_callback;
      break;    
    case Going_Into_DeepSleep:
      WiFi_Control_Variables.Deep_Sleep_Enabled = WIFI_TRUE;    
      break;
    case Resuming_From_DeepSleep:
      WiFi_Control_Variables.Deep_Sleep_Enabled = WIFI_FALSE;
      Start_DeepSleep_Timer();
      break;
    default:
      break;
  }
}

/**
* @brief  Queue_Http_Get_ Event
*         Queue an HTTP-Request Event (GET/POST)
* @param  hostname hostname for HTTP-GET/POST
* @param  path path for HTTP-GET
* @param  port_number port_number for HTTP-GET
* @param  pURL_path full URL for HTTP-POST
* @retval None
*/

void Queue_Http_Event(uint8_t * hostname, uint8_t * path, uint32_t port_number, uint8_t http_ind) 
{
   Wait_For_Sock_Read_To_Complete();
   
      WiFi_Counter_Variables.curr_hostname = hostname;
      WiFi_Counter_Variables.curr_path = path;
      WiFi_Counter_Variables.curr_port_number = port_number;
      WiFi_Counter_Variables.http_ind = http_ind;

    wifi_instances.wifi_event.event = WIFI_HTTP_EVENT;
    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
     __enable_irq();
    
    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief  Queue_Client_Write_Event
*         Queues a Client Socket write event.
* @param  sock_id socket ID to write to
* @param  DataLength length of the data to be written
* @param  pData pointer to data
* @retval None
*/
void Queue_Client_Write_Event(uint8_t sock_id, uint16_t DataLength, char * pData)
{
    Wait_For_Sock_Read_To_Complete();
    WiFi_Counter_Variables.curr_DataLength = DataLength;
    WiFi_Counter_Variables.curr_data = pData;
    WiFi_Counter_Variables.curr_sockID = sock_id;

    wifi_instances.wifi_event.event = WIFI_CLIENT_SOCKET_WRITE_EVENT;
    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief Queue_Wifi_File_Event
*        Queue a File Event
* @param HostName : hostname
* @param FileName : filename within host
* @param port_number : port number to connect to
* @param offset   : The byte from where the file is printed (default: 0)
* @param Length   : Number of bytes that are to be printed (default: Filesize-offset)
* @retval None
*/
void Queue_Wifi_File_Event(uint8_t * HostName, uint8_t * FileName, uint32_t port_number,int16_t offset, int16_t length)
{
    Wait_For_Sock_Read_To_Complete();
    wifi_instances.wifi_event.event         = WIFI_FILE_EVENT;
    WiFi_Counter_Variables.curr_filename    = FileName;
    WiFi_Counter_Variables.curr_hostname    = HostName;
    WiFi_Counter_Variables.curr_port_number = port_number;
    WiFi_Counter_Variables.datalen          = length;
    WiFi_Counter_Variables.offset           = offset;


    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief Queue_File_Event
*        Queue a File Event
* @param FileName : filename within host
* @param Mod_Filename : Modified Filename
* @param Volume : Indicates the memory volume. Default: 0.  <1:User Flash 0:SD Card>
*        Erase  : Default: 0. when 1, the specified volume is erased.
* @retval None
*/
void Queue_File_Event(char * FileName, char *Mod_Filename, int8_t volume, int8_t erase)
{
    Wait_For_Sock_Read_To_Complete();
    WiFi_Counter_Variables.curr_filename      = (uint8_t*)FileName;

    WiFi_Counter_Variables.mod_filename       = (uint8_t*)Mod_Filename;
    WiFi_Counter_Variables.volume             = volume;
    WiFi_Counter_Variables.erase              = erase;

    wifi_instances.wifi_event.event = WIFI_FILE1_EVENT;
    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}



/**
* @brief Queue_Wifi_FW_Update_Event
*        Queue a Firmware update Event
* @param hostname hostname
* @param filename_path filename and path within host
* @param port_number port number to connect to
* @retval None
*/
void Queue_Wifi_FW_Update_Event(uint8_t * hostname, uint8_t * filename_path, uint32_t port_number)
{
    Wait_For_Sock_Read_To_Complete();
    wifi_instances.wifi_event.event = WIFI_FW_UPDATE_EVENT;
    WiFi_Counter_Variables.curr_filename = filename_path;
    WiFi_Counter_Variables.curr_hostname = hostname;
    WiFi_Counter_Variables.curr_port_number = port_number;

    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief Queue_Client_Open_Event
*        Queue a Client Open Event
* @param hostname hostname
* @param port_number port number to connect to
* @param protocol protocol required to connect to server (t for TCP socket, u for UDP socket, s for secure socket)
* @retval void
*/
void Queue_Client_Open_Event(uint8_t * hostname, uint32_t port_number, uint8_t * protocol)
{
    Wait_For_Sock_Read_To_Complete();
    WiFi_Counter_Variables.curr_hostname = hostname;
    WiFi_Counter_Variables.curr_port_number = port_number;
    WiFi_Counter_Variables.curr_protocol = protocol;

    wifi_instances.wifi_event.event = WIFI_CLIENT_SOCKET_OPEN_EVENT;
     __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}


/**
* @brief Queue_Server_Open_Event
*        Queue a Server Open Event
* @param port_number port number to connect to
* @param protocol protocol required to connect to server (t for TCP socket, u for UDP socket, s for secure socket)
* @retval void
*/
void Queue_Server_Open_Event(uint32_t port_number, uint8_t * protocol)
{
    Wait_For_Sock_Read_To_Complete();
    WiFi_Counter_Variables.curr_port_number = port_number;
    WiFi_Counter_Variables.curr_protocol = protocol;

    wifi_instances.wifi_event.event = WIFI_SERVER_SOCKET_OPEN_EVENT;
     __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief Queue_Client_Close_Event
*        Queue a Client Close Event
* @param sock_id socket ID to close
* @retval void
*/
void Queue_Client_Close_Event(uint8_t sock_id)
{
    Wait_For_Sock_Read_To_Complete();
    
    wifi_instances.wifi_event.event = WIFI_CLIENT_SOCKET_CLOSE_EVENT;
    wifi_instances.wifi_event.socket_id = sock_id;
     __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief Queue_Server_Close_Event
*        Queue a server close event
* @param server_id: ID of the server
* @param client_id: ID of the client
* @return void
*/
void Queue_Server_Close_Event(int8_t server_id, int8_t client_id)
{
    Wait_For_Sock_Read_To_Complete();
    wifi_instances.wifi_event.server_id = server_id;
    wifi_instances.wifi_event.socket_id = client_id;

    wifi_instances.wifi_event.event = WIFI_SERVER_SOCKET_CLOSE_EVENT;
    __disable_irq();
    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
    __enable_irq();

    reset_event(&wifi_instances.wifi_event);
}

/**
* @brief Wait_For_Sock_Read_To_Complete
*        Wait till sock read is over and the OK of read arrives
* @param None
* @retval None
*/
void Wait_For_Sock_Read_To_Complete(void)
{
  //wait if read is ongoing or read OK is yet to arrive
  while(IO_status_flag.sock_read_ongoing == WIFI_TRUE || 
       (IO_status_flag.prevent_push_OK_event == WIFI_TRUE && IO_status_flag.client_socket_close_ongoing != WIFI_TRUE)) // to make sure the prevent_push_OK_event is of socket read and not of socket close.
    {
        __NOP(); //nothing to do
    }
}

/**
* @brief  Reset_AT_CMD_Buffer
*         Clear USART2 Rx buffer and Wi-Fi AT cmd buffer
* @param  None
* @retval None
*/
void Reset_AT_CMD_Buffer()
{
  memset(WiFi_AT_Cmd_Buff, 0x00, sizeof WiFi_AT_Cmd_Buff); 
}

#ifdef  USE_FULL_ASSERT

/**
* @brief  Reports the name of the source file and the source line number
*         where the assert_param error has occurred.
* @param  file: pointer to the source file name
* @param  line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
    {
      __NOP(); //nothing to do
    }
}

#endif

/**
* @brief  Read_WiFi_Mode
*         Read Wi-Fi mode 0: idle,1 =STA,2 =IBSS,3 =MiniAP
* @param  string : return wifi mode type
* @retval return status of AT cmd request
*/
WiFi_Status_t Read_WiFi_Mode(char *string)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  char *mode = "wifi_mode";
  char *pStr;
  
    /* AT+S.GCFG=wifi_mode */
  Reset_AT_CMD_Buffer();
  
  /* AT : send AT command */  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_GET_CONFIGURATION_VALUE,mode);  
  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
  
  pStr = (char *) strstr((const char *)&WiFi_Counter_Variables.get_cfg_value,"wifi_mode = ");
  if(pStr != NULL)
    {
      string[0] = *(pStr + 12) ;
    }

  return status ;
}

/**
* @brief  Write_WiFi_SSID
*         Store SSID in flash memory of WiFi module
* @param  string : pointer of SSID
* @retval return status of AT cmd request
*/
WiFi_Status_t Write_WiFi_SSID(char *string)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  Reset_AT_CMD_Buffer(); 
  
  /* AT+S.SSIDTXT=abcd <ExampleSSID> //set SSID */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_SSID,string);  
  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }

  /* AT&W :Save the settings on the flash memory */
  Reset_AT_CMD_Buffer();
  Save_Current_Setting();

  return status;
}

/**
* @brief  Write_WiFi_SecKey
*         Store security key in flash memory of WiFi module
* @param  string : pointer of security key
* @retval return status of AT cmd request
*/
WiFi_Status_t Write_WiFi_SecKey(char *string)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  Reset_AT_CMD_Buffer(); 
  
  /* AT+S.SCFG=wifi_wpa_psk_text,helloworld : set password */
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SCFG=wifi_wpa_psk_text,%s\r",string);
  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
  
  /* AT&W :Save the settings on the flash memory */
  Reset_AT_CMD_Buffer();
  Save_Current_Setting();
  
  return status;     
}

/**
* @brief  PrintErrorMsg
*         Print error message on UART terminal
* @param  None
* @retval None
*/
void PrintErrorMsg (void)
{
  Print_Msg("error in AT cmd",sizeof("error in AT cmd"));
}

/**
  * @brief  Print_Msg
  *         Print messages on UART terminal
  * @param  msgBuff : Contains data that need to be print
  * @param  length  : leghth of the data
  * @retval None
  */
void Print_Msg(char * msgBuff,uint8_t length)
{

}

/**
* @brief  Error_Handler
*         This function is executed in case of error occurrence.
* @param  None
* @retval None
*/
void Error_Handler(void)
{
  /* Turn LED2 on */
  BSP_LED_On(LED2);
  //The following while(1) is commented as it prevents standby functionality
  /*while(1)
  {
    //Error if LED2 is slowly blinking (1 sec. period)
    BSP_LED_Toggle(LED2); 
    HAL_Delay(1000); 
  } */ 
#ifdef SPWF01
  Receive_Data();
#endif
}

/**
* @brief  USART_Transmit_AT_Cmd
*         send AT cmd on UART port of wifi module.
* @param  size size of the AT command
* @retval WiFi_Status_t : status of AT cmd
*/
WiFi_Status_t USART_Transmit_AT_Cmd(uint16_t size)
{
  //Check for Hardware Started
  if(IO_status_flag.WiFi_Enabled == WIFI_FALSE && IO_status_flag.radio_off == WIFI_FALSE) 
    return WiFi_NOT_READY;
  //Check for Deep-Sleep or Standby Mode, return error if true
  if (WiFi_Control_Variables.Standby_Enabled == WIFI_TRUE || WiFi_Control_Variables.Deep_Sleep_Enabled == WIFI_TRUE)
    return WiFi_IN_LOW_POWER_ERROR;

  WiFi_Control_Variables.AT_Cmd_Processing = WIFI_TRUE;//Stop Any Rx between the TX call

  if (size == 0)
    {
        printf("ERROR in USART_Transmit_AT_Cmd!");
        return WiFi_UNHANDLED_IND_ERROR;
    }

#if defined(USART3_INT_MODE)
  if(HAL_UART_Transmit_IT(&UartWiFiHandle, (uint8_t *)WiFi_AT_Cmd_Buff, size)!= HAL_OK)
    {
      Error_Handler();
      return WiFi_HAL_UART_ERROR;
    }
  while (IO_status_flag.UartReady != SET)
    {
      __NOP(); //nothing to do
    }
  IO_status_flag.UartReady = RESET; 

#elif defined(USART3_POLLING_MODE)
  //while(Uartx_Rx_Processing!=WIFI_FALSE);
  if(HAL_UART_Transmit(&UartWiFiHandle, (uint8_t *)WiFi_AT_Cmd_Buff, size, 1000)!= HAL_OK)
    {
      Error_Handler();
      #if DEBUG_PRINT
      printf("HAL_UART_Transmit Error");
      #endif
      return WiFi_HAL_UART_ERROR;
    }

#else
 #error "Please select USART mode in your application (in wifi_module.h file)"
#endif
 
  WiFi_Control_Variables.AT_Cmd_Processing = WIFI_FALSE;//Re-enable Rx for UART
  if(WiFi_Control_Variables.Uartx_Rx_Processing == WIFI_FALSE)
    Receive_Data();//Start receiving Rx from the UART again, if and only if it was stopped in the previous Uartx_Rx_Handler
  return WiFi_MODULE_SUCCESS;
}

/**
* @brief  Start_DeepSleep_Timer
*         start the deep sleep timer.
* @param  None
* @retval void
*/
void Start_DeepSleep_Timer(void)
{
  WiFi_Control_Variables.Deep_Sleep_Timer = WIFI_TRUE;
  WiFi_Counter_Variables.sleep_count = 0;
}

/**
* @brief  Stop_DeepSleep_Timer
*         stop the deep sleep timer.
* @param  None
* @retval void
*/
void Stop_DeepSleep_Timer()
{
  WiFi_Control_Variables.Deep_Sleep_Timer = WIFI_FALSE;
  WiFi_Counter_Variables.sleep_count = 0;
}

/**
* @brief  WiFi_switch_to_command_mode
*         switch to command mode from data mode
* @param  None
* @retval None
*/
void WiFi_switch_to_command_mode(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  /* AT+S.*/  
  Reset_AT_CMD_Buffer();  

  sprintf((char*)WiFi_AT_Cmd_Buff,AT_DATA_TO_CMD_MODE);   //Notice the lower case

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    //nothing to do
  }
}

/**
* @brief  WiFi_switch_to_data_mode
*         switch to data mode from command mode
* @param  None
* @retval None
*/
void WiFi_switch_to_data_mode(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  /* AT+S.*/  
  Reset_AT_CMD_Buffer();    

  sprintf((char*)WiFi_AT_Cmd_Buff,AT_CMD_TO_DATA_MODE);   //Notice the upper case

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));

  if(status == WiFi_MODULE_SUCCESS)
    {
      //nothing to do
    }
}

/**
* @brief  Attention_Cmd
*         Attention command
* @param  None
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t Attention_Cmd()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Reset_AT_CMD_Buffer(); 

  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_ATTENTION);  

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
  return status; 
}


/**
* @brief  Display_Help_Text
*         this function will print a list of all commands supported with a brief help text for each cmd
* @param  None
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t Display_Help_Text()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  Reset_AT_CMD_Buffer();

  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_HELP_TEXT);  

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
  return status; 
}

/**
* @brief  GET_Configuration_Value
*         Get a wifi configuration value from the module
* @param  sVar_name : Name of the config variable
*         aValue    : value of config variable to be returned to user
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t GET_Configuration_Value(char* sVar_name,uint32_t *aValue)
{
  int cfg_value_length;
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Reset_AT_CMD_Buffer(); 
#if defined(CONSOLE_UART_ENABLED)
  /* AT : send AT command */  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_GET_CONFIGURATION_VALUE,sVar_name);   

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      cfg_value_length = strlen((const char*)WiFi_Counter_Variables.get_cfg_value);
      memcpy(aValue,WiFi_Counter_Variables.get_cfg_value,cfg_value_length);   //copy user pointer to get_cfg_value
      memset(WiFi_Counter_Variables.get_cfg_value, 0x00,cfg_value_length);
    }
#else
  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GCFG=%s",sVar_name);
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  IO_status_flag.AT_event_processing = WIFI_GCFG_EVENT;
  status = USART_Receive_AT_Resp( );
  if(status == WiFi_MODULE_SUCCESS)
    {
      cfg_value_length = strlen((const char*)WiFi_Counter_Variables.get_cfg_value);
      memcpy(aValue,WiFi_Counter_Variables.get_cfg_value,cfg_value_length);   //copy user pointer to get_cfg_value
      memset(WiFi_Counter_Variables.get_cfg_value, 0x00,cfg_value_length);
    }
#endif    
  return status; 
}

/**
* @brief  SET_Configuration_Addr
*         Get a wifi configuration address from the module
* @param  sVar_name : Name of the config variable
*         addr    : value of config address to be returned to user
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t SET_Configuration_Addr(char* sVar_name,char* addr)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Reset_AT_CMD_Buffer(); 
#if defined(CONSOLE_UART_ENABLED)
  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_CONFIGURATION_ADDRESS,sVar_name,addr);  

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
#else
  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SCFG=%s,%s",sVar_name,addr);
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  IO_status_flag.AT_event_processing = WIFI_GCFG_EVENT;
  status = USART_Receive_AT_Resp( );
#endif  
  return status;
}

/**
* @brief  SET_Configuration_Value
*         SET the value of configuration variable
* @param  sVar_name : Name of the config variable
*         aValue    : value of config variable
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t SET_Configuration_Value(char* sVar_name,uint32_t aValue)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Reset_AT_CMD_Buffer(); 

#if defined(CONSOLE_UART_ENABLED)
  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_CONFIGURATION_VALUE,sVar_name,(int)aValue);  

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
#else
  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SCFG=%s,%d",sVar_name,(int)aValue);
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
#endif
  
  return status; 
}

/**
* @brief  SET_SSID
*         SET SSID in flash memory of Wi-Fi module
* @param  ssid : pointer of SSID
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t SET_SSID(char* ssid)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  Reset_AT_CMD_Buffer(); 

#if defined(CONSOLE_UART_ENABLED)
  /* AT+S.SSIDTXT=abcd <ExampleSSID>  */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_SSID,ssid);  

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
#else
  
  /* Set AT+S.SSIDTXT=<SSID>*/
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SSIDTXT=%s",ssid);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
#endif  
  
  return status; 
}


/**
* @brief  SET_WiFi_SecKey
*         SET wifi security key
* @param  seckey : pointer of security key
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t SET_WiFi_SecKey(char* seckey)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  Reset_AT_CMD_Buffer(); 
  
#if defined(CONSOLE_UART_ENABLED)
  /* AT+S.SCFG=wifi_wpa_psk_text,helloworld : set password */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_SEC_KEY,seckey);  

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
#else  
  /* Set WIFI_WPA_SECURITY*/  
  status = SET_Configuration_Addr(WIFI_WPA_SECURITY, seckey);
  if(status != WiFi_MODULE_SUCCESS) return status;
#endif
return status;    
}


/**
* @brief  Restore_Default_Setting
*         Restore the factory default values of the configuration variables 
*         and writes them to non volatile storage
* @param  None
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t Restore_Default_Setting()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Reset_AT_CMD_Buffer(); 

#if defined(CONSOLE_UART_ENABLED)
  /* AT&F: restore default setting */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_RESTORE_DEFAULT_SETTING);
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.FCFG");
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = WiFi_MODULE_SUCCESS;
#endif
  
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
  return status;
}

/**
* @brief  Save_Current_Setting
*         Store the current RAM-based setting to non-volatile storage
* @param  None
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t Save_Current_Setting()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  Reset_AT_CMD_Buffer();
  /* AT&W :Save the settings on the flash memory */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SAVE_CURRENT_SETTING);
  
#if defined(CONSOLE_UART_ENABLED)
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
    }
#else
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
#endif
  
  return status; 
}

/**
* @brief  ResetBuffer
*         Reset receive data/indication msg buffer
* @param  None
* @retval None
*/
void ResetBuffer()
{  
  
}

/**
* @brief  config_init_value
*         initalize config values before reset
* @param  sVar_name : Name of the config variable
*         aValue    : value of config variable
* @retval None
*/
WiFi_Status_t config_init_value(char* sVar_name,uint8_t aValue)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  Reset_AT_CMD_Buffer();   
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_CONFIGURATION_VALUE,sVar_name,aValue);
  if(HAL_UART_Transmit(&UartWiFiHandle, (uint8_t *)WiFi_AT_Cmd_Buff, strlen((char*)WiFi_AT_Cmd_Buff),1000)!= HAL_OK)
    {
      Error_Handler();    
      return WiFi_HAL_UART_ERROR;
    }
  
  status = WaitForResponse(AT_RESP_LEN_OK);
  return status;
}

/**
* @brief  config_init_addr
*         initalize config strings/addresses before reset
* @param  sVar_name : Name of the config variable
*         addr    : value of config address to be returned to user
* @retval None
*/
WiFi_Status_t config_init_addr(char* sVar_name,char* addr)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  Reset_AT_CMD_Buffer();   
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_CONFIGURATION_ADDRESS,sVar_name,addr);
  if(HAL_UART_Transmit(&UartWiFiHandle, (uint8_t *)WiFi_AT_Cmd_Buff, strlen((char*)WiFi_AT_Cmd_Buff),1000)!= HAL_OK)
    {
      Error_Handler();    
      return WiFi_HAL_UART_ERROR;
    }
  
  status = WaitForResponse(AT_RESP_LEN_OK);
  return status;

}


/**
* @brief  WaitForResponse
*         Wait for OK response
* @param  alength length of the data to be received
* @retval None
*/
WiFi_Status_t WaitForResponse(uint16_t alength)
{
  uint8_t USART_RxBuffer[64];    //This buffer is only used in the Init phase (to receive "AT-S.OK")
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  if(alength <= RxBufferSize)
  {
    if(HAL_UART_Receive(&UartWiFiHandle, (uint8_t *)USART_RxBuffer, alength,1000)!= HAL_OK)
      {
        Error_Handler();
        return WiFi_HAL_UART_ERROR;
      }
    if(((strstr((const char *)&USART_RxBuffer,"AT-S.OK"))) == NULL)
      {
        return WiFi_AT_CMD_RESP_ERROR;
      }
  }
  return status;  
}
/**** Wi-Fi indication call back *************/
__weak void ind_wifi_warning(WiFi_Status_t warning_code)
{
}
	
__weak void ind_wifi_error(WiFi_Status_t error_code)
{
}

__weak void ind_wifi_connection_error(WiFi_Status_t status_code)
{
}

__weak void ind_wifi_connected(void)
{
}
	
__weak void ind_wifi_ap_ready(void)
{
}

__weak void ind_wifi_ap_client_joined(uint8_t * client_mac_address)
{
}

__weak void ind_wifi_ap_client_left(uint8_t * client_mac_address)
{
}

__weak void ind_wifi_on(void)
{
}

__weak void ind_wifi_packet_lost(WiFi_Status_t status_code)
{
}

__weak void ind_wifi_gpio_changed(void)
{
}

__weak void ind_wifi_socket_data_received(int8_t server_id, int8_t socket_id, uint8_t * data_ptr, uint32_t message_size, uint32_t chunk_size)
{
}

__weak void ind_wifi_socket_client_remote_server_closed(uint8_t *socketID)
{
}

__weak void ind_wifi_socket_server_data_lost(void)
{
}

__weak void ind_socket_server_client_joined(void)
{
}

__weak void ind_socket_server_client_left(void)
{
}

__weak void ind_wifi_http_data_available(uint8_t * data_ptr,uint32_t message_size)
{
}

__weak void ind_wifi_file_data_available(uint8_t * data_ptr)
{
}
__weak void ind_wifi_resuming(void)
{
}



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

