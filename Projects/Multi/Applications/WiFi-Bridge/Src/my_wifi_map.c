  /**
  ******************************************************************************
  * @file    my_wifi_map.c
  * @author  Central LAB
  * @version V1.0.0
  * @date    20-January-2016
  * @brief   Mapping to link packet headers to a specific wifi socket
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
#include "my_wifi_types.h"
#include "my_wifi_map.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "ip64-conf.h"
#include "ip64-addr.h"
#include "wifi_interface.h"
#include <string.h>

/** @addtogroup FP-NET-6LPWIFI1_Applications
 *  @{
 */

/** @addtogroup WIFI_Bridge
* @{
*/

/** @defgroup Contiki_wifi_mapping
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


#define FIRST_MAPPED_PORT 10000

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* static allocation of the entire structure */
MEMB(wifientrymemb, struct my_wifimap_entry, NUM_SOCKETS);
LIST(wifientrylist);
/*---------------------------------------------------------------------------*/


/**
* @brief  my_wifimap_list
*         This function returns the top item in the
* @param  None
* @retval None
*/
struct my_wifimap_entry *
my_wifimap_list(void)
{
  return list_head(wifientrylist);
}
/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_init
*         This function init the list
* @param  None
* @retval None
*/
void
my_wifimap_init(void)
{
  memb_init(&wifientrymemb);
  list_init(wifientrylist);
}

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_lookup
*         This function looks for a specific entry, given the port number and the transport protocol
*         it returns a pointer to the found entry, or NULL
* @param  uint16_t port
* 		  uint8_t protocol
* @retval struct my_wifimap_entry *
*/
struct my_wifimap_entry *
my_wifimap_lookup(uint16_t mapped_port, uint8_t protocol)
{
  struct my_wifimap_entry *w;

  for(w = list_head(wifientrylist); w != NULL; w = list_item_next(w)) {
    if(w->ip64_mapped_port == mapped_port &&
       w->v4hdr->proto == protocol) {
      return w;
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_lookupby_socket
*         This function looks for a specific entry, given the socket number
*         it returns a pointer to the found entry, or NULL
* @param  uint8_t socket_number
* @retval struct my_wifimap_entry *
*/
struct my_wifimap_entry *my_wifimap_lookupby_socket(uint8_t socket_id)
{
  struct my_wifimap_entry *w;

  for(w = list_head(wifientrylist); w != NULL; w = list_item_next(w)) {
    if(w->sock_id == socket_id) {
      return w;
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_create
*         This function creates a new entry
*         it returns a pointer to the new entry, or NULL
*
* @note The purpose for this mapping is the creation of a list where each member
* 		stores information about the headers created by Contiki and the socket opened by the wifi module.
* 		This is necessary since the wifi module doesn't allow the transfer of
* 		a complete, well formed IPV4 packet, but takes the payload and manages
* 		the creation of all the headers at the firmware level.
* 		At this point we already have a well formed IPV4 packet, so we need to extract the payload and
* 		store the headers in order to be able to send back, to the
* 		remote nodes, the responses from the server.
* 		We need an association between the headers and the open socket, so we will know the
* 		remote node that made a specific request when we will receive a response
* 		in a specific socket
*
* @param  struct ipv4_hdr *v4hdr,
*		  struct udp_hdr *udphdr,
*		  uint8_t socket_id
* @retval struct my_wifimap_entry *
*/
struct my_wifimap_entry *my_wifimap_create(struct ipv4_hdr *v4hdr,
							struct udp_hdr *udphdr,
							uint8_t socket_id)
{
  struct my_wifimap_entry *w;

  uip_ip4addr_t temp;
  uint16_t temp_port;

  w = memb_alloc(&wifientrymemb);

  if(w == NULL) {
	  PRINTF("wifimap_create: could not allocate memory\n");
  }

  if(w != NULL) {

	w->ip64_mapped_port = uip_ntohs(udphdr->srcport);

	//invert src and dest then copy ipv4/udp header to uip_buf

	ip64_addr_copy4(&temp, &v4hdr->destipaddr);
	ip64_addr_copy4(&v4hdr->destipaddr, &v4hdr->srcipaddr);
	ip64_addr_copy4(&v4hdr->srcipaddr, &temp);

	memcpy(&temp_port, &udphdr->destport, sizeof(uint16_t));
	memcpy(&udphdr->destport, &udphdr->srcport, sizeof(uint16_t));
	memcpy(&udphdr->srcport, &temp_port, sizeof(uint16_t));

	memcpy(w->v4hdr, v4hdr, IPV4_HDRLEN);

	if(v4hdr->proto == IP_PROTO_UDP)
	{
		//w->tcphdr = NULL;
		memcpy(w->udphdr, udphdr, sizeof(struct udp_hdr));
	}
	else
	{
		PRINTF("my_wifimap: protocol not supported\n");
	}

	w->sock_id = socket_id;

	w->flags = FLAGS_NONE;

    timer_set(&w->timer, 0);

    list_add(wifientrylist, w);
    return w;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_set_recycleble
*         This function sets the an entry as recyclable
* @param  struct my_wifimap_entry *w
* @retval None
*/
void
my_wifimap_set_recycleble(struct my_wifimap_entry *w)
{
  if(w != NULL) {
    w->flags |= FLAGS_RECYCLABLE;
  }
}

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_recycle
*         This function deletes the oldest recyclable entry.
*         It returns 1 if a deletion occurred, 0 otherwise
* @param  None
* @retval int
*/
int my_wifimap_recycle(void)
{
  struct my_wifimap_entry *w, *oldest;

  /* Walk through the list of address mappings, throw away the
     oldest recyclable one. */
  oldest = NULL;
  for(w = list_head(wifientrylist);
      w != NULL;
      w = list_item_next(w)) {
    if(w->flags & FLAGS_RECYCLABLE) {
      if(oldest == NULL) {
        oldest = w;
      } else {
        if(timer_remaining(&w->timer) <
           timer_remaining(&oldest->timer)) {
          oldest = w;
        }
      }
    }
  }

  /* If we found the (oldest) recyclable entry, remove it and return
     non-zero. */
  if(oldest != NULL) {
	  if (WiFi_MODULE_SUCCESS == wifi_socket_client_close(oldest->sock_id)){
		  PRINTF("my_wifimap_recycle: closing socket %d\n", oldest->sock_id);
		  list_remove(wifientrylist, oldest);
		  memb_free(&wifientrymemb, oldest);
		  return 1;
	  } else {
		  PRINTF("my_wifimap_recycle: error, trying to close an already closed socket?\n");
		  return 0;
	  }
  } else {
	  PRINTF("my_wifimap_recycle: no entry needs recycling\n");
	  return 0;
  }
}

/*---------------------------------------------------------------------------*/
/**
* @brief  my_wifimap_free
*         This function deletes the oldest entry (without checking if it is
*         of recyclable type: it is used for socket reusing).
*         It returns 1 if a deletion occurred, 0 otherwise
* @param  None
* @retval int
*/
int my_wifimap_free(void)
{
  struct my_wifimap_entry *w, *oldest;

  /* Walk through the list of address mappings, throw away the oldest one. */
  oldest = NULL;
  for(w = list_head(wifientrylist); w != NULL; w = list_item_next(w)) {
      if(oldest == NULL) {
        oldest = w;
      } else {
        if(timer_remaining(&w->timer) < timer_remaining(&oldest->timer)) {
          oldest = w;
        }
      }
  }

  /* If we found an (oldest) entry, remove it and return non-zero. */
  if(oldest != NULL) {
	if (WiFi_MODULE_SUCCESS == wifi_socket_client_close(oldest->sock_id)){
		PRINTF("my_wifimap_free: closing socket %d\n", oldest->sock_id);
		list_remove(wifientrylist, oldest);
		memb_free(&wifientrymemb, oldest);
		return 1;
	} else {
		PRINTF("my_wifimap_free: error, trying to close an already closed socket?\n");
		return 0;
	}
  } else {
	  PRINTF("my_wifimap_free: something went wrong trying to free memory\n");
	  return 0;
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
