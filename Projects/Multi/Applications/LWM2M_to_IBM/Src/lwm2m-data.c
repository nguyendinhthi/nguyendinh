/**
  ******************************************************************************
  * @file    lwm2m-data.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M Data structures management
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
/* This file implements the interfaces for the data structures used to actually
 * store the values of the resources.
 *  active_observations[] for the Observe/Notifications
 *  active_get[]          for the periodic GETs (loop)
 *  cmd_get[]             for the "one shot" GET/REPLY from/to cloud
 * */
/*---------------------------------------------------------------------------*/
#include <stdlib.h>
#include "er-coap-observe-client.h"
#include "lwm2m-resource-directory.h"
#include "lwm2m-simple-server.h"
#include "lwm2m-data.h"
/*---------------------------------------------------------------------------*/
/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup LWM2M_to_IBM
  * @{
  */

/** @defgroup LWM2M_to_IBM_data
  * @{
  */

#define DEBUG DEBUG_NONE//DEBUG_PRINT
#if DEBUG
#include <stdio.h>
#define LWM2M_PRINTF(...) printf(__VA_ARGS__)
#else
#define LWM2M_PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
//Local array of varios type of data sources
lwm2m_srv_data_res_t active_observations[MAX_OBS_]; // Array of all the observable resources according to the Template
uint32_t numObs = 0;                                 // Number of Objects/Resources to be observed
lwm2m_srv_data_res_t active_get[MAX_GET_];          // Array of all the "gettable" resources according to the Template
uint32_t numGet = 0;                                 // Number of Objects/Resources to be periodically gotten
lwm2m_srv_cmd_res_t cmd_get[MAX_CMD_];              // Array of the commands for GETs coming from the cloud.
uint8_t numCmd = 0;                                 // Number of concurrent GET commands
/*---------------------------------------------------------------------------*/
//Extern Data, from lwm2m-resource-directory.c
extern nodeInfo_t nodes_array[MAX_NODES_]; // Nodes Array structure to store all the information for each registered node
extern uint32_t numNode;                        // Number of registered nodes
/*---------------------------------------------------------------------------*/
/**
* @brief  init_lwm2m_data_structures
*         Initialize to 0x00 all the LWM2M data structures and counters.
* @param  None
* @retval None
*/
void init_lwm2m_data_structures(void)
{
	memset(active_observations, 0x00, sizeof(active_observations));
	memset(active_get, 0x00, sizeof(active_get));
	memset(cmd_get, 0x00, sizeof(cmd_get));
	numObs = numGet = numCmd = 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  add_periodic_read
*         Add a NODE_ID/URI to the list of periodic GETs
* @param  uint32_t node_id:   ID of the target node
* @param  char * uri:  URI in the form of object_id/instance_id/resource_id
* @param  uint8_t type:  Type of the resource (float, int or string)
* @param  char * readable_name:  Alternative name to the numeric format defined by LWM2M
* @retval int: 1 success, 0 failure
*/
int add_periodic_read(uint32_t node_id, char * uri, uint8_t type, char * readable_name)
{
	uint32_t index_get = 0;
	uint32_t i = 0;
	uint8_t found = 0;
	int obj, inst, res;

	LWM2M_PRINTF("ADD PERIODIC READ node_id %lu, uri '%s'\n", node_id, uri);
	/*First of all we check this was already in the list and has been disabled before.*/
	for (i=0; i<numGet; i++){
		if (active_get[i].node_id == node_id &&  !strcmp(active_get[i].uri, uri) &&
				active_get[i].res_type == type && active_get[i].valid == 0){
			strcpy(active_get[i].readable_name, readable_name);
			active_get[i].valid = 1;
			LWM2M_PRINTF("RE-ENABLED at position %lu\n", i);
			return 1;
		}
	}

	if (numGet == MAX_GET_) {
		for (i=0; i<MAX_GET_; i++){
			if (active_get[i].valid == 0){
				index_get = i;
				found = 1;
			}
		}
		if (!found) {
			printf("Reached maximum number of Periodic GETs (%lu), skipping the command request.\n", (uint32_t) MAX_GET_);
			return 0;
		}
	} else {
		index_get = numGet ;
		numGet++;
		LWM2M_PRINTF("Number of Active GETs is now %lu\n", numGet);
	}

	if (sscanf(uri, "%d/%d/%d", &obj, &inst, &res) != 3) {
		printf("ERROR: invalid path ('%s').\n", uri);
		return 0;
	}

	active_get[index_get].res_type = type;
	strncpy(active_get[index_get].uri, uri, strlen(uri));
	memset(active_get[index_get].readable_name, 0x00, READABLE_NAME_LEN);
	strncpy(active_get[index_get].readable_name,readable_name, strlen(readable_name));
	active_get[index_get].node_id = node_id;
	active_get[index_get].valid = 1;
	active_get[index_get].flags = 0x00 ;
	active_get[index_get].ipaddr = nodes_array[node_id].ipaddr;

	LWM2M_PRINTF("ADDED at position %lu\n", index_get);
	return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  remove_periodic_read
*         Remove a NODE_ID/URI to the list of periodic GETs
* @param  uint32_t node_id:   ID of the target node
* @param  char * uri:  URI in the form of object_id/instance_id/resource_id
* @retval int: 1 success, 0 failure
*/
int remove_periodic_read(uint32_t node_id, char * uri)
{
	uint32_t i = 0;

	for (i=0; i<numGet; i++){
		if (active_get[i].node_id == node_id &&  !strcmp(active_get[i].uri, uri)){
			active_get[i].valid = 0;
			return 1;
		}
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  add_observation
*         Add a NODE_ID/URI to the list of Observations
* @param  uint32_t node_id:   ID of the target node
* @param  char * uri:  URI in the form of object_id/instance_id/resource_id
* @param  uint8_t type:  Type of the resource (float, int or string)
* @param  char * readable_name:  Alternative name to the numeric format defined by LWM2M
* @retval int: 1 success, 0 failure
*/
int add_observation(uint32_t node_id, char * uri, uint8_t type, char * readable_name)
{
	uint32_t index_obs = 0;
	uint32_t i = 0;
	uint8_t found = 0;
	int obj, inst, res;

	LWM2M_PRINTF("ADD OBSERVATION node_id %lu, uri '%s'\n", node_id, uri);
	/*First of all we check this was already in the list and has been disabled before.*/
	for (i=0; i<numObs; i++){
		if (active_observations[i].node_id == node_id &&  !strcmp(active_observations[i].uri, uri) &&
				active_observations[i].res_type == type && active_observations[i].valid == 0){
			strcpy(active_observations[i].readable_name, readable_name);
			active_observations[i].valid = 1;
			active_observations[i].flags = OPR_TO_OBSERVE ;
			LWM2M_PRINTF("RE-ENABLED at position %lu\n", i);
			return 1;
		}
	}

	if (numObs == MAX_OBS_) {
		for (i=0; i<MAX_OBS_; i++){
			if (active_observations[i].valid == 0){
				index_obs = i;
				found = 1;
			}
		}
		if (!found) {
			printf("Reached maximum number of Observatios (%lu), skipping the command request.\n", (uint32_t) MAX_OBS_);
			return 0;
		}
	} else {
		index_obs = numObs ;
		numObs++;
		LWM2M_PRINTF("Number of Active Observation is now %lu\n", numObs);
	}

	if (sscanf(uri, "%d/%d/%d", &obj, &inst, &res) != 3) {
		printf("ERROR: invalid path ('%s')\n", uri);
		return 0;
	}
	active_observations[index_obs].res_type = type;
	strncpy(active_observations[index_obs].uri, uri, strlen(uri));
	memset(active_observations[index_obs].readable_name, 0x00, READABLE_NAME_LEN);
	strncpy(active_observations[index_obs].readable_name, readable_name, strlen(readable_name));
	active_observations[index_obs].node_id = node_id;
	active_observations[index_obs].valid = 1;
	active_observations[index_obs].flags = OPR_TO_OBSERVE ;
	active_observations[index_obs].ipaddr = nodes_array[node_id].ipaddr;

	LWM2M_PRINTF("ADDED at position %lu\n", index_obs);
	return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  remove_observation
*         Remove a NODE_ID/URI to the list of Observations
* @param  uint32_t node_id:   ID of the target node
* @param  char * uri:  URI in the form of object_id/instance_id/resource_id
* @retval int: 1 success, 0 failure
*/
int remove_observation(uint32_t node_id, char * uri)
{
	uint32_t i = 0;

	for (i=0; i<numObs; i++){
		if (active_observations[i].node_id == node_id &&  !strcmp(active_observations[i].uri, uri)){
			active_observations[i].valid = 0;
			return 1;
		}
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  get_observation_by_url
*         Retrieve the entry in the Observations list that matches a given [IP_ADDR]/URI
* @param  uip_ipaddr_t *addr_p:  IP Address of the target node (pointer)
* @param  const char * uri:  URI in the form of object_id/instance_id/resource_id
* @param  uint32_t *curr_obs: To be filled with the observation id
* @retval int: 1 success, 0 failure
*/
int get_observation_by_url(uip_ipaddr_t *addr_p, const char * uri, uint32_t *curr_obs)
{
	uint32_t i = 0 ;

	if (addr_p == NULL){
		ERROR_OUT("Unexpected null pointer as address.\n");
		return 0;
	}

    for(i = 0; i < numObs; i++){
    	if(uip_ip6addr_cmp(&active_observations[i].ipaddr, addr_p)){
    		if(!strcmp(uri, active_observations[i].uri) && active_observations[i].valid){
    			*curr_obs = i ;
    			return 1;
    		}
    	}
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  refresh_observations
*         For a given node, cancel all the active observations and sets this entry
*         to be Observed again. It is useful when a node re-register, maybe in
*         case of reset.
* @param  uint32_t node_id:   ID of the target node
* @retval int: 1 success, 0 failure
*/
int refresh_observations(uint32_t node_id)
{
	uint32_t i = 0;

	for (i=0; i<numObs; i++){
		if (active_observations[i].node_id == node_id){
			active_observations[i].valid = 1;//...
			LWM2M_PRINTF("[%lu] Removing observee to uri '%s' on node %lu", i, active_observations[i].uri, node_id);
		    //coap_obs_remove_observee_by_url(&active_observations[i].ipaddr, REMOTE_PORT, active_observations[i].uri);
		    coap_obs_remove_observee_by_url(&nodes_array[node_id].ipaddr, REMOTE_PORT, active_observations[i].uri);
			active_observations[i].flags = OPR_TO_OBSERVE;
		}
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  invalidate_observations
*         For a given node, cancel all the active observations.
* @param  uint32_t node_id:   ID of the target node
* @retval int: 1 success, 0 failure
*/
int invalidate_observations(uint32_t node_id)
{
	uint32_t i = 0;

	for (i=0; i<numObs; i++){
		if (active_observations[i].node_id == node_id){
			active_observations[i].valid = 0;
			LWM2M_PRINTF("[%lu] Removing observee to uri '%s' on node %lu", i, active_observations[i].uri, node_id);
		    //coap_obs_remove_observee_by_url(&active_observations[i].ipaddr, REMOTE_PORT, active_observations[i].uri);
		    coap_obs_remove_observee_by_url(&nodes_array[node_id].ipaddr, REMOTE_PORT, active_observations[i].uri);
			active_observations[i].flags = OPR_NONE;
		}
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  invalidate_periodic_gets
*         For a given node, cancel all the active periodice GETs.
* @param  uint32_t node_id:   ID of the target node
* @retval int: 1 success, 0 failure
*/
int invalidate_periodic_gets(uint32_t node_id)
{
	uint32_t i = 0;

	for (i=0; i<numGet; i++){
		if (active_get[i].node_id == node_id){
			active_get[i].valid = 0;
			active_get[i].flags = OPR_NONE;
		}
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  add_command_get
*         Add one entry in the slots of current GETs request issued as per a
*         command received from cloud.
* @param  uint32_t node_id:   ID of the target node
* @param  char * uri:  URI in the form of object_id/instance_id/resource_id
* @param  int res_type:  Type of the resource (float, int or string)
* @retval int: The command slot id
*/
uint8_t add_command_get(uint32_t node_id, char *uri, int res_type)
{
	uint8_t retval = numCmd;

	LWM2M_PRINTF("Preparing response data in slot %d\n", (int) numCmd);

	cmd_get[numCmd].node_id = node_id;
	strcpy(cmd_get[numCmd].uri, uri);
	cmd_get[numCmd].res_type = res_type;
	cmd_get[numCmd].flags = CMD_SET;
	numCmd = (numCmd+1) % MAX_CMD_;

	return retval;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  add_command_get_response
*         Add the response received from a node to the corresponding slot of
*         current GETs request (issued as per a command received from cloud)
* @param  uint8_t cmd_id:  ID of the command slot
* @param  void * value: The value of the response
* @retval int: 1 success, 0 failure
*/
int add_command_get_response(uint8_t cmd_id, void* value)
{
	LWM2M_PRINTF("Setting received response in slot %d\n", (int) cmd_id);
	if (value == NULL){
		ERROR_OUT("Unexpected error, received value is NULL\n");
		return 0;
	}
	switch (cmd_get[cmd_id].res_type){
	case RES_INT:
		cmd_get[cmd_id].value.valueInt = *((int*) value);
		break;
	case RES_FLOAT:
		cmd_get[cmd_id].value.valueFloat = *((floatfix32_t*) value);
		break;
	case RES_STRING:
		memset(cmd_get[cmd_id].value.valueString, 0x00, RES_STRING_LEN);
		strncpy(cmd_get[cmd_id].value.valueString, (char*)value, strlen((char*)value));
		break;
	default:
		ERROR_OUT("Unexpected type error (%d)\n", cmd_get[cmd_id].res_type);
		break;
	}
	cmd_get[cmd_id].flags |= CMD_ANS_GOT;

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
