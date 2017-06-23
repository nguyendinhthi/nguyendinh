  /**
  ******************************************************************************
  * @file    my_wifi_driver.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    20-January-2016
  * @brief   Contiki driver for the IDWM01M1 wifi board
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
#include "wifi_interface.h"
#include "my_wifi_driver.h"
#include "ip64.h"
#include "ip64-addr.h"
#include "my_wifi_types.h"
#include "my_wifi_map.h"
#include <stdio.h>
#include <inttypes.h>

/** @addtogroup FP-NET-6LPWIFI1_Applications
* @{
*/

/** @addtogroup WIFI_Bridge
* @{
*/

/** @defgroup Contiki_SPWF01SA_driver
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

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS(my_wifi_process, "WIFI radio driver");
/*---------------------------------------------------------------------------*/
/**
* @brief  init
*         This function init the driver
* @param  None
* @retval None
*/
static void
init(void)
{
	process_start(&my_wifi_process, NULL);

	PRINTF("\ninit my_driver.c\n");
}

/*---------------------------------------------------------------------------*/
/**
* @brief  output
*         This function is the driver output
* @param  uint8_t *packet
* 		  uint16_t packet_len
* @retval None
*/
static int
output(uint8_t *packet, uint16_t len)
{
	PRINTF("\noutput funct --> my_driver.c\n");

	//here the wifi operations
	//we receive the packet ready to be sent, but we need to extract
	//dest ipaddress, port and payload

	struct my_wifimap_entry *w;
	struct ip64_addrmap_entry *m;

	struct ipv4_hdr *v4hdr;
	struct udp_hdr *udphdr;

	uint8_t sock_id;
	uint16_t DataLength;
	char *pData;
	WiFi_Status_t status = WiFi_MODULE_SUCCESS;

	v4hdr = (struct ipv4_hdr *)packet;
	udphdr = (struct udp_hdr *)&packet[IPV4_HDRLEN];


    uint8_t host[20];

    sprintf((char*)host, "%d.%d.%d.%d", v4hdr->destipaddr.u8[0], v4hdr->destipaddr.u8[1], v4hdr->destipaddr.u8[2], v4hdr->destipaddr.u8[3]);
    //ip64_addr_4to6(&v4hdr->srcipaddr, &tmp_ipv6addr);

    wifi_wakeup(WIFI_TRUE);

    switch(v4hdr->proto) {
      case IP_PROTO_UDP:
    	  /*
    	   * Currently only UDP is supported, because this application acts almost as a
    	   * router but we have only application level (socket) API for the protocol stack
    	   * running on the WiFi module: there is no way to let the TCP handshake work
    	   * properly.
    	   */
    	  pData = (char *)&packet[IPV4_HDRLEN + sizeof(struct udp_hdr)];
    	  DataLength = len - IPV4_HDRLEN - sizeof(struct udp_hdr);

    	  m = ip64_addrmap_lookup_port(uip_ntohs(udphdr->srcport), v4hdr->proto);

    	  if (m == NULL)
    	  {
    		  PRINTF("my_wifi_driver_output: addrmap_lookup failed\n");
    	  }
    	  else
    	  {
    		  //check if already exist an open socket mapped to the current addrmap
    		  //open socket if not existing
    		  //create addrmap<->socket mapping

    		  w = my_wifimap_lookup(uip_ntohs(udphdr->srcport), v4hdr->proto);

    		  if(w == NULL)
    		  {

    			  PRINTF("trying to recycle wifimap entries\n");

    			  if(my_wifimap_recycle())
    			  {
    				  PRINTF("a recyclable entry was deleted and a socket was closed\n");
    			  }
    			  else
    			  {
    				  PRINTF("no entry was deleted and no socket was closed\n");
    			  }

    			  //we should open a socket now
    			  PRINTF("my_wifi_driver_output: trying to open a socket via wifi\n");

    			  status = wifi_socket_client_open((uint8_t *)host, (uint32_t)uip_ntohs(udphdr->destport), (uint8_t *)"u", &sock_id);

    			  while((status != WiFi_MODULE_SUCCESS))
				  {
    				  /*
    				   * If we get an error, it should be because we are already using
    				   * all the available sockets (NUM_SOCKETS=8) of the WiFi Module
    				   * In this application we implemented a sort of socket reuse, so
    				   * we call my_wifimap_free() in order to free the oldest socket for
    				   * the new data flow. This is in line with a WSN scenario in which
    				   * the nodes (more than the available sockets) periodically send out
    				   * data to a server. There is no (socket-)limitation to this number of
    				   * clients, since the sockets are reused. Inbound traffic (i.e. from
    				   * server to the nodes) is limited to NUM_SOCKETS clients per time.
    				   * Note that this behavior is in line with the SKIP_LIFETIME option
    				   * that has been implemented in ip64-addrmap.c/ip64.c modules of Contiki OS,
    				   * since no "flow" is closed for time reasons.
    				   *
    				   * To summarize:
    				   *  - the maximum number of nodes/outbound connections is defined by
    				   *    IP64_ADDRMAP_CONF_ENTRIES or NUM_ENTRIES (ip64-addrmap.c) that
    				   *    defaults to 32.
    				   *  - the maximum number of bidirectional connections is defined by
    				   *    NUM_SOCKETS, that is set to his HARD limit of 8.
    				   */
					  if(my_wifimap_free())
					  {
						  PRINTF("the oldest entry was deleted and a socket was closed\n");
					  }
					  else
					  {
						  /*
						   * Since we reuse old sockets this should not happen, or it may
						   * indicate some problem with the WiFi
						   */
						  PRINTF("no entry was deleted and no socket was closed\n");
					  }

					  status = wifi_socket_client_open((uint8_t *)host, (uint32_t)uip_ntohs(udphdr->destport), (uint8_t *)"u", &sock_id);
					  if (status !=  WiFi_MODULE_SUCCESS)
					  {
						  /*This should not happen, we print a special warning note.*/
						  PRINTF("WARNING: Socket reuse failed, this is an unexpected behavior.\n");
					  }
				  }

				  PRINTF("\r\n >>Socket Open OK... host = %s, port = %d, protocol = udp \r\n", host, uip_ntohs(udphdr->destport));

    		      PRINTF("creating new wifi_map entry\n");

    		      w = my_wifimap_create(v4hdr, udphdr, sock_id);

      	          if(udphdr->srcport == UIP_HTONS(53)) //port 53 is DNS request
      	          {
      	        	  /*
      	        	   * DNS requests are one-shot (like it is done in ip64.c), we mark the
      	        	   * relative entries are recyclable to get rid of them at the next packet.
      	        	   */
      	        	  my_wifimap_set_recycleble(w);
      	          }

      	          PRINTF("my_wifi_driver_output: writing payload to socket number %d via wifi\n", sock_id);

      	          status = wifi_socket_client_write(w->sock_id, DataLength, pData);

      	          if(status == WiFi_MODULE_SUCCESS)
      	          {
      	        	  PRINTF("\r\n >>Socket Write OK\r\n");
      	          }

      	          else
      	          {
      	        	  PRINTF("Socket connection Error: status = %d\n", status);
      	          }

    		  }
    		  else //socket is already opened
    		  {
    			PRINTF("my_wifi_driver_output: writing payload to socket number %d via wifi\n", w->sock_id);
    			status = wifi_socket_client_write(w->sock_id, DataLength, pData);

    			if(status == WiFi_MODULE_SUCCESS)
				{
					PRINTF("\r\n >>Socket Write OK\r\n");
				}

				else
				{
					PRINTF("Socket connection Error: status = %d\n", status);
				}
    		  }

    	  }
        break;
    default:
    	/*As noted above, UDP only is supported by this application. */
        PRINTF("ip64_packet_4to6: protocol type %d not supported\n",
    	   v4hdr->proto);
        return 0;
      }

	return 0;
}
/*---------------------------------------------------------------------------*/
/* This struct defines the init and output functions configured in the
 * /Middlewares/ST/Contiki_STM32_Library/Inc/ip64-conf.h file
 */
const struct ip64_driver my_wifi_driver = {
  init,
  output
};
/*---------------------------------------------------------------------------*/
/**
* @brief  ind_wifi_socket_data_received
*         This function is the callback for data available in one of the open sockets
* @param  uint8_t socket_id
* 		  uint8_t * data_ptr
* 		  uint32_t message_size
* 		  uint32_t chunk_size
* @retval None
*/
void ind_wifi_socket_data_received(int8_t server_id, int8_t socket_id, uint8_t * data_ptr, uint32_t message_size, uint32_t chunk_size)
{
		struct my_wifimap_entry *w;

		//here we should see every packet that the wifi module receives as an answer from the server
		//we need to decide whether to forward each packet back to contiki or let the wifi module work
		PRINTF("packet received via wifi socket: socket number %d, ptr %d, mes_size %d, chunk_size %d\n",
				(int)socket_id, (int)(&data_ptr), (int)message_size, (int)chunk_size);

		w = my_wifimap_lookupby_socket(socket_id);

		if(w == NULL)
		{
			PRINTF("wifi_callback: failed lookup by socket_id\n");
		}
		else
		{
			//compute the new ipv4len

			w->v4hdr->len[0] = (uint16_t)(chunk_size + IPV4_HDRLEN + sizeof(struct udp_hdr)) >> 8;
			w->v4hdr->len[1] = (uint16_t)(chunk_size + IPV4_HDRLEN + sizeof(struct udp_hdr)) & 0xff;

			memcpy(&uip_buf[UIP_LLH_LEN], w->v4hdr, IPV4_HDRLEN);

			if(w->v4hdr->proto == IP_PROTO_UDP)
			{

				//compute the new udplen
				w->udphdr->udplen = (uint16_t)(chunk_size + sizeof(struct udp_hdr));

				memcpy(&uip_buf[UIP_LLH_LEN + IPV4_HDRLEN], w->udphdr, sizeof(struct udp_hdr));

				//w->udphdr->udplen = uip_htons((uint16_t)(chunk_size + sizeof(struct udp_hdr)));

			}
			else
			{
				PRINTF("wifi_callback: trying to copy an unsupported transport header with code : %d\n", w->v4hdr->proto);
				return;
			}

			if(w->v4hdr->proto == IP_PROTO_UDP)
			{
				memcpy(&uip_buf[UIP_LLH_LEN + IPV4_HDRLEN + sizeof(struct udp_hdr)], data_ptr, chunk_size);
			}
			else
			{
				PRINTF("wifi_callback: trying to copy the payload to a packet with an unsupported transport header with code : %d\n", w->v4hdr->proto);
				return;
			}

		}
		//wifi_state = wifi_state_idle;

		uip_len = chunk_size + IPV4_HDRLEN + sizeof(struct udp_hdr);
		PRINTF("wifi callback: successfully copied packet to uip_buf... calling IP64_INPUT\n");
		process_poll(&my_wifi_process);
		//back_to_contiki = 1;
}
/*---------------------------------------------------------------------------*/
/**
* @brief  ind_wifi_socket_client_remote_server_closed
*         This function is called whenever the remote server closes the connection
* @param  uint8_t socket_id
* @retval None
*/
void ind_wifi_socket_client_remote_server_closed(uint8_t * socket_closed_id)
{
#if DEBUG
  uint8_t id = *socket_closed_id;
  PRINTF("\r\n>>User Callback>>remote server socket (%d) closed\r\n", id);
#endif /*DEBUG*/
}
/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifi_process
*         This process ensures that as soon as a packet is received from the wifi,
*         the interface_input is called
* @param  struct pt* process
* 		  process_event_t ev
	      process_data_t data)
* @retval None
*/
PROCESS_THREAD(my_wifi_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {

    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    IP64_INPUT(&uip_buf[UIP_LLH_LEN], uip_len);
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
