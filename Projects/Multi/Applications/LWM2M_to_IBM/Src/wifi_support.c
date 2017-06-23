/**
******************************************************************************
* @file    wifi_support.c 
* @author  AAS / CL
* @version V1.0.0
* @date    23-Nov-2015
* @brief   This file implements the generic functions for WiFi communication.
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

/*******************************************************************************
 * Include Files
*******************************************************************************/
#include "string.h"
#include "stdio.h"
#include "wifi_support.h"
#include "cube_hal.h"
#include "stdbool.h"
#include "wifi_module.h"
#include "wifi_globals.h"

/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup LWM2M_to_IBM
 *  @{
 */
 
/** @defgroup WIFI_SUPPORT 
 * @{
 */
 
/*******************************************************************************
 * Macros
*******************************************************************************/

/******************************************************************************
 * Local Variable Declarations
******************************************************************************/

/******************************************************************************
 * Global Variable Declarations
******************************************************************************/
/** @defgroup WIFI_SUPPORT_Exported_Variables
 *  @{
 */
WiFi_Priv_Mode mode = WPA_Personal;
uint8_t console_input[1];
uint8_t console_count = 0;
wifi_bool set_AP_config = WIFI_FALSE;
wifi_bool SSID_found = WIFI_FALSE;
char console_ssid[NDEF_WIFI] = "";
char console_psk[NDEF_WIFI] = "";
uWifiTokenInfo UnionWifiToken;
extern wifi_scan net_scan[WIFI_SCAN_BUFFER_LIST];
   
/**
 * @}
 */
 
/******************************************************************************
 * Function Declarations
******************************************************************************/
/** @defgroup UART_SUPPORT_Functions
 *  @{
 */

/******************************************************************************
 * Function Definitions 
******************************************************************************/
/**
  * @brief  Query the User for SSID, password and encryption mode
  * @param  None
  * @retval System_Status_t
  */
System_Status_t wifi_get_AP_settings(void)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  int i = 0;

  printf("\r\n >>running WiFi Scan...\r\n");
  status = wifi_network_scan(net_scan, WIFI_SCAN_BUFFER_LIST);
  if(status==WiFi_MODULE_SUCCESS)
  {
	for (i=0; i<WIFI_SCAN_BUFFER_LIST; i++)
	{
	  if (strlen(net_scan[i].ssid) > 0)
	  {
		 printf(net_scan[i].ssid);
		 printf("\r\n");
	  }
	}
	memset(net_scan, 0x00, sizeof(net_scan));
  }
  else
  {
	 printf("\r\n >>Scan failure...\r\n");
  }
  fflush(stdout);

  set_AP_config = WIFI_TRUE;
  printf("\r\n\nEnter the SSID:\n");
  fflush(stdout);
  console_count = 0;
  console_count = scanf("%s", console_ssid);
  printf("\r\n");
  fflush(stdout);

  if(console_count == NDEF_WIFI)
  {
	printf("Exceeded permitted number of ssid characters.");
	return MODULE_ERROR;
  }

  printf("\r\nEnter the encryption mode(0:Open, 1:WEP, 2:WPA2/WPA2-Personal):");
  fflush(stdout);
  scanf("%s",console_input);
  printf("\r\n");
  switch(console_input[0])
  {
	case '0':
	  mode = None;
	  break;
	case '1':
	  mode = WEP;
	  break;
	case '2':
	  mode = WPA_Personal;
	  break;
	default:
	  printf("\r\nWrong Entry. Priv Mode is not compatible\n");
	  return MODULE_ERROR;
  }

  strcpy(console_psk, "");
  if (mode != None)
  {
	  printf("\r\nEnter the password:");
	  fflush(stdout);
	  console_count = 0;
	  console_count = scanf("%s", console_psk);
	  printf("\r\n");
	  fflush(stdout);
	  if(console_count == NDEF_WIFI)
	  {
		printf("Exceeded permitted number of psk characters.");
		return MODULE_ERROR;
	  }
  }

  memcpy(WIFITOKEN.NetworkSSID, console_ssid, strlen(console_ssid));
  memcpy(WIFITOKEN.NetworkKey, console_psk, strlen(console_psk));
  if (mode == None) {
    memcpy(WIFITOKEN.AuthenticationType, "NONE", strlen("NONE"));
  } else if (mode == WEP) {
    memcpy(WIFITOKEN.AuthenticationType, "WEP", strlen("WEP"));
  } else {
    memcpy(WIFITOKEN.AuthenticationType, "WPA2", strlen("WPA2"));
  }

  return MODULE_SUCCESS;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Save Access Point parameters to FLASH
  * @param  None
  * @retval System_Status_t (MODULE_SUCCESS/MODULE_ERROR)
  */
System_Status_t SaveSSIDPasswordToMemory(void)
{
  System_Status_t status = MODULE_ERROR;

  /* Reset Before The data in Memory */
  status = ResetSSIDPasswordInMemory();

  if (status) {
    /* Store in Flash Memory */
    uint32_t Address = BLUEMSYS_FLASH_ADD;
    int32_t WriteIndex;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Write the Magic Number */
    {
      uint32_t MagicNumber = WIFI_CHECK_SSID_KEY;
      if (HAL_FLASH_Program(TYPEPROGRAM_WORD, Address,MagicNumber) == HAL_OK) {
        Address = Address + 4;
      } else {
        printf("\r\nError while writing in FLASH");
        status = MODULE_ERROR;
      }
    }

    /* Write the Wifi */
    for (WriteIndex=0;WriteIndex<UNION_DATA_SIZE;WriteIndex++) {
      if (HAL_FLASH_Program(TYPEPROGRAM_WORD, Address,UnionWifiToken.Data[WriteIndex]) == HAL_OK){
        Address = Address + 4;
      } else {
        printf("\r\nError while writing in FLASH");
        status = MODULE_ERROR;
      }
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    printf("\n\rSaveSSIDPasswordToMemory OK");
  } else {
    printf("\n\rError while resetting FLASH memory");
  }

  return status;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Erase Access Point parameters from FLASH
  * @param  None
  * @retval System_Status_t (MODULE_SUCCESS/MODULE_ERROR)
  */
System_Status_t ResetSSIDPasswordInMemory(void)
{
  /* Reset Calibration Values in FLASH */
  System_Status_t status = MODULE_ERROR;

  /* Erase First Flash sector */
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SectorError = 0;

  EraseInitStruct.TypeErase = TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = BLUEMSYS_FLASH_SECTOR;
  EraseInitStruct.NbSectors = 1;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
    /* Error occurred while sector erase.
      User can add here some code to deal with this error.
      SectorError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
      FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError(); */
      printf("\n\rError while erasing FLASH memory");
  } else {
      status = MODULE_SUCCESS;
  }
  /* Lock the Flash to disable the flash control register access (recommended
  to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return status;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Read Access Point parameters from FLASH
  * @param  None
  * @retval System_Status_t (MODULE_SUCCESS/MODULE_ERROR)
  */
System_Status_t ReCallSSIDPasswordFromMemory(void)
{
  System_Status_t status = MODULE_ERROR;

  uint32_t Address = BLUEMSYS_FLASH_ADD;
  __IO uint32_t data32 = *(__IO uint32_t*) Address;
  if(data32== WIFI_CHECK_SSID_KEY){
    int32_t ReadIndex;

    for(ReadIndex=0;ReadIndex<UNION_DATA_SIZE;ReadIndex++){
      Address +=4;
      data32 = *(__IO uint32_t*) Address;
      UnionWifiToken.Data[ReadIndex]=data32;
    }
    status = MODULE_SUCCESS;
  } else {
    printf("\r\nFLASH Keyword not found.");
  }

  return status;
}
/*---------------------------------------------------------------------------*/
/**
  * @brief  Configure Access Point parameters (SSID, PWD, Authentication) :
  * @brief  1) Read from FLASH
  * @brief  2) If not available in FLASH or when User Button is pressed : read from serial terminal
  * @param  None
  * @retval System_Status_t (MODULE_SUCCESS/MODULE_ERROR)
  */
System_Status_t ConfigAPSettings(void)
{
  System_Status_t status = MODULE_ERROR;
  bool b_set_AP_pars = false;
  uint8_t i;

  printf("\r\nKeep pressed user button within next 5 seconds to set Wi-Fi Access Point parameters (SSID and PWD) ");
  printf("\r\nvia serial terminal. Otherwise parameters saved to FLASH will be used.\n");
  for (i=0; i<5; i++) {
    printf("\b\b\b\b...%d", 4-i);
    fflush(stdout);
    HAL_Delay(1000);
  }
  printf("\b\b\b\b");
  fflush(stdout);

  /* User Button Pressed --> set parameters */
  if(BSP_PB_GetState(BUTTON_USER) != GPIO_PIN_RESET) {
    /* Read from FLASH */
    if(ReCallSSIDPasswordFromMemory() > 0) {
      printf("\n\rRead from FLASH:\n\r");
      printf("\tSSID ............ = %s\n\r", WIFITOKEN.NetworkSSID);
      printf("\tKey ............. = %s\n\r", WIFITOKEN.NetworkKey);
      printf("\tAuthentication .. = %s\n\r", WIFITOKEN.AuthenticationType);

      status = MODULE_SUCCESS;
    } else {
      printf("\n\rNo data written in FLASH.");
      b_set_AP_pars = true;
    }
  } else {
    b_set_AP_pars = true;
  }

  if (b_set_AP_pars) {
    // Blink LED
    /*
    printf("\r\nLED ON....\n\r");
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    BSP_LED_On(LED2);
    HAL_Delay(3000);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    BSP_LED_Off(LED2);
    */
    status = MODULE_ERROR;

    if (status==MODULE_SUCCESS) {
      printf("\r\n[E]. Error in ConfigAPSettings. \r\n");
    } else {
      printf("\n\rRead parameters from serial terminal.");

      /* Read from Serial.  */
      status = wifi_get_AP_settings();
      if (status != MODULE_SUCCESS) {
        printf("\r\n[E]. Error in AP Settings\r\n");
        return status;
      }
    }
    /* Save to FLASH */
    status = SaveSSIDPasswordToMemory();
    if (status != MODULE_SUCCESS) {
      printf("\r\n[E]. Error in AP Settings\r\n");
      return status;
    }
  }

  return status;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  wifi_get_status_variable
*         Get a status variable, this generic API is missing in current WiFi
*         middleware so it is added here. The main purpose is to retrieve the
*         "version" status variable.
* @param  const char * variable: the name of the status variable
* @param  uint8_t *result: pointer to the result buffer
* @retval status  : status of AT cmd request
*/
WiFi_Status_t wifi_get_status_variable(const char * variable, uint8_t *result)
{
  WiFi_Status_t status = WiFi_MODULE_SUCCESS;
  int cfg_value_length;

   /* AT : send AT command */
  Reset_AT_CMD_Buffer();
#if defined(CONSOLE_UART_ENABLED)
  sprintf((char*)WiFi_AT_Cmd_Buff,AT_GET_STATUS_VALUE,variable);
  status = USART_Transmit_AT_Cmd(strlen((char*)WiFi_AT_Cmd_Buff));
#else
  if (!strcmp(variable, "")){
	  sprintf((char*)WiFi_AT_Cmd_Buff,"AT+S.STS\r");
  } else {
	  sprintf((char*)WiFi_AT_Cmd_Buff,AT_GET_STATUS_VALUE,variable);
  }
  run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_DMA);
  IO_status_flag.AT_event_processing = WIFI_GCFG_EVENT;
  status = WiFi_MODULE_SUCCESS;
#endif

  if(status == WiFi_MODULE_SUCCESS)
    {
      status = USART_Receive_AT_Resp( );
      cfg_value_length = strlen((const char*)WiFi_Counter_Variables.get_cfg_value);

      /* copy user pointer to get_cfg_value */
      memcpy(result,WiFi_Counter_Variables.get_cfg_value,cfg_value_length);
      memset(WiFi_Counter_Variables.get_cfg_value, 0x00,cfg_value_length);
    }
  return status;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  check_wifi_firmware_version
*         Retrieves the WiFi firmware information, prints it and checks if it
*         is >= "the latest" with regards to the time of coding.
*         If no, it warns the  user that it is better to update it.
* @param  None
* @retval None
*/
void check_wifi_firmware_version(void)
{
	uint8_t firmware_info[FIRMWARE_VERSION_LEN] = "";

	printf("\nWiFi Module firmware information:\n");
	memset(firmware_info, 0x00, FIRMWARE_VERSION_LEN);
	if (WiFi_MODULE_SUCCESS == GET_Configuration_Value("nv_serial", (uint32_t *)firmware_info)){
	  printf("'nv_serial': '%s'\n", firmware_info);
	} else {
	  printf("Error getting the 'nv_serial' configuration variable.\n");
	}
	memset(firmware_info, 0x00, FIRMWARE_VERSION_LEN);
	if (WiFi_MODULE_SUCCESS == wifi_get_status_variable("version", firmware_info)){
	  printf("'version': '%s'\n", firmware_info);
	} else {
	  printf("Error getting the 'version' status variable.\n");
	}

	if (strncmp((char *)firmware_info, WIFI_LATEST_FIRMWARE_VERSION_STR, strlen(WIFI_LATEST_FIRMWARE_VERSION_STR))>=0){
	  printf("WiFi firmware version is up to date.\n");
	} else {
	  printf(" ****************************************************\n");
	  printf(" *** WARNING THE WIFI FIRMWARE IS AN OLD VERSION, ***\n");
	  printf(" ***   YOU ARE STRONGLY SUGGESTED TO UPDATE IT.   ***\n");
	  printf(" ****************************************************\n");
	}
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
 
 /**
 * @}
 */
 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
