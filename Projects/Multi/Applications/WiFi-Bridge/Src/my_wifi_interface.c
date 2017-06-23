  /**
  ******************************************************************************
  * @file    my_wifi_interface.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    20-January-2016
  * @brief   Contiki interface for the IDWM01M1 wifi board
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
#include "net/ip/uip.h"
#include "net/ip/tcpip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"

#include "ip64.h"

#include <string.h>
#include <stdio.h>

/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup WIFI_Bridge
  * @{
  */

/** @defgroup Contiki_SPWF01SA_interface
  * @{
  */
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* Private macro -------------------------------------------------------------*/
#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

/* Private variables ---------------------------------------------------------*/
static uip_ipaddr_t last_sender;

/* Private function prototypes ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifi_interface_input
*         This function is the interface input called when a packet
*         is received from the wifi
* @param  uint8_t *packet
* 		  uint16_t packet_len
* @retval None
*/

/* This function is configured in the
 * /Middlewares/ST/Contiki_STM32_Library/Inc/ip64-conf.h file
 */
void
my_wifi_interface_input(uint8_t *packet, uint16_t len)
{
    uint16_t length = ip64_4to6(&uip_buf[UIP_LLH_LEN], uip_len,
			     ip64_packet_buffer);
    if(len > 0) {
      memcpy(&uip_buf[UIP_LLH_LEN], ip64_packet_buffer, length);
      uip_len = length;
      /*      PRINTF("send len %d\n", len); */
    } else {
      uip_clear_buf();
    }

    /* Save the last sender received over IPv4 interface to avoid bouncing the
       packet back if no route is found */
    uip_ipaddr_copy(&last_sender, &UIP_IP_BUF->srcipaddr);
    tcpip_input();
}
/*---------------------------------------------------------------------------*/
/**
* @brief  init
*         This function is the interface init
* @param  None
* @retval None
*/
static void
init(void)
{
	PRINTF("my-interface: init\n");
}
/*---------------------------------------------------------------------------*/
/**
* @brief  output
*         This function is the interface output called when a packet
*         should be sent via wifi
* @param  None
* @retval None
*/
static int
output(void)
{
  int len;

  /*
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF(" destination ");
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);
  PRINTF("\n");
  */
  if(uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
	  PRINTF("my-interface: output, not sending bounced message\n");
  } else {
    len = ip64_6to4(&uip_buf[UIP_LLH_LEN], uip_len,
		    ip64_packet_buffer);
    PRINTF("my-interface: output len %d\n", len);
    if(len > 0) {
      memcpy(&uip_buf[UIP_LLH_LEN], ip64_packet_buffer, len);
      uip_len = len;
      IP64_ETH_DRIVER.output(&uip_buf[UIP_LLH_LEN], len);    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/* This struct defines the init and output functions configured in the
 * /Middlewares/ST/Contiki_STM32_Library/Inc/ip64-conf.h file
 */
const struct uip_fallback_interface my_wifi_interface = {
  init, output
};
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
