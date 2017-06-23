/**
  ******************************************************************************
  * @file    wifi_support.h 
  * @author  CL
  * @version V1.0.0
  * @date    23-Nov-2015
  * @brief   
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _WIFI_SUPPORT_H_
#define _WIFI_SUPPORT_H_

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "wifi_interface.h"
#include "wifi_const.h"   
   
/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup LWM2M_to_IBM
 *  @{
 */
 
/** @addtogroup WIFI_SUPPORT 
 * @{
 */

/** @addtogroup WIFI_SUPPORT_Exported_Defines
 *  @{
 */
#define WIFI_SCAN_BUFFER_LIST   32
#define MQTT_PUBLISH_DELAY      4000
#define NDEF_WIFI               32
#define BLUEMSYS_FLASH_ADD      ((uint32_t)0x08060000)
#define BLUEMSYS_FLASH_SECTOR   FLASH_SECTOR_7
#define WIFI_CHECK_SSID_KEY     ((uint32_t)0x12345678)
#define UNION_DATA_SIZE         ((32+32+6+6)>>2)
#define USART_SPEED_MSG	        460800
/*---------------------------------------------------------------------------*/
#define WIFITOKEN UnionWifiToken.TokenInfo
/*---------------------------------------------------------------------------*/
/*Added to check the WiFi Firmware version. Keep it aligned with the latest release */
#define WIFI_LATEST_FIRMWARE_VERSION_STR "160129-c5bf5ce-SPWF01S"
#define WIFI_LATEST_FIRMWARE_VERSION_NBR "3.5"
#define FIRMWARE_VERSION_LEN    32
/*---------------------------------------------------------------------------*/
 typedef enum
 {
   MODULE_SUCCESS = 1,
   MODULE_ERROR   = -1
 } System_Status_t;
/*---------------------------------------------------------------------------*/
 typedef struct
 {
   char NetworkSSID[32];
   char AuthenticationType[6];
   char EncryptionType[6];
   char NetworkKey[32];
 } sWifiTokenInfo;
/*---------------------------------------------------------------------------*/
 typedef union
 {
   uint32_t Data[UNION_DATA_SIZE];
   sWifiTokenInfo TokenInfo;
 } uWifiTokenInfo;
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
   
   
/** @addtogroup WIFI_SUPPORT_Exported_Types
 *  @{
 */
#ifdef TCP_SOCKET_SERVER
typedef enum {
  wifi_state_reset = 0,
  wifi_state_ready,
  wifi_state_idle,
  wifi_state_connected,
  wifi_state_connecting,
  wifi_state_disconnected,
  wifi_state_socket,
  wifi_state_sending,
  wifi_state_error,
  wifi_undefine_state = 0xFF,  
} wifi_state_t;  
#endif
/*---------------------------------------------------------------------------*/

#ifdef IBM_BLUEMIX
typedef enum {
  wifi_state_reset = 0,
  wifi_state_ready,
  wifi_state_idle,
  wifi_state_connected,
  wifi_state_connecting,
  wifi_state_disconnected,
  wifi_state_error,
  wifi_state_socket_close,
  mqtt_socket_create,
  client_conn,
  mqtt_connect,
  mqtt_sub,
  mqtt_pub,
  mqtt_device_control,
  wifi_undefine_state = 0xFF,  
} wifi_state_t;
#endif
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

/** @addtogroup WIFI_SUPPORT_Exported_Variables
 *  @{
 */

/**
 * @}
 */
 
/** @addtogroup WIFI_SUPPORT_Exported_Functions
 *  @{
 */
/*---------------------------------------------------------------------------*/
System_Status_t ConfigAPSettings(void);
System_Status_t wifi_get_AP_settings(void);
System_Status_t ReCallSSIDPasswordFromMemory(void);
System_Status_t ResetSSIDPasswordInMemory(void);
System_Status_t SaveSSIDPasswordToMemory(void);

WiFi_Status_t wifi_get_status_variable(const char * variable, uint8_t *result);
void check_wifi_firmware_version(void);
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

/**
 * @}
 */
 
#ifdef __cplusplus
}
#endif

#endif /* _SAMPLE_SERVICE_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

