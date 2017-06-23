  /**
  ******************************************************************************
  * @file    my_wifi_types.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    20-January-2016
  * @brief   Common structs and typedefs used across the project
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
#ifndef MY_WIFITYPES_H
#define MY_WIFITYPES_H

/* Includes ------------------------------------------------------------------*/
#include "ip64-addrmap.h"

/* Exported types ------------------------------------------------------------*/
/**
 * @brief IPV4 header structure.
 */
struct ipv4_hdr {
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ip4addr_t srcipaddr, destipaddr;
};

/**
 * @brief TCP header structure (not supported yet).
 */
struct tcp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint8_t seqno[4];
  uint8_t ackno[4];
  uint8_t tcpoffset;
  uint8_t flags;
  uint8_t  wnd[2];
  uint16_t tcpchksum;
  uint8_t urgp[2];
  uint8_t optdata[4];
};

/**
 * @brief UDP header structure.
 */
struct udp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint16_t udplen;
  uint16_t udpchksum;
};

/**
 * @brief ICMP header structure (not supported yet).
 */
struct icmpv4_hdr {
  uint8_t type, icode;
  uint16_t icmpchksum;
};

/**
 * @brief Wifi Map structure.
 */
struct my_wifimap_entry {
  struct my_wifimap_entry *next;
  struct timer timer;
  struct ipv4_hdr v4hdr[IPV4_HDRLEN];
  struct udp_hdr udphdr[sizeof(struct udp_hdr)];
  uint16_t ip64_mapped_port; /*The mapped port is the link between IP64 maps and WiFi maps*/
  uint8_t sock_id;
  uint8_t flags;
};

/**
 * @brief WiFi states needed to implement a state machine (main.c file).
 */
//#error "my types"
typedef enum {
  wifi_state_reset = 0,
  wifi_state_ready,
  wifi_state_connected,
  wifi_state_connecting,
  wifi_state_disconnected,
  wifi_state_socket,
  wifi_state_activity,
  wifi_state_inter,
  wifi_state_print_data,
  wifi_state_error,
  wifi_state_idle,
  wifi_undefine_state       = 0xFF,
} wifi_state_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */



#endif /* MY_WIFITYPES_H */
