/**
******************************************************************************
  * @file    lwm2m-resource-directory.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M Resource Directory
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
/*---------------------------------------------------------------------------*/
/* This file implements the LWM2M Resource Directory and so the interfaces
 * for the nodes data structure
 *  nodes_array[] stores the nodes list
 * Here also the TEMPLATE() is instantiated to pre-program the LWM2M to IBM
 * gateway with a list of object_id/resource_id to retrieve by maens of
 * Observations or Periodic GETs, if discovered on the nodes.
 * */
/*---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "lib/random.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "rest-engine.h"
#include "er-coap.h"
#include "lwm2m-simple-server.h"
#include "lwm2m-data.h"
#include "lwm2m-resource-directory.h"
/*---------------------------------------------------------------------------*/
/* USE_RANDOM_LOCATION=1 sets a pseudo random string as location for each node*/
/* USE_RANDOM_LOCATION=0 takes the last KEY_LEN characters of the client Endpoint Name as location */
#define USE_RANDOM_LOCATION 1
/*---------------------------------------------------------------------------*/
/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup LWM2M_to_IBM
  * @{
  */

/** @defgroup LWM2M_to_IBM_resource_directory
  * @{
  */


#define DEBUG  DEBUG_NONE
//#define DEBUG  DEBUG_PRINT
#if DEBUG
#include <stdio.h>
#define LWM2M_PRINTF(...) printf(__VA_ARGS__)
#else
#define LWM2M_PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
nodeInfo_t nodes_array[MAX_NODES_];	// Nodes Array structure to store all the information for each registered node
uint32_t numNode = 0;     			    // Number of registered nodes
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
/*---------------------------------------------------------------------------*/
/**** Functions ****/
static int parse_payload(const uint8_t *payload, int node_id);
static void resources_assignment(int object_id, int ist, int node_id);
static int is_payload_to_be_parsed(int node_id, uint8_t block_register, uint32_t num_block);
static int is_registered_node(uip_ipaddr_t current_ipaddr);
static int is_new_node(uip_ipaddr_t current_ipaddr);
#if NODES_WITH_OBJECTS
static int add_object_instance(int node_id, uint32_t object_id, uint32_t instance_id);
#endif
#if USE_RANDOM_LOCATION
static void random_key(char*key, int key_len);
#else
static void key_from_endpoint(const char *endpoint_name,char *key, int size);
#endif /*USE_RANDOM_LOCATION*/
static int retrieve_key(uip_ipaddr_t ipaddr, char * key);
static int is_unique_key(char*key, int key_len);
/*---------------------------------------------------------------------------*/
/* Declaration of rd resource, resource directory for nodes registration (CoAP POST) */
static void post_handler(void *request,void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *post_offset);
PARENT_RESOURCE(res_rd, NULL, NULL, post_handler, NULL, NULL);
/*---------------------------------------------------------------------------*/
const char lwm2m_rd_path[LOCATION_PATH_LEN] = LWM2M_RD_PATH;	// Root path
/*---------------------------------------------------------------------------*/
/**********Template used to fill the active_observation array and the active_get array *********/
/* Syntax:
 * TEMPLATE(template_name,
 * 			IPSO_RESOURCE_TYPE(OBJECT_ID, RESOURCE_ID, OPERATION_TYPE, ALTERNATIVE_NAME),
 * 			);
 *
 * 			Note: The IPSO types as well as all the needed #define are in lwm2m-simple-server.h and lwm2m-defs.h
 */
TEMPLATE(template,
			IPSO_RESOURCE_FLOAT (IPSO_ACCELEROMETER, Z_VALUE, OPR_TO_OBSERVE, ACCELERATION_Z_STR),
			IPSO_RESOURCE_INT   (IPSO_BUTTON, BUTTON_COUNTER, OPR_TO_GET, BUTTON_COUNTER_STR),
			IPSO_RESOURCE_FLOAT (IPSO_TEMPERATURE_SENSOR, SENSOR_VALUE, OPR_TO_OBSERVE, TEMPERATURE_VALUE_STR),
			IPSO_RESOURCE_FLOAT (IPSO_TEMPERATURE_SENSOR, MAX_MEASURED_VALUE, OPR_TO_GET, TEMPERATURE_MAX_VALUE_STR),
			IPSO_RESOURCE_FLOAT (IPSO_HUMIDITY_SENSOR, SENSOR_VALUE, OPR_TO_OBSERVE, HUMIDITY_VALUE_STR),
			IPSO_RESOURCE_INT   (IPSO_PRESENCE_SENSOR, DIGITAL_INPUT_STATE, OPR_TO_OBSERVE, PRESENCE_STATE_STR),
		);
static int length_of_template = 0;
/*---------------------------------------------------------------------------*/
/**
* @brief  post_handler
*         The post_handler function is called whenever a node registers to the OMA_LWM2M server (using the POST Method)
*         It manages all the registration phase of a node, that may be split in more than one packet, and also the
*         periodic updates.
* @param  void *request: Pointer to the CoAP Request
* @param  void *response: Pointer to the CoAP Response
* @param  uint8_t *buffer: CoAP Response Buffer
* @param  uint16_t preferred_size: CoAP Response Buffer size
* @param  int32_t *post_offset: offset of the current request
* @retval None
*/
static void post_handler(void *request,void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *post_offset)
{
	const char *uri_path_p = NULL;
	char uri_path[LOCATION_PATH_LEN] = "";
	const uint8_t *payload = NULL;
	char key[KEY_MAX_LEN] = "";
	const char *endpoint_name = NULL;
	char path_loc[LOCATION_PATH_LEN] = "";
	int len_payload = 0;
	int len_query = 0;
	uint32_t num_block = 0;
	uint8_t more_blocks = 0;
	uint8_t first_block = 0, last_block = 0;
	uint16_t size =0;
	uint32_t offset=0;
	uint8_t block_register = 0;
	int len = REST.get_url(request, &uri_path_p);
	uint32_t node_id = 0;

	memset(path_loc, 0x00, sizeof(path_loc)); // Clears the path_loc array

	//We need this copy, otherwise strcmp() is not working properly on the char *
	memcpy(uri_path, uri_path_p, len);

	/* Registration path to the server*/
	LWM2M_PRINTF("\r\n[POST_HANDLER] Uri_path request: '%.*s'\r\n", len, (char*) uri_path);

	/*Here, this control avoids multiple registrations under the same path*/
	if(!strcmp(res_rd.url, uri_path)){
		/* POST on "/rd/" this is a new registration, but it might happen that the node missed a previous reply  */
		if (is_new_node(UIP_IP_BUF->srcipaddr)){
			if (!add_node(&node_id, UIP_IP_BUF->srcipaddr)){
				REST.set_response_status(response, REST.status.BAD_REQUEST);
				return;
			}
		} else {
			if (!retrieve_node_id_by_address(UIP_IP_BUF->srcipaddr, &node_id)) {
				printf("Unexpected server error. Rejecting LWM2M Register\n");
				REST.set_response_status(response, REST.status.BAD_REQUEST);
				return;
			}
			LWM2M_PRINTF("Processing a registration of a node already in the table (%lu)\n", node_id);
		}
		if (coap_get_header_block1(request, &num_block, &more_blocks, &size, &offset)) {
			LWM2M_PRINTF("Register payload is split in more packets: curr packet is %lu , to the end: %d\n", num_block, (int) more_blocks);
			/*We are possibly dealing with a registration split over more blocks*/
			block_register = 1;
			if (num_block == 0)     first_block = 1;
			if (more_blocks == 0)   last_block = 1;
			SET_OPTION((coap_packet_t*) response, COAP_OPTION_BLOCK1);
		} else {
			LWM2M_PRINTF("Register payload is all in this packet\n");
			first_block = 1;
			last_block = 1;
		}

		if (first_block){ //PARSING OF QUERIES
			LWM2M_PRINTF("Parsing queries (because this is the first packet).\n");
			nodes_array[node_id].node_flags = NODE_FLAG_REGISTERING ;
#if NODES_WITH_OBJECTS
			nodes_array[node_id].numObj = 0;
#endif
#if NODES_WITH_REGISTATION_PAYLOAD
			strcpy(nodes_array[node_id].registration_payload, "");
#endif
			nodes_array[node_id].reg_block_flags = 0x00;

			invalidate_observations(node_id);
			invalidate_periodic_gets(node_id);

			/* Query --> Endpoint name */
			len_query = REST.get_query_variable(request, LWM2M_ENDPOINT_QUERY_NAME, &endpoint_name);
			printf("Endpoint name: '%.*s'\n", len_query, (char*) endpoint_name);
			strncpy(nodes_array[node_id].endpoint_name, endpoint_name, len_query);
#if DEBUG
			/*We Parse these registration parameters but currently we ignore them.*/
			int tmp_len = 0;
			const char * value_str = NULL;
			if ((tmp_len = REST.get_query_variable(request, LWM2M_LIFETIME_QUERY_NAME, &value_str))){
				LWM2M_PRINTF("Lifetime: '%.*s'\n", tmp_len, (char*) value_str);
			}
			if ((tmp_len = REST.get_query_variable(request, LWM2M_SMS_NUMBER_QUERY_NAME, &value_str))){
				LWM2M_PRINTF("SMS Number: '%.*s'\n", tmp_len, (char*) value_str);
			}
			if ((tmp_len = REST.get_query_variable(request, LWM2M_VERSION_QUERY_NAME, &value_str))){
				LWM2M_PRINTF("LWM2M Version: '%.*s'\n", tmp_len, (char*) value_str);
			}
			if ((tmp_len = REST.get_query_variable(request, LWM2M_BINDING_QUERY_NAME, &value_str))){
				LWM2M_PRINTF("Binding Mode: '%.*s'\n", tmp_len, (char*) value_str);
			}
#endif
		}

		if(is_payload_to_be_parsed(node_id, block_register, num_block)){
			if (block_register){
				nodes_array[node_id].reg_block_flags |= (1 << num_block) ;
			}
			/* Payload --> list of objects available in the node */
			len_payload = REST.get_request_payload(request, &payload);
			printf("Payload of request packet: '%.*s'\n", len_payload, payload);

			/* PARSING OF THE PAYLOAD */
			(void) parse_payload(payload, node_id);

			if (block_register && more_blocks){
				REST.set_response_status(response, CONTINUE_2_31);
                                LWM2M_PRINTF("Response status set to CONTINUE_2_31 (%d)\n", CONTINUE_2_31);
			} else {
				REST.set_response_status(response, REST.status.CREATED);
                                LWM2M_PRINTF("Response status set to CREATED1 (%d)\n", REST.status.CREATED);
			}
		} else { /* This packet has already been processed but the client missed the response */
			/*In that case, the Server does not create a new node, but sends the same response back again. */
			if (is_registered_node(UIP_IP_BUF->srcipaddr)){
				if (!retrieve_key(UIP_IP_BUF->srcipaddr, key)){
					/*Unexpected error, should not happen */
					LWM2M_PRINTF("Unexpected error while managing a re-registration.\n");
					REST.set_response_status(response, REST.status.BAD_REQUEST);
					return ;
				}
				LWM2M_PRINTF("Retrieved Location: '%.*s'\n", strlen((char*) key), (char*) key);
				snprintf(path_loc, sizeof(path_loc),"%s%s", lwm2m_rd_path, key);
				REST.set_header_location(response, path_loc);
				*buffer = 'b'; //check
				REST.set_response_payload(response,buffer, strlen((char*)buffer));
				REST.set_response_status(response, REST.status.CREATED);
                                LWM2M_PRINTF("Response status set to CREATED1 (%d)\n", REST.status.CREATED);
				return ;
			} else {
				/* It is some intermediate block */
				LWM2M_PRINTF("Skipping duplicate intermediate packet.\n");
				// We use CoAP code since no REST.status generic code for CONTINUE is defined
				REST.set_response_status(response, CONTINUE_2_31);
                                LWM2M_PRINTF("Response status set to CONTINUE_2_31 (%d)\n", CONTINUE_2_31);
				return ;
			}
		}

		if (last_block){ //We have to assign a LOCATION send the final response
			LWM2M_PRINTF("Generating Location (last packet).\n");
			 // Set the location path to be sent to the client
#if USE_RANDOM_LOCATION
			do {
				random_key(key, KEY_LEN);
			} while(!is_unique_key(key, KEY_LEN));
#else
			key_from_endpoint(endpoint_name, key, len_query);
			while (!is_unique_key(key, KEY_LEN)){
				key[0]++;
			}
#endif /*USE_RANDOM_LOCATION*/

			LWM2M_PRINTF("Location: '%.*s'\n", strlen((char*) key), (char*) key);
			snprintf(path_loc, sizeof(path_loc),"%s%s", lwm2m_rd_path, key);
			printf("Node registered. Full update path: '%.*s'\n", strlen((char*) path_loc), (char *) path_loc);
			strncpy(nodes_array[node_id].location, key, strlen((char*) key));
			REST.set_header_location(response, path_loc);
			*buffer = 'a'; //check
			REST.set_response_payload(response,buffer, strlen((char*)buffer));
			nodes_array[node_id].node_flags = NODE_FLAG_REGISTERED ;
			REST.set_response_status(response, REST.status.CREATED);
		}
	} else { /* UPDATE --> TODO handle new or modified parameters */
		LWM2M_PRINTF("\r[POST HANDLER] Update received...\n");
		LWM2M_PRINTF("Uri path: '%.*s'\n",len, (char*) uri_path);
		if (retrieve_key(UIP_IP_BUF->srcipaddr, key)) {
				snprintf(path_loc, sizeof(path_loc),"%s%s", lwm2m_rd_path, key);
				if (!strcmp(path_loc, uri_path)){
					/*Everything is OK */
					REST.set_response_status(response, REST.status.CHANGED);
					return;
				} else {
					/*Node is registered with a location different from the provded one */
					REST.set_response_status(response, REST.status.NOT_FOUND);
					return;
				}
			}
		/* Node is not registered */
		REST.set_response_status(response, REST.status.BAD_REQUEST);
	}
}
/*---------------------------------------------------------------------------*/
/**
* @brief  add_node
*         Add one node in the node list.
* @param  uint32_t *node_id:  To be filled with the assigned node_id
* @param  uip_ipaddr_t ipaddr:  IP Address of the node
* @retval int: 1 success, 0 failure
*/
int add_node (uint32_t *node_id, uip_ipaddr_t ipaddr)
{
	if(numNode == MAX_NODES_){
		LWM2M_PRINTF("Rejecting LWM2M Register because already registered MAX_NODES_ (%lu) nodes\n",
				(uint32_t) MAX_NODES_);
		return 0;
	}
	*node_id = numNode;
	numNode++;
	uip_ipaddr_copy(&nodes_array[*node_id].ipaddr, &UIP_IP_BUF->srcipaddr);
	nodes_array[*node_id].valid = 1; //currently unused since we do not handle de-register
	nodes_array[*node_id].short_id = (uint16_t) ((ipaddr.u8[14] << 8) + ipaddr.u8[15]);
	printf("Registration of a new node (id = %lu), IPv6 address:\n", (uint32_t) *node_id);
	uip_debug_ipaddr_print(&ipaddr);
	printf("\n");
	LWM2M_PRINTF("Short ID: %04x\n", nodes_array[*node_id].short_id);

	return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  remove_node_by_id
*         Remove from the node list the node with a given ID.
* @param  uint32_t node_id:  The ID of the node to be removed
* @retval int: 1 success, 0 failure
*/
int remove_node_by_id (uint32_t node_id) //currently we do not hande deregister
{
	if (node_id < MAX_NODES_) {
		if (nodes_array[node_id].valid){
			nodes_array[node_id].valid = 0;
			memset(&nodes_array[node_id], 0x00, sizeof(nodeInfo_t));
			return 1;
		}
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  remove_node_by_address
*         Remove from the node list the node with a given IP Address.
* @param  uip_ipaddr_t ipaddr:  IP Address of the node to be removed
* @retval int: 1 success, 0 failure
*/
int remove_node_by_address (uip_ipaddr_t ipaddr)  //currently we do not hande deregister
{
	uint32_t node_id = 0;
	if (retrieve_node_id_by_address(ipaddr, &node_id)){
		return remove_node_by_id (node_id) ;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  retrieve_node_id_by_address
*         Retrieve the ID in the node list of a node given its IP Address.
* @param  uip_ipaddr_t ipaddr:  IP Address of the node to be removed
* @param  uint32_t *node_id:  To be filled with the assigned node_id
* @retval int: 1 success, 0 failure
*/
int retrieve_node_id_by_address(uip_ipaddr_t ipaddr, uint32_t *node_id)
{
  uint32_t i = 0;
  while(i < numNode){
 		if(uip_ip6addr_cmp(&ipaddr, &nodes_array[i].ipaddr)){
 			*node_id = i;
 			return 1;
  		}
  		i++;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  retrieve_node_id_by_address
*         Initialize to 0x00 the node data structure and counter.
* @param  None
* @retval None
*/
void init_lwm2m_nodes_structure(void)
{
	memset(nodes_array, 0x00, sizeof(nodes_array));
	numNode = 0;
}
/*---------------------------------------------------------------------------*/
#if USE_RANDOM_LOCATION
/**
* @brief  random_key
*         Generate a string of characters [0-9 A-Z a-z] to be used as location
*         for the node update.
* @param  char *key: to be filled with the generated location
* @param  int key_len: Length of the location string
* @retval None
*/
static void random_key(char*key, int key_len)
{
   int i, c;
   int num_valid_chars = (int)('9'- '0' + 1 + 'Z' - 'A' + 1 + 'z' - 'a' + 1);

   if ((key_len < 0) || (key_len > KEY_MAX_LEN)){
	   memset(key, 0xAB, KEY_LEN);
	   return;
   }

   for( i = 0 ; i < key_len ; i++ ) {
	  c = (random_rand() %  num_valid_chars) + (int)'0';
	  if (c > (int)'9') c+= (int)('A' - '9' - 1);
	  if (c > (int)'Z') c+= (int)('a' - 'Z' - 1);

	  key[i] = (char) c;
   }
}
#else
/*---------------------------------------------------------------------------*/
/**
* @brief  key_from_endpoint
*         Takes the last KEY_LEN characters of the endpoint name and uses them
*         as the registration key
* @param  const char *endpoint_name: Client Endpoint of the current node
* @param  char *key: to be filled with the generated location
* @param  int size: Length of the Endpoint string
* @retval None
*/
static void  key_from_endpoint(const char *endpoint_name,char *key, int size)
{
	if(size >= KEY_LEN) {
		strncpy(key, endpoint_name + size - KEY_LEN, KEY_LEN);
		key[KEY_LEN] = '\0';
		LWM2M_PRINTF("set_key() : %.*s\n",strlen((char*)key),key);
	} else {
		LWM2M_PRINTF("\r[SET_KEY] Not able to get the key.\n");
		strcpy(key, "D_key");
	}
}
#endif
/*---------------------------------------------------------------------------*/
/**
* @brief  parse_payload
*         Link format parsing function: the 6LoWPAN node sends a payload in the form
*         <object_0/instance_0>,<object_0/instance_1>,<object_1/instance_0>,...
*         Every pair object_id/instance_id is parsed and then the resources_assignment()
*         function is called, in order to compare the TEMPLATE with the discovered objects:
*         when a match is found, the specified resources will be set for retrieval, with
*         Observe or Periodic GETs
* @param  const uint8_t * payload: The payload (in link format) to be parsed
* @param  uint32_t node_id: The ID of the current node
* @retval int: number of successfully parsed objects
*/
static int parse_payload(const uint8_t * payload, int node_id)
{
	char *c = NULL;
	char *p = (char *) payload;
	uint8_t done = 0;
	int object_id, instance_id, num_obj = 0;

#if NODES_WITH_REGISTATION_PAYLOAD
	if ( payload && node_id >=0 && node_id < numNode) {
		strncat(nodes_array[node_id].registration_payload, payload,
				REGISTRATION_PALYOALD_LEN - strlen(nodes_array[node_id].registration_payload)- 1 );
	}
#endif

	while (!done) {
		p = strchr(p, '<');
		if (p!= NULL) {
			c = strchr(p, '>');
		}
		if (p == NULL || c == NULL){
			done = 1;
		} else {
			if (sscanf(p, "<%d/%d>", &object_id, &instance_id) != 2){
				LWM2M_PRINTF("Parsing error (string '%s')\n", p);
				return 0;
			} else {
				//LWM2M_PRINTF("Found OBJ %d, INST %d\n", object_id, instance_id);
				num_obj++;
				p = c;
				resources_assignment(object_id, instance_id, node_id);
			}
		}
	}
	return num_obj;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  resources_assignment
*         The resource_assignement function is responsible of filling the
*         active_observations and active_get arrays. According to the TEMPLATE,
*         the object_id is compared to what is specified in the template.
*         If matched, the object with the corresponding ID, the discovered
*         instance and the specified resource, composing a URI
*         (OBJECT/INSTANCE/RESOURCE) are added in the relevant data array.
*         Optionally, objects/instances can be saved in the node_array as well.
* @param  int object_id: Object ID as per LWM2M/IPSO specification
* @param  int instance_id: Instance ID of the current object
* @param  uint32_t node_id: The ID of the current node
* @retval None
*/
static void resources_assignment(int object_id, int instance_id, int node_id)
{
	/* Scanning of template structure, only the objects declared in the TEMPLATE are added,
	 *  but any Resource (for a registered node) can be reached using its URI.
	 * */
	int i = 0;
	int found = 0;
	char uri[URI_LEN] = "";

	length_of_template = sizeof(template)/sizeof(template_t);
#if NODES_WITH_OBJECTS
	if (!add_object_instance(node_id, object_id, instance_id)) {
		return;
	}
#endif

	for(i = 0; i < length_of_template; i++){
		if(template[i].object_id == object_id){ //The assumption is that if the Object exists, also the resource should be supported.
			found = 1;
			sprintf(uri, "%d/%d/%lu", object_id, instance_id, template[i].resource_id);
			switch(template[i].operation_info){
			case OPR_TO_OBSERVE:
				printf("Adding observation node %d uri %s\n", node_id, uri);
				if (!add_observation(node_id, uri, template[i].res_type, template[i].readable_name)){
					LWM2M_PRINTF("Failed to add Observation\n");
				}
				break;
			case OPR_TO_GET:
				printf("Adding periodic get node %d uri %s\n", node_id, uri);
				if (!add_periodic_read(node_id, uri, template[i].res_type, template[i].readable_name)){
					LWM2M_PRINTF("Failed to add Periodic GET\n");
				}
				break;
			default:
				ERROR_OUT("Unexpected operation type %d\n", template[i].operation_info);
				return;
			}
		}
	}
	if (!found){
		LWM2M_PRINTF("[RESOURCES_ASSIGNMENT] No match found in template for ID %d. \n", object_id);
	}
}
/*---------------------------------------------------------------------------*/
#if NODES_WITH_OBJECTS
/**
* @brief  add_object_instance
*         Adds the OBJECT_ID/INSTANCE_ID pair in the relevant data structure for
*         the current node.
* @param  int node_id: The ID of the current node
* @param  uint32_t object_id: Object ID as per LWM2M/IPSO specification
* @param  uint32_t instance_id: Instance ID of the current object
* @retval int: 1 success, 0 failure
*/
static int add_object_instance(int node_id, uint32_t object_id, uint32_t instance_id)
{
	int index_obj =  0;

	if (nodes_array[node_id].numObj == MAX_OBJ_INST){
		printf("Reached maximum number of final resources (%d) for node %d, skipping the command.\n", MAX_OBJ_INST, node_id);
		return 0;
	}
	index_obj =	nodes_array[node_id].numObj++;
	nodes_array[node_id].object[index_obj].object_id = object_id;
	nodes_array[node_id].object[index_obj].instance_id = instance_id;

	LWM2M_PRINTF("Node %d has now %d 'end resources'\n", node_id, nodes_array[node_id].numObj);

	return 1;
}
#endif
/*---------------------------------------------------------------------------*/
/**
* @brief  is_new_node
*         Checks if the given IP Address corresponds to a new node or not.
* @param  uip_ipaddr_t ipaddr: The IP Address of the current node
* @retval int: 1 it is a new node, 0 it already started the registration phase
*/
static int is_new_node(uip_ipaddr_t ipaddr)
{
  uint32_t i = 0;
  while(i < numNode){
		if(uip_ip6addr_cmp(&ipaddr, &nodes_array[i].ipaddr)){
  			return 0;
  		}
  		i++;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  is_registered_node
*         Checks if the given IP Address corresponds to a node that has completed
*         the Register phase.
* @param  uip_ipaddr_t ipaddr: The IP Address of the current node
* @retval int: 1 it is registered, 0 it not registered
*/
static int is_registered_node(uip_ipaddr_t ipaddr)
{
  uint32_t i = 0;
  while(i < numNode){
		if(uip_ip6addr_cmp(&ipaddr, &nodes_array[i].ipaddr)){
			if (nodes_array[i].node_flags & NODE_FLAG_REGISTERED)
				return 1;
			else
				return 0; //in list but registration not yet completed
  		}
  		i++;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  is_payload_to_be_parsed
*         Checks if the current registration payload fragment has already been
*         processed or not (retransmissions can occur because of some packet loss).
* @param  uint32_t node_id: The ID of the current node
* @param  uint8_t block_register: Boolean to signal if the registration is split among more than one block
* @param  uint32_t num_block: Number of the current registration payload block
* @retval int: 1 block still to be parsed, 0 block already parsed
*/
static int is_payload_to_be_parsed(int node_id, uint8_t block_register, uint32_t num_block)
{
	if (node_id >= 0 && node_id < numNode){
		if (nodes_array[node_id].node_flags & NODE_FLAG_REGISTERED){
			return 0;
		} else if (block_register){
			if (nodes_array[node_id].reg_block_flags & (1<<num_block)){
				return 0;
			}
		}
		return 1;
	}
	return 0; //Should not happen
}
/*---------------------------------------------------------------------------*/
/**
* @brief  retrieve_key
*         Retrieve a previously assigned location (key) for a given node.
* @param  uip_ipaddr_t: ipaddr The IP Address of the current node
* @param  char *key: To be filled with the location string
* @retval int: 1 success, 0 failure
*/
static int retrieve_key(uip_ipaddr_t ipaddr, char * key)
{
  uint32_t i = 0;
  while(i < numNode){
 		if(uip_ip6addr_cmp(&ipaddr, &nodes_array[i].ipaddr)){
 			strcpy(key, nodes_array[i].location);
  			return 1;
  		}
  		i++;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  is_unique_key
*         Check if a generated location is unique, as it should be according to
*         LWM2M specification.
* @param  char *key: The key that needs to be checked
* @param  int key_len: The length of the key
* @retval int: 1 it is unique, 0 it is not unique
*/
static int is_unique_key(char*key, int key_len)
{
	uint32_t i = 0;
	while(i < numNode){
		if(!memcmp(nodes_array[i].location, key, key_len)){
			return 0;
		}
		i++;
	}
	return 1;
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

