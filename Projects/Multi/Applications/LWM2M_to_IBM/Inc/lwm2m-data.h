/**
  ******************************************************************************
  * @file    lwm2m-data.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M Data structures management header file
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
#ifndef LWM2M_DATA_H_
#define LWM2M_DATA_H_
/*---------------------------------------------------------------------------*/
int add_periodic_read(uint32_t node_id, char * path, uint8_t type, char * readable_name);
int remove_periodic_read(uint32_t node_id, char * path);
int add_observation(uint32_t node_id, char * path, uint8_t type, char * readable_name);
int remove_observation(uint32_t node_id, char * path);
int refresh_observations(uint32_t node_id);
int invalidate_observations(uint32_t node_id);
int invalidate_periodic_gets(uint32_t node_id);
void init_lwm2m_data_structures(void);
int get_observation_by_url(uip_ipaddr_t *addr_p, const char * uri, uint32_t *curr_obs);
uint8_t add_command_get(uint32_t node_id, char *uri, int res_type);
int add_command_get_response(uint8_t cmd_id, void* value);
/*---------------------------------------------------------------------------*/
#endif /*LWM2M_DATA_H_*/
