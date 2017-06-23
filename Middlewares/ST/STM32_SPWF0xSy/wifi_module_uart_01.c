/**
 ******************************************************************************
 * @file    wifi_module_uart_01.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    25-Nov-2016
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


#if defined(SPWF01) && defined(CONSOLE_UART_ENABLED)

extern wifi_instances_t wifi_instances;

#ifdef SPWF01
/**
* @brief  SET_Power_State
*         SET power mode of wifi module
* @param  state : power mode of wi-fi module i.e active,sleep,standby,powersave
* @retval WiFi_Status_t : status of AT cmd Request
*/
WiFi_Status_t SET_Power_State(WiFi_Power_State_t state)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

#if DEBUG_PRINT  
  printf("\r\n >>Soft Reset Wi-Fi module\r\n");
#endif

  Reset_AT_CMD_Buffer();

  /* AT : send AT command */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_POWER_STATE,state);  
//  WiFi_WIND_State.WiFiReset = WIFI_FALSE;
  IO_status_flag.WiFi_WIND_State = Undefine_state;
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status != WiFi_MODULE_SUCCESS) 
    return status;
  memset((void*)&IO_status_flag.WiFi_WIND_State,0x00,sizeof(IO_status_flag.WiFi_WIND_State)); /*reset the WIND State?*/
  /* AT+CFUN=1 //Soft reset */
  while(IO_status_flag.WiFi_WIND_State != WiFiHWStarted)
  {
    __NOP(); //nothing to do
  }
  return status;
}
#endif

/**
* @brief  Process_Buffer
*         Process and construct a Wind Line buffer
* @param  ptr: pointer to one single byte
* @retval None
*/

void Process_Buffer(uint8_t * ptr)
{ 
  static uint32_t Fillptr=0;
  static uint8_t index, chan_value;
  unsigned char rxdata = 0;
  int rssi_value = 0;
  char SocketId_No[2];
  char databytes_No[4];
  char * pStr;
  static uint8_t HTTP_Runway_Buff[6];  //Used to store the last 6 bytes in between User Callbacks during HTTP tx
  static uint8_t process_buffer[MAX_BUFFER_GLOBAL];

  rxdata =  *(ptr+0);  
  //printf(&rxdata);    //check prints for debug...to be removed or kept in DEBUG statement
  if(WiFi_Control_Variables.enable_receive_data_chunk == WIFI_FALSE)
    process_buffer[Fillptr++] = rxdata;
   reset_event(&wifi_instances.wifi_event);

    if((process_buffer[Fillptr-2]==0xD) && (process_buffer[Fillptr-1]==0xA) 
       && !WiFi_Control_Variables.enable_receive_http_response && !IO_status_flag.sock_read_ongoing)
      {
          if((strstr((const char *)process_buffer,"WIND:")) != NULL)
          {
              //end of msg received. Will not receive any other msg till we process this.
              Stop_Timer();
              #if defined (USE_STM32L0XX_NUCLEO) || (USE_STM32F4XX_NUCLEO) || (USE_STM32L4XX_NUCLEO)
              __disable_irq();
              #endif
              Process_Wind_Indication(&process_buffer[0]);
              #if defined (USE_STM32L0XX_NUCLEO) || (USE_STM32F4XX_NUCLEO) || (USE_STM32L4XX_NUCLEO)
              __enable_irq();
              #endif
              Start_Timer();

              if(!WiFi_Control_Variables.prevent_push_WIFI_event)
                {
                    __disable_irq();
                    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                     __enable_irq();
                    reset_event(&wifi_instances.wifi_event);
                }

              else if(!WiFi_Control_Variables.queue_wifi_wind_message)  //if we do not want to queue a WIND: message
                {
                    reset_event(&wifi_instances.wifi_event);
                    WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;
                    WiFi_Control_Variables.queue_wifi_wind_message = WIFI_TRUE;
                }
              if (!WiFi_Control_Variables.do_not_reset_push_WIFI_event) 
                WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;

              Fillptr=0;
              if(WiFi_Control_Variables.enable_sock_read && WiFi_Control_Variables.Q_Contains_Message)
                WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
              else
                return;
          }

          else if((strstr((const char *)process_buffer,"\r\nOK\r\n")) != NULL)
          {
              /*Now Check to which AT Cmd response this OK belongs to so that correct parsing can be done*/

              // SOCKON ID (Open a client socket)
              if(((pStr=(strstr((const char *)process_buffer,"ID: "))) != NULL))      
                {
                    SocketId_No[0]    = *(pStr + 4) ;
                    SocketId_No[1]    = *(pStr + 5) ;
                    wifi_instances.wifi_event.socket_id = (((SocketId_No[0] - '0') * 10 ) + (SocketId_No[1] - '0'));
                    wifi_instances.wifi_event.event     =  WIFI_SOCK_ID_EVENT;
                    __disable_irq();
                    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                     __enable_irq();
                    reset_event(&wifi_instances.wifi_event);
                 }

                // DATALEN from SOCKQ
               else if((pStr=(strstr((const char *)process_buffer,"DATALEN: "))) != NULL)
                 {
                    //Find the DataLength and do a socket read
                    databytes_No[0] = *(pStr + 9);
                    databytes_No[1] = *(pStr + 10);
                    databytes_No[2] = *(pStr + 11);
                    databytes_No[3] = *(pStr + 12);

                    if( databytes_No[1] == '\r')
                      {
                        WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                      }
                    else if( databytes_No[2] == '\r')
                      {
                        WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                      }
                    else if( databytes_No[3] == '\r')
                      {
                        WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                      }
                    else //it's a 4-digit number
                      {
                        WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                      }
                    if(WiFi_Counter_Variables.Socket_Data_Length != 0)
                      {
                        WiFi_Control_Variables.start_sock_read = WIFI_TRUE;
                      }
                    else if(WiFi_Counter_Variables.Socket_Data_Length == 0)  //no data remaining to be read
                      {
                        if(WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.sockon_query_id])
                          {
                            // Q socket_close event for that socket for which sock_close command could not be processed earlier due to ERROR: pending data.
                             if(open_sockets[WiFi_Counter_Variables.sockon_query_id])
                              {
                                Queue_Client_Close_Event(WiFi_Counter_Variables.sockon_query_id);
                              }
                            WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.sockon_query_id] = WIFI_FALSE;
                          }
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;  //continue popping events if nothing to read
                      }
                 }

               else if((pStr = (char *)(strstr((const char *)process_buffer," = "))) != NULL)
                 {
                    // AT command GCFG
                    wifi_instances.wifi_event.event = WIFI_GCFG_EVENT;
                    //we need to copy only the value in get_cfg_value variable
                    memcpy(WiFi_Counter_Variables.get_cfg_value, pStr+3, (strlen(pStr)-11));
                    __disable_irq();
                    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                     __enable_irq();
                    reset_event(&wifi_instances.wifi_event);
                 }

               else
                 {
                    //This is a standalone OK
                    /*Cases possible
                    - TLSCERT,TLSCERT2, TLSDOMAIN, SETTIME
                    - S.SOCKW, SOCKR, S.SOCKC, S.SOCKD (open a server socket)
                    - File Operations
                    - S.GPIOC and S.GPIOW
                    */
                    //Push a simple OK Event, if this is an OK event required to be pushed to Q 
                    if(IO_status_flag.prevent_push_OK_event)
                      {
                          //This OK is not to be handled, hence the pop action on OK completion to be done here                              
                          if(IO_status_flag.client_socket_close_ongoing) //OK received is of the sock close command
                            {
                                if(WiFi_Counter_Variables.no_of_open_client_sockets > 0)
                                  WiFi_Counter_Variables.no_of_open_client_sockets--;
                                IO_status_flag.prevent_push_OK_event                         = WIFI_FALSE;
                                open_sockets[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_FALSE;
                                //socket ID for which OK is received.
                                WiFi_Counter_Variables.closed_socket_id                      = WiFi_Counter_Variables.remote_socket_closed_id;
                                IO_status_flag.client_socket_close_ongoing                   = WIFI_FALSE;

                                //User Callback not required in case if sock_close is called by User (Only in WIND:58)
                                if(WiFi_Control_Variables.enable_SockON_Server_Closed_Callback)
                                  {
                                    // User callback after successful socket Close
                                    WiFi_Control_Variables.SockON_Server_Closed_Callback         = WIFI_TRUE;
                                  }
                                else
                                    WiFi_Control_Variables.enable_SockON_Server_Closed_Callback  = WIFI_TRUE;
                                WiFi_Control_Variables.stop_event_dequeue                        = WIFI_FALSE;
                            }
                      }

                    else
                      {
                          wifi_instances.wifi_event.event = WIFI_OK_EVENT;
                          __disable_irq();
                          push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                           __enable_irq();
                          reset_event(&wifi_instances.wifi_event);
                      }
                    IO_status_flag.prevent_push_OK_event = WIFI_FALSE;
                 }
              Fillptr=0;
              memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

              if(WiFi_Control_Variables.enable_sock_read && WiFi_Control_Variables.Q_Contains_Message)
                WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
              else
                return;
          }

         else if((((strstr((const char *)process_buffer,"ERROR"))) != NULL))
          {
              //This is an ERROR
              //There can be only ONE outstanding AT command and hence this ERROR belongs to that
              //HTTP -> ERROR: host not found
              //@TBD: Check all Errors Possible here???
              if((strstr((const char *)process_buffer,"\r\nERROR: Pending data\r\n")) != NULL) //if Error after sock close command and not 'OK'
                {
                    printf("\r\nERROR: Socket could not be closed..PENDING DATA\r\n");
                    //prevent the OK received after socket close command to be Q'ed
                    IO_status_flag.prevent_push_OK_event        = WIFI_FALSE;     
                    IO_status_flag.client_socket_close_ongoing  = WIFI_FALSE;
                    
                    //when error while user trying to close a socket, so now callback required whenever the socket gets closed.
                    if(!WiFi_Control_Variables.enable_SockON_Server_Closed_Callback)
                      {
                        //enable the SockON_Server_Closed_Callback
                        WiFi_Control_Variables.enable_SockON_Server_Closed_Callback = WIFI_TRUE;
                      }
                    //close the socket for which ERROR is received afterwards, after reading the data on that socket
                    WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_TRUE;
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                }
                /* occur when there is no client and we are trying to switch to data/command mode */
               else if(strstr((const char *)process_buffer,"\r\nERROR: Data mode not available\r\n") != NULL ||
                       strstr((const char *)process_buffer,"\r\nERROR: Command mode not available\r\n") != NULL)
                {
                     /* in case of wind 64: if error, then enable the deQueue and continue popping */
                    if(WiFi_Control_Variables.data_pending_sockD)
                      {
                        WiFi_Control_Variables.data_pending_sockD = WIFI_FALSE;
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                        WiFi_Control_Variables.enable_sock_read   = WIFI_FALSE;
                        WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
                        WiFi_Counter_Variables.chunk_size         = 0;
                        WiFi_Counter_Variables.message_size       = 0;
                      }

                }
              else
                {
                    wifi_instances.wifi_event.event = WIFI_ERROR_EVENT;
                    __disable_irq();
                    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                     __enable_irq();
                    reset_event(&wifi_instances.wifi_event);
                    if(WiFi_Control_Variables.stop_event_dequeue)
                      /*ERROR:Illegal Socket ID*/
                      WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;//continue popping events if nothing to read
                    if(WiFi_Control_Variables.enable_timeout_timer)
                      {
                        WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                        WiFi_Counter_Variables.timeout_tick = 0;
                      }
                }
              Fillptr=0;
              memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

              if(WiFi_Control_Variables.enable_sock_read && WiFi_Control_Variables.Q_Contains_Message)
                WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
              else
                return;
          }

         else if((((strstr((const char *)process_buffer,"GPIO "))) != NULL)) 
          {
              // Receive GPIO Read
              pStr = (char *) strstr((const char *)process_buffer,"= 0,");                                  
              if(pStr != NULL)
                {
                    WiFi_Counter_Variables.gpio_value = 0;
                } 
              else 
                {
                  WiFi_Counter_Variables.gpio_value = 1;   
                }
              pStr = (char *) strstr((const char *)process_buffer,"out");                                  
              if(pStr != NULL)
                {
                    WiFi_Counter_Variables.gpio_dir= 0;    //out
                }
              else
                {
                    WiFi_Counter_Variables.gpio_dir= 1;    //in
                }
              //Push GPIO Read Event on Event_Queue
              wifi_instances.wifi_event.event = WIFI_GPIO_EVENT;
              __disable_irq();
              push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
               __enable_irq();
              reset_event(&wifi_instances.wifi_event);
              Fillptr=0;
              memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

              if(WiFi_Control_Variables.enable_sock_read && WiFi_Control_Variables.Q_Contains_Message)
                WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
              else
                return;
          }

         else
           {
             //the data of wind:55 has \r\n...\r\n or before the actual data arrival there is a message having \r\n..\r\n and not matching any of the above conditions.
             if(WiFi_Control_Variables.enable_sock_read && WiFi_Control_Variables.Q_Contains_Message && !WiFi_Control_Variables.message_pending)
                {
                   /* data is D Q'ed in pop_buffer and we return from here(process_buffer()) without processing this data. Next time we enter in this procedure the data will be overe written in pop_buffer
                        To avoid this, rewinding is necessary. */
                    if(WiFi_Counter_Variables.pop_buffer_size)
                      {
                        __disable_irq();
                         rewind_buffer_queue(&wifi_instances.big_buff,WiFi_Counter_Variables.pop_buffer_size); //in this the case of rewinding past the buffer->end can happen
                         __enable_irq();
                         memset(ptr, 0x00, WiFi_Counter_Variables.pop_buffer_size);
                      }
                    //used for bypassing enable_receive_data_chunk part for the first time \r\n..\r\n is received in case of socket data
                    WiFi_Control_Variables.enable_sock_data   = WIFI_TRUE;
                    WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                    WiFi_Counter_Variables.pop_queue_length   = Fillptr;
                }

              //if in data mode, reset on \r\n
              if(!IO_status_flag.sock_read_ongoing && IO_status_flag.data_mode)
                {
                    Fillptr = 0;
                    memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                }
           }
      }

    else if (WiFi_Control_Variables.http_req_pending)   //HTTP Response Check
      {
           if((strstr((const char *)&process_buffer[0],"200 OK\r")) != NULL || (strstr((const char *)&process_buffer[0],"200 \r")) != NULL 
                  || (strstr((const char *)&process_buffer[0],"400 Bad Request\r")) != NULL || (strstr((const char *)&process_buffer[0],"400 \r")) != NULL 
                  || (strstr((const char *)&process_buffer[0],"401 Unauthorized\r")) != NULL || (strstr((const char *)&process_buffer[0],"403 Forbidden\r")) != NULL 
                  || (strstr((const char *)&process_buffer[0],"404 Not Found\r")) != NULL || (strstr((const char *)&process_buffer[0],"408 Request Timeout\r")) != NULL  
                  || (strstr((const char *)&process_buffer[0],"500 Internal Server Error\r")) != NULL  || (strstr((const char *)&process_buffer[0],"502 Bad Gateway\r")) != NULL 
                  || (strstr((const char *)&process_buffer[0],"504 Gateway Timeout\r")) != NULL)
            {
                WiFi_Control_Variables.enable_receive_http_response = WIFI_TRUE;
                WiFi_Control_Variables.enable_receive_data_chunk    = WIFI_TRUE;
                WiFi_Control_Variables.http_req_pending             = WIFI_FALSE;
                WiFi_Counter_Variables.last_process_buffer_index = 5;
                WiFi_Control_Variables.enable_timeout_timer      = WIFI_FALSE; //stop the timeout timer as response from server received.
                WiFi_Counter_Variables.timeout_tick              = 0;
                return;
            }
      }

    else if ((process_buffer[Fillptr-1]==0x09) && (process_buffer[Fillptr-2]==':') && (process_buffer[Fillptr-3]=='1'))//<ht> Horizontal Tab for Scan Result?
      {
          WiFi_Control_Variables.enable_receive_wifi_scan_response = WIFI_TRUE;
      }
    else if(WiFi_Control_Variables.in_standby_mode)
      {
        /* Eliminate the NULL values received after WIND:67:Going into Standby */
        if(process_buffer[Fillptr-1] == '\0')
           Fillptr = 0;
      }
    else if(Fillptr >= MAX_BUFFER_GLOBAL-1)
      {
        #if DEBUG_PRINT
        printf("\rJust Looping with unhandled data!\r\n");
        #endif          
        Fillptr=0;          
        memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL); 
      }
    
        //Check Process Buffer for any pending message
        if(WiFi_Control_Variables.enable_receive_data_chunk && !WiFi_Control_Variables.enable_sock_data)
          {
            WiFi_Counter_Variables.pop_queue_length = WiFi_Counter_Variables.pop_buffer_size;
            if(Fillptr + WiFi_Counter_Variables.pop_queue_length > MAX_BUFFER_GLOBAL-1)
            {
               uint32_t length = (Fillptr + WiFi_Counter_Variables.pop_queue_length) - (MAX_BUFFER_GLOBAL - 1);
              __disable_irq();
               rewind_buffer_queue(&wifi_instances.big_buff,length);
               __enable_irq();
               memset(ptr+((MAX_BUFFER_GLOBAL-1) - Fillptr), 0x00, length);
               WiFi_Counter_Variables.pop_queue_length = (MAX_BUFFER_GLOBAL-1) - Fillptr;
            }
            memcpy(process_buffer+Fillptr,(char const *)pop_buffer, WiFi_Counter_Variables.pop_queue_length);
            Fillptr = Fillptr + WiFi_Counter_Variables.pop_queue_length;
            
             // do nothing in this case as we do not know what data is coming next.
            if(Fillptr == 1 && process_buffer[0]=='\r')
              return;

            if((strstr((const char *)process_buffer,"ERROR: ")) != NULL)
            {
                 WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                 WiFi_Control_Variables.message_pending = WIFI_FALSE;
            }
            else if(!IO_status_flag.sock_read_ongoing && !WiFi_Control_Variables.enable_receive_http_response) 
            {
                if(process_buffer[0]!='\0') 
                {
                  // in case of space " " SockON_Data_Length = 2 as only \r\n, so No Q_contains_message.
                  if(((process_buffer[0]==0xD) && (process_buffer[1]==0xA)) && WiFi_Counter_Variables.SockON_Data_Length != 2)
                  {
                    WiFi_Control_Variables.message_pending = WIFI_TRUE;
                    if((pStr = (strstr((const char *)process_buffer+2,"\r\n"))) != NULL) 
                    {
                          // process buffer has complete message
                          int wind_length = ((uint8_t *)pStr - (uint8_t *)process_buffer)+2;
                          if(strstr((const char *)process_buffer+2,"DATALEN:")) 
                          {
                                pStr = strstr((const char *)process_buffer + wind_length,"\r\nOK\r\n"); //find OK, as DATALEN has to be terminated by OK
                                if(pStr!=NULL)
                                {
                                    wind_length = ((uint8_t *)pStr-(uint8_t *)process_buffer)+6;
                                }
                          }

                          if(Fillptr - wind_length)  //rewind only if extra bytes present in process_buffer
                          {
                                __disable_irq();
                              rewind_buffer_queue(&wifi_instances.big_buff, Fillptr - wind_length);             
                              __enable_irq();
                              memset(process_buffer + wind_length,0x00,Fillptr - wind_length);
                              Fillptr = wind_length;
                          }
                          WiFi_Control_Variables.message_pending = WIFI_FALSE;
                      }
                      WiFi_Control_Variables.Q_Contains_Message = WIFI_TRUE;
                    
                      //if in /r/n.../r/n the data between a pair of /r/n is more than 511
                      if(Fillptr == 511 && WiFi_Control_Variables.message_pending)
                        {
                            WiFi_Control_Variables.message_pending    = WIFI_FALSE;
                            WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                            WiFi_Counter_Variables.pop_queue_length   = 511;
                        }
                    }
                }
            }
          }

        /*********************************************************************************************
         *                                                                                           *
         *                          Socket Read Is Enabled.                                       *
         *                                                                                           *
         ********************************************************************************************/
        
        
        if(!WiFi_Control_Variables.Q_Contains_Message && WiFi_Control_Variables.enable_sock_read)     /*read is enabled*/
        {
            IO_status_flag.sock_read_ongoing = WIFI_TRUE;
            WiFi_Counter_Variables.sock_total_count = WiFi_Counter_Variables.sock_total_count + WiFi_Counter_Variables.pop_queue_length;
            
            /* Check for "ERROR: Not enough data in buffer " */
            pStr = (char *) strstr((const char *)&process_buffer + WiFi_Counter_Variables.last_process_buffer_index - 5,"ERROR: ");
            if (pStr != NULL)
              {
                  if((process_buffer[Fillptr-2]==0xD) && (process_buffer[Fillptr-1]==0xA)) //check if end of message received
                    {
                      if((pStr = (char *)strstr((const char *)&process_buffer,"\r\nERROR: Too many sockets\r\n")) !=NULL)
                        {
                           #if DEBUG_PRINT
                             printf("\r\nERROR: TOO MANY SOCKETS \r\n");
                           #endif

                            if(*(pStr+27)!='\0')
                              {
                                  int len = (uint8_t *)pStr - (uint8_t *)process_buffer;
                                  int extra_bytes = Fillptr - (len+27);
                                  __disable_irq();
                                  rewind_buffer_queue(&wifi_instances.big_buff, extra_bytes);
                                  __enable_irq();
                              }
                            IO_status_flag.AT_Response_Received = WIFI_TRUE;
                            WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                        }
                      else if((pStr = (char *)strstr((const char *)&process_buffer,"\r\nERROR: Pending data\r\n")) !=NULL)
                        {
                           #if DEBUG_PRINT
                             printf("\r\nERROR: PENDING DATA \r\n");
                           #endif

                            if(*(pStr+23)!='\0')
                              {
                                  int len = (uint8_t *)pStr - (uint8_t *)process_buffer;
                                  int extra_bytes = Fillptr - (len+23);
                                  __disable_irq();
                                  rewind_buffer_queue(&wifi_instances.big_buff, extra_bytes);
                                  __enable_irq();
                              }
                            WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_TRUE;
                        }
                      
                      else if((pStr = (char *)strstr((const char *)&process_buffer,"\r\nERROR: Socket error\r\n")) !=NULL)
                        {
                            #if DEBUG_PRINT
                             printf("\r\nERROR: Socket error\r\n");
                            #endif

                           if(*(pStr+23)!='\0')
                            {
                                int len = (uint8_t *)pStr - (uint8_t *)process_buffer;
                                int extra_bytes = Fillptr - (len+23);
                                __disable_irq();
                                rewind_buffer_queue(&wifi_instances.big_buff, extra_bytes);
                                __enable_irq();
                            }
                        }

                      #if DEBUG_PRINT
                        printf("\rERROR DURING SOCK READ\r\n");
                      #endif
                      WiFi_Counter_Variables.sock_total_count = 0;
                      WiFi_Counter_Variables.Socket_Data_Length = 0;
                      WiFi_Counter_Variables.SockON_Data_Length = 0;
                      WiFi_Counter_Variables.last_process_buffer_index = 5;
                      WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                      WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                      WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
                      IO_status_flag.WIND64_count=0;
                      IO_status_flag.sock_read_ongoing = WIFI_FALSE;
                      if(WiFi_Control_Variables.data_pending_sockD)
                        {
                          WiFi_Control_Variables.data_pending_sockD = WIFI_FALSE;
                          WiFi_Counter_Variables.number_of_bytes=0;
                          WiFi_Control_Variables.switch_by_default_to_command_mode=WIFI_TRUE;
                          WiFi_switch_to_command_mode(); //switch by default
                        }

                      memset(process_buffer, 0x00, Fillptr);
                      Fillptr=0;
                      return;
                   }
              }
            
            //In case of "ERROR" we wait till \r\n is received, till that do not update last_process_buffer_index variable
            if(Fillptr > 5 && pStr == NULL) 
              WiFi_Counter_Variables.last_process_buffer_index = Fillptr;

            /*now check if end of msg received*/
            if(WiFi_Counter_Variables.sock_total_count >= WiFi_Counter_Variables.SockON_Data_Length)
              {
                #if DEBUG_PRINT
                 printf("\nReached SockON_Data_Length \r\n");
                #endif

                 //rewind the buffer, if some extra bytes along with required data
                if(WiFi_Counter_Variables.sock_total_count > WiFi_Counter_Variables.SockON_Data_Length)
                  {
                    int databytes = WiFi_Counter_Variables.sock_total_count - WiFi_Counter_Variables.SockON_Data_Length;
                    
                    __disable_irq();
                    rewind_buffer_queue(&wifi_instances.big_buff, databytes);
                    __enable_irq();

                    memset(process_buffer+(Fillptr - databytes), 0x00, databytes);
                    Fillptr = Fillptr - databytes;
                  }
                WiFi_Counter_Variables.chunk_size = Fillptr;
                WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.SockON_Data_Length;
                memcpy(UserDataBuff, process_buffer, Fillptr);
                Fillptr = 0;
                WiFi_Counter_Variables.sock_total_count = 0;
                WiFi_Counter_Variables.Socket_Data_Length = 0;
                WiFi_Counter_Variables.SockON_Data_Length = 0;
                WiFi_Counter_Variables.pop_queue_length = 0;
                WiFi_Counter_Variables.last_process_buffer_index = 5;
                WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
                WiFi_Counter_Variables.sockdon_id_user = -1;

                if(WiFi_Control_Variables.data_pending_sockD)
                  {
                    WiFi_Counter_Variables.number_of_bytes=0;
                    WiFi_Counter_Variables.sockon_id_user = 0;
                    if(IO_status_flag.WIND64_count>0)
                        IO_status_flag.WIND64_count--;                         //decrease the number of pending WIND:64 Events
                    if(IO_status_flag.WIND64_count==0) 
                      {
                        WiFi_Control_Variables.switch_by_default_to_command_mode=WIFI_TRUE;
                        WiFi_switch_to_command_mode();              //switch by default
                      }
                    WiFi_Control_Variables.enable_query = WIFI_FALSE;
                  }
                else
                  {
                    //do we have more data?
                    WiFi_Control_Variables.enable_query = WIFI_TRUE;
                    WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
                    /*@TODO: Do not need to prevent OK push in case of server socket*/
                    IO_status_flag.prevent_push_OK_event = WIFI_TRUE; //prevent the qeueuing of the OK after this read operation
                  }

                WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                IO_status_flag.sock_read_ongoing = WIFI_FALSE;
                Stop_Dequeue();                           //Stop dequeue till user callback returns

                if(WiFi_Control_Variables.data_pending_sockD)
                  {
                    WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE;
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                    WiFi_Control_Variables.data_pending_sockD = WIFI_FALSE;
                  }
                else
                  {
                    WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE;   //set this to callback to user with User Buffer pointer
                  }
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                return;
              }

            //if Process_buffer is completely filled, callback to user with User Buffer pointer
            if(Fillptr >= MAX_BUFFER_GLOBAL-1)
              {
                WiFi_Counter_Variables.last_process_buffer_index = 5;
                WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.SockON_Data_Length;
                WiFi_Counter_Variables.chunk_size = MAX_BUFFER_GLOBAL-1;
                memcpy(UserDataBuff, process_buffer, Fillptr);
                Fillptr = 0;
                WiFi_Counter_Variables.last_process_buffer_index = 5;
                WiFi_Counter_Variables.sockdon_id_user = -1;
                Stop_Dequeue();
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                if(WiFi_Control_Variables.data_pending_sockD)
                  {
                    WiFi_Counter_Variables.sockon_id_user = 0;
                    WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE; //callback to user with User Buffer pointer, in case of server socket
                  }
                else
                  {
                    WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
                    WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE; //set this to callback to user with User Buffer pointer
                  }
              }
            return;
        }

         /********************************************************************************************
         *                                                                                           *
         *                          HTTP Response Is Enabled.                                     *
         *                                                                                           *
         ********************************************************************************************/

        if (WiFi_Control_Variables.enable_receive_http_response) // http response enabled
        {
            IO_status_flag.sock_read_ongoing = WIFI_TRUE;
            if((pStr = (strstr((const char *)process_buffer + WiFi_Counter_Variables.last_process_buffer_index - 5,"\r\nOK\r\n"))) != NULL)  
              {
                  #if DEBUG_PRINT
                    printf("\r\nOK\r\n");         //http response completed
                  #endif

                  if(*(pStr+7) != '\0') //extra data apart from the required HTTP Response
                    {
                        int len = (uint8_t *)pStr - (uint8_t *)process_buffer;
                        int extra_bytes = Fillptr - (len+6);
                        __disable_irq();
                        rewind_buffer_queue(&wifi_instances.big_buff, extra_bytes);
                        __enable_irq();
                        
                        memset(process_buffer+len+7, 0x00, extra_bytes);
                        Fillptr = Fillptr - extra_bytes;
                    }
                  
                  WiFi_Counter_Variables.chunk_size = Fillptr;
                  memcpy(UserDataBuff, process_buffer, Fillptr);
                  WiFi_Counter_Variables.UserDataBuff_index = Fillptr;
                  IO_status_flag.AT_Response_Received = WIFI_TRUE;
                  WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
                  WiFi_Control_Variables.enable_receive_http_response = WIFI_FALSE;
                  Stop_Dequeue();
                  WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                  IO_status_flag.sock_read_ongoing = WIFI_FALSE;
                  WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                  WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                  WiFi_Counter_Variables.last_process_buffer_index = 5;
                  memset(process_buffer, 0x00, Fillptr);
                  Fillptr=0;

                  if(WiFi_Control_Variables.enable_receive_file_response)
                    WiFi_Control_Variables.FILE_Data_available = WIFI_TRUE;
                  else
                    WiFi_Control_Variables.HTTP_Data_available=WIFI_TRUE;
              }

            else if(( pStr = (strstr((const char *)process_buffer + WiFi_Counter_Variables.last_process_buffer_index-5,"ERROR"))) != NULL)
              {
                  /* wait for complete error message */
                   WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
                  if((process_buffer[Fillptr-2]==0xD) && (process_buffer[Fillptr-1]==0xA))
                  {
                      #if DEBUG_PRINT
                      printf("\r\nERROR\r\n");
                      #endif
                      IO_status_flag.AT_Response_Received = WIFI_TRUE;
                      WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;
                      IO_status_flag.sock_read_ongoing = WIFI_FALSE;
                      WiFi_Control_Variables.enable_receive_data_chunk = WIFI_FALSE;
                      WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
                      Fillptr=0;
                      WiFi_Counter_Variables.last_process_buffer_index = 5;
                      WiFi_Control_Variables.enable_receive_http_response = WIFI_FALSE;
                      memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);

                      if(WiFi_Control_Variables.enable_receive_file_response)
                        WiFi_Control_Variables.FILE_Data_available = WIFI_FALSE;
                      else
                        WiFi_Control_Variables.HTTP_Data_available=WIFI_FALSE;
                  }
              }

            if((Fillptr > 5) & (pStr == NULL))
                  WiFi_Counter_Variables.last_process_buffer_index = Fillptr;

            if(Fillptr == MAX_BUFFER_GLOBAL-1 )
              {
                  memcpy(UserDataBuff, process_buffer, Fillptr);
                  memcpy(&HTTP_Runway_Buff, &UserDataBuff[505], 6); //for storing the last 6 bytes of Userdatabuff, in case 'OK' is splitted
                  memset(&UserDataBuff[505], 0x00, 6);
                  memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                  memcpy(&process_buffer, &HTTP_Runway_Buff, 6);
                  memset(HTTP_Runway_Buff, 0x00, 6);
                  Fillptr = 6;
                  WiFi_Counter_Variables.last_process_buffer_index = 5;
                  WiFi_Counter_Variables.UserDataBuff_index = 505;
                  Stop_Dequeue();

                  if(WiFi_Control_Variables.enable_receive_file_response)
                    WiFi_Control_Variables.FILE_Data_available = WIFI_TRUE;
                  else
                    WiFi_Control_Variables.HTTP_Data_available = WIFI_TRUE;
              }
            return;
        }
    
     /*********************************************************************************************
      *                                                                                           *
      *                             WiFi Scan Is Enabled.                                       *
      *                                                                                           *
      ********************************************************************************************/
    
    if((!WiFi_Control_Variables.Q_Contains_Message) && WiFi_Control_Variables.enable_receive_wifi_scan_response)
      {
            /*now check if end of msg received*/
            if((process_buffer[Fillptr-2]==0xD) && (process_buffer[Fillptr-1]==0xA))
              {
                if(WiFi_Counter_Variables.scanned_ssids < WiFi_Counter_Variables.user_scan_number)
                  {
                      pStr = (char *) strstr((const char *)&process_buffer,"CHAN:");            
                      if(pStr != NULL)
                          {
                              databytes_No[0] = *(pStr + 6) ;
                              databytes_No[1] = *(pStr + 7) ;
                    
                              chan_value = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }

                      wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].channel_num = chan_value;

                      pStr = (char *) strstr((const char *)&process_buffer,"RSSI:");            
                      if(pStr != NULL)
                          {
                              databytes_No[0] = *(pStr + 7) ;
                              databytes_No[1] = *(pStr + 8) ;
                    
                              rssi_value = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }

                      wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].rssi = -(rssi_value);

                      pStr = (char *) strstr((const char *)&process_buffer,"SSID:");
                      if(pStr != NULL)
                          {
                              index = 7;
                              while(*(pStr + index) != 0x27)
                                  {
                                      wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].ssid[index-7] = *(pStr + index);
                                      index++;
                                      if(index==35) break; //max ssid lenght is 30 characters
                                  }                                
                          }

                      pStr = (char *) strstr((const char *)&process_buffer,"WPA ");            
                      if(pStr != NULL)
                          {
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa = WIFI_TRUE;
                          } else
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa = WIFI_FALSE;
                      
                      pStr = (char *) strstr((const char *)&process_buffer,"WPA2 ");            
                      if(pStr != NULL)
                          {
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa2 = WIFI_TRUE;
                          } else
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa2 = WIFI_FALSE;
                      
                      pStr = (char *) strstr((const char *)&process_buffer,"WPS ");            
                      if(pStr != NULL)
                          {
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wps = WIFI_TRUE;
                          } else
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wps = WIFI_FALSE;

                      WiFi_Counter_Variables.scanned_ssids++;//increment total_networks
                  }

                //end of one line from SCAN result       
                pStr = (char *) strstr((const char *)&process_buffer,"ERROR");
                if(pStr != NULL)
                  {
                      #if DEBUG_PRINT
                        printf("ERROR Scan Failed"); 
                      #endif
                      memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
                      Fillptr=0;
                      IO_status_flag.AT_Response_Received = WIFI_TRUE;
                      WiFi_Control_Variables.enable_receive_wifi_scan_response = WIFI_FALSE;
                      WiFi_Control_Variables.Scan_Ongoing = WIFI_FALSE; //Enable next scan
                      WiFi_Counter_Variables.AT_RESPONSE = WiFi_AT_CMD_RESP_ERROR;     
                      return;
                  }

                #if DEBUG_PRINT
                printf((const char*)process_buffer);
                #endif

                if(((strstr((const char *)process_buffer,"OK\r\n"))) != NULL /*|| scanned_ssids==10*/)/*Max 10 networks supported*/
                  {
                      //print and go for next line
                      //If Any part of scan line contains "OK" this will exit!!
                      #if DEBUG_PRINT
                        printf("\nOK\r\n");   
                      #endif
                      WiFi_Control_Variables.Scan_Ongoing = WIFI_FALSE; //Enable next scan             
                      WiFi_Counter_Variables.scanned_ssids=0;                       
                      Fillptr=0;
                      IO_status_flag.AT_Response_Received = WIFI_TRUE;
                      WiFi_Control_Variables.enable_receive_wifi_scan_response = WIFI_FALSE;
                      WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                      memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);    
                      return;
                  }

                Fillptr=0;
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);          
              }

            if(Fillptr>=MAX_BUFFER_GLOBAL-1)
              {
                #if DEBUG_PRINT
                printf("\rHTTP: process_buffer Max Buffer Size reached\r\n");
                #endif          
                Fillptr=0;          
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL); 
              }
      }

    
     /*********************************************************************************************
      *                                                                                           *
      *                             FW Update Is Enabled.                                         *
      *                                                                                           *
      ********************************************************************************************/
    
    if((!WiFi_Control_Variables.Q_Contains_Message) && WiFi_Control_Variables.enable_fw_update_read)
      {
            #ifdef DEBUG_PRINT
            printf("*");     //print * till finish    
            #endif

            pStr = (char *) strstr((const char *)process_buffer,"Complete!");

            if(pStr != NULL)
              {
                #ifdef DEBUG_PRINT
                printf("\r\nUpdate complete\r\n");
                #endif
                IO_status_flag.AT_Response_Received = WIFI_TRUE;
                WiFi_Counter_Variables.AT_RESPONSE = WiFi_MODULE_SUCCESS;
                WiFi_Control_Variables.enable_fw_update_read = WIFI_FALSE;
                Fillptr=0;
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
              }
            if(Fillptr>=MAX_BUFFER_GLOBAL)
              {
                Fillptr=0;
                memset(process_buffer, 0x00, MAX_BUFFER_GLOBAL);
              }

            //No change of state till we get "+WIND:17:F/W update complete!"
      }    

}



/**
* @brief  Process_Wind_Indication_Cmd
*         Process Wind indication before queueing
* @param  process_buff_ptr: pointer of WiFi indication buffer
* @retval None
*/

void Process_Wind_Indication(uint8_t *process_buff_ptr)
{
  char * process_buffer_ptr = (char*)process_buff_ptr; //stores the starting address of process_buffer.
  char * ptr_offset = (char*)process_buff_ptr;
  char Indication_No[2];
  char databytes_No[4];
  #if DEBUG_PRINT
  printf((const char*)process_buff_ptr);
  #endif
  int i;

  WiFi_Indication_t Wind_No = Undefine_Indication;

  if(ptr_offset != NULL)
    {
      ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"WIND:");

      if(ptr_offset != NULL)
        {
            Indication_No[0] = *(ptr_offset + 5);
            Indication_No[1] = *(ptr_offset + 6);

            if( Indication_No[1] == ':')
              {
                /* Convert char to integer */
                Wind_No = (WiFi_Indication_t)(Indication_No[0] - '0'); 
              }
            else
              {
                /* Convert char to integer */
                Wind_No = (WiFi_Indication_t)(((Indication_No[0] - '0') * 10 ) + (Indication_No[1] - '0'));
              }

            wifi_instances.wifi_event.wind = Wind_No;
            wifi_instances.wifi_event.event = WIFI_WIND_EVENT;

            switch (Wind_No)
            {
                case SockON_Data_Pending: /*WIND:55*/
                    /*+WIND:55:Pending Data:%d:%d*/   
                    ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"Data:");

                    /*Need to find out which socket ID has data pending*/
                    databytes_No[0] = *(ptr_offset + 5);

                    wifi_instances.wifi_event.socket_id = (databytes_No[0] - '0'); //Max number of sockets is 8 (so single digit)
                    break;

                case SockON_Server_Socket_Closed:
                    //Find the id of the socket closed
                    databytes_No[0] = *(ptr_offset + 22) ;
                    wifi_instances.wifi_event.socket_id = databytes_No[0] - '0'; //Max number of sockets is 8 (so single digit)
                    break;      

                case SockD_Pending_Data:
                    if(WiFi_Control_Variables.prevent_push_WIFI_event) 
                      {
                        #ifdef DEBUG_PRINT
                        printf(">>IG\r\n");
                        #endif
                        return; ///exit if push is prevented
                      }

                    /* @TODO: Do something to delay the de-Q of pending packets so that any further
                              pending WIND64 packets are allowed to arrive and get Q-ed */

                    if(WiFi_Control_Variables.event_deQ_x_wind64)      //which means there is already a previous WIND64
                      {
                        WiFi_Counter_Variables.wind64_DQ_wait=0;         //reset the timer
                      }
                    else
                      {
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;//Stop the event de-Q
                        WiFi_Control_Variables.event_deQ_x_wind64 = WIFI_TRUE;//Set the flag
                      }

                    //Start Reading data from Client Here.    
                    // +WIND:64:Sockd Pending Data:1:130:130
                    ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"Data:");
                    if(ptr_offset != NULL)
                      {
                        //Store the packet number
                        databytes_No[0] = *(ptr_offset + 5) ;
                        wifi_instances.wifi_event.wind64_pending_packet_no = databytes_No[0] - '0';

                        //And now find the data length
                        databytes_No[0] = *(ptr_offset + 8) ;//points to number just after 2nd colon
                        databytes_No[1] = *(ptr_offset + 9) ;
                        databytes_No[2] = *(ptr_offset + 10) ;
                        databytes_No[3] = *(ptr_offset + 11) ;
              
                        if( databytes_No[0] == ':')//then it is a 1 digit number
                          {
                            databytes_No[0] = *(ptr_offset + 7) ;
                            databytes_No[1] = *(ptr_offset + 8) ;
                          }
                        else if(databytes_No[1] == ':')//two digit number
                          {
                            databytes_No[0] = *(ptr_offset + 7) ;
                            databytes_No[1] = *(ptr_offset + 8) ;
                            databytes_No[2] = *(ptr_offset + 9) ;
                          }
                        else if(databytes_No[2] == ':')//three digit number
                          {
                            databytes_No[0] = *(ptr_offset + 7) ;
                            databytes_No[1] = *(ptr_offset + 8) ;
                            databytes_No[2] = *(ptr_offset + 9) ;
                            databytes_No[3] = *(ptr_offset + 10) ;
                          }
                        else if(databytes_No[3] == ':')//four digit number
                          {
                            databytes_No[0] = *(ptr_offset + 7) ;
                            databytes_No[1] = *(ptr_offset + 8) ;
                            databytes_No[2] = *(ptr_offset + 9) ;
                            databytes_No[3] = *(ptr_offset + 10) ;
                          }

                        if( databytes_No[1] == ':')
                          {
                            wifi_instances.wifi_event.data_length = databytes_No[0] - '0'; 
                          }
                        else if( databytes_No[2] == ':')
                          {
                            wifi_instances.wifi_event.data_length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }
                        else if( databytes_No[3] == ':')
                          {
                            wifi_instances.wifi_event.data_length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                          }
                        else //it's a 4-digit number
                          {
                            wifi_instances.wifi_event.data_length = (((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10 ) + (databytes_No[3] - '0'));
                          }            

                        /*WIND:64 came after pop of previous event and switch to data mode was issued*/
                        IO_status_flag.WIND64_count++;           //Count of the number of queued WIND:64 Events
                      }
                    break;

                case In_Command_Mode:
                    IO_status_flag.command_mode= WIFI_TRUE;
                    IO_status_flag.data_mode= WIFI_FALSE;
                    IO_status_flag.WIND64_count=0;     //reset the WIND64 count since the previous data mode would have consumed all data
                    WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
                    WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
                    break;

                case In_Data_Mode:
                    IO_status_flag.command_mode = WIFI_FALSE;
                    IO_status_flag.data_mode = WIFI_TRUE;
                    WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
                    WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;

                    if(WiFi_Control_Variables.switch_by_default_to_command_mode == WIFI_TRUE)
                      {
                        if(!IO_status_flag.command_mode)
                          {
                            WiFi_switch_to_command_mode();//switch by default
                          }    
                      }
                    if(WiFi_Control_Variables.data_pending_sockD == WIFI_TRUE)
                      {
                         WiFi_Counter_Variables.last_process_buffer_index =5;
                         WiFi_Control_Variables.enable_receive_data_chunk = WIFI_TRUE;       // read data in chunk now from ring buffer
                         WiFi_Control_Variables.enable_sock_read = WIFI_TRUE;//now data will start coming
                        //Set the data-length to read
                        WiFi_Counter_Variables.SockON_Data_Length = WiFi_Counter_Variables.number_of_bytes;
                      }
                    break;

                case WiFi__MiniAP_Associated:
                    //Find out which client joined by parsing the WIND //+WIND:28
                    ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"+WIND:28");
                    for(i=17;i<=33;i++)
                    WiFi_Counter_Variables.client_MAC_address[i-17] = *(ptr_offset + i) ;    
                    IO_status_flag.WiFi_WIND_State = WiFiAPClientJoined;
                    break;

                case WiFi_MiniAP_Disassociated:
                    //Find out which client left by parsing the WIND //+WIND:72
                    ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"+WIND:72");
                    for(i=17;i<=33;i++)
                    WiFi_Counter_Variables.client_MAC_address[i-17] = *(ptr_offset + i) ;
                    IO_status_flag.WiFi_WIND_State = WiFiAPClientLeft;
                    break;
                    
                 case Incoming_socket_client:
                case Outgoing_socket_client:
                    if(WiFi_Control_Variables.prevent_push_WIFI_event)
                      WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;
                     break;

                case Remote_Configuration:
                    wifi_connected = 0;
                    WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
                    WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
                    break;
                    
                case Going_Into_Standby:
                    wifi_instances.wifi_event.event = WIFI_STANDBY_CONFIG_EVENT;
                    WiFi_Control_Variables.in_standby_mode = WIFI_TRUE;
                    break;
                    
                case Resuming_From_Standby:
                    wifi_instances.wifi_event.event = WIFI_RESUME_CONFIG_EVENT;
                    WiFi_Control_Variables.in_standby_mode = WIFI_FALSE;
                    break;

                case Console_Active:           // Queueing of these events not required.
                case Poweron:
                case WiFi_Reset:
                case Watchdog_Running:
                case Watchdog_Terminating:
                case SysTickConfigure:
                case CopyrightInfo:
                case WiFi_BSS_Regained:
                case WiFi_Signal_OK:
                case FW_update:
                case Encryption_key_Not_Recognized:
                case WiFi_Join:
                case WiFi_Scanning:
                case WiFi_Association_Successful:
                case WiFi_BSS_LOST:
                case WiFi_NETWORK_LOST:
                case WiFi_Unhandled_Event:
                case WiFi_UNHANDLED:
                case WiFi_MiniAP_Mode :
                case DOT11_AUTHILLEGAL:
                case Creating_PSK:
                case WPA_Terminated :
                case WPA_Supplicant_Failed:
                case WPA_Handshake_Complete:
                case GPIO_line:
                case Wakeup:
                case Factory_debug:
                case Low_Power_Mode_Enabled:
                case Rejected_Found_Network:
                    WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
                    WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
                    break;

                  //Queue these Events.
                case MallocFailed:                                          
                    if(WiFi_Control_Variables.stop_event_dequeue)
                      WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                    break;
                case Heap_Too_Small:            
                case WiFi_Hardware_Dead:
                case Hard_Fault:
                case StackOverflow:                   
                case Error:
                case WiFi_PS_Mode_Failure:
                case WiFi_Signal_LOW:
                case JOINFAILED:
                case SCANBLEWUP:
                case SCANFAILED:
                case WiFi_Started_MiniAP_Mode:
                case Start_Failed :
                case WiFi_EXCEPTION :
                case WiFi_Hardware_Started :
                case Scan_Complete:
                case WiFi_UNHANDLED_IND:
                case WiFi_Powered_Down:
                case WiFi_Deauthentication:
                case WiFi_Disassociation:
                case RX_MGMT:
                case RX_DATA:
                case RX_UNK:
                     break;
                default:
                     break;
              }
           memset(process_buffer_ptr, 0x00, MAX_BUFFER_GLOBAL);
        }
     }
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
