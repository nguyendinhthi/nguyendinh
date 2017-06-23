  /**
  ******************************************************************************
  * @file    project-conf.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    21-February-2017
  * @brief   Per-project configuration file
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


#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define MAX_NODES_ 20
//#include "ipso-example-server.h"

/*Here we refer to the default settings for the nodes (client side), that is
 * COAP_CONF_MAX_OBSERVERS = COAP_MAX_OPEN_TRANSACTIONS - 1 = 4 - 1 = 3
 * This can be changed up to memory costraints.
 * */
#define COAP_MAX_OBSERVERS_PER_NODE 3

/*We don't need to support observation at LWM2M Server side, so this set to minimum value */
#define COAP_MAX_OBSERVERS 1

/*So we support COAP_CONF_MAX_OBSERVEES possible observations that can be:
 * COAP_MAX_OBSERVERS_PER_NODE (from node configuration) times how many
 * nodes we support (MAX_NODES_)*/
#undef COAP_CONF_MAX_OBSERVEES
#define COAP_CONF_MAX_OBSERVEES COAP_MAX_OBSERVERS_PER_NODE * MAX_NODES_

/* Multiplies with chunk size, be aware of memory constraints. */
#undef COAP_MAX_OPEN_TRANSACTIONS
#define COAP_MAX_OPEN_TRANSACTIONS     COAP_CONF_MAX_OBSERVEES + 1 //4

//#undef IEEE802154_CONF_PANID
//#define IEEE802154_CONF_PANID       0xFEDE
/**
 * Disabling RDC and CSMA to save memory on constrained devices.
 */
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC              nullrdc_driver

#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC              csma_driver

/* Disabling TCP on CoAP nodes. */
#undef UIP_CONF_TCP
#define UIP_CONF_TCP                   0

/* Increase rpl-border-router IP-buffer when using more than 64. */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE            64

/* Filtering .well-known/core per query can be disabled to save space. */
#undef COAP_LINK_FORMAT_FILTERING
#define COAP_LINK_FORMAT_FILTERING     0
#undef COAP_PROXY_OPTION_PROCESSING
#define COAP_PROXY_OPTION_PROCESSING   0

/* Enable client-side support for COAP observe */
#define COAP_OBSERVE_CLIENT 1

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 16

/*---------------------------------------------------------------------------*/
/*To be validated in your scenario */
//#define UIP_CONF_RECEIVE_WINDOW 150
#undef UIP_CONF_RECEIVE_WINDOW
#undef UIP_CONF_TCP_MSS

/*
 * Constraints and suggestions:
 ** 60 <= UIP_CONF_BUFFER_SIZE <= 1514 (uipopt.h)
 ** (UIP_CONF_BUFFER_SIZE - UIP_UDPIP_HLEN) < 548 (ip64-dhcp.c)
 ** COAP_MAX_PACKET_SIZE > (UIP_CONF_BUFFER_SIZE - UIP_IPH_LEN - UIP_UDPH_LEN) (er-coap.h)
 * The smaller, the lower the TCP throughput, and vv.
 * IP buffer size must match all other hops, in particular the border router.
 * This value limits the maximum size for an IP packet to be received by the system.
 * A good value is 600.
*/
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 1300
/*---------------------------------------------------------------------------*/

#define UIP_CONF_ROUTER			1
#define UIP_CONF_IPV6_RPL		1
#ifdef UIP_CONF_ND6_SEND_RA
#undef UIP_CONF_ND6_SEND_RA
#define UIP_CONF_ND6_SEND_RA	1
#endif
#define UIP_CONF_ND6_RA_RDNSS   1
/*---------------------------------------------------------------------------*/

#endif /* PROJECT_CONF_H_ */
