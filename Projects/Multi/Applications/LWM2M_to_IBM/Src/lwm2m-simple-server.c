/**
  ******************************************************************************
  * @file    lwm2m-simple-server.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M simple server
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
/*---------------------------------------------------------------------------*/
/* This file implements the Simple LWM2M Server core, so here we manage the
 * Observations / Notifications, the loop of periodic GETs with their answers
 * and we also process the command received from cloud and, in case it is a
 * READ, the relevant one shot GET is sent and its response is processed.
 * */
/*---------------------------------------------------------------------------*/
#include "ipso-objects.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "uip-ds6.h"
#include "net/ip/resolv.h"
#include "net/ip/ip64-addr.h"
#include "rest-engine.h"
#include "net/ip/uip-debug.h"
#include "dev/temperature-sensor.h"
#include "er-coap.h"
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"
#include "net/ip/uip-debug.h"
#include "er-coap-constants.h"
#include "er-coap-engine.h"
#include "er-coap-observe-client.h"
#include "lwm2m-engine.h"
#include "oma-tlv.h"
#include "dev/serial-line.h"
#include "serial-protocol.h"
#include "lwm2m-simple-server.h"
#include "json.h"
#include "jsonparse.h"
#include "lwm2m-data.h"
/*---------------------------------------------------------------------------*/
/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup LWM2M_to_IBM
  * @{
  */

/** @defgroup LWM2M_to_IBM_simple_server
  * @{
  */

#define DEBUG DEBUG_NONE
//#define DEBUG DEBUG_PRINT
#if DEBUG
#include <stdio.h>
#define LWM2M_PRINTF(...) printf(__VA_ARGS__)
#else
#define LWM2M_PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#define LWM2M_SERVER_ADDRESS_v6 "aaaa::ff:fe00:1"
#undef UIP_DS6_DEFAULT_PREFIX
#define UIP_DS6_DEFAULT_PREFIX 0xaaaa
/*---------------------------------------------------------------------------*/
#define ROUTER_PROCESS_TIMER   4 //in sec
#define JSON_VALUE_STRING_LEN 20
#define JSON_VALUE_NUM_LEN    10
/*---------------------------------------------------------------------------*/
typedef enum { Node, Path, Operation, Type, Value, Nd} state_value_t;
/*---------------------------------------------------------------------------*/
extern resource_t res_rd; /* RD "/rd" resource defined in lwm2m-resource-directory.c */
/*---------------------------------------------------------------------------*/
//Extern Data, from lwm2m-resource-directory.c
extern nodeInfo_t nodes_array[MAX_NODES_]; // Nodes Array structure to store all the information for each registered node
extern uint32_t numNode;                        // Number of registered nodes
/*---------------------------------------------------------------------------*/
//Extern declarations for data structures, from lwm2m-data.c
extern lwm2m_srv_data_res_t active_observations[MAX_OBS_];// Array of all the observable resources according to the Template
extern uint32_t numObs;  			                      // Number of Objects/Resources to be observed
extern lwm2m_srv_data_res_t active_get[MAX_GET_];         // Array of all the "gettable" resources according to the Template
extern uint32_t numGet;  			                      // Number of Objects/Resources to be periodically gotten
extern lwm2m_srv_cmd_res_t cmd_get[MAX_CMD_];             // Array of the commands for GETs coming from the cloud.
/*---------------------------------------------------------------------------*/
/*********** Variables and functions from main.c **************************/
extern char cloud_command_str[CMD_BUFFER_LEN];
/*---------------------------------------------------------------------------*/
static coap_observee_t *obs = NULL;
static uint32_t index_GET = 0;
static uint8_t cmd_id = 0;
static struct jsonparse_state state;
uint8_t type_value_ = RES_NONE;
/*---------------------------------------------------------------------------*/
PROCESS(oma_lwm2m_light_server, "OMA-LWM2M Light Server process");
PROCESS(command_process, "Command IBM process");
AUTOSTART_PROCESSES(&oma_lwm2m_light_server, &command_process);
/*---------------------------------------------------------------------------*/
/**
* @brief  notification_callback
*         Notification Callback to manage responses to observe requests, i.e.
*         "Notifications".
* @param  coap_observee_t *obs:  Pointer to the observee structure related to this observation
* @param  void *notification:  Pointer to the notification (CoAP Response)
* @param  coap_notification_flag_t flag: Response Code
* @retval None
*/
static void
notification_callback(coap_observee_t *obs, void *notification,
                      coap_notification_flag_t flag)
{
  int len = 0;
  oma_tlv_t tlv_notif_value;
  const uint8_t *payload = NULL;
  unsigned int format = 0;
  int32_t value_obs = 0;
  uint32_t curr_obs = 0;

  LWM2M_PRINTF("[NOTIFICATION HANDLER]\n");
  if (obs == NULL){
	  ERROR_OUT("Unexpected error, observee pointer is null.\n");
	  return ;
  }
  LWM2M_PRINTF("Observee URI: '%s'\n", obs->url);
  if(notification) {
    len = coap_get_payload(notification, &payload);
  } else {
	  ERROR_OUT("ERROR IN NOTIFICATION CALLBACK: notification pointer is null.\n");
	  return ;
  }
  switch(flag) {
  case OBSERVE_OK: /* server accepted observation request, First notification is handled here */
  case NOTIFICATION_OK: /* notifications to handle, in the same way as the first reply        */
    LWM2M_PRINTF("NOTIFICATION OK\n");
    LWM2M_PRINTF("CoAP notification packet from: ");
#if DEBUG
    uip_debug_ipaddr_print(&obs->addr);
#endif
    LWM2M_PRINTF(" , path: '%s', Observe: '%lu'\n", (char *) obs->url, obs->last_observe);

    if (!get_observation_by_url(&obs->addr, obs->url, &curr_obs)){
    	printf("Unexpected error, no active observation for \n");
    	uip_debug_ipaddr_print(&obs->addr);
    	printf("/%s found.\n", obs->url);
    	return;
    }
	active_observations[curr_obs].flags = OPR_OBSERVED ;
	LWM2M_PRINTF("Observation %lu, uri '%s', node_id %lu set to OPR_OBSERVED\n", curr_obs, active_observations[curr_obs].uri, active_observations[curr_obs].node_id );
    coap_get_header_content_format(notification, &format);
    if(format == LWM2M_TLV) {
		if(oma_tlv_read(&tlv_notif_value, payload, len) > 0){
			LWM2M_PRINTF("TLV.type='%d' len='%lu' id='%d' value='%ld'\n",
				tlv_notif_value.type, tlv_notif_value.length, tlv_notif_value.id,
				(int32_t)(*((int32_t *)tlv_notif_value.value)));
			switch(active_observations[curr_obs].res_type) {
			case RES_INT:
				value_obs = oma_tlv_get_int32(&tlv_notif_value);
				LWM2M_PRINTF("TLV_INT value: %ld\n", value_obs);
				active_observations[curr_obs].value.valueInt = value_obs;
				break;
			case RES_FLOAT:
				oma_tlv_float32_to_fix(&tlv_notif_value, &value_obs, LWM2M_FLOAT32_BITS);
				LWM2M_PRINTF("TLV_FLOATFIX value: %ld\n", value_obs);
				active_observations[curr_obs].value.valueFloat = value_obs;
				break;
			default:
				LWM2M_PRINTF("Resource type error ('%d') in notification_callback()\n", active_observations[curr_obs].res_type);
				break;
			}
		}
	} else if(format == LWM2M_TEXT_PLAIN) {
		switch(active_observations[curr_obs].res_type){
		case RES_STRING:
			 LWM2M_PRINTF("STRING: '%*s'\n", len, (char*) payload);
			 strcpy(active_observations[curr_obs].value.valueString, (char *) payload);
			 break;
		default:
			 LWM2M_PRINTF("Error in notification_callback()-> Format is TEXT PLAIN but argument ('%d') is not String\n", active_observations[curr_obs].res_type);
			 break;
		}
	}
    break;
  case OBSERVE_NOT_SUPPORTED:
    printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case ERROR_RESPONSE_CODE:
	printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case NO_REPLY_FROM_SERVER:
	printf("NO_REPLY_FROM_SERVER: "
           "removing observe registration with token %x%x\n",
           obs->token[0], obs->token[1]);
    obs = NULL;
    break;
  }
}
/*----------------------------------------------------------------------------*/
/**
* @brief  periodic_get_callback
*         The callback for the periodic GETs, currently index_GET is used to
*         identify the correct slot: it would be possible and clean to use the CoAP
*         token, instead.
* @param  void *response:  Pointer to the CoAP Response
* @retval None
*/
static void
periodic_get_callback(void *response)
{
  const uint8_t *chunk;
  unsigned int format = 0;
  char value_get_str[RES_STRING_LEN] = "";
  int32_t value_get = 0;
  int len = coap_get_payload(response, &chunk);
  oma_tlv_t tlv;

  LWM2M_PRINTF("[PERIODIC GET HANDLE] For request %lu\n", index_GET);
  coap_get_header_content_format(response, &format);
  if(len > 0) {
	if(format == LWM2M_TLV){
		  if(oma_tlv_read(&tlv, chunk, len) > 0){
			  switch(active_get[index_GET].res_type) {
			  case RES_INT:
				  value_get = oma_tlv_get_int32(&tlv);
				  active_get[index_GET].value.valueInt = value_get;
				  break;
			  case RES_FLOAT:
				  oma_tlv_float32_to_fix(&tlv, &value_get, LWM2M_FLOAT32_BITS);
				  active_get[index_GET].value.valueFloat = value_get;
				  break;
			  default:
				  LWM2M_PRINTF("[PERIODIC GET HANDLE] Type Error ('%d')\n", active_get[index_GET].res_type);
			  }
			  LWM2M_PRINTF("[PERIODIC GET HANDLE] Value: '%ld'\n", (int32_t) value_get);
		  }
	  } else if(format == LWM2M_TEXT_PLAIN) {
		  if(active_get[index_GET].res_type == RES_STRING) {
			  memset(value_get_str, 0x00, sizeof(value_get_str));
			  strncpy(value_get_str, (char *) chunk, len);
			  strcpy((char*)active_get[index_GET].value.valueString, value_get_str);
			  LWM2M_PRINTF("Value_string '%*s'\n", len, value_get_str);
		  } else {
			  LWM2M_PRINTF("[PERIODIC GET HANDLE] Type Error ('%d')\n", active_get[index_GET].res_type);
		  }
	  }
  }
}
/*----------------------------------------------------------------------------*/
/**
* @brief  client_get_handle
*         The callback for the GETs that are issued as per a single command
*         received from cloud.
* @param  void *response:  Pointer to the CoAP Response
* @retval None
*/
static void
client_get_handle(void *response)
{
  const uint8_t *chunk;
  unsigned int format;
  char value_get_str[RES_STRING_LEN] = "";
  int32_t value_get = 0;
  int len = coap_get_payload(response, &chunk);
  oma_tlv_t tlv;

  if (cmd_id >= MAX_CMD_){
	  ERROR_OUT("Unexpected value for cmd_id (%d), must be [0,%d]\n", (int) cmd_id, MAX_CMD_-1);
  }
  coap_get_header_content_format(response, &format);

  LWM2M_PRINTF("[CLIENT GET HANDLE] --> (format = %d)\n", format);
  if (len > 0) {
	  if(format == LWM2M_TLV) {
		  if(oma_tlv_read(&tlv,chunk,len) > 0) {
			  switch(cmd_get[cmd_id].res_type) {
			  case RES_INT:
				  value_get = oma_tlv_get_int32(&tlv);
				  add_command_get_response(cmd_id, (int32_t*)&value_get);
				  LWM2M_PRINTF("[CLIENT GET HANDLE] Value as Integer: '%ld'\n",value_get);
				  break;
			  case RES_FLOAT:
				   oma_tlv_float32_to_fix(&tlv, &value_get, LWM2M_FLOAT32_BITS);
				   add_command_get_response(cmd_id, (floatfix32_t *)&value_get);
				   LWM2M_PRINTF("[CLIENT GET HANDLE] Value as Float: '%ld'\n",value_get);
				   break;
			  case RES_STRING:
				  memcpy(value_get_str, tlv.value, MIN(tlv.length,sizeof(value_get_str)));
				  value_get_str[tlv.length] = '\0';
				  add_command_get_response(cmd_id, (char *)value_get_str);
				  LWM2M_PRINTF("[CLIENT GET HANDLE] Value as String: '%*s'",len,(char*)value_get_str);
				  break;
			  default:
				  ERROR_OUT("Unexpected error in res_type %d\n", cmd_get[cmd_id].res_type);
			  }
		  } else {
			  LWM2M_PRINTF("[CLIENT GET HANDLE] Failed to read TLV numeric response\n");
		  }
	  } else if(format == LWM2M_TEXT_PLAIN) {
		  if(cmd_get[cmd_id].res_type == RES_STRING){
			  memset(value_get_str, 0x00, sizeof(value_get_str));
			  strncpy(value_get_str, (char *) chunk, len);
			  add_command_get_response(cmd_id, (char *)value_get_str);
			  LWM2M_PRINTF("[CLIENT GET HANDLE] Value as String: '%*s'",len,(char*)value_get_str);
		  } else {
			  LWM2M_PRINTF("[CLIENT GET HANDLE] Type (%d) not accepted in TEXT PLAIN format\n", (int) cmd_get[cmd_id].res_type);
		  }
	  } else {
		  printf("[CLIENT GET HANDLE] Error Format : '%d'\n", format);
	  }
  }
}
/*----------------------------------------------------------------------------*/
/**
* @brief  client_put_handle
*         The callback for the PUTs that are issued as per a single command
*         received from cloud.
* @param  void *response:  Pointer to the CoAP Response
* @retval None
*/
static void
client_put_handle(void *response)
{
	LWM2M_PRINTF("[WRITE COMMAND RESPONSE] Response to the PUT request -> ");
	switch(((coap_packet_t *)response)->code){
	case CHANGED_2_04:
		LWM2M_PRINTF("CHANGED_2_04\n");
		break;
	case NOT_ACCEPTABLE_4_06:
		LWM2M_PRINTF("NON_ACCEPTABLE_4_06: Format not acceptable\n");
		break;
	case METHOD_NOT_ALLOWED_4_05:
		LWM2M_PRINTF("METHOD_NOT_ALLOWED_4_05: No Callback or Resource found\n");

	}
}
/*----------------------------------------------------------------------------*/
/**
* @brief  client_post_handler
*         The callback for the POSTs that are issued as per a single command
*         received from cloud.
* @param  void *response:  Pointer to the CoAP Response
* @retval None
*/
static void
client_post_handler(void *response)
{
	LWM2M_PRINTF("[EXECUTE COMMAND RESPONSE] Response to the POST request -> ");
	switch(((coap_packet_t *)response)->code){
	case CHANGED_2_04:
		LWM2M_PRINTF("CHANGED_2_04\n");
		break;
	case METHOD_NOT_ALLOWED_4_05:
		LWM2M_PRINTF("METHOD_NOT_ALLOWED_4_05: No Callback or Resource found\n");
	}
}
/*---------------------------------------------------------------------------*/
/**
* @brief  setup_network
*         Sets the Wireless Sensor Network (using the SPIRIT1 radio) up, by
*         assigning to the LWM2M to IBM Gateway the predefined address and
*         calling the RPL initialization function, in order to set this node
*         as Root of the RPL tree.
* @param  None
* @retval None
*/
static void
setup_network(void)
{
  uip_ipaddr_t ipaddr;
  struct uip_ds6_addr *root_if;
  rpl_dag_t *dag;
  int i;
  uint8_t state;

#if UIP_CONF_ROUTER
/**
 * The choice of server address determines its 6LoWPAN header compression.
 * Obviously the choice made here must also be selected in udp-client.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the 6LowPAN protocol preferences,
 * e.g. set Context 0 to fd00::.  At present Wireshark copies Context/128 and then overwrites it.
 * (Setting Context 0 to fd00::1111:2222:3333:4444 will report a 16 bit compressed address of fd00::1111:22ff:fe33:xxxx)
 * Note Wireshark's IPCMV6 checksum verification depends on the correct uncompressed addresses.
 */
#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
//  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
//Serevr address is moved in a string defined at the beginning of the file, like in the code that is currently used for the nodes.
   if(! uiplib_ip6addrconv(LWM2M_SERVER_ADDRESS_v6, &ipaddr)) {
	    printf("ERROR: the provided IPv6 address '%s' for LWM2M Server is not valid.\r\n", LWM2M_SERVER_ADDRESS_v6);
  }
#else
/* Mode 3 - derived from link local (MAC) address */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
#endif

  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  root_if = uip_ds6_addr_lookup(&ipaddr);
  if(root_if != NULL) {
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
    uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &ipaddr, 64);
    LWM2M_PRINTF("created a new RPL dag\n");
  } else {
    LWM2M_PRINTF("failed to create a new RPL DAG\n");
  }
#endif /* UIP_CONF_ROUTER */

  printf("\r\nIPv6 addresses: \r\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(state == ADDR_TENTATIVE || state == ADDR_PREFERRED) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
      printf("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
    	  uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
/**
* @brief  oma_lwm2m_light_server
*         Contiki PROCESS_THREAD that implements the OMA LWM2M server that
*         manages GETs and Observations
* @param process_event_t ev: As per Contiki Thread API
* @param process_data_t data: As per Contiki Thread API
* @retval None
*/
PROCESS_THREAD(oma_lwm2m_light_server, ev, data)
{
  static uint32_t i = 0;
  static uint32_t j = 0;
  static coap_packet_t request[1];
  static struct etimer timer;

  PROCESS_BEGIN();
  PROCESS_PAUSE();

  /* receives all CoAP messages */
  coap_init_engine();

  setup_network();

  /* The data sink runs with a 100% duty cycle in order to ensure high
     packet reception rates. */
  NETSTACK_MAC.off(1);

  /* Initializes and starts the REST engine process*/
  rest_init_engine();

  /*Activates the resource*/
  rest_activate_resource(&res_rd, LWM2M_RD_RESOURCE);

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * ROUTER_PROCESS_TIMER);
    PROCESS_YIELD();
	LWM2M_PRINTF("[ROUTER_PROCESS] Executing OMA LWM2M Server process...\n");

      /*Loop over all the object_resource elements contained in active_observation list
       * - If the object_resource has to be observed, and is valid, an observe request is sent
       * - If the object_resource has been observed (so an observation is running) but is
       *     not valid any more (because a cancellation has been requested from cloud),
       *     an observe cancellation is issued
       * */
      if(etimer_expired(&timer)){
    	  for(i = 0; i < numObs; i++){
    		  /*Here we send observation requests or we cancel them. */
			  if(active_observations[i].flags & OPR_TO_OBSERVE){
				  if (active_observations[i].valid == 1){
					  LWM2M_PRINTF("Sending observe request (%lu) for uri '%s' on node %lu\n",
							  i, active_observations[i].uri, active_observations[i].node_id);
					  obs = coap_obs_request_registration(&(active_observations[i].ipaddr),
							  REMOTE_PORT, active_observations[i].uri, notification_callback, NULL);
					  //active_observations[i].flags = OPR_OBSERVED;//moved in the callback to be on the safe side...
				  }
			  } else if (active_observations[i].flags & OPR_OBSERVED){
				  if (active_observations[i].valid == 0){
					  LWM2M_PRINTF("Removing observation (%lu) for uri '%s' on node %lu\n",
							  i, active_observations[i].uri, active_observations[i].node_id);
					  coap_obs_remove_observee_by_url(&(active_observations[i].ipaddr),
				  			  REMOTE_PORT, active_observations[i].uri);
					  active_observations[i].flags = OPR_NONE;
				  }
			  }
		  }
    	  /*Loop over all the elements of active_get array and issue a CoAP GET for the valid ones.  */
		  for(j = 0; j < numGet; j++){
			  if (active_get[j].valid == 1) {
				  LWM2M_PRINTF("[GET REQUEST %lu] Preparing the GET request at '%s'... \n", j, active_get[j].uri);
				  coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
				  coap_set_header_uri_path(request, active_get[j].uri);
				  /*{ //In case we want to use the token to match the response, instead of index_GET...
					  uint8_t * token = NULL;
					  int token_len = coap_generate_token(&token);
					  coap_set_token(request, local_token, 8);
				  }*/
				  index_GET = j;
				  COAP_BLOCKING_REQUEST(&(active_get[j].ipaddr),
						  REMOTE_PORT, request, periodic_get_callback);
			  }
		  }
      }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
* @brief  command_process
*         Contiki PROCESS_THREAD that implements the parser for the JSON
*         buffer containing a command from the cloud.
* @param process_event_t ev: As per Contiki Thread API
* @param process_data_t data: As per Contiki Thread API
* @retval None
*/
PROCESS_THREAD(command_process, ev, data)
{
	PROCESS_BEGIN();
	static coap_packet_t request[1];
    int type = -1;
    char str[JSON_VALUE_STRING_LEN]= "";
    char path[JSON_VALUE_STRING_LEN] = "";
    char cmd_value[JSON_VALUE_NUM_LEN] = "";
    state_value_t current_Value = Nd;
    int node_id = -1;
    cloud_command_t cloud_command;
    int value = 0;

    LWM2M_PRINTF("[COMMAND PROCESS]\n");

    while(1){
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
		printf("\r\nProcessing IBM command: '%s'\r\n", cloud_command_str);
		jsonparse_setup(&state, cloud_command_str, strlen(cloud_command_str));
		/* Message Template: {"Node":node_id,
		 * 					  "Path":"resource_path", (i.e. "object_id/isntance_id/resource_id")
		 * 					  "Operation":"Read|Write|Execute|Observe|Cancel|PRead|CRead",
		 * 					  "Type":"Int|Float|String",
		 * 					  "Value":value}
		 */
		while((type = jsonparse_next(&state)) != 0){
			  if(type == JSON_TYPE_PAIR_NAME) {
				  if(jsonparse_strcmp_value(&state,"Node")==0){
					  current_Value = Node;
					  jsonparse_next(&state); // Parses the :
  					  node_id = jsonparse_get_value_as_int(&state);
					  LWM2M_PRINTF("Node: %d\n", node_id);
				  } else if(jsonparse_strcmp_value(&state,"Path")==0){
					  current_Value = Path;
					  jsonparse_next(&state); //Parses the :
					  memset(path, 0x00, sizeof(path));
					  jsonparse_copy_value(&state,path,sizeof(path));
					  LWM2M_PRINTF("Path: '%*s'\n", strlen(path),path);
				  } else if(jsonparse_strcmp_value(&state,"Operation")==0){
					  current_Value = Operation;
					  jsonparse_next(&state); //Parses the :
					  memset(str, 0x00, sizeof(str));
					  jsonparse_copy_value(&state, str, sizeof(str));
					  if(!strcmp(str, "Read")){
						  cloud_command = Read;
						  LWM2M_PRINTF("Read Operation\n");
					  } else if(!strcmp(str, "Write")){
						  cloud_command = Write;
						  LWM2M_PRINTF("Write Operation\n");
					  } else if(!strcmp(str,"Execute")){
						  cloud_command = Execute;
						  LWM2M_PRINTF("Execute Operation\n");
					  } else if(!strcmp(str,"Cancel")){
						  cloud_command = Cancel;
						  LWM2M_PRINTF("Cancel Observation Operation\n");
					  }  else if(!strcmp(str,"Observe")){
						  cloud_command = Observe;
						  LWM2M_PRINTF("Observe Operation\n");
					  }  else if(!strcmp(str,"PRead")){
						  cloud_command = PeriodicRead;
						  LWM2M_PRINTF("Periodic Read Operation\n");
					  }  else if(!strcmp(str,"CRead")){
						  cloud_command = CancelPeriodicRead;
						  LWM2M_PRINTF("Cancel Periodic Read Operation\n");
					  }
				  } else if(jsonparse_strcmp_value(&state,"Type")==0){
					  current_Value = Type;
					  jsonparse_next(&state); //Parses the :
					  memset(str, 0x00, sizeof(str));
					  jsonparse_copy_value(&state, str, sizeof(str));
					  if(!strcmp(str, "Int")){
						  type_value_ = RES_INT;
						  LWM2M_PRINTF("Int Type\n");
					  } else if(!strcmp(str, "Float")){
						  type_value_ = RES_FLOAT;
						  LWM2M_PRINTF("Float Type\n");
					  } else if(!strcmp(str,"String")){
						  type_value_ = RES_STRING;
						  LWM2M_PRINTF("String type\n");
					  }
				  } else if(jsonparse_strcmp_value(&state,"Value")==0){
					  current_Value = Value;
				  }
			  } else if(type == JSON_TYPE_NUMBER){
				  if(current_Value == Node){
					  node_id = jsonparse_get_value_as_int(&state);
					  LWM2M_PRINTF("Node: %d\n", node_id);
				  } else if(current_Value == Value){
					  value = jsonparse_get_value_as_int(&state);
					  LWM2M_PRINTF("Value: %d\n",value);
				  } else{
					  LWM2M_PRINTF("[COMMAND_PROCESS] Error in JSON\n");
				  }
			  }
		  }
		/*We have parsed the JSON with the command from IBM Cloud, now we can translate this to the corresponding
		 * OMA LWM2M operation, i.e. CoAP Request. */
		if((node_id >= 0) && (node_id < numNode)){
			if(cloud_command == Read) {
				LWM2M_PRINTF("GET request to '%s'...\n",path);
				cmd_id = add_command_get(node_id, path, RES_FLOAT);
				coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
				coap_set_header_content_format(request, LWM2M_TLV);
				coap_set_header_uri_path(request, path);
				COAP_BLOCKING_REQUEST(&nodes_array[node_id].ipaddr, REMOTE_PORT, request, client_get_handle);
			} else if(cloud_command == Write) {
				LWM2M_PRINTF("PUT request to '%s'...\n", path);
				coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
				coap_set_header_uri_path(request, path);
				memset(cmd_value, 0x00, sizeof(cmd_value));
				sprintf(cmd_value, "%d", value);
				coap_set_header_content_format(request, LWM2M_TEXT_PLAIN);
				coap_set_payload(request,cmd_value,sizeof(cmd_value));
				COAP_BLOCKING_REQUEST(&nodes_array[node_id].ipaddr, REMOTE_PORT, request, client_put_handle);
			} else if(cloud_command == Execute) {
				LWM2M_PRINTF("POST request on '%s'...\n",path);
				coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
				coap_set_header_uri_path(request, path);
				memset(cmd_value, 0x00, sizeof(cmd_value));
				sprintf(cmd_value, "%d", value);
				coap_set_payload(request,cmd_value, sizeof(cmd_value));
				COAP_BLOCKING_REQUEST(&nodes_array[node_id].ipaddr, REMOTE_PORT, request, client_post_handler);
			} else if(cloud_command == Cancel) {
				LWM2M_PRINTF("Cancel Observation request to '%s'\n",path);
				if (!remove_observation(node_id, path)){
					printf("ERROR: unable to cancel '%s' path from the Observations list.\n", path);
				}
			} else if(cloud_command == Observe) {
				LWM2M_PRINTF("Observe request to '%s'...\n", path);
				if (!add_observation(node_id, path, type_value_, path)){
					printf("ERROR: unable to add '%s' path in the Observations list.\n", path);
				}
			} else if(cloud_command == PeriodicRead) {
				LWM2M_PRINTF("New periodic Read request for '%s'...\n", path);
				if (!add_periodic_read(node_id, path, type_value_, path)){
					printf("ERROR: unable to add '%s' path in the periodic READ list.\n", path);
				}
			} else if(cloud_command == CancelPeriodicRead) {
				LWM2M_PRINTF("Cancel Read request for '%s'...\n", path);
				if (!remove_periodic_read(node_id, path)){
					printf("ERROR: unable to cancel '%s' path from the periodic READ list.\n", path);
				}
			}
		} else {
			LWM2M_PRINTF("[COMMAND_PROCESS] Error: selected node ('%d') does not exist!!\n", node_id);
		}
    }
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
