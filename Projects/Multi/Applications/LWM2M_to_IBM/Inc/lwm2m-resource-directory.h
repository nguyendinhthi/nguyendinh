/**
******************************************************************************
  * @file    lwm2m-resource-directory.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M Resource Directory header file
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
#ifndef LWM2M_RESOURCE_DIRECTORY_H_
#define LWM2M_RESOURCE_DIRECTORY_H_
/*---------------------------------------------------------------------------*/
#include <stdlib.h>
#include "net/ip/uip.h"
/*---------------------------------------------------------------------------*/
int retrieve_node_id_by_address(uip_ipaddr_t ipaddr, uint32_t *node_id);
int add_node (uint32_t *node_id, uip_ipaddr_t ipaddr);
int remove_node_by_id (uint32_t node_id);
int remove_node_by_address (uip_ipaddr_t ipaddr);
void init_lwm2m_nodes_structure(void);
/*---------------------------------------------------------------------------*/
#endif /*LWM2M_RESOURCE_DIRECTORY_H_*/
