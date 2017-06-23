  /**
  ******************************************************************************
  * @file    wifi_module.h
  * @author  Central LAB 
  * @version V2.0.1
  * @date    17-May-2016
  * @brief   Header file for Wi-Fi module
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
#ifndef __WIFI_MODULE_H
#define __WIFI_MODULE_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/
#include "stm32_spwf_wifi.h"
#include "wifi_const.h"   
#include "wifi_interface.h"
#include "event_buffer.h"
#include "ring_buffer.h"
#include "spwf04WiFi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
   
/** @addtogroup MIDDLEWARES
* @{
*/ 


/** @addtogroup  NUCLEO_WIFI_MODULE 
  * @brief Wi-Fi_driver modules
  * @{
  */ 


/** @addtogroup NUCLEO_WIFI_MODULE_Private_Macros
  * @{
  */
   
//#define USART3_INT_MODE
#define USART3_POLLING_MODE
 /**
  * @}
  */


/** @addtogroup NUCLEO_WIFI_MODULE_Private_Variables
  * @{
  */
/* Private variables ---------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/******* Wi-Fi Configuration Setting Parameters *****************/
    
/**
  * @brief  WiFi config values structure definition
  */    
typedef struct
{
  char* ip_ipaddr;
  char* wifi_ssid;  
  char* ip_hostname;
  char* Wifi_Sec_key;
  char* console1_speed;

  uint8_t wifi_mode;
  uint8_t standby_time;
  uint8_t blink_led :1;
  uint8_t* IBSS_IP_Addr;
  uint8_t* IBSS_IP_Mask;
  uint8_t wifi_priv_mode;
  uint8_t wifi_powersave;
  uint8_t sleep_enabled :1;
  uint8_t* IBSS_IP_DNS_Addr;
  uint8_t wifi_beacon_wakeup;
  uint8_t standby_enabled :1;
  uint8_t wifi_listen_interval;
  uint8_t* IBSS_DefaultGateway;
  uint8_t wifi_operational_mode;
}WiFi_Config_HandleTypeDef;

/**
  * @brief  WiFi control variables structure definition
  */ 
#pragma pack(1)
typedef struct
{
  unsigned int switch_by_default_to_command_mode: 1;
  unsigned int queue_wifi_wind_message: 1;
  unsigned int http_req_pending       : 1;
  unsigned int WiFi_Configuration_Done: 1;
  unsigned int stop_event_dequeue     : 1;
  unsigned int Standby_Timer_Running  : 1;
  unsigned int trigger_wakeup_callback: 1;
  unsigned int Deep_Sleep_Enabled     : 1;
  unsigned int Standby_Enabled        : 1;
  unsigned int Scan_Ongoing           : 1;
  unsigned int AT_Cmd_Ongoing         : 1;
  unsigned int AT_Cmd_Processing      : 1;
  unsigned int Uartx_Rx_Processing    : 1;
  unsigned int Client_Connected       : 1;
  unsigned int Client_Disconnected    : 1;
  unsigned int start_sock_read        : 1;
  unsigned int enable_receive_data_chunk: 1;
  unsigned int data_pending_sockD     : 1;
  unsigned int enable_sock_read       : 1;
  unsigned int enable_query           : 1;
  unsigned int afterOK_enable_sock_read       : 1;
  unsigned int afterOK_enable_query           : 1;
  unsigned int enable_fw_update_read  : 1;
  unsigned int Q_Contains_Message     : 1;
  unsigned int enable_receive_http_response: 1;
  unsigned int enable_receive_file_response: 1;
  unsigned int enable_receive_wifi_scan_response: 1;
  unsigned int prevent_push_WIFI_event: 1;
  unsigned int event_deQ_x_wind64     : 1;
  unsigned int do_not_reset_push_WIFI_event: 1;
  unsigned int message_pending        : 1;
  unsigned int Pending_SockON_Callback: 1;
  unsigned int Pending_SockD_Callback : 1;
  unsigned int SockON_Server_Closed_Callback: 1;
  unsigned int Client_Socket_Close_Cmd: 1;
  unsigned int HTTP_Data_available    : 1;
  unsigned int FILE_Data_available    : 1;
  unsigned int resume_receive_data    : 1;
  unsigned int Deep_Sleep_Timer       : 1;
  unsigned int enable_timeout_timer   : 1;
  unsigned int enable_sock_data       : 1;
  unsigned int wifi_socket_client_close :1;
  uint8_t wait_for_cmd_response       : 1;
  uint8_t enable_SockON_Server_Closed_Callback :1; 
  uint8_t start_sockd_read :1;
  uint8_t afterOK_start_sockd_read :1;
  uint8_t afterOK_start_sock_read :1;
  uint8_t enable_server_query :1;
  uint8_t afterOK_enable_server_query :1;
  uint8_t in_standby_mode :1;
  uint8_t close_specific_client:1;
  uint8_t is_secure_socket:1;
}WiFi_Control_Variables_t;
#pragma pack()
/**
  * @brief  WiFi mode enumeration definition
  */ 
typedef enum 
{
  WiFi_IDLE_MODE =0,
  WiFi_STA_MODE,
  WiFi_IBSS_MODE,
  WiFi_MiniAP_MODE
} WiFi_Mode_TypeDef;

typedef enum {
  CMD_ID_AT = 0x01,
  CMD_ID_STS = 0x05,
  CMD_ID_RESET = 0x03,
  CMD_ID_GCFG = 0x09,
  CMD_ID_SCFG = 0x0A,
  CMD_ID_WCFG = 0x0B,
  CMD_ID_FCFG = 0x0C,
  
  CMD_ID_FWUPDATE = 0x56,
  CMD_ID_TIME = 0x11,
  CMD_ID_TLSCERT = 0x2B,
  CMD_ID_WIFI = 0x32,//AT+S.WIFI
  CMD_ID_SCAN = 0x33,
  CMD_ID_SSIDTXT = 0x34,
  CMD_ID_PING = 0x39,
  
  CMD_ID_SOCKON = 0x41,
  CMD_ID_SOCKQ = 0x42,
  CMD_ID_SOCKC = 0x43,
  CMD_ID_SOCKW = 0x44,
  CMD_ID_SOCKR = 0x45,
  CMD_ID_SOCKL = 0x46,
  CMD_ID_SOCKDON = 0x47,
  CMD_ID_SOCKDQ = 0x48,
  CMD_ID_SOCKDC = 0x49,
  CMD_ID_SOCKDW = 0x4A,
  CMD_ID_SOCKDR = 0x4B,
  CMD_ID_SOCKDL = 0x4C
} CMD_ID_t;


typedef struct key_value
{
   char* key;
   char* str_value;
   int8_t int_value;
} KEY_VAL_t;

typedef struct spi_cmd {
    KEY_VAL_t key_val[5];
    int32_t params[5];//max 10 parameters for non-key-value params (to be given serially)
    CMD_ID_t id;
    uint8_t num_params;
} SPI_CMD_t;

typedef enum
{
  SPI_HEADER_TRANSMIT,
  SPI_PAYLOAD_TRANSMIT,
  SPI_WRITE_PAYLOAD_TRANSMIT
} SPI_TRANSMIT_REQUEST_t;

typedef enum
{
  SPI_HEADER_TRANSMITTED,
  SPI_PAYLOAD_TRANSMITTED
} SPI_TRANSMIT_EVENT_t;

typedef enum
{
  SPI_READ_HEADER_FOR_RX,
  SPI_READ_HEADER_FOR_TX,
  SPI_READ_PAYLOAD,
  SPI_READ_DATA_PAYLOAD
} SPI_RECEIVE_REQUEST_t;

typedef enum
{
  SPI_RECEIVED_HEADER_FOR_RX,
  SPI_RECEIVED_HEADER_FOR_TX,
  SPI_RECEIVE_PAYLOAD_DATA,
  SPI_RECEIVE_INTERIM_PAYLOAD_DATA,
  SPI_RECEIVE_END
} SPI_RECEIVE_EVENT_t;

typedef enum
{
  SPI_WIND_NOTIFICATION = 0x1,
  SPI_ERROR_NOTIFICATION,
  SPI_INCOMING_DATA,
  SPI_FRAME_ERROR
} KIND_OF_EVENT_t;

typedef enum
{
  SPI_WIND_PACKET = 0x1,
  SPI_WIND_PAYLOAD_PACKET,
  SPI_ERROR_PACKET,
  SPI_DATA_PACKET,
  SPI_DATA_PAYLOAD_PACKET,
  SPI_DATA_INTERIM_PAYLOAD_PACKET,
  SPI_OK_PACKET,
  SPI_IGNORE_PACKET,
} KIND_OF_PACKET_t;

typedef enum
{
  SPI_POLL = 0x1,
  SPI_DMA,
  SPI_IT
} Tx_TYPE_t;

/********** Wi-Fi Indications*************/


/**
  * @brief  WiFi wind state structure definition
  */ 
#pragma pack(1) 
typedef enum
{
  WiFiHWFailure = 0,
  HardFault = 1,
  StackOverFlow,
  Mallocfailed,
  InitFailure,
  StartFailed,
  PS_Mode_Failure,
  HeapTooSmall,
  WiFiSignalLOW,
  WiFiUp,
  WiFiStarted_MiniAPMode,
  WiFiAPClientJoined,
  WiFiAPClientLeft,
  WiFiException,
  WiFiHWStarted,
  WiFiPowerDown,
  WiFiDeauthentication,
  
  /*Wifi Connection Errors*/
  WiFiJoinFailed,
  WiFiScanBlewUp,
  WiFiScanFailed,
  WiFiDeAuth,
  WiFiDisAssociation,
  
  /*Wifi packet lost INDs*/
  WiFiUnHandledInd,
  WiFiRXMgmt,
  WiFiRXData,
  WiFiRxUnk,
  WiFiSockdDataLost,
  Deep_Sleep_Callback,
  standby_resume_callback,
  Undefine_state = 0xFF
} WiFi_WIND_State_TypeDef;
#pragma pack()


/**
  * @brief  WiFi WIND indication enumeration definition
  */ 
typedef enum {
  Invalid_Wind            = -1,
  Console_Active          = 0,
  Poweron                 = 1,
  WiFi_Reset,
  Watchdog_Running,
  Heap_Too_Small,
  WiFi_Hardware_Dead      = 5,
  Watchdog_Terminating,
#if defined(SPWF01) 
  SysTickConfigure,
#else
  Configuration_Failure,/*SPWF04*/
#endif
  Hard_Fault              = 8,   
  StackOverflow,
  MallocFailed,
  Error,
  WiFi_PS_Mode_Failure    = 12,
  CopyrightInfo,
  WiFi_BSS_Regained       = 14,
  WiFi_Signal_LOW         = 15,
  WiFi_Signal_OK          = 16,
  FW_update               = 17,
  Encryption_key_Not_Recognized,
  WiFi_Join               = 19,
  JOINFAILED              = 20,
  WiFi_Scanning           = 21,
  SCANBLEWUP,
  SCANFAILED,
  WiFi_Up                 = 24,
  WiFi_Association_Successful   = 25,
  WiFi_Started_MiniAP_Mode      = 26,
  Start_Failed                  = 27,
  WiFi__MiniAP_Associated       = 28,
  WiFi_BSS_LOST                 = 30,
  WiFi_EXCEPTION          = 31,    
  WiFi_Hardware_Started   = 32,
  WiFi_NETWORK_LOST,
  WiFi_Unhandled_Event,
  Scan_Complete           = 35,
  WiFi_UNHANDLED_IND,
  WiFi_UNHANDLED,
  WiFi_Powered_Down,
  WiFi_MiniAP_Mode        = 39,
  WiFi_Deauthentication   = 40,     
  WiFi_Disassociation,
  RX_MGMT,
  RX_DATA,
  RX_UNK,
  DOT11_AUTHILLEGAL,
  Creating_PSK            = 46,  
  WPA_Terminated          = 49,  
  WPA_Supplicant_Failed,
  WPA_Handshake_Complete  = 51,
  GPIO_line,
  Wakeup,
  Factory_debug,
  SockON_Data_Pending           = 55,
  Input_To_Remote               = 56, /*SPWF04*/
#if defined(SPWF01) 
  Remote_Configuration          = 57,
#else
  Output_From_Remote            = 57,
#endif
  SockON_Server_Socket_Closed   = 58,
  In_Command_Mode         = 59,/*Reserved in SPWF04*/
  In_Data_Mode            = 60,/*Reserved in SPWF04*/
  Incoming_socket_client  = 61,
  Outgoing_socket_client  = 62,
  SockD_Dropping_Data     = 63,
#if defined(SPWF01) 
  SockD_Pending_Data      = 64,/*SPWF01*/
#else
  Remote_Configuration    = 64,/*SPWF04*/
#endif
  Fatory_Reset            = 65,
  Low_Power_Mode_Enabled  = 66,
  Going_Into_Standby      = 67,
  Resuming_From_Standby   = 68,
  Going_Into_DeepSleep    = 69,
  Resuming_From_DeepSleep = 70,
  WiFi_MiniAP_Disassociated     = 72,
  System_Conf_Updated           = 73, /*SPWF04*/
  Rejected_Found_Network  = 74,
  
  /*Additional WINDs for SPWF04*/
  Rejected_Association          = 75,
  SockD_Dropping_Client         = 83,
  NTP_Server_Delivery           = 84,
  MQTT_Published                = 86,
  MQTT_Closed                   = 87,
  Websocket_Data                = 88,
  Websocket_Closed              = 89,
  UDP_Broadcast_Received        = 90,
  TFTP_File_Received            = 91,
  
  Undefine_Indication     = 0xFF
} WiFi_Indication_t;

typedef enum WiFi_Indication_t WiFi_Indication;

/**
  * @brief  WiFi power state enumeration definition
  */ 
typedef enum
{ 
  Active_State,
  PowerSave_State,    
  Sleep_State=3,
  StandBy_State=4
} WiFi_Power_State_t;

/**
  * @brief Structure definition containing instances of other structures
  */ 
typedef struct
{
  buffer_t big_buff;
  wifi_event_buffer event_buff;
  wifi_event_TypeDef wifi_event;
  wifi_event_TypeDef * DeQed_wifi_event;
  HAL_StatusTypeDef receive_status;
  GPIO_InitTypeDef GPIO_InitStruct;
}wifi_instances_t;


/**
  * @brief  WiFi control variables structure definition
  */ 
typedef struct
{
  uint8_t  Socket_Open_ID   :3;
  uint8_t  sockon_query_id  :3;
  uint8_t  sockdon_query_id :3;
  uint8_t  sockon_id_user   :3;
  uint8_t  curr_sockID      :3;
  uint8_t  curr_serverID    :3;
  int8_t   sockdon_id_user  :3;
  uint8_t  closed_socket_id;
  uint8_t  remote_socket_closed_id   :4;
  uint8_t  remote_server_closed_id   :3;
  uint8_t  client_socket_close_id    :3;
  uint8_t  Server_Socket_Open_ID     :3;
  uint8_t  no_of_open_client_sockets :3;
  uint8_t  no_of_open_server_sockets :3;
  uint8_t  wifi_up :2;
  uint8_t  gpio_value;
  uint8_t  gpio_dir;
  uint8_t  http_ind;
  uint8_t  user_scan_number;
  uint8_t  scanned_ssids;
  uint8_t  get_cfg_value[64];
  uint8_t  uart_byte[1];              /* Used to store one byte that is popped from the ring buffer */
  uint8_t  client_MAC_address[17];    /* current client MAC address store */  
  uint8_t * curr_hostname;
  uint8_t * curr_path;
  uint8_t * curr_pURL;
  uint8_t * curr_protocol;
  uint8_t * curr_filename;
  uint8_t * mod_filename;
  uint8_t * temp;       /* pop buffer temporary */

  int8_t volume; 
  int8_t erase;
  
  char * curr_data;
  
  uint16_t curr_DataLength;
  
  int16_t  datalen;
  int16_t  offset;
  
  uint16_t UserDataBuff_index;
  uint16_t chunk_size;
  uint16_t pop_buffer_size;
  uint16_t last_process_buffer_index;
  uint16_t pop_queue_length;
  uint32_t Socket_Data_Length;
  uint32_t SockON_Data_Length;
  uint32_t number_of_bytes;
  uint32_t wind64_DQ_wait;
  uint32_t sock_total_count;
  uint32_t message_size;
  uint32_t interim_number_of_bytes;
  uint32_t epoch_time;
  uint32_t curr_port_number;
  uint32_t sleep_count;
  uint32_t standby_time;
  uint32_t timeout_tick;
  WiFi_Status_t AT_RESPONSE;

  /* Keeps a track of the socket ID that needs to be closed.
     Each array element depicts one socket(true = socket needs to be closed) */
  wifi_bool socket_close_pending[8];
  wifi_bool server_socket_close_pending[2][5];  // 2server socket and 5client per server
}WiFi_Counter_Variables_t;

/**
  * @brief  All __IO type variable structure definition
  */ 
typedef struct
{
  __IO uint8_t sock_read_ongoing     :1;
  __IO uint8_t prevent_push_OK_event :1;
  __IO uint8_t Timer_Running  :1;
  __IO uint8_t enable_dequeue :1;
  __IO uint8_t WiFi_Enabled   :1;
  __IO uint8_t data_mode      :1;
  __IO uint8_t radio_off    :1;
  __IO uint8_t command_mode   :1;
  __IO uint8_t AT_Response_Received  :1;
  __IO uint8_t wifi_ready     :2;       //Set once if wifi is ready for first time
  __IO uint8_t client_socket_close_ongoing :1;
  __IO uint8_t server_socket_close_ongoing :1;
  __IO uint32_t WIND64_count;
  __IO uint32_t tickcount;
  __IO uint32_t AT_CMD_count;
  __IO wifi_bool send_data;

  __IO ITStatus UartReady;
  __IO ITStatus Uart2Ready;
  __IO ITStatus TxUartReady;
  __IO WiFi_Events AT_event_processing; //describes the current Event under processing
#ifdef SPI_VCOM  
  __IO WiFi_Events AT_vcom_processing; //describes the current Event under processing
#endif
  __IO WiFi_WIND_State_TypeDef WiFi_WIND_State; //describes the current WIND number in processing
} IO_status_flag_typedef;


#ifdef WIFI_USE_VCOM
void console_input(void);
#endif

#ifdef SPWF01
	WiFi_Status_t SET_Power_State(WiFi_Power_State_t state);
#endif

void PowerUp_WiFi_Module(void);
void WiFi_Module_Init(void);
void WiFi_Application(void);

/******* Wi-Fi AT CMD SET ****************/
WiFi_Status_t Attention_Cmd(void);
WiFi_Status_t USART_Transmit_AT_Cmd(uint16_t size);
WiFi_Status_t USART_Receive_AT_Resp(void);
WiFi_Status_t Save_Current_Setting(void);
WiFi_Status_t Restore_Default_Setting(void);
WiFi_Status_t SET_SSID(char* ssid);
WiFi_Status_t GET_SSID(void);
WiFi_Status_t SET_Configuration_Value(char* sVar_name,uint32_t aValue);
WiFi_Status_t GET_Configuration_Value(char* sVar_name,uint32_t *aValue);
WiFi_Status_t SET_Configuration_Addr(char* sVar_name,char* addr);
WiFi_Status_t Display_Help_Text(void);
//WiFi_Status_t SET_Power_State(WiFi_Power_State_t state);
WiFi_Status_t Soft_Reset(void);
WiFi_Status_t Read_WiFi_SSID(char *string);
WiFi_Status_t Open_Serial_Port(void);
WiFi_Status_t WaitForResponse(uint16_t alength);
WiFi_Status_t config_init_value(char* sVar_name,uint8_t aValue);
WiFi_Status_t config_init_addr(char* sVar_name,char* addr);
void Process_Wind_Indication(uint8_t *process_buff_ptr);
void Process_DeQed_Wind_Indication(wifi_event_TypeDef * L_DeQued_wifi_event);
void Reset_AT_CMD_Buffer(void);
void USART2_SendBuffer(USART_TypeDef* USARTx, uint8_t *pData, uint8_t length);


char* Delete_Colon(char* );
WiFi_Status_t Read_WiFi_Mode(char *string);
WiFi_Status_t Read_WiFi_SecKey(char *string);

WiFi_Status_t Write_WiFi_SSID(char *string);
WiFi_Status_t Write_WiFi_SecKey(char *string);
WiFi_Status_t SET_WiFi_SecKey(char* seckey);
void PrintErrorMsg (void);
void Print_Msg(char * msgBuff,uint8_t length);
void Error_Handler(void);
void WiFi_Receive_Indication_Msg(void);
char *search_buffer(char *pSourceBuff, uint16_t sourceBuffLen, char *pSearchStringBuff, uint16_t seartchStringLen);

void ResetBuffer(void);
void Start_Timer(void);
void Stop_Timer(void);
void Request_Time_Out(void);
void Start_DeepSleep_Timer(void);
void Stop_DeepSleep_Timer(void);

void HTTP_Read_Data(void);
WiFi_Status_t Socket_Read(uint16_t DataLength);
WiFi_Status_t Socket_Server_Read(uint16_t DataLength);
void Read_Socket_Data(void);
void Socket_Pending_Data(void);
void Server_Pending_Data(void);
void WiFi_switch_to_command_mode(void);
void WiFi_switch_to_data_mode(void);
void WiFi_Configuration(void);
void Set_WiFi_Counter_Variables(void);
void Set_WiFi_Control_Variables(void);

void Receive_Data(void);
void Process_Buffer(uint8_t * ptr);
void Process_WiFi(void);
void Stop_Dequeue(void);
void Resume_Dequeue(void);
void wait_for_command_mode(void);
void Wifi_SysTick_Isr(void);
void RX_EXTI_Isr(uint16_t GPIO_Pin);
void Wifi_TIM_Handler(TIM_HandleTypeDef *htim);


/****** Queue Events ********/
void Queue_Client_Close_Event(uint8_t sock_id);
void Queue_Server_Open_Event(uint32_t port_number, uint8_t * protocol);
void Queue_Client_Write_Event(uint8_t sock_id, uint16_t DataLength, char * pData);
void Queue_Client_Open_Event(uint8_t * hostname, uint32_t port_number, uint8_t * protocol);
void Queue_File_Event(char * FileName, char *Mod_Filename, int8_t volume, int8_t erase);
void Queue_Http_Event(uint8_t * hostname, uint8_t * path, uint32_t port_number,uint8_t http_ind);
void Queue_Wifi_FW_Update_Event(uint8_t * hostname, uint8_t * filename_path, uint32_t port_number);
void Queue_Server_Write_Event(uint8_t server_id, uint8_t client_id, uint16_t DataLength, char * pData);
void Queue_Wifi_File_Event(uint8_t * HostName, uint8_t * FileName, uint32_t port_number, int16_t offset, int16_t length);
void Queue_Server_Close_Event(int8_t server_id, int8_t client_id);
void Wait_For_Sock_Read_To_Complete(void);
void WiFi_HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandleArg);
void WiFi_HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandleArg);
void WiFi_HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle);

void WIFI_SPI_IRQ_Callback(void);
void SPI_Send_AT_Command(int offset, int mode);
void WiFi_DMA_TxCallback(void);
void WiFi_DMA_RxCallback(void);
void WiFi_SPI_Write(uint8_t* header_data, uint8_t* payload_data, uint8_t header_size, uint8_t payload_size);

void ResumePayloadReception(void);
void ResumePacketReception(void);
/*----------------------------- Private variables -------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
  
/**
  * @}
  */
  
#ifdef __cplusplus
  }
#endif
#endif  /* __WIFI_MODULE_H */
