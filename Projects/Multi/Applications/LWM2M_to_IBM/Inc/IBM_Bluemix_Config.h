
#ifndef __IBM_Bluemix_Config_H
#define __IBM_Bluemix_Config_H

#include "stdio.h"
#include "stdint.h"
#include "MQTTClient.h"


typedef enum {
  QUICKSTART = 0,
  REGISTERED = 1,
} ibm_mode_t;


typedef struct mqtt_vars
{
	uint8_t pub_topic[48];
	uint8_t sub_topic[48];
	uint8_t clientid[64];
	enum QoS qos;
	uint8_t username[32];
	uint8_t password[32]; // feedid?
	uint8_t hostname[64];
	uint8_t device_type[32];
	uint8_t org_id[32];
	uint32_t port;
	uint8_t protocol; // //t -> tcp , s-> secure tcp, c-> secure tcp + certificates
	ibm_mode_t ibm_mode; // QUICKSTART, REGISTERED
} MQTT_vars;





/* MQTT IBM Functions */
void Config_MQTT_IBM ( MQTT_vars *mqtt_ibm_setup, uint8_t *macadd ); 
void Compose_Quickstart_URL ( uint8_t *url_ibm, uint8_t *macadd ); 
void MQTTtimer_init(void);

/* Time config variables for MQTT Paho */
#define SCALE 84000000/10000     
#define COUNTER_TM 10000/1000    
 
void TIM4_IRQHandler(void);
void Error_Handler(void);
void messageArrived(MessageData* md);


#endif 


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
