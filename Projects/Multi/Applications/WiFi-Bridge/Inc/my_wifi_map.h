  /**
  ******************************************************************************
  * @file    my_wifi_map.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    20-January-2016
  * @brief   Headers for the wifi mapping
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MY_WIFIMAP_H
#define MY_WIFIMAP_H

/* Includes ------------------------------------------------------------------*/
#include "sys/timer.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define FLAGS_NONE       0
#define FLAGS_RECYCLABLE 1

#define NUM_SOCKETS 8/* 8 because SPWF01M1 supports up to 8 open sockets */


/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/**
 * @brief Initialize the my_wifimap module.
 */
void my_wifimap_init(void);

/**
 * @brief Look up a wifi mapping given the
 * 		  mapped port number and protocol.
 */
struct my_wifimap_entry *my_wifimap_lookup(uint16_t mapped_port,
						    uint8_t protocol);

/**
 * @brief Look up a wifi mapping given the
 * 		  socket number.
 */
struct my_wifimap_entry *my_wifimap_lookupby_socket(uint8_t socket_id);

/**
 * Create a new mapping
 *  */
struct my_wifimap_entry *my_wifimap_create(struct ipv4_hdr *v4hdr,/*
							struct tcp_hdr *tcphdr,*/
							struct udp_hdr *udphdr,
							uint8_t socket_id);

/**
 * @brief Mark a mapping to be recyclable.
 */
void my_wifimap_set_recycleble(struct my_wifimap_entry *w);

/**
 * @brief Recycle a mapping.
 */
int my_wifimap_recycle(void);

/**
 * @brief Freee memory in the wifi_map.
 */
int my_wifimap_free(void);


/**
 * @brief Obtain a pointer to the first element of the list of mappings.
 */
struct my_wifimap_entry *my_wifimap_list(void);

#endif /* MY_WIFIMAP_H */
