/**
 ******************************************************************************
 * @file    wifi_interface.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    17-May-2016
 * @brief   User APIs implementation for X-CUBE-WIFI1
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
#include "stdarg.h"

/** @addtogroup MIDDLEWARES
* @{
*/ 

/** @defgroup  NUCLEO_WIFI_INTERFACE
  * @brief Wi-Fi User API modules
  * @{
  */

/** @defgroup NUCLEO_WIFI_INTERFACE_Private_Defines
  * @{
  */

/**
  * @}
  */

/** @addtogroup NUCLEO_WIFI_INTERFACE_Private_Variables
  * @{
  */
/* Private variables ---------------------------------------------------------*/

/**
  * @}
  */
  
/** @defgroup NUCLEO_WIFI_INTERFACE_Private_Functions
  * @{
  */

GPIO_InitTypeDef  WAKEUP_InitStruct;

/**
  * @brief  wifi_init
  *         User API for wifi init
  * @param  None
  * @retval None
  */
WiFi_Status_t wifi_init(wifi_config* config)
{

  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

#if DEBUG_PRINT
#if defined(SPWF04)  
  printf("\r\nInitializing SPWF04SA Interface..\r\n");
#else
  printf("\r\nInitializing SPWF01SA Interface..\r\n");
#endif
  fflush(stdout);
#endif  

  WiFi_Module_Init();
  
#ifndef WIFI_USE_VCOM  
  wifi_wakeup(WIFI_TRUE);//Prevent from going to sleep during configuration    

  /* Soft reset the module */
  wifi_reset();

#if defined(CONSOLE_UART_ENABLED)

  /* Restore default setting*/    
  status = Restore_Default_Setting();
  if(status != WiFi_MODULE_SUCCESS) 
    return status;

  /* Switch on HW Flow Control*/  
#ifdef SPWF04
  status = SET_Configuration_Value(CONSOLE1_HWFC, 0);
#else
  status = SET_Configuration_Value(CONSOLE1_HWFC, 1);
#endif
  if(status != WiFi_MODULE_SUCCESS) return status; 

  if(config->wifi_baud_rate)
  {
    /* Set USART Speed*/  
    status = SET_Configuration_Value(CONSOLE1_SPEED, config->wifi_baud_rate);
    if(status != WiFi_MODULE_SUCCESS) return status; 
  }

#ifdef SPWF04
    /*AT+S.WIFI=0<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 0);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
  
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
  
#endif  
  
  /* Set wifi_mode to idle*/  
  status = SET_Configuration_Value(WIFI_MODE, WiFi_IDLE_MODE);
  if(status != WiFi_MODULE_SUCCESS) return status;   
  
  switch(config->ht_mode)
  {
  case WIFI_FALSE:
    status = SET_Configuration_Value(WIFI_HT_MODE, 0);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Addr(WIFI_OPR_RATE_MASK, "0x00003FCF");
    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  case WIFI_TRUE:
    status = SET_Configuration_Value(WIFI_HT_MODE, 1);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Addr(WIFI_OPR_RATE_MASK, "0x003FFFCF");
    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  default:
    break;
  }

  switch(config->power)
  {
  case wifi_active:
    status = SET_Configuration_Value(WIFI_POWERSAVE, 0);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_SLEEP_ENABLED, 0);  
    if(status != WiFi_MODULE_SUCCESS) return status;

//check    status = SET_Configuration_Value(WIFI_STANDBY_ENABLED, 0); //MGR: Force No StandBy
//check    if(status != WiFi_MODULE_SUCCESS) return status;
//check    status = SET_Configuration_Value("ip_sockd_timeout", 5); //MGR: Reduce baud timeout
//check    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  case wifi_reactive:
    status = SET_Configuration_Value(WIFI_POWERSAVE, 1);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_OPERATIONAL_MODE, 11);  
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_BEACON_WAKEUP, 0);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_LISTEN_INTERVAL, 0);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_SLEEP_ENABLED, 0);  
    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  case wifi_sleep:
    status = SET_Configuration_Value(WIFI_POWERSAVE, 1);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_OPERATIONAL_MODE, 12);  
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_BEACON_WAKEUP, 10);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_LISTEN_INTERVAL, 1);
    if(status != WiFi_MODULE_SUCCESS) return status;
    status = SET_Configuration_Value(WIFI_SLEEP_ENABLED, 1);  
    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  default:
    break;
  }

  switch(config->power_level)
  {
  case low:
  case medium:
  case high:
  case max:
    status = SET_Configuration_Value(WIFI_TX_POWER, config->power_level*6);
    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  default:
    break;
  }

  switch(config->dhcp)
  {
  case off:
  case on:
  case custom:
    status = SET_Configuration_Value(IP_USE_DHCP_SERVER, config->dhcp);
    if(status != WiFi_MODULE_SUCCESS) return status;
    break;
  default:
    break;
  }

  /* Set IP address */
  if(config->ip_addr)
  {
    status = SET_Configuration_Addr(WIFI_IP_ADDRESS, config->ip_addr);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  /* Set netmask address */
  if(config->netmask_addr)
  {
    status = SET_Configuration_Addr(WIFI_IP_NETMASK, config->netmask_addr);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  /* Set default gateway address */
  if(config->gateway_addr)
  {
    status = SET_Configuration_Addr(WIFI_IP_DEFAULT_GATEWAY, config->gateway_addr);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  /* Set dns address */
  if(config->dns_addr)
  {
#ifdef SPWF04
    status = SET_Configuration_Addr(WIFI_IP_DNS1, config->dns_addr);
#else
    status = SET_Configuration_Addr(WIFI_IP_DNS, config->dns_addr);
#endif
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  /* Set hostname */
  if(config->host_name)
  {
    status = SET_Configuration_Addr(WIFI_IP_HOSTNAME, config->host_name);  
    if(status != WiFi_MODULE_SUCCESS) return status;
  }

  if(config->ap_domain_name)
  {
    status = SET_Configuration_Addr(WIFI_IP_APDOMAINNAME, config->ap_domain_name);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }

  if(config->ap_config_page_name)
  {
    status = SET_Configuration_Addr(WIFI_IP_APREDIRECT, config->ap_config_page_name);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }

  if(config->http_timeout)
  {
    status = SET_Configuration_Value(WIFI_IP_HTTP_TIMEOUT, config->http_timeout*1000);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  if(config->dhcp_timeout)
  {
    status = SET_Configuration_Value(WIFI_IP_DHCP_TIMEOUT, config->dhcp_timeout);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  if(config->wifi_region)
  {
    status = SET_Configuration_Value(WIFI_REG_COUNTRY, config->wifi_region);
    if(status != WiFi_MODULE_SUCCESS) return status;
  }   

#ifdef MODULE_VERSION_SPWF01Sx_1y
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_HTTPD, config->web_server);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
  }  
#endif  

#ifdef SPWF04  
  /*AT+S.TLSCERT2=content,2 */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=content,2\r");        
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
  }  

    /*AT+S.WIFI=1<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 1);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_FALSE;
  }
  
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
#else
  /* AT+S.TLSCERT2=clean,all */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT2=clean,all\r");        
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
#endif
  
  /* save current setting in flash */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SAVE_CURRENT_SETTING);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
  }   

  if(config->wifi_baud_rate)
  {
    UART_Configuration(config->wifi_baud_rate); 
    Receive_Data();//Restart data reception
  }

  /* Soft reset the module, Do the second reset after setting all parameters and saving in flash */
  #ifdef SPWF01
    wifi_reset();
  #endif
#else
  
  /*SPI Interface in use*/
  
  /* Set localecho to 0*/
  status = SET_Configuration_Value(LOCALECHO1, 0);
  if(status != WiFi_MODULE_SUCCESS) return status;
  
  /* Restore default setting*/    
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.FCFG");  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  
  /*AT+S.WIFI=0<cr>*/  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 0);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
  
  /* Set wifi_mode to idle*/  
  status = SET_Configuration_Value(WIFI_MODE, WiFi_IDLE_MODE);
  if(status != WiFi_MODULE_SUCCESS) return status;
  
  /*AT+S.WIFI=1<cr>*/  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 1);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
  
  /*AT+S.TLSCERT2=content,2 -> clean certificates*/
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=content,2");        
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status; 
  
  //No need to reset again in case of new module SPI
#endif  //#if defined(CONSOLE_UART_ENABLED)
  
  wifi_wakeup(WIFI_FALSE);//De-assert wakeup signal (PC13) to allow sleep if enabled
#endif  //WIFI_USE_VCOM
  
#if DEBUG_PRINT
  printf("\r\nEnd of Initialization..\r\n");
  fflush(stdout);
#endif

  return status;
}


/**
* @brief  wifi_socket_client_security
*         Set the security certificates and key for secure socket (TLS)
* @param  tls_mode : 'o': one way authentication 'm': mutual authentication
* @param  ca_certificate: Certificate Authority (CA) certificate (rootca_of_server.pem/rootca_of_client.pem)
* @param  certificate: could be client/server certificate (client_cert.pem/server_cert.pem)
* @param  certificate_key : key corresponding to the certificate stored (client_key.rsa.pem/server_key.rsa.pem)
* @param  client_domain: common name of the server certificate 'server' for SPWF01 and auth file corresponding to the rootca_of_server/client.pem for SPWF04 (rootca_of_client.bin/root_ca_of_server.bin) 
* @param  tls_epoch_time : epoch time
* @retval WiFi_Status_t : return status
*/
WiFi_Status_t wifi_socket_client_security(uint8_t* tls_mode, uint8_t* ca_certificate, uint8_t* certificate, uint8_t* certificate_key, uint8_t* client_domain, uint32_t tls_epoch_time) 
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  if(tls_epoch_time==0) 
    WiFi_Counter_Variables.epoch_time = EPOCH_TIME;
  else
    WiFi_Counter_Variables.epoch_time = tls_epoch_time;

#if defined(CONSOLE_UART_ENABLED)

  /* Clean the already stored certificates */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_CLEAN_TLS_CERT);        
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
  }

  /* Set the epoch time */  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SETTIME,(unsigned long)WiFi_Counter_Variables.epoch_time);        
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
  }
  
  /* Store the CA Cert(if specified) */
  if(ca_certificate) {
    Reset_AT_CMD_Buffer();
    sprintf((char*)WiFi_AT_Cmd_Buff,AT_STORE_CA_CERT,strlen((const char *)ca_certificate)-1, ca_certificate);

    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      if(status != WiFi_MODULE_SUCCESS) return status;
    }
  }

  /* Store the 'CA Authority Key ID' for SPWF04 and 'CA domain name' for SPWF01 module */
  if(client_domain) {
    Reset_AT_CMD_Buffer();
    #ifdef SPWF04
      sprintf((char*)WiFi_AT_Cmd_Buff,AT_STORE_AUTH_KEY, strlen((const char *)client_domain), client_domain);
    #else
      sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_CA_DOMAIN_NAME, client_domain);
    #endif

    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      if(status != WiFi_MODULE_SUCCESS) return status;
    }  
  }
  
  /* Store Certificate */
  if(certificate) {
    Reset_AT_CMD_Buffer();
    sprintf((char*)WiFi_AT_Cmd_Buff,AT_STORE_CERT,strlen((const char *)certificate) - 1, certificate);

    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      if(status != WiFi_MODULE_SUCCESS) return status;
    }
  }
  
  /* Store the key corresponding to the above certificate */
  if(certificate_key) {
    Reset_AT_CMD_Buffer();
    sprintf((char*)WiFi_AT_Cmd_Buff,AT_STORE_KEY,strlen((const char *)certificate_key) - 1, certificate_key);        

    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status == WiFi_MODULE_SUCCESS)
      {
        status = USART_Receive_AT_Resp( );
        if(status != WiFi_MODULE_SUCCESS) return status;
      }
  }
  
#else
  
   /*AT+S.TLSCERT =content,2 -> clean certificates*/
//  Reset_AT_CMD_Buffer();  
//  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=content,2");        
//  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
//  status = USART_Receive_AT_Resp( );
//  if(status != WiFi_MODULE_SUCCESS) return status; 

  /* AT+S.TIME=<seconds> */  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TIME=%lu",(unsigned long)WiFi_Counter_Variables.epoch_time);        
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status; 

  /*AT+S.TLSCERT=ca,<size><CR><Peer CA>*/
  if(ca_certificate) {
    Reset_AT_CMD_Buffer();
    IO_status_flag.AT_event_processing = WIFI_CERT_WRITE_EVENT;
    IO_status_flag.send_data = WIFI_TRUE;
    WiFi_Counter_Variables.curr_DataLength = strlen((const char *)ca_certificate) - 1;
    WiFi_Counter_Variables.curr_data = (char *)ca_certificate;
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=Ca,%d %s",WiFi_Counter_Variables.curr_DataLength, ca_certificate);
    run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
    status = USART_Receive_AT_Resp( );
    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
    if(status != WiFi_MODULE_SUCCESS) return status; 
  }
  
    /*AT+S.TLSCERT=auth,<Peer CA Authority Key Id>*/
  if(client_domain) {
    Reset_AT_CMD_Buffer();
    IO_status_flag.AT_event_processing = WIFI_CERT_WRITE_EVENT;
    IO_status_flag.send_data = WIFI_TRUE;
    WiFi_Counter_Variables.curr_DataLength = strlen((const char *)client_domain);
    WiFi_Counter_Variables.curr_data = (char *)client_domain;
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=Auth,%d %s", WiFi_Counter_Variables.curr_DataLength, client_domain);        
    run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
    status = USART_Receive_AT_Resp( );
    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
    if(status != WiFi_MODULE_SUCCESS) return status; 
  }

  /*AT+S.TLSCERT=cert,<size><CR><SPWF04S certificate>*/
  if(certificate) {
    Reset_AT_CMD_Buffer();
    IO_status_flag.AT_event_processing = WIFI_CERT_WRITE_EVENT;
    IO_status_flag.send_data = WIFI_TRUE;
    WiFi_Counter_Variables.curr_DataLength = strlen((const char *)certificate) - 1;
    WiFi_Counter_Variables.curr_data = (char *)certificate;
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=cert,%d %s",WiFi_Counter_Variables.curr_DataLength, certificate);
    run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
    status = USART_Receive_AT_Resp( );
    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
    if(status != WiFi_MODULE_SUCCESS) return status; 
  }

  /*AT+S.TLSCERT=key,<size><CR><SPWF04S private key>*/
  if(certificate_key) {
    Reset_AT_CMD_Buffer();
    IO_status_flag.AT_event_processing = WIFI_CERT_WRITE_EVENT;
    IO_status_flag.send_data = WIFI_TRUE;
    WiFi_Counter_Variables.curr_DataLength = strlen((const char *)certificate_key) - 1;
    WiFi_Counter_Variables.curr_data = (char *)certificate_key;
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.TLSCERT=key,%d %s",WiFi_Counter_Variables.curr_DataLength, certificate_key);        
    run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
    status = USART_Receive_AT_Resp( );
    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
    if(status != WiFi_MODULE_SUCCESS) return status; 
  }
#endif
  return status;
}

/**
* @brief  wifi_ping
*         issues a single ICMP ECHO request to the given host.
* @param  hostname: Target host. DNS resolvable name or IP address.
* @retval WiFi_Status_t: status of PING Command.
*/
WiFi_Status_t wifi_ping(uint8_t * hostname)
{
   WiFi_Status_t status = WiFi_MODULE_SUCCESS;
   
   sprintf((char*)WiFi_AT_Cmd_Buff,AT_PING,hostname);        
   status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status == WiFi_MODULE_SUCCESS)
    {
        status = USART_Receive_AT_Resp( );
    }

    return status;
}


/**
* @brief  wifi_socket_client_open
*         Open a network socket
* @param  Hostname hostname to connect to
*         portnumber portnumber of the Host to connect to
*         protocol tcp or udp protocol
*         sock_id socket id of the opened socket returned to the user
* @retval WiFi_Status_t : return status of socket open request
*/
WiFi_Status_t wifi_socket_client_open(uint8_t * hostname, uint32_t port_number, uint8_t * protocol, uint8_t * sock_id) 
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Queue_Client_Open_Event(hostname,port_number,protocol);
  status = USART_Receive_AT_Resp( );

  *sock_id = WiFi_Counter_Variables.Socket_Open_ID; //return the socket id to the user
  return status; 
}

/**
* @brief  Open_Serial_Port
*         Open a network socket
* @param  None
* @retval WiFi_Status_t : Wifi status
*/
WiFi_Status_t Open_Serial_Port()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  /* AT+S.SOCKOS=2<cr> */
  Reset_AT_CMD_Buffer();
  //sprintf((char*)WiFi_AT_Cmd_Buff,"\rAT+S.SOCKOS=%d\r",SerialPortNo);        
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));

  if(status==WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( ); 
  }
  return status;
}

/**
* @brief  wifi_socket_client_write
*         Write len bytes of data to socket
* @param  sock_id socket ID of the socket to write to
*         DataLength: data length to send
*         pData : pointer of data buffer to be written
* @retval WiFi_Status_t : return status of socket write request
*/
WiFi_Status_t wifi_socket_client_write(uint8_t sock_id, uint16_t DataLength, char * pData)
{
  /* AT+S.SOCKW=00,11<cr> */
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  //Check if sock_id is open
  if(!open_sockets[sock_id])
    return WiFi_NOT_READY;

  if(DataLength>=1021 || DataLength<=0)
    return WiFi_NOT_SUPPORTED;

  Queue_Client_Write_Event(sock_id,DataLength,pData);
  status = USART_Receive_AT_Resp( );
  return status;
}

/**
* @brief  Socket_Read
*         Return len bytes of data from socket
* @param  DataLength: data length to read
* @retval WiFi_Status_t : return status of socket read request
*/
WiFi_Status_t Socket_Read(uint16_t DataLength)
{
  /* AT+S.SOCKR=01,12<cr> */
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
#if defined(CONSOLE_UART_ENABLED)
  wait_for_command_mode();
#endif

  /* AT+S.SOCKON=myserver,1234,t<cr> */
  #if defined(CONSOLE_UART_ENABLED)
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SOCKET_READ,WiFi_Counter_Variables.sockon_query_id,DataLength);
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  //printf("\r\nAT+S.SOCKR");
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SOCKET_READ,WiFi_Counter_Variables.sockon_query_id,DataLength);
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
  //@CHECK-TBD!!: Changing AT_event_processing state without checking status flag
  IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_READ_EVENT;
  status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
#endif
  if(status == WiFi_MODULE_SUCCESS)
  {
    if(WiFi_Control_Variables.stop_event_dequeue == WIFI_FALSE)
        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
    WiFi_Counter_Variables.SockON_Data_Length = DataLength;
    WiFi_Control_Variables.enable_sock_read = WIFI_TRUE;
   
    /* enable the data receive in chunk mode only in spwfsa01 *///--------> TBD->Enable chunk mode in both modules
#ifdef SPWF01
    WiFi_Counter_Variables.last_process_buffer_index = 5;
    WiFi_Control_Variables.enable_receive_data_chunk = WIFI_TRUE;
#else
    WiFi_Counter_Variables.last_process_buffer_index = 10;
#endif

  }
  return status;
}

#ifdef SPWF04
/**
* @brief  Socket_Server_Read
*         Return len bytes of data from socket
* @param  DataLength: data length to read
* @retval WiFi_Status_t : return status of socket read request
*/
WiFi_Status_t Socket_Server_Read(uint16_t DataLength)
{
  /* AT+S.SOCKR=01,12<cr> */
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  #if defined(CONSOLE_UART_ENABLED)
  wait_for_command_mode();
#endif

  /* AT+S.SOCKDR=%d,%d,%d<cr> */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SERVER_SOCKET_READ,WiFi_Counter_Variables.sockdon_query_id, WiFi_Counter_Variables.sockon_query_id,DataLength);
  
#if defined(CONSOLE_UART_ENABLED)
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
  //@CHECK-TBD!!: Changing AT_event_processing state without checking status flag
  IO_status_flag.AT_event_processing = WIFI_SERVER_SOCKET_READ_EVENT;
  status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
#endif
  
  if(status == WiFi_MODULE_SUCCESS)
  {
    if(WiFi_Control_Variables.stop_event_dequeue == WIFI_FALSE)
        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
    WiFi_Counter_Variables.SockON_Data_Length = DataLength;
    WiFi_Control_Variables.enable_sock_read = WIFI_TRUE;
    WiFi_Counter_Variables.last_process_buffer_index = 10;
    //WiFi_Control_Variables.enable_receive_data_chunk = WIFI_TRUE;

  }
  return status;
}
#endif

/**
* @brief  wifi_socket_client_close
*         The SOCKC command allows to close socket
* @param  sock_close_id the socket ID of the socket which needs to be closed.
* @retval WiFi_Status_t : return status of socket close request
*/
WiFi_Status_t wifi_socket_client_close(uint8_t sock_close_id)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  WiFi_Control_Variables.enable_SockON_Server_Closed_Callback = WIFI_FALSE;
  if(open_sockets[sock_close_id])
  {
    Queue_Client_Close_Event(sock_close_id);
    return status;
  }
  else
    return WiFi_MODULE_ERROR;
}

/**
* @brief  Socket_Pending_Data
*         Query pending data.It will returns the number of bytes of data waiting on socket
* @param None
* @retval uint8_t :number of bytes of data waiting on socket
*/
void Socket_Pending_Data()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  /* AT+S.SOCKQ=01<cr> */
  Reset_AT_CMD_Buffer();
  
#if defined(SPWF01)
  wait_for_command_mode();
#endif
  
  if(open_sockets[WiFi_Counter_Variables.sockon_query_id])
    {
      if(WiFi_Control_Variables.stop_event_dequeue == WIFI_FALSE)
        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
      
#if defined(CONSOLE_UART_ENABLED)
      sprintf((char*)WiFi_AT_Cmd_Buff,AT_QUERY_PENDING_DATA,WiFi_Counter_Variables.sockon_query_id);        
      status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
      //printf("\r\nAT+S.SOCKQ");
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKQ=%d",WiFi_Counter_Variables.sockon_query_id);        
      run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
      //@CHECK-TBD!!: Changing AT_event_processing state without checking status flag
      IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_QUERY_EVENT;
      status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
#endif
      
#ifndef CONSOLE_UART_ENABLED
      if(status == WiFi_MODULE_SUCCESS) {
        //prevent the OK received after socket close command to be Q'ed
        IO_status_flag.prevent_push_OK_event       = WIFI_TRUE;
      }
#endif
    }
}

#ifdef SPWF04
/**
* @brief  Server_Pending_Data
*         Query pending data.It will returns the number of bytes of data waiting on socket
* @param  None
* @retval void
*/
void Server_Pending_Data()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  /* "AT+S.SOCKDQ=%d,%d\r" */
  Reset_AT_CMD_Buffer();

#if defined(CONSOLE_UART_ENABLED)
  wait_for_command_mode();
#endif
  
  if(open_server_sockets[WiFi_Counter_Variables.sockdon_query_id])
    {
      if(WiFi_Control_Variables.stop_event_dequeue == WIFI_FALSE)
        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;
      #if defined(CONSOLE_UART_ENABLED)
      sprintf((char*)WiFi_AT_Cmd_Buff,AT_SERVER_SOCKET_QUERY,WiFi_Counter_Variables.sockdon_query_id, WiFi_Counter_Variables.sockon_query_id);        
      status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
      #else
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SOCKDQ=%d,%d",WiFi_Counter_Variables.sockdon_query_id, WiFi_Counter_Variables.sockon_query_id);        
      run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
      //@CHECK-TBD!!: Changing AT_event_processing state without checking status flag
      IO_status_flag.AT_event_processing = WIFI_SERVER_SOCKET_QUERY_EVENT;
      status = WiFi_MODULE_SUCCESS;//@TBD: Check return error code from run_spi_cmd
      #endif
#ifndef CONSOLE_UART_ENABLED
      if(status == WiFi_MODULE_SUCCESS) { 
        //prevent the OK received after socket close command to be Q'ed
        IO_status_flag.prevent_push_OK_event       = WIFI_TRUE;
      }
#endif
    }
}
#endif

/**
* @brief  wifi_socket_server_open
*         Open a Server socket
* @param  None
* @retval WiFi_Status_t : return status of server socket request
*/
#ifdef SPWF04
WiFi_Status_t wifi_socket_server_open(uint32_t port_number, uint8_t *protocol, uint8_t *sock_id)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Queue_Server_Open_Event(port_number,protocol);
  status = USART_Receive_AT_Resp( );
  *sock_id = WiFi_Counter_Variables.Server_Socket_Open_ID; //return the socket id to the user
  return status;
}
#else
WiFi_Status_t wifi_socket_server_open(uint32_t port_number, uint8_t * protocol)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  Queue_Server_Open_Event(port_number,protocol);
  status = USART_Receive_AT_Resp( );

  return status;
}

#endif

/**
* @brief  wifi_socket_server_write
*         Write to a Server socket
* @param  server_id 
*         client_id
*         Datalength
* @retval WiFi_Status_t : return status of server socket request
*/
#ifdef SPWF04
WiFi_Status_t wifi_socket_server_write(uint8_t server_id, uint8_t client_id, uint16_t DataLength,char *Data) 
{
  /* AT+S.SOCKW=00,11<cr> */
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  //Check if sock_id is open
  if(!open_server_sockets[server_id])
    return WiFi_NOT_READY;

  if(DataLength>=1021 || DataLength<=0)
    return WiFi_NOT_SUPPORTED;

  Queue_Server_Write_Event(server_id, client_id, DataLength,Data);
  status = USART_Receive_AT_Resp( );
  return status;
}
#else


WiFi_Status_t wifi_socket_server_write(uint16_t DataLength,char * Data) 
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  /*Can only write if there is a client connected*/
  if(!wifi_client_connected)

      return WiFi_NOT_READY;
  
  if(DataLength == 0)
    return WiFi_MODULE_ERROR;
    

  __disable_irq();

  WiFi_Control_Variables.do_not_reset_push_WIFI_event = WIFI_TRUE;
  WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
  __enable_irq();

  while(IO_status_flag.sock_read_ongoing || IO_status_flag.WIND64_count!= 0)//wait till any pending data is read
    {
      __NOP(); //nothing to do
    }

  wait_for_command_mode();

  /*to make sure that by default the mode is not switched to command mode from data mode*/
  WiFi_Control_Variables.switch_by_default_to_command_mode = WIFI_FALSE;

  /*Switch to Data Mode first*/
  if(!IO_status_flag.data_mode)
    {
      WiFi_switch_to_data_mode();//switch by default
      while(!IO_status_flag.data_mode && wifi_client_connected)
        {
          //Wait till data_mode is active
          __NOP(); //nothing to do
        }
      // if client got disconnected
      if(!wifi_client_connected)
        {
          WiFi_Control_Variables.switch_by_default_to_command_mode = WIFI_TRUE;  /*back to default behaviour*/
          __disable_irq();
          WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;
          WiFi_Control_Variables.do_not_reset_push_WIFI_event = WIFI_FALSE;
          __enable_irq();
          return WiFi_MODULE_ERROR;
        }
    }

  /*Write the data on the uart*/
  if(HAL_UART_Transmit(&UartWiFiHandle, (uint8_t *)Data, DataLength,1000)!= HAL_OK)
    {
      Error_Handler();
      status = WiFi_HAL_UART_ERROR;
    }
  //HAL_Delay(100);//Wait for tx before switching back to command mode

  /*Switch back to Command Mode*/
  if(!IO_status_flag.command_mode)
    {
      WiFi_switch_to_command_mode();//switch by default
      while(!IO_status_flag.command_mode)
        {
          //Wait till command_mode is active
          __NOP(); //nothing to do
        }
    }

  WiFi_Control_Variables.switch_by_default_to_command_mode = WIFI_TRUE;  /*back to default behaviour*/

  __disable_irq();
  WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;
  WiFi_Control_Variables.do_not_reset_push_WIFI_event = WIFI_FALSE;
  __enable_irq();
  
  return status;
}
#endif

/**
* @brief  Server Socket Close
*         Queue the server socket close command
* @param  count: the number of arguments 
* @retval WiFi_Status_t : return status of server socket request
*/
#ifdef SPWF04
WiFi_Status_t socket_server_close(int count,...)
{
  va_list v;
  va_start(v,count);
  int sid = va_arg(v,int);
  int cid;
  (count == 2) ? (cid = va_arg(v,int)): (cid = 9);
  Queue_Server_Close_Event(sid,cid);
  va_end(v);
  return WiFi_MODULE_SUCCESS;
}
#else
/**
* @brief  Server Socket Close
*         Close a Server socket
* @param  None
* @retval WiFi_Status_t : return status of server socket request
*/
WiFi_Status_t wifi_socket_server_close() 
{
  Queue_Server_Close_Event(-1,-1);
  return WiFi_MODULE_SUCCESS; 
}
#endif

#ifdef CONSOLE_UART_ENABLED
/**
* @brief  wait_for_command_mode
*         Waits till we are in command mode
* @param  None
* @retval None
*/
void wait_for_command_mode(void)
{
    while(!IO_status_flag.command_mode)
      {
        //Make sure we are in command mode, ideally we should do this in every User API?
        __NOP(); //nothing to do
      }
}
#endif


/**
* @brief  wifi_file_rename
*         Rename an existing file
* @param  FileName : Filename to be renamed
*         Mod_FileName : New File Name
* @retval WiFi_Status_t : return status of AT cmd request
*/
WiFi_Status_t wifi_file_rename(char *FileName, char *Mod_FileName)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  if(FileName ==NULL || Mod_FileName == NULL)
    return WiFi_MODULE_ERROR;

  Queue_File_Event(FileName,Mod_FileName,-1,-1);
  status = USART_Receive_AT_Resp( );
  return status;
}

#ifdef SPWF04
/**
* @brief  wifi_unmount_user_memory
*         Unmount/erase user memory volumes
* @param  Volume : Indicates the memory volume. Default: 0.  <1:User Flash 0:SD Card>
*         Erase  : Default: 0. when 1, the specified volume is erased.
* @retval WiFi_Status_t : return status of AT cmd request
*/
WiFi_Status_t wifi_unmount_user_memory(int8_t volume, int8_t erase)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  volume = (volume==-1)?0:volume;
  erase = (erase==-1)?0:erase;
  Queue_File_Event(NULL,NULL,volume,erase);
  status = USART_Receive_AT_Resp( );
  return status; 
}
#endif

/**
* @brief  wifi_file_delete
*         Delete a file
* @param  pFileName : File Name to be deleted
* @retval WiFi_Status_t : return status of delete file request
*/
WiFi_Status_t wifi_file_delete(char *FileName)
{
  if(FileName==NULL)
    return WiFi_MODULE_ERROR;

  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  Queue_File_Event(FileName,NULL,-1,-1);
  
  status = USART_Receive_AT_Resp( );

  return status; 
}


/**
* @brief  wifi_file_list
*         List existing filename
* @param  None
* @retval WiFi_Status_t : return status of AT cmd request
*/

WiFi_Status_t wifi_file_list()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  Queue_Wifi_File_Event(NULL,NULL,0,-1,-1);
  status = USART_Receive_AT_Resp( );
  return status;
}

#ifdef SPWF01
/**
* @brief  wifi_file_print_content
*         Print the contents of an existing file
* @param  FileName : pinter of file name
* @retval WiFi_Status_t : return status of AT cmd request
*/
WiFi_Status_t wifi_file_print_content(uint8_t * FileName)
{
  if(FileName==NULL)
      return WiFi_MODULE_ERROR;

  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  Queue_Wifi_File_Event(NULL,FileName,0,-1,-1);
  status = USART_Receive_AT_Resp( );
  return status; 
}
#else

/**
* @brief  wifi_file_print_content
*         Print the contents of an existing file
* @param  FileName : pointer to the file name
*         offset   : Byte from where the file will be printf (Default: 0) 
*         length   : Number of bytes that are printed (Default: Filesize-offset)
* @retval WiFi_Status_t : return status of AT cmd request
*/
WiFi_Status_t wifi_file_print_content(uint8_t *FileName, int16_t offset, int16_t length)

{
  if(FileName == NULL)
    return WiFi_MODULE_ERROR;

  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  Queue_Wifi_File_Event(NULL,FileName,0,offset,length);
  status = USART_Receive_AT_Resp( );
  return status; 
}
#endif

/**
* @brief  wifi_file_create
*         Create file for HTTP server
* @param  FileName : pointer of file name to be created
*         length   : length of file
*         UserFileBuff : pointer to the buffer
* @retval WiFi_Status_t : return status of AT cmd request
*/

WiFi_Status_t wifi_file_create(char *FileName, uint16_t length, char *UserFileBuff)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  Reset_AT_CMD_Buffer();
 
  if(FileName==NULL || UserFileBuff==NULL)
    return WiFi_MODULE_ERROR;

  if(length > 1024)
    return WiFi_AT_FILE_LENGTH_ERROR;

  /* AT+S.FSC=/index.html */
  sprintf((char*)WiFi_AT_Cmd_Buff, AT_CREATE_FILE, FileName, length);

  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if (status != WiFi_MODULE_SUCCESS)
      return WiFi_MODULE_ERROR;
    else
    {
#ifdef SPWF01
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS)  return status;

    /* AT+S.FSA=/index.html */
    sprintf((char*)WiFi_AT_Cmd_Buff,AT_APPEND_FILE,FileName,length);

    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status != WiFi_MODULE_SUCCESS) return status;
#endif

    Reset_AT_CMD_Buffer();
    memcpy((char*)WiFi_AT_Cmd_Buff, (char*) UserFileBuff,length);
    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
    if(status == WiFi_MODULE_SUCCESS) {
      status = USART_Receive_AT_Resp();
      if(status != WiFi_MODULE_SUCCESS) return status;
    }
  }
  return status;
}

/**
* @brief  wifi_http_get
*         Issue an HTTP GET of the given path to the specified host
* @param  None
* @retval WiFi_Status_t : return status of AT cmd response
*/

WiFi_Status_t wifi_http_get(uint8_t * hostname, uint8_t * path, uint32_t port_number)
{
  if(hostname == NULL || path == NULL)
    return WiFi_MODULE_ERROR;
  
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;

  while(IO_status_flag.sock_read_ongoing || IO_status_flag.WIND64_count!= 0) //wait till any pending data is read
    {
      __NOP(); //nothing to do
    }

  // AT+S.HTTPGET=host.example.com,/index.html, port_number<cr>
  //Queue the http-get command
  Queue_Http_Event(hostname, path, port_number,0);

  //Make the user wait anyway
  status = USART_Receive_AT_Resp( );

  return status; 
}

/**
* @brief  wifi_http_post
*         Issue an HTTP GET of the given path to the specified host
* @param  None
* @retval WiFi_Status_t : status of Http Post Request
*/

WiFi_Status_t wifi_http_post(uint8_t * hostname, uint8_t * path, uint32_t port_number)
{
  if(hostname == NULL || path == NULL)
    return WiFi_MODULE_ERROR;
  
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  while(IO_status_flag.sock_read_ongoing || IO_status_flag.WIND64_count!= 0)//wait till any pending data is read
    {
      __NOP(); //nothing to do
    }
  
  // AT+S.HTTPPOST = posttestserver.com,/post.php,name=demo&email=mymail&subject=subj&body=message<cr>
  Queue_Http_Event(hostname, path, port_number,1);
  
  //Make the user wait anyway
  status = USART_Receive_AT_Resp( );
  return status;
}

/**
* @brief  wifi_file_image_create
*         Downloads an updated file system via a single HTTP GET request to the
*         named host and path.
* @param  None
* @retval WiFi_Status_t
*/
WiFi_Status_t wifi_file_image_create(uint8_t * HostName, uint8_t * FileName, uint32_t port_number)
{
  if(HostName == NULL || FileName == NULL ||  port_number ==0)
    return WiFi_MODULE_ERROR;
  
    WiFi_Status_t status = WiFi_MODULE_SUCCESS;
    Queue_Wifi_File_Event(HostName,FileName,port_number,-1,-1);
    status = USART_Receive_AT_Resp( );
    
     if(status == WiFi_MODULE_SUCCESS) 
    {
      /* Soft reset the module */
      #ifdef SPWF04
        Soft_Reset();
      #else
        SET_Power_State(PowerSave_State); 
      #endif
    }
    return status;
}

/**
* @brief  wifi_file_erase_external_flash
*         This API allows to erase the content of the external flash
* @param  None
* @retval WiFi_Status_t
*/
WiFi_Status_t wifi_file_erase_external_flash()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  

  Reset_AT_CMD_Buffer();
  ResetBuffer();

  /* AT+S.HTTPDFSERASE */
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_ERASE_FLASH_MEMORY);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));

  /* Soft reset the module */
#ifdef SPWF04
  /* Soft reset the module */
   Soft_Reset();
#else
  /* Soft reset the module */
   SET_Power_State(PowerSave_State); 
#endif

   return status;
}

/**
* @brief  wifi_fw_update
*         Issue an HTTP GET of the given path to the specified host and get the firmware updated
* @param  None
* @retval None
*/
WiFi_Status_t wifi_fw_update(uint8_t * hostname, uint8_t * filename_path, uint32_t port_number)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  Queue_Wifi_FW_Update_Event(hostname,filename_path,port_number);
  status = USART_Receive_AT_Resp( );
  
  if(status == WiFi_MODULE_SUCCESS) {
#if defined(CONSOLE_UART_ENABLED)
      /* Soft reset the module */
      #ifdef SPWF04
         Soft_Reset();
      #else
         SET_Power_State(PowerSave_State); 
      #endif
#else
      printf("\r\nFW Updated...\r\nResetting module...");
      Reset_AT_CMD_Buffer();
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.RESET");
      run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);     
      //status = USART_Receive_AT_Resp( );//there is no OK after this?
      //wifi_connected = 0; //reset wifi_connected to get user callback
      IO_status_flag.WiFi_WIND_State = Undefine_state;
      while(IO_status_flag.WiFi_WIND_State != WiFiHWStarted)
      {
        //__NOP(); //nothing to do
      }
#endif
    }
  return status;
}

/**
* @brief  wifi_network_scan
*         Performs an immediate scan for available network
* @param  None
* @retval WiFi_Status_t : WiFi status error
*/
WiFi_Status_t wifi_network_scan(wifi_scan *scan_result, uint16_t max_scan_number)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  wifi_scanned_list = scan_result;
  if(max_scan_number>MAX_WIFI_SCAN_NETWORK)
    return WiFi_NOT_SUPPORTED;
  WiFi_Counter_Variables.user_scan_number = max_scan_number;
  
  if(WiFi_Control_Variables.Scan_Ongoing)
  {
    return WiFi_AT_CMD_BUSY;
  }
  
  WiFi_Control_Variables.Scan_Ongoing = WIFI_TRUE;

  /* AT+S.SCAN: performs an immediate scan for available networks */
  Reset_AT_CMD_Buffer();
#if defined(CONSOLE_UART_ENABLED)
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WiFi_SCAN);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.SCAN=s,NULL");
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  IO_status_flag.AT_event_processing = WIFI_SCAN_EVENT;
  status = WiFi_MODULE_SUCCESS;
#endif
  
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
  }
  
  /*At this point we have WiFi_Scan_Buffer filled with RSSI and SSID values*/
  return status;
}

/**
* @brief  Set_MiniAP_Mode
*         Configure Wi-Fi module in AP mode.
          MiniAP is always configured in open mode (WEP not supported)
* @param  None
* @retval WiFi_Status_t : status of AT cmd 
*/
WiFi_Status_t wifi_ap_start(uint8_t * ssid, uint8_t channel_num)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  
  /* Set the SSID : AT+S.SSIDTXT=<SSID>*/    
  if(ssid)
      status = SET_SSID((char*)ssid);
  else 
    /* default SSID : AT+S.SSIDTXT=SPWF_AP*/
    status = SET_SSID(WiFi_Config_Variables.wifi_ssid);
  
  if(status != WiFi_MODULE_SUCCESS)
    return WiFi_SSID_ERROR;

#if defined(CONSOLE_UART_ENABLED)
  /* Set the network privacy mode : AT+S.SCFG=wifi_priv_mode,0*/ 
   status = SET_Configuration_Value(WIFI_PRIV_MODE, None);
  if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;
  
  /* Set the network mode (1 = STA, 2 = IBSS, 3 = MiniAP) :: AT+S.SCFG=wifi_mode,3*/  
   status = SET_Configuration_Value(WIFI_MODE, WiFi_MiniAP_MODE);
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;
   
  /* Set the channel number */  
   status = SET_Configuration_Value(WIFI_CHANNEL_NUMBER, channel_num);
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;    
      
  /* Save the settings on the flash memory : AT&W*/ 
   Save_Current_Setting();
   WiFi_Control_Variables.WiFi_Configuration_Done = WIFI_TRUE;
   
#ifdef SPWF04
  /* Soft reset the module */
   Soft_Reset();
#else
  /* Soft reset the module */
   SET_Power_State(PowerSave_State); 
#endif
   
#else
   /* Set radio OFF */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 0);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
  
   /* Set the network privacy mode : AT+S.SCFG=wifi_priv_mode,0*/ 
   status = SET_Configuration_Value(WIFI_PRIV_MODE, None);
  if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;
  
  /* Set the network mode (1 = STA, 2 = IBSS, 3 = MiniAP) :: AT+S.SCFG=wifi_mode,3*/  
   status = SET_Configuration_Value(WIFI_MODE, WiFi_MiniAP_MODE);
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;
   
  /* Set the channel number */  
   status = SET_Configuration_Value(WIFI_CHANNEL_NUMBER, channel_num);
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR; 
  
  /* Set radio ON */  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 1);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
  WiFi_Control_Variables.WiFi_Configuration_Done = WIFI_TRUE;
  
#endif
  
  return status; 
}

/**
* @brief  SET_WiFi_STA_Mode
*         Configure Wi-Fi module in STA mode
* @param  SSID     : SSID name
* @param  sec_key  : security key
* @param  priv_mode : network privecy mode
* @retval WiFi_Status_t : status of AT cmd 
*/
WiFi_Status_t wifi_connect(char * ssid, char * sec_key, WiFi_Priv_Mode priv_mode)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
   if(WiFi_Control_Variables.AT_Cmd_Ongoing == WIFI_FALSE)
    WiFi_Control_Variables.AT_Cmd_Ongoing = WIFI_TRUE;
  else 
  {
    return WiFi_AT_CMD_BUSY;
  }
  
  if(sec_key) {
    status = SET_WiFi_SecKey((char*)sec_key);
    if(status != WiFi_MODULE_SUCCESS)
        return WiFi_SecKey_ERROR;
  }
 
  /* Set the SSID : AT+S.SSIDTXT=<SSID>*/    
  if(ssid)
      status = SET_SSID((char*)ssid);
  else 
    /* default SSID : AT+S.SSIDTXT=SPWF_STA*/
    status = SET_SSID(WiFi_Config_Variables.wifi_ssid);
  
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_SSID_ERROR;

#if defined(CONSOLE_UART_ENABLED)
   
#ifdef SPWF04
   /*AT+S.WIFI=0<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 0);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
  while(IO_status_flag.WiFi_WIND_State != WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
#endif
  
   /* Set the network privacy mode : AT+S.SCFG=wifi_priv_mode,2*/ 
   status = SET_Configuration_Value(WIFI_PRIV_MODE, priv_mode);
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;
  
   /* Set the network mode (1 = STA, 2 = IBSS, 3 = MiniAP) :: AT+S.SCFG=wifi_mode,1*/  
   status = SET_Configuration_Value(WIFI_MODE, WiFi_STA_MODE);
   if(status != WiFi_MODULE_SUCCESS)
    return WiFi_CONFIG_ERROR;

#ifdef SPWF04
  /*AT+S.WIFI=1<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 1);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
#else
  /* Save the settings on the flash memory : AT&W*/ 
  Save_Current_Setting();
  
  SET_Power_State(PowerSave_State);
#endif
  
#else
  
   /* Set radio OFF */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 0);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
  
   /* Set the network privacy mode : AT+S.SCFG=wifi_priv_mode,2 */  
  status = SET_Configuration_Value(WIFI_PRIV_MODE, priv_mode);
  if(status != WiFi_MODULE_SUCCESS) return status;
  
  /* Set wifi_mode to STA */  
  status = SET_Configuration_Value(WIFI_MODE, WiFi_STA_MODE);
  if(status != WiFi_MODULE_SUCCESS) return status;
  
  /* Set radio ON */  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 1);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
  
#endif
  
  WiFi_Control_Variables.WiFi_Configuration_Done = WIFI_TRUE;  
  WiFi_Control_Variables.AT_Cmd_Ongoing = WIFI_FALSE;

  return status; 
}

/**
* @brief  SET_WiFi_IBSS_Mode
*         Configure Wi-Fi module in IBSS mode
* @param  SSID     : SSID name
* @param  priv_mode : network privecy mode
* @retval WiFi_Status_t : status of AT cmd
*/
WiFi_Status_t wifi_adhoc_create(uint8_t * ssid, WiFi_Priv_Mode priv_mode)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  /* Set the SSID : AT+S.SSIDTXT=<SSID>*/    
  status = SET_SSID((char*)ssid);  
  if(status != WiFi_MODULE_SUCCESS)
    return WiFi_SSID_ERROR;
  
#ifdef SPWF04
/*AT+S.WIFI=0<cr>*/
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 0);
  
#ifdef CONSOLE_UART_ENABLED
status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
#else
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;  
#endif //end of CONSOLE_UART_ENABLED

    while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
#endif // end of SPWF04

  /* Set the network privacy mode : AT+S.SCFG=wifi_priv_mode,0*/ 
    status = SET_Configuration_Value(WIFI_PRIV_MODE, priv_mode);
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;  

  /* Set the network mode (1 = STA, 2 = IBSS, 3 = MiniAP) :: AT+S.SCFG=wifi_mode,2*/  
    status = SET_Configuration_Value(WIFI_MODE, WiFi_IBSS_MODE);
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;

  /* Set IP address */
    status = SET_Configuration_Addr(WIFI_IP_ADDRESS, WIFI_IBSS_IP_ADDR);
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;

  /* Set IP default gateway */
    status = SET_Configuration_Addr(WIFI_IP_DEFAULT_GATEWAY, WIFI_IBSS_DEFAULT_GATEWAY);
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;  
  
  /* Set IP DNS */
#ifdef SPWF04
    status = SET_Configuration_Addr(WIFI_IP_DNS1, WIFI_IBSS_IP_DNS_ADDR);
#else
    status = SET_Configuration_Addr(WIFI_IP_DNS, WIFI_IBSS_IP_DNS_ADDR);
#endif
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;

  /* Set IP netmask */
    status = SET_Configuration_Addr(WIFI_IP_NETMASK, WIFI_IBSS_IP_MASK);
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;

  /* Turn OFF the DHCP */
    SET_Configuration_Value(IP_USE_DHCP_SERVER, WIFI_IP_USE_DHCP);   
    if(status != WiFi_MODULE_SUCCESS)
      return WiFi_CONFIG_ERROR;

#ifdef SPWF04  
  /*AT+S.WIFI=1<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 1);  

#ifdef CONSOLE_UART_ENABLED
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
#else
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
#endif //end of CONSOLE_UART_ENABLED
  
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
#endif  //end of SPWF04  

/* Save the settings on the flash memory : AT&W*/ 
  Save_Current_Setting();

/* Soft reset the module */
#ifdef SPWF04
  Soft_Reset();
#else
  SET_Power_State(PowerSave_State);   
#endif

return status;
}

/**
* @brief  wifi_standby
*         Configured WiFi module to enter standby
* @param  arg_standby_time: standby time
* @retval WiFi_Status_t : status of AT cmd
*/
WiFi_Status_t wifi_standby(uint8_t arg_standby_time)
{
  /*
  For Standby, the presence of Jumpers on JP4 and JP3 has the following behaviour:
  JP3 (middle and bottom): prevents standby and immediately wakes-up module
  JP3 (middle and top): no effect on standby
  JP4 (middle and right): prevents wakeup and standby runs forever
  JP4 (middle and left): no effect on standby
  */
  
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  /*if(arg_standby_time<2)
    return WiFi_NOT_SUPPORTED;*/
      
  SET_Configuration_Value(WIFI_SLEEP_ENABLED, 0);  
  SET_Configuration_Value(WIFI_STANDBY_ENABLED, 1);  
  status = SET_Configuration_Value(WIFI_STANDBY_TIME, arg_standby_time);  
  
  /* save current setting in flash */
  Save_Current_Setting();

  /* Soft reset the module */
#ifdef SPWF04
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_RESET);  
#else
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_SET_POWER_STATE,1);  //cfun=4
#endif
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  return status;
}

/**
* @brief  wifi_wakeup
*         wakeup the module from sleep by setting the GPIO6 through PC13
*         or allow it to go to sleep
*         Jumper needed on JP4
* @param  wakeup wakeup (WIFI_TRUE) or allow sleep(WIFI_FALSE)
* @retval WiFi_Status_t : status of function call 
*/
WiFi_Status_t wifi_wakeup(wifi_bool wakeup)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;    
  
  GPIO_InitTypeDef  WAKEUP_InitStruct;
  RESET_WAKEUP_GPIO_CLK_ENABLE();
  
  WAKEUP_InitStruct.Pin       = WiFi_WAKEUP_GPIO_PIN;
  WAKEUP_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
  WAKEUP_InitStruct.Pull      = GPIO_NOPULL;
  WAKEUP_InitStruct.Speed     = GPIO_SPEED_HIGH;

  HAL_GPIO_Init(WiFi_WAKEUP_GPIO_PORT, &WAKEUP_InitStruct);
  
  if(wakeup)
    HAL_GPIO_WritePin(WiFi_WAKEUP_GPIO_PORT, WiFi_WAKEUP_GPIO_PIN, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(WiFi_WAKEUP_GPIO_PORT, WiFi_WAKEUP_GPIO_PIN, GPIO_PIN_RESET);
  
  return status;
}

/**
* @brief  wifi_disconnect
*         disconnect the module from any AP
* @param  None
* @retval WiFi_Status_t : status of AT cmd
*/
WiFi_Status_t wifi_disconnect(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
  
  Reset_AT_CMD_Buffer();  

#ifdef SPWF04  
  /*AT+S.WIFI=0<cr>*/  
#if defined(CONSOLE_UART_ENABLED)
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 0);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 0);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = WiFi_MODULE_SUCCESS;
#endif
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
  
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
#endif
  
  /* Set wifi_mode to idle*/
  status = SET_Configuration_Value(WIFI_MODE, WiFi_IDLE_MODE);
  
  /*If module was connected, reset the status*/
  if(wifi_connected == 1)
      {
        wifi_connected = 0;//this will allow the TIM handler to make the callback on connection(WiFiUp)
      }  

#ifdef SPWF04  
  /*AT+S.WIFI=1<cr>*/  
  Reset_AT_CMD_Buffer(); 
  #if defined(CONSOLE_UART_ENABLED)
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 1);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 1);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = WiFi_MODULE_SUCCESS;
#endif
  
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_FALSE;
  }  
  
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
#else

  /* save current setting in flash */
  Save_Current_Setting();
#endif
  
  /* Soft reset the module */
#if defined (SPWF04)
  //status = Soft_Reset();
#else
  status = SET_Power_State((WiFi_Power_State_t)PowerSave_State);//CFUN=1
#endif
       
  return status;
}

/**
* @brief  wifi_enable
*         Enable/Disable the Wi-Fi interface
* @param  enable enable Wi-Fi (WIFI_TRUE) disable Wi-Fi (WIFI_FALSE)
* @retval WiFi_Status_t : status of AT cmd
*/
WiFi_Status_t wifi_enable(wifi_bool enable)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  if((enable && IO_status_flag.WiFi_Enabled == WIFI_TRUE) || (!enable && IO_status_flag.WiFi_Enabled == WIFI_FALSE))
    return status;//already in desired state
    
  /* Enable or Disable wifi interface*/  
  Reset_AT_CMD_Buffer();  
  
#if defined(CONSOLE_UART_ENABLED)
  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, enable);  
  if(HAL_UART_Transmit(&UartWiFiHandle, (uint8_t *)WiFi_AT_Cmd_Buff, strlen((char*)WiFi_AT_Cmd_Buff),1000)!= HAL_OK)
  {
    Error_Handler();
    #if DEBUG_PRINT
    printf("HAL_UART_Transmit Error");
    #endif
    return WiFi_HAL_UART_ERROR;
  }  
  
#else
  /*AT+S.WIFI=0<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 0);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = WiFi_MODULE_SUCCESS;  
  
#endif
  
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
  
  //wait for power down/hw started
  if(enable)
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE) {
        __NOP(); //nothing to do
      }
  else
    while(IO_status_flag.WiFi_Enabled != WIFI_FALSE) {
        __NOP(); //nothing to do
      }
  
  return status;
}

/**
* @brief  wifi_restore
*         Restore the Wi-Fi with default values.
* @param  None
* @retval WiFi_Status_t : status of AT cmd
*/
WiFi_Status_t wifi_restore()
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  
  /* Restore default setting*/
  Restore_Default_Setting();  
  
#if defined(CONSOLE_UART_ENABLED)
#ifdef SPWF04    
  /*AT+S.WIFI=0<cr>*/  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 0);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_TRUE;
  }
  
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
#endif  
  /* Set wifi_mode to idle*/
  SET_Configuration_Value(WIFI_MODE, WiFi_IDLE_MODE);
#ifdef SPWF04    
  /*AT+S.WIFI=1<cr>*/  
  Restore_Default_Setting();  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_WIFI_ENABLE, 1);  
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
  if(status == WiFi_MODULE_SUCCESS)
  {
    status = USART_Receive_AT_Resp( );
    if(status != WiFi_MODULE_SUCCESS) return status;
    else IO_status_flag.radio_off = WIFI_FALSE;
  }
  
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
#endif  
#else
  
  /* Set radio OFF */
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 0);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_WIND_State!= WiFiPowerDown);//Till +WIND:38:WiFi Powered Down arrives
  
   /* Set wifi_mode to idle*/
  SET_Configuration_Value(WIFI_MODE, WiFi_IDLE_MODE);
  
  /* Set radio ON */  
  Reset_AT_CMD_Buffer();  
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.WIFI=%d", 1);  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  status = USART_Receive_AT_Resp( );
  if(status != WiFi_MODULE_SUCCESS) return status;
  while(IO_status_flag.WiFi_Enabled != WIFI_TRUE);//Till Hardware Started arrives
  
#endif  
  
  /* set the local echo */
  SET_Configuration_Value(LOCALECHO1, 0); 
  
  /* Soft reset the module */
#if defined (SPWF04)
  //status = Soft_Reset();
#else
  /* save current setting in flash */
  Save_Current_Setting();
  
  status = SET_Power_State((WiFi_Power_State_t)PowerSave_State);//CFUN=1
#endif
  
  return status;
}

/*GPIO Configuration Functions*/

/**
* @brief  wifi_gpio_init
*         Configure a GPIO pin as in or out with IRQ setting
* @param  pin GPIO pin number
* @param  irq configuration of the pin
* @retval WiFi_Status_t : status of AT cmd
*/
uint8_t wifi_gpio_init(GpioPin pin, char* dir, char irq)
{
    WiFi_Status_t status = WiFi_MODULE_SUCCESS;
    
    /* AT+S.GPIOC=pin,dir,irq */
    Reset_AT_CMD_Buffer();
#if defined(CONSOLE_UART_ENABLED)
    if(irq!=GPIO_Off)
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOC=%d,%s,%c\r", pin, dir, irq);
    else
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOC=%d,%s\r", pin, dir);
    
    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
    if(irq!=GPIO_Off)
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOC=%d,%s,%c", pin, dir, irq);
    else
      sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOC=%d,%s", pin, dir);
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
* @brief  wifi_gpio_read
*         Read the configuration of a GPIO pin
* @param  pin GPIO pin number
* @param  val value returned
* @param  dir configuration direction returned
* @retval WiFi_Status_t : status of AT cmd
*/
uint8_t wifi_gpio_read(GpioPin pin, uint8_t *val, uint8_t *dir)
{
    WiFi_Status_t status = WiFi_MODULE_SUCCESS;    

    /* AT+S.GPIOR=pin */
    Reset_AT_CMD_Buffer();  
#if defined(CONSOLE_UART_ENABLED)
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOR=%d\r", pin);
    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOR=%d", pin);
    run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
    IO_status_flag.AT_event_processing = WIFI_GPIO_EVENT;
    status = WiFi_MODULE_SUCCESS;
#endif

    if(status == WiFi_MODULE_SUCCESS)
      {
        status = USART_Receive_AT_Resp( );
      }
    *val = WiFi_Counter_Variables.gpio_value;
    *dir = WiFi_Counter_Variables.gpio_dir;

    return status;
}

/**
* @brief  wifi_gpio_write
*         Read the value of a GPIO pin
* @param  pin GPIO pin number
* @param  val value to be configured
* @retval WiFi_Status_t : status of AT cmd
*/
uint8_t wifi_gpio_write(GpioPin pin, GpioWriteValue value)
{
    WiFi_Status_t status = WiFi_MODULE_SUCCESS;

    /* AT+S.GPIOW=pin,value */
    Reset_AT_CMD_Buffer(); 
#if defined(CONSOLE_UART_ENABLED)
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOW=%d,%d\r", pin, value);

    status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
    sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GPIOW=%d,%d", pin, value);
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
* @brief  wifi_get_IP_address
*         Get the ip address
* @param  ip_addr : pointer to ip address
* @retval status  : status of AT cmd request
*/
WiFi_Status_t wifi_get_IP_address(uint8_t *ip_addr)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  int cfg_value_length;

   /* AT : send AT command */
  Reset_AT_CMD_Buffer(); 
#if defined(CONSOLE_UART_ENABLED)  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_GET_STATUS_VALUE,"ip_ipaddr");
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.STS=%s", "ip_ipaddr");  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  IO_status_flag.AT_event_processing = WIFI_GCFG_EVENT;
  status = WiFi_MODULE_SUCCESS;
#endif
  
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      cfg_value_length = strlen((const char*)WiFi_Counter_Variables.get_cfg_value);
      
      /* copy user pointer to get_cfg_value */
      memcpy(ip_addr,WiFi_Counter_Variables.get_cfg_value,cfg_value_length);   
      memset(WiFi_Counter_Variables.get_cfg_value, 0x00,cfg_value_length);
    }
  return status;
}

/**
* @brief  WiFi_get_MAC_address
*         Get the MAC address
* @param  ip_addr : pointer to MAC address
* @retval status  : status of AT cmd request
*/
WiFi_Status_t wifi_get_MAC_address(uint8_t *mac_addr)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  int cfg_value_length;

  /* AT : send AT command */
  Reset_AT_CMD_Buffer(); 
  #if defined(CONSOLE_UART_ENABLED)  
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_GET_CONFIGURATION_VALUE,"nv_wifi_macaddr");
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.GCFG=%s", "nv_wifi_macaddr");  
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  IO_status_flag.AT_event_processing = WIFI_GCFG_EVENT;
  status = WiFi_MODULE_SUCCESS;
#endif   
  
  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      cfg_value_length = strlen((const char*)WiFi_Counter_Variables.get_cfg_value);
      
      /* copy user pointer to get_cfg_value */
      memcpy(mac_addr,&WiFi_Counter_Variables.get_cfg_value,cfg_value_length);  //IP address will have max length fixed at 18 bytes.
      memset(WiFi_Counter_Variables.get_cfg_value, 0x00,cfg_value_length);
    }
  return status;
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

