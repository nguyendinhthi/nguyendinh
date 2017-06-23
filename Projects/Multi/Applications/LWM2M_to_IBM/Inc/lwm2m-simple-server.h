/**
  ******************************************************************************
  * @file    lwm2m-simple-server.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M simple server header file
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
#ifndef LWM2M_SIMPLE_SERVER_H_
#define LWM2M_SIMPLE_SERVER_H_
/*---------------------------------------------------------------------------*/
#include "uip.h"
#include "lwm2m-defs.h"
#include "er-coap-constants.h"
/*---------------------------------------------------------------------------*/
/* If these macros are enabled, additional information is sotred in the nodes
 * array */
#define NODES_WITH_OBJECTS             0
#define NODES_WITH_REGISTATION_PAYLOAD 0
/*---------------------------------------------------------------------------*/
#define ERROR_OUT(...) printf("%s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);
/*---------------------------------------------------------------------------*/
/* Port used by the CoAP servers running on the nodes                        */
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)
/*---------------------------------------------------------------------------*/
#define MAX_OBJ_INST   15 // Max objects*instances per node
#define DATAPOINT_CNT  10 //Max Number of datapoint for any JSON packet
/*---------------------------------------------------------------------------*/
//#define MAX_NODES_ 20  //Moved to project-conf.h
#define MAX_OBS_  COAP_CONF_MAX_OBSERVEES
#define MAX_GET_  COAP_CONF_MAX_OBSERVEES //Equal number of obs and get, or MAX_NODES_ * "something"
#define MAX_CMD_  3 //Number of concurrent GETs command from cloud
/*---------------------------------------------------------------------------*/
#define CMD_BUFFER_LEN            200
#define URI_LEN                   15
#define RES_STRING_LEN            20
#define KEY_MAX_LEN               20
#define KEY_LEN                   5
#define LOCATION_PATH_LEN         2 * KEY_MAX_LEN
#define ENDPOINT_LEN              25
#define MAX_OBJ_LEN               10
#define READABLE_NAME_LEN         20
#define REGISTRATION_PALYOALD_LEN REST_MAX_CHUNK_SIZE * 3
/*---------------------------------------------------------------------------*/
//Possible values for Resource Types
#define RES_NONE    0x00
#define RES_INT     0x01
#define RES_FLOAT   0x02
#define RES_STRING  0x03
#define RES_BOOLEAN 0x04 //Currently not used/implemented
/*---------------------------------------------------------------------------*/
//Possible Flags for the kind of operation
#define OPR_NONE       0x00
#define OPR_TO_OBSERVE 0x01
#define OPR_OBSERVED   0x02
#define OPR_TO_GET     0x04
/*---------------------------------------------------------------------------*/
//Enumeration for the possible command we support from the cloud
typedef enum {
	Read,               // mapped to a LWM2M2 READ
	Write,              // mapped to a LWM2M2 WRITE
	Execute,            // mapped to a LWM2M2 EXECUTE
	Observe,            // mapped to a LWM2M2 OBSERVE
	Cancel,             // mapped to a LWM2M2 CANCEL (observation)
	PeriodicRead,       // specific for our gateway, mapped to a periodic GET
	CancelPeriodicRead  // specific for our gateway, cancels a periodic GET
} cloud_command_t;
/*---------------------------------------------------------------------------*/
/*Float values as we receive using TLV */
typedef int32_t floatfix32_t ;
/*---------------------------------------------------------------------------*/
/*Structure to store the value of a resource. It can be Int, Float or String
 * For Float type, the floatfix32_t conversion is used, to store the values
 * that arrive using the TLV encoding.*/
typedef union resourceValue_u {
	int valueInt;
	floatfix32_t valueFloat;
	char valueString[RES_STRING_LEN];
} resourceValue_t ;
/*---------------------------------------------------------------------------*/
/*Following structure defines the base types for the array of
 * gettable and observable resources.
 */
typedef struct lwm2m_srv_data_res_s {
	char uri[URI_LEN];
	int res_type; // RES_INT || RES_FLOAT || RES_STRING
	uint32_t node_id;
	resourceValue_t value;
	uint32_t flags;
	char readable_name[READABLE_NAME_LEN];
	uint8_t valid;
	uip_ipaddr_t ipaddr;
} lwm2m_srv_data_res_t ;
/*---------------------------------------------------------------------------*/
/* These are the possible values for the lwm2m_srv_cmd_res_t.flags field*/
#define CMD_DONE 0x00
#define CMD_SET 0x01
#define CMD_ANS_GOT 0x02
/*---------------------------------------------------------------------------*/
/*Following structure defines the base types for the array of
 * response to cloud command resources. */
typedef struct lwm2m_srv_cmd_res_s {
	char uri[URI_LEN];
	int res_type; // RES_INT || RES_FLOAT || RES_STRING
	uint32_t node_id;
	resourceValue_t value;
	uint32_t flags;
} lwm2m_srv_cmd_res_t ;
/*---------------------------------------------------------------------------*/
#if NODES_WITH_OBJECTS
typedef struct objects_instance{
	uint32_t object_id;
	uint32_t instance_id;
} obj_inst_t;
#endif
/*---------------------------------------------------------------------------*/
//Possible values for the 'node_flags' field.
#define NODE_FLAG_REGISTERING 0x01
#define NODE_FLAG_REGISTERED  0x02
/*---------------------------------------------------------------------------*/
/*object field is actually  object+instance. It is optional since the data
 * structures used to retrieve the values are the arrays of type
 * "lwm2m_srv_data_res_t". Storing nodes and/or payload can be useful if
 * we need to forward this info to the cloud.  */
typedef struct nodeInfo{
	uip_ipaddr_t ipaddr;
	uint16_t short_id; //currently not used but can be sent to cloud instead of the array id
	char endpoint_name[ENDPOINT_LEN];
	uint32_t node_flags;
	uint32_t reg_block_flags; //We assume there will be no more than 32 blocks in a registration.
	char location[LOCATION_PATH_LEN];
	uint8_t valid;
#if NODES_WITH_OBJECTS
	int numObj;
	obj_inst_t object[MAX_OBJ_INST];
#endif
#if NODES_WITH_REGISTATION_PAYLOAD
	char registration_payload[REGISTRATION_PALYOALD_LEN];
#endif
} nodeInfo_t;
/*---------------------------------------------------------------------------*/
/*Following structure is used to define the "templates", i.e. the object/resources
 * the LWM2M gateway is pre-programmed to observe/get
 */
typedef struct template_s {
	uint32_t object_id;
	uint32_t resource_id;
	uint8_t  res_type;        // RES_INT || RES_FLOAT || RES_STRING
	uint8_t  operation_info;  // OPR_TO_OBSERVE || OPR_TO_GET
	char readable_name[READABLE_NAME_LEN];
} template_t ;
/*---------------------------------------------------------------------------*/
#define TEMPLATE(template, ...) \
		template_t template[] = {__VA_ARGS__}

#define IPSO_RESOURCE_FLOAT(obj_id, res_id, operation, memo_name)   \
		 {obj_id, res_id, RES_FLOAT, operation, memo_name}

#define IPSO_RESOURCE_STRING(obj_id, res_id, operation, memo_name)	\
		 {obj_id, res_id, RES_STRING, operation, memo_name}

#define IPSO_RESOURCE_INT(obj_id, res_id, operation, memo_name)	    \
		 {obj_id, res_id, RES_INT, operation, memo_name}

//#define IPSO_RESOURCE_BOOLEAN(obj_id, res_id, operation, memo_name)

#endif // LWM2M_SIMPLE_SERVER_H_
