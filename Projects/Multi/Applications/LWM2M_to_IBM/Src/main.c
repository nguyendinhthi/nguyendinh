/**
******************************************************************************
  * @file    main.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M To IBM Cloud, main file.
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
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
/* main file for the LWM2M to IBM gateway. Here the initializations are performed,
 * then the FSM for WiFi and IBM connection (so, the sensor data in JSON format)
 * is implemented. */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "lwm2m-engine.h"
#include "process.h"
#include "contiki.h"
#include "net/ip/uip.h"
#include "cube_hal.h"
#include "radio_shield_config.h"
#include "spirit1.h"
#include "jsonparse.h"
#include "wifi_globals.h"
#include "wifi_interface.h"
#include "wifi_support.h"
#include "IBM_Bluemix_Config.h"
#include "lwm2m-simple-server.h"
#include "lwm2m-data.h"
#include "lwm2m-resource-directory.h"

/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @defgroup LWM2M_to_IBM
  * @{
  */

/*------------------------------------------------------------------------------*/
#define DEBUG DEBUG_NONE
#if DEBUG
#include <stdio.h>
#define LWM2M_PRINTF(...) printf(__VA_ARGS__)
#else
#define LWM2M_PRINTF(...)
#endif
/*------------------------------------------------------------------------------*/
/* Local Configuration----------------------------------------------------------*/
#define UART_BAUD_RATE          115200
#define READ_PERIPHERAL_TIMEOUT 1000
#define MQTT_TIMEOUT            4000 // in ms
#define WIFI_ENABLE             1 // 1 to use Wi-Fi module
#define USE_SPIRIT              1 // 1 To use SPIRIT module
#define COMMANDS_ENABLE         1 // 1 to receive commands from IBM, in registered mode
/*---------------------------------------------------------------------------*/
/*To regulate how many digits for the decimal value, i.e. 2 means 23.546 -> 23.55 */
#define FLOAT_DECIMAL_PRECISION_DIGITS 2
#define FLOAT_DECIMAL_PRECISION (pow(10, FLOAT_DECIMAL_PRECISION_DIGITS))
/*---------------------------------------------------------------------------*/
/* Buffers lengths -----------------------------------------------*/
#define JSON_BUFFER_LEN   512
#define MQTT_BUFFER_LEN   512
#define WIFI_DATA_LEN     256
#define PSK_LEN            20
#define HOST_LEN           20
#define SSID_LEN           40
#define URL_LEN            80
#define MAC_LEN            17
#define NAME_RES_LEN       50
#define TEMP_BUFF_LEN     256
#define NAME_RES_IBM_LEN   40
#define MAC_TEMP_LEN       64
/*---------------------------------------------------------------------------*/
/* Function prototypes ------------------------------------------------------*/
void WiFi_Init(WiFi_Status_t* status);
void WiFi_Process(WiFi_Status_t* status);
extern WiFi_Status_t GET_Configuration_Value(char* sVar_name,uint32_t *aValue);
void Get_MAC_Add (uint8_t *macadd);
/*----------------------------------------------------------------------------*/
void MQTT_Init(void);
void UART_Msg_Gpio_Init();
void USART_PRINT_MSG_Configuration(UART_HandleTypeDef *UART_MsgHandle, uint32_t baud_rate);
/*----------------------------------------------------------------------------*/
static void floatfix32ToInt(floatfix32_t in, uint8_t *negative, int32_t *out_int, uint32_t *out_dec, int32_t dec_prec);
/*------------------------------------------------------------------------------*/
void prepare_json_pkt(uint8_t* buffer);
static int add_data_point(char *nameResIBM);
static int add_cmd_response_point(char *nameResIBM);
/*---------------------------------------------------------------------------*/
void Stack_6LoWPAN_Init(void);
/*---------------------------------------------------------------------------*/
/* Process declared in lwm2m-simple-server.c, it is "polled" when a commend is
 * received from cloud */
extern struct process command_process;
/*---------------------------------------------------------------------------*/
/* Command received from cloud, it will be processed by the  "command_process"
 * implemented in lwm2m-simple-server.c file                                 */
char cloud_command_str[CMD_BUFFER_LEN] = "";
/*---------------------------------------------------------------------------*/
PROCESS(wifi_process,"WiFi Process");
/*---------------------------------------------------------------------------*/
/* WiFi related Variables ---------------------------------------------------------*/
wifi_state_t wifi_state;
wifi_config config;
wifi_scan net_scan[WIFI_SCAN_BUFFER_LIST];
uint8_t wifi_data[WIFI_DATA_LEN];
uint8_t socket_id;
uint8_t socket_open = 0;
uint8_t macadd[MAC_LEN];
extern uWifiTokenInfo UnionWifiToken;
extern WiFi_Priv_Mode mode;
extern wifi_bool set_AP_config;
extern wifi_bool SSID_found;
#if 0
  /* Default configuration SSID/PWD */
  char * ssid = "STM";
  char * seckey = "STMdemoPWD";
#else
  char * ssid = NULL;
  char * seckey = NULL;
#endif
  /*---------------------------------------------------------------------------*/
uint8_t json_buffer[JSON_BUFFER_LEN];
/*---------------------------------------------------------------------------*/
/* MQTT Variables -----------------------------------------------------------*/
Network  n;
Client  c;
MQTTMessage  MQTT_msg;
uint8_t url_ibm[URL_LEN];
MQTT_vars mqtt_ibm_setup;
MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
unsigned char MQTT_read_buf[MQTT_BUFFER_LEN];
unsigned char MQTT_write_buf[MQTT_BUFFER_LEN];
/*---------------------------------------------------------------------------*/
/* UART ---------------------------------------------------------------------*/
UART_HandleTypeDef UART_MsgHandle;
/*---------------------------------------------------------------------------*/
/* Variables from lwm2m-data.c, used to compose the JSON buffer -------------*/
extern lwm2m_srv_data_res_t active_observations[MAX_OBS_];// Array of all the observable resources according to the Template
extern uint32_t numObs;  			                      // Number of Objects/Resources to be observed
extern lwm2m_srv_data_res_t active_get[MAX_GET_];         // Array of all the "gettable" resources according to the Template
extern uint32_t numGet;  			                      // Number of Objects/Resources to be periodically gotten
extern lwm2m_srv_cmd_res_t cmd_get[MAX_CMD_];             // Array of the commands for GETs coming from the cloud.
extern uint8_t numCmd;                                    // Number of concurrent GET commands
/*---------------------------------------------------------------------------*/
/* Global variables used to create the JSON packet, if the DATAPOINT_CNT is
 * lesser than the available data points, global_index, being static, is used
 * to send a new set of data in the next JSON message */
static uint32_t global_index = 0;
/*---------------------------------------------------------------------------*/
/* Main function */
/**
* @brief  Main Program
* 	      Initializes HAL structures, the SPWF01SA (wifi) and calls the contiki main
* @param  None
* @retval int: 1 success, 0 failure
*/
int main()
{
  uint32_t board_uid[3] = {0, 0, 0};

#if WIFI_ENABLE
	WiFi_Status_t status = WiFi_MODULE_SUCCESS;
#endif
    MX_GPIO_Init();
    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();
    SystemCoreClockUpdate();

    HAL_EnableDBGStopMode();
    
    /* Initialize LEDs */
    BSP_LED_Init(LED2);

    RadioShieldLedInit(RADIO_SHIELD_LED);

    //BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);

#if WIFI_ENABLE
    /* Configure the timers */
    Timer_Config( );
#endif
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
  printf("\r * LWM2M to IBM Cloud Application                       *\n");
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

#if WIFI_ENABLE
  /* Initialize WiFi module and connection to the Access Point */
  WiFi_Init(&status);
  /* Initialize MQTT & IBM related parts */
  MQTT_Init();
#endif


  /* Initialize RTC */
  RTC_Config();
  RTC_TimeStampConfig();

  init_lwm2m_data_structures();
  init_lwm2m_nodes_structure();

#if USE_SPIRIT
    Stack_6LoWPAN_Init();
    printf("LWM2M to IBM Watson Gateway parameters:\n");
    PRINT_PARAMETER(MAX_NODES_, "%d");
    PRINT_PARAMETER(MAX_OBS_, "%d");
    PRINT_PARAMETER(MAX_GET_, "%d");
#endif
#if WIFI_ENABLE
    process_start(&wifi_process,NULL);
#endif

    while(1) {
#if USE_SPIRIT
      int r = 0;
      do {
        r = process_run();
      } while(r > 0);
#endif
    }

    return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  wifi_process
*         Contiki PROCESS_THREAD that calls in a temporized loop the WiFi_Process
*         function.
* @param process_event_t ev: As per Contiki Thread API
* @param process_data_t data: As per Contiki Thread API
* @retval None
*/
PROCESS_THREAD(wifi_process, ev, data){
	PROCESS_BEGIN();
	static struct etimer timer_wifi;
	WiFi_Status_t status_wifi = WiFi_MODULE_SUCCESS;

	while(1){
		etimer_set(&timer_wifi, CLOCK_SECOND * 1);
		PROCESS_YIELD();
		if(etimer_expired(&timer_wifi)){
			WiFi_Process(&status_wifi);
		}
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Start the WiFi process
  * @param  The WiFi status
  * @retval None
  */
void WiFi_Process(WiFi_Status_t* status)
{
  uint8_t i = 0;
  uint8_t wifi_channelnum[4];

  switch (wifi_state) {
  case wifi_state_reset:
    break;

  case wifi_state_ready:
    *status = wifi_network_scan(net_scan, WIFI_SCAN_BUFFER_LIST);

    if(*status==WiFi_MODULE_SUCCESS) {
      for (i=0; i<WIFI_SCAN_BUFFER_LIST; i++) {
        if(( (char *) strstr((const char *)net_scan[i].ssid,(const char *)WIFITOKEN.NetworkSSID)) !=NULL) {
          printf("\r\n >>network present...connecting to AP...\r\n");
          SSID_found = WIFI_TRUE;
		  if(!((*status =  wifi_connect(WIFITOKEN.NetworkSSID, WIFITOKEN.NetworkKey, mode))== WiFi_MODULE_SUCCESS))
		  {
			printf("\r\n >>Wrong SSID-KEY Detected. Please try again...\r\n");
		  }
          break;
        }
      }
      if(!SSID_found) {
        printf("\r\nGiven SSID not found!\r\n");
      }
      memset(net_scan, 0x00, sizeof(net_scan));
    } else {
	  printf("\r\n >>Scan failure...\r\n");
	}

    wifi_state = wifi_state_idle;

    break;

  case wifi_state_connected:
    HAL_Delay(2000); //Let module go to sleep
    printf("\r\n >>connected...\r\n");

    wifi_wakeup(WIFI_TRUE); /* wakeup from sleep if module went to sleep */
    wifi_state = wifi_state_idle;
    break;

  case wifi_state_disconnected:
    wifi_state = wifi_state_reset;
    break;

  case mqtt_socket_create:
    if(spwf_socket_create (&n,mqtt_ibm_setup.hostname, mqtt_ibm_setup.port, &mqtt_ibm_setup.protocol)<0) {
      printf("\r\n [E]. Socket opening with IBM MQTT broker failed. Please check MQTT configuration. Trying reconnection.... \r\n");
      printf((char*)mqtt_ibm_setup.hostname);
      printf("\r\n");
      printf((char*)(&mqtt_ibm_setup.protocol));
      printf("\r\n");
      wifi_state=mqtt_socket_create;
    } else {
      /* Initialize MQTT client structure */
      MQTTClient(&c,&n, MQTT_TIMEOUT, MQTT_write_buf, sizeof(MQTT_write_buf), MQTT_read_buf, sizeof(MQTT_read_buf));
      wifi_state=mqtt_connect;

      printf("\r\n [D]. Created socket with MQTT broker. \r\n");
    }
    break;

  case mqtt_connect:
	  printf("Connect state \n");
    options.MQTTVersion = 3;
    options.clientID.cstring = (char*)mqtt_ibm_setup.clientid;
    options.username.cstring = (char*)mqtt_ibm_setup.username;
    options.password.cstring = (char*)mqtt_ibm_setup.password;
    if (MQTTConnect(&c, &options) < 0) {
      printf("\r\n [E]. Client connection with IBM MQTT broker failed. Please check MQTT configuration. Trying reconnection....\r\n");
      wifi_state=mqtt_connect;
    } else {
      if (mqtt_ibm_setup.ibm_mode == QUICKSTART) {
        printf("\r\n [D]. Connected with IBM MQTT broker for Quickstart mode (only MQTT publish supported) \r\n");
        /* Quickstart mode. We only publish data. */
        wifi_state=mqtt_pub;
      } else {
        printf("\r\n [D]. Connected with IBM MQTT broker for Registered devices mode (requires account on IBM Bluemix; publish/subscribe supported) \r\n");
        /* Registered device mode. */
        wifi_state=mqtt_sub;
      }
    }
    HAL_Delay(1500);

    GET_Configuration_Value("wifi_channelnum",(uint32_t *)wifi_channelnum);
    printf("wifi_channelnum = %d\n", (uint8_t)atoi((char const *)wifi_channelnum));

    break;

  case mqtt_sub:
    /* Subscribe to topic */
    if( MQTTSubscribe(&c, (char*)mqtt_ibm_setup.sub_topic, mqtt_ibm_setup.qos, messageArrived) < 0) {
      printf("\r\n [E]. Subscribe with IBM MQTT broker failed. Please check MQTT configuration. \r\n");
    } else {
      printf("\r\n [D] Subscribed to topic:  \r\n");
      printf((char *)mqtt_ibm_setup.sub_topic);
    }
    HAL_Delay(1500);
    wifi_state=mqtt_pub;
    break;

  case mqtt_pub:
	LWM2M_PRINTF("Publication state \n");
    /* Prepare MQTT message */
    prepare_json_pkt(json_buffer);
    MQTT_msg.qos        = QOS0;
    MQTT_msg.dup        = 0;
    MQTT_msg.retained   = 1;
    MQTT_msg.payload    = (char *)json_buffer;
    MQTT_msg.payloadlen = strlen((char *)json_buffer);

    printf("*** Sending JSON message '%s'\n", json_buffer);
    /* Publish MQTT message */
    if (MQTTPublish(&c,(char*)mqtt_ibm_setup.pub_topic,&MQTT_msg) < 0) {
      printf("\r\n [E]. Failed to publish data. Reconnecting to MQTT broker.... \r\n");
      wifi_state=mqtt_connect;
    } else {
      if (mqtt_ibm_setup.ibm_mode == REGISTERED) {
        printf("\r\n [D]. Wait some sec to see if any data is received \r\n");
#if COMMANDS_ENABLE
        /* Wait 2*READ_PERIPHERAL_TIMEOUT seconds to see if data is received */
        MQTTYield(&c, 2*READ_PERIPHERAL_TIMEOUT);
#endif
      }
    }
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
/*---------------------------------------------------------------------------*/
/**
* @brief  WiFi_Init
*         WiFi Initialization, it also calls the ConfigAPSettings() function to
*         retrieve SSID and Password from Flash memory.
* @param WiFi_Status_t* status: To be filled with the result of the initialization
* @retval None
*/
void WiFi_Init(WiFi_Status_t* status)
{
  /* Config WiFi: disable low power mode */
  config.power = wifi_active;
  config.power_level=high;
  config.dhcp=on; //use DHCP IP address
  config.ht_mode = WIFI_FALSE;
  config.web_server=WIFI_TRUE;

  wifi_state = wifi_state_idle;

  printf("\r\n\nInitializing the wifi module...");
  printf("\r\n");
  fflush(stdout);

  /* Init the wi-fi module */
  *status = wifi_init(&config);
  if(*status!=WiFi_MODULE_SUCCESS) {
    printf("Error in Config");
    return;
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
      printf("\r\nFailed to set AP settings.");
      return;
    } else {
      printf("\r\nAP settings set.");
    }
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @brief  Initialize MQTT & IBM related parts
 * @param  None
 * @retval None
 */
void MQTT_Init(void)
{
  /* Initialize network interface for MQTT */
  NewNetwork(&n);
  /* Initialize MQTT timers */
  MQTTtimer_init();
  /* Get MAC address from WiFi */
  Get_MAC_Add (macadd);

  /* Config MQTT pars for IBM */
  Config_MQTT_IBM ( &mqtt_ibm_setup, macadd );
  /* Compose Quickstart URL */
  Compose_Quickstart_URL (url_ibm, macadd);
  printf("\r\n [D] IBM Quickstart URL\r\n https://");
  printf((char *)url_ibm);
  printf("\r\n");
  printf("\r\n");
  HAL_Delay(200);
}

/*---------------------------------------------------------------------------*/
/******** Wi-Fi Indication User Callbacks *********/
/*---------------------------------------------------------------------------*/
void ind_wifi_on()
{
  printf("\r\nWiFi Initialized and Ready..\r\n");
  wifi_state = wifi_state_ready;
}
/*---------------------------------------------------------------------------*/
void ind_wifi_connected()
{
  wifi_state = mqtt_socket_create;
}
/*---------------------------------------------------------------------------*/
void ind_wifi_resuming()
{
  printf("\r\nwifi resuming from sleep user callback... \r\n");
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Wifi callback activated when an error occurs on the connection with the AP
  * @param  WiFi_Status_t WiFi_DE_AUTH
  * @retval None
  */
void ind_wifi_connection_error(WiFi_Status_t WiFi_DE_AUTH)
{
  printf("\r\n [E]. WiFi connection error. Trying to reconnect to WiFi... \r\n");
  wifi_state=wifi_state_ready;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Wifi callback activated when remote server is closed
  * @param  socket_closed_id : socket identifier
  * @retval None
  */
void ind_wifi_socket_client_remote_server_closed(uint8_t * socket_closed_id)
{
   printf("\r\n[E]. Remote disconnection from server. Trying to reconnect to MQTT broker... \r\n");
   wifi_state = mqtt_socket_create;
}
/*---------------------------------------------------------------------------*/
/**
 * @brief prepare_json_pkt
 *        This function prepares the JSON packet to be sent. If the device is running in registered mode,
 *        and the commands are enabled, as a first thing it looks if we have some pending reply (by the nodes)
 *        to a GET received from the cloud. If this is the case, one of this data (that should be somehow more urgent,
 *        is retrieved and sent as unique datapoint in the JSON packet.
 *        If there are no pending answers, it then collects up to DATAPOINT_CNT values and store them in the JSON
 *        buffer, taking them from both the Observations and periodic GETs arrays. If the total number of entries in these
 *        arrays is greater than DATAPOINT_CNT, the buffer will be filled with this number and the collection of datapoint will
 *        start (for the next JSON packet) from the first one that has not been served in this iteration.
 * @param  uint8_t *buffer: Buffer that will contain sensor data in JSON format
 * @retval None
 */
void prepare_json_pkt (uint8_t * buffer)
{
  char nameResIBM[NAME_RES_IBM_LEN];
  uint32_t datapoint_cnt = 0;
  uint8_t buffer_full = 0;
  uint32_t data_point_first_index = global_index;

  memset(nameResIBM, 0x00, sizeof(nameResIBM));
  memset(buffer, 0x00, sizeof(json_buffer));

  strcpy((char *)buffer,"{\"d\":{\"myName\":\"Node6LoWPAN\"");
#if COMMANDS_ENABLE
  if (mqtt_ibm_setup.ibm_mode == REGISTERED) {
	  printf("Adding cmd response...\n");
	  if (add_cmd_response_point(nameResIBM)){
		  LWM2M_PRINTF("GOT CMD RESPONSE:\n");
		  LWM2M_PRINTF("%s\n",nameResIBM);
		  if (strlen((const char *)nameResIBM) < sizeof(buffer) - strlen((char*) buffer) - 3) {
			  strcat((char *)buffer, (const char *) nameResIBM);
			  strcat((char *)buffer,"}}");
			  /*If we have a command response we send only this.*/
			  return;
		  }
	  }
  }
#endif /*COMMANDS_ENABLE*/
  while(datapoint_cnt < DATAPOINT_CNT && !buffer_full){
	  LWM2M_PRINTF("Prepare JSON: calling add_data_point (index = %lu)...\n", global_index);
	  strcpy(nameResIBM, "");
	  if (add_data_point(nameResIBM)) {
		  LWM2M_PRINTF("GOT Data Point:\n");
		  LWM2M_PRINTF("'%s'\n",nameResIBM);
		  datapoint_cnt++;
		  if (strlen((const char *) nameResIBM) < sizeof(buffer) - strlen((char *) buffer) - 3) {
			  strcat((char *)buffer, (const char *) nameResIBM);
		  } else {
			  buffer_full = 1;
		  }
		  if (data_point_first_index == global_index){
			  LWM2M_PRINTF("JSON: got all the valid datapoints (index = %lu)\n", global_index);
			  /*It means we retrieved all the available valid datapoints (global_index is incremented in add_data_point function) */
			  break;
		  }
	  } else {
		  LWM2M_PRINTF("GOT NO Data Point on this call.\n");
		  /* No datapoint retrieved from current call */
		  break;
	  }
  }
  if (datapoint_cnt > 0) {
	  strcat((char *)buffer,"}}");
  } else {
	  printf("No datapoint retrieved, resetting the buffer\n");
	  strcpy((char *) buffer, "");
  }

  return;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  GET MAC Address from WiFi
  * @param  uint8_t *macadd: Buffer for MAC data
  * @retval None
  */
void Get_MAC_Add (uint8_t *macadd)
{
  uint8_t mactemp[MAC_TEMP_LEN]="";
  uint16_t i,j;

  GET_Configuration_Value("nv_wifi_macaddr",(uint32_t *)mactemp);
  mactemp[17] = '\0';

  printf("\r\n [D]. WiFI Board MAC Address: '%s'\r\n", mactemp);

  for(i=0,j=0;i<strlen((char *)mactemp);i++) {
    if(mactemp[i]!=':'){
      macadd[j]=mactemp[i];
      j++;
    }
  }
  macadd[j]='\0';

  return;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  MQTT Callback function
  * @param  MessageData *md: MQTT received message
  * @retval None
  */
void messageArrived(MessageData* md)
{
  MQTTMessage* message = md->message;

  printf("\r\n [D]. MQTT received payload is: '\r\n");
  printf((char*)message->payload);
  printf("'\r\n");
#if COMMANDS_ENABLE
  memset(cloud_command_str,0,sizeof(cloud_command_str));
  strncpy(cloud_command_str, message->payload, message->payloadlen);
  memset((uint8_t *)message->payload, 0x00, message->payloadlen);
  process_poll(&command_process);
#endif
 }
/*---------------------------------------------------------------------------*/
/**
  * @brief  Convert Float (floatfix32) to Integer
  * @param  floatfix32_t in: Value to be converted
  * @param  uint8_t *negative: Sign conversion result
  * @param  int32_t *out_int: Integer conversion result
  * @param  uint32_t *out_dec: Decimal conversion result\
  * @param  int32_t dec_prec: Precision as power of 10
  * @retval None
  */
static void floatfix32ToInt(floatfix32_t in, uint8_t *negative, int32_t *out_int, uint32_t *out_dec, int32_t dec_prec)
{
	double value = (double) in / LWM2M_FLOAT32_FRAC;

	*negative = 0;
	if (value < 0 ) *negative = 1;
	*out_int = (int32_t)(value);
	value = value - *out_int ;
	if (value < 0) value = -value;
	*out_dec = (uint32_t)round(value * dec_prec);
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  GET a DataPoint to be sent to IBM from the Command Response slots which have been filled with the data
  * @param  char *nameResIBM: buffer to store the current data point
  * @retval int: 1 on success, 0 on failure (no available datapoint)
  */
static int add_cmd_response_point(char *nameResIBM)
{
	int cnt = 0;
	char resource_name[NAME_RES_LEN]="";
	lwm2m_srv_cmd_res_t *data_p = NULL;
    int32_t integer = 0;
	uint32_t decimal = 0;
	uint8_t negative = 0; //useful to detect negative numbers with integer part == 0.

	for (cnt=0; cnt< MAX_CMD_; cnt++){
		if (cmd_get[cnt].flags == (CMD_SET | CMD_ANS_GOT)){
			LWM2M_PRINTF("Got a slot (%d) for a Command Response to serve\n", cnt);
			data_p = &cmd_get[cnt] ;
			if (data_p == NULL){
				ERROR_OUT("Unexpected null pointer for command response\n");
			}
			strncpy(resource_name, data_p->uri, strlen(data_p->uri));

			switch(data_p->res_type) {
			case RES_INT:
				sprintf(nameResIBM, ",\"%s_Node_%lu\":%d", resource_name, data_p->node_id, data_p->value.valueInt);
				break;
			case RES_FLOAT:
				floatfix32ToInt(data_p->value.valueFloat,	&negative, &integer, &decimal, FLOAT_DECIMAL_PRECISION);
				if (integer || !negative) {
					sprintf(nameResIBM, ",\"%s_Node_%lu\":%ld.%0" xstr(FLOAT_DECIMAL_PRECISION_DIGITS) "lu", resource_name, data_p->node_id, integer, decimal);
				} else {
					/*We explicitely put the '-' sign since integer part is '0' and the number must be negative. */
					sprintf(nameResIBM, ",\"%s_Node_%lu\":-0.%0" xstr(FLOAT_DECIMAL_PRECISION_DIGITS) "lu", resource_name, data_p->node_id, decimal);
				}
				break;
			case RES_STRING:
				sprintf(nameResIBM,",\"%s_Node_%lu\":\"%s\"", resource_name, data_p->node_id, data_p->value.valueString);
				break;
			default:
				ERROR_OUT("Unexpected error in resource type (%d)\n", data_p->res_type);
				break;
			}
			cmd_get[cnt].flags = CMD_DONE;
			return 1;
		}
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  GET a DataPoint to be sent to IBM from the OBS/GET arrays which have been filled with the data
  * @param  char *nameResIBM: buffer to store the current data point
  * @retval int: 1 on success, 0 on failure (no available datapoint)
  */
static int add_data_point(char *nameResIBM)
{
	uint8_t found = 0;
    int32_t integer = 0;
	uint32_t decimal = 0;
	uint8_t negative = 0; //useful to detect negative numbers with integer part == 0.
	uint32_t local_index = 0;
	uint32_t start_index = 0;
	lwm2m_srv_data_res_t * data_p = NULL;
	char resource_name[NAME_RES_LEN]="";

	//LWM2M_PRINTF("Total GET %d, total OBS %d\n", numGet, numObs);
	memset(resource_name, 0x00, sizeof(resource_name));

	if (!numGet && !numObs){
		return 0;
	}

	start_index = global_index ;
    while(!found){
    	if (global_index < numGet){
    		local_index = global_index ;
    		if (active_get[local_index].valid){
				LWM2M_PRINTF("Found GET data to send at position %lu (total possible data points %lu)\n", local_index, global_index);
    			data_p = &active_get[local_index];
    			found = 1 ;
    		}
    	} else {
    		local_index = global_index - numGet ;
    		if (local_index < numObs){
				if (active_observations[local_index].valid){
					LWM2M_PRINTF("Found OBS data to send at position %lu (total possible data points %lu)\n", local_index, global_index);
					data_p = &active_observations[local_index];
					found = 1 ;
				}
    		}
    	}
    	global_index++;
    	if (global_index == numObs + numGet){
    		global_index = 0;
    	}
    	if (global_index == start_index && !found){
    		LWM2M_PRINTF("No valid data found.\n");
    		/* No "valid" entry */
    		return 0;
    	}
    }

    if (data_p == NULL) {
		ERROR_OUT("Unexpected error: NULL datapointer.\n");
    	return 0;
    }

    /* So if we are here we found the next valid entry in GETs or OBS arrays */
#if USE_LWM2M_FORMAT_FOR_DATAPOINT_NAME
	strncpy(resource_name, data_p->uri, strlen(data_p->uri));
#else
	strncpy(resource_name, data_p->readable_name, strlen(data_p->readable_name));
#endif

    switch(data_p->res_type) {
    case RES_INT:
		sprintf(nameResIBM, ",\"%s_Node_%lu\":%d", resource_name, data_p->node_id, data_p->value.valueInt);
    	break;
	case RES_FLOAT:
		floatfix32ToInt(data_p->value.valueFloat, &negative, &integer, &decimal, FLOAT_DECIMAL_PRECISION);
		if (integer || !negative) {
			sprintf(nameResIBM, ",\"%s_Node_%lu\":%ld.%0" xstr(FLOAT_DECIMAL_PRECISION_DIGITS) "lu", resource_name, data_p->node_id, integer, decimal);
		} else {
			/*We explicitely put the '-' sign since integer part is '0' and the number must be negative. */
			sprintf(nameResIBM, ",\"%s_Node_%lu\":-0.%0" xstr(FLOAT_DECIMAL_PRECISION_DIGITS) "lu", resource_name, data_p->node_id, decimal);
		}
		break;
	case RES_STRING:
	    sprintf(nameResIBM,",\"%s_Node_%lu\":\"%s\"", resource_name, data_p->node_id, data_p->value.valueString);
		break;
	default:
		ERROR_OUT("Unexpected error in resource type\n");
		break;
    }

    return 1;
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
/*---------------------------------------------------------------------------*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/**
 * @}
 */
/**
 * @}
 */
