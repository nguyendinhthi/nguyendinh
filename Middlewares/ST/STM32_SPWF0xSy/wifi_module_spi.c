/**
 ******************************************************************************
 * @file    wifi_module_spi.c
 * @author  Central LAB
 * @version V0.0.1
 * @date    10-Nov-2016
 * @brief   Enable Wi-Fi functionality using SPI
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
#include "wifi_module.h"
#include "wifi_globals.h"
#include "enum.h"

/** @addtogroup MIDDLEWARES
* @{
*/ 

/** @defgroup  NUCLEO_WIFI_MODULE
  * @brief Wi-Fi driver modules
  * @{
  */

/** @defgroup NUCLEO_WIFI_MODULE_Private_Defines
  * @{
  */

/**
  * @}
  */

/** @addtogroup NUCLEO_WIFI_MODULE_Private_Variables
  * @{
  */
//#define SPI_IRQ_MODE
#define SPI_DMA_MODE
#define HEADER_SIZE 5 //1[0x2], (1)[KOE], 1[Wind_No], 2[Payload Len] = 5 bytes
#define SPI_READ_PACKET_SIZE 512
#define CS_PULSE_700NS_NBR_CYCLES_REQ1  352
#define CS_PULSE_LENGTH1 (CS_PULSE_700NS_NBR_CYCLES_REQ1/4)
#define DUMMY_RAM_ADDRESS_TO_READ1 (0x20000000)
#define SOCKET_WRITE_DELAY 250

uint8_t Write_Header_CMD[HEADER_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t Read_Header_CMD[HEADER_SIZE] = {0x02, 0x00, 0x00, 0x00, 0x00};
uint8_t Received_Header[HEADER_SIZE];
uint8_t dummy_Header[HEADER_SIZE];
uint8_t dataBuff[SPI_READ_PACKET_SIZE];

const uint8_t dummy_bytes = 0xFF;
char dummy_char = 0xFF;
uint8_t wind_no, sync;
uint16_t http_packet_len, payload_len, payload_to_read, payload_read_len;
uint8_t* tx_payload_data;
uint32_t payload_size_to_transmit;
uint8_t* tx_header_data;
uint8_t  tx_header_size;
uint8_t  tx_payload_size;
uint8_t packet_cont;
static uint8_t scan_ignore=0;
wifi_bool wind_55_in_Q = WIFI_FALSE;

extern wifi_instances_t wifi_instances;

SPI_RECEIVE_EVENT_t Spi_Receive_Event;
SPI_TRANSMIT_EVENT_t Spi_Transmit_Event;
KIND_OF_EVENT_t kind_of_event;
wifi_bool SPI_Tx_Transfer;

void ReceiveHeader(SPI_RECEIVE_EVENT_t ReceiveEvent, uint8_t * DataHeader, uint8_t DataLen);
//void ReceiveDummy(uint8_t * Data, uint8_t DataLen);
void SPI_Transmit_Manager(SPI_TRANSMIT_REQUEST_t TransmitRequest);
void SPI_Transmit_Manager_Poll(uint8_t * Data, uint32_t DataLen);
void SPI_Receive_Manager(SPI_RECEIVE_REQUEST_t ReceiveRequest);

void ProcessEndOfReceive(KIND_OF_PACKET_t kop, WiFi_Indication_t wind_no);
void Process_Packet(KIND_OF_PACKET_t kop, WiFi_Indication_t wind_no);
void Process_Wind_Payload(WiFi_Indication_t wind_no);

void ReceiveClosure(void);
void TransmitClosure(void);
void no_op(int dummy) { }

#if defined(SPWF04)
/**
 * @brief  WIFI SPI IRQ Callback
 * @param  None
 * @retval None
 */
void WIFI_SPI_IRQ_Callback(void)
{
  volatile uint8_t localloop;
  Disable_SPI_Receiving_Path();
  Enable_SPI_CS();//Enable nCS (Chip Select) 
  
  /*TIme needed to stabilize the Clk ???*/
  for (localloop = 0 ; localloop < CS_PULSE_LENGTH1 ; localloop++)
  {
    no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
  }  
  
  if(IO_status_flag.send_data)
  {
      /*TIme needed to make sure internal DMA has been re-programmed?*/
      for (localloop = 0 ; localloop < SOCKET_WRITE_DELAY ; localloop++)
      {
        no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
      }  
      
      SPI_Transmit_Manager(SPI_WRITE_PAYLOAD_TRANSMIT);//now send the data
      IO_status_flag.send_data = WIFI_FALSE;
  } else
  {
    //if(!SPI_Tx_Transfer)/*Ignore IRQ if in between a AT Cmd, Fix for Socket Server read!*/
    SPI_Receive_Manager(SPI_READ_HEADER_FOR_RX);
  }

}

#ifdef SPI_DMA_MODE
/**
 * @brief  Manage the SPI receive
 * @param  ReceiveRequest: the receive request
 * @retval None
 */
void SPI_Receive_Manager(SPI_RECEIVE_REQUEST_t ReceiveRequest)
{
  uint16_t byte_count;
  volatile uint8_t i, dummy = 0x00;
  //uint16_t payload_len;
  
  /*
   *  Disable both DMA
   */
  __HAL_DMA_DISABLE(SpiHandle.hdmatx);
  __HAL_DMA_DISABLE(SpiHandle.hdmarx);
  
  /**
   * Flush the Rx register or FIFO
   */
  for (i = 0 ; i < SPI_FIFO_RX_DEPTH ; i++)
  {
    no_op(*(volatile uint8_t*)__HAL_WIFI_SPI_GET_RX_DATA_REGISTER_ADDRESS(&SpiHandle));
  }
  
  __HAL_DMA_ENABLE_IT(SpiHandle.hdmarx, DMA_IT_TC);	/**< Enable Receive packet notification */
  __HAL_DMA_DISABLE_IT(SpiHandle.hdmatx, DMA_IT_TC); /**< Disable Transmit packet notification */
  
  switch (ReceiveRequest)
  {
  case SPI_READ_HEADER_FOR_RX:
    ReceiveHeader(SPI_RECEIVED_HEADER_FOR_RX, (uint8_t *)Read_Header_CMD, HEADER_SIZE);
    break;
    
  case SPI_READ_HEADER_FOR_TX:
    ReceiveHeader(SPI_RECEIVED_HEADER_FOR_TX, (uint8_t *)tx_payload_data, payload_size_to_transmit);
    break;
    
  case SPI_READ_PAYLOAD:
    
    byte_count = payload_to_read;
    if (byte_count > (sizeof dataBuff))
    {
      byte_count = (sizeof dataBuff);
    }
    
    payload_read_len = byte_count;

    Spi_Receive_Event = SPI_RECEIVE_END;/*For Wind Payload*/        
    
    __HAL_WIFI_DMA_CLEAR_MINC(SpiHandle.hdmatx); /**< Configure DMA to send same Byte */
    
    /*
     *  Set counter in both DMA
     */
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmarx, byte_count);
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmatx, byte_count);
    
    /*
     *  Set memory address in both DMA
     */
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmarx, (uint32_t)&dataBuff);
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmatx, (uint32_t)&dummy_bytes);

    break;
  
  case SPI_READ_DATA_PAYLOAD:  
    
    //What happens when payload_to_read becomes > sizeof(dataBuff)?
    //we first read sizeof(datBuff) and repeat read till payload_to_read is complete
    byte_count = payload_to_read;
    
    if (byte_count > (sizeof dataBuff))
    {
      byte_count = (sizeof dataBuff);
      payload_to_read -= byte_count; //reduce payload_to_read by amount read(sizeof(dataBuff)) so far
      Spi_Receive_Event = SPI_RECEIVE_INTERIM_PAYLOAD_DATA;/*For Data Payload higher than dataBuff Size*/
    }
    else
    {
      payload_to_read = 0;
      Spi_Receive_Event = SPI_RECEIVE_PAYLOAD_DATA;/*For Data Payload*/
    }
    
    payload_read_len = byte_count;            
    
    __HAL_WIFI_DMA_CLEAR_MINC(SpiHandle.hdmatx); /**< Configure DMA to send same Byte */
    
    /*
     *  Set counter in both DMA
     */
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmarx, byte_count);
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmatx, byte_count);
    
    /*
     *  Set memory address in both DMA
     */    
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmarx, (uint32_t)&dataBuff);
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmatx, (uint32_t)&dummy);
    break;      

  default:
    break;
  }
  
  /*
   *  Enable both DMA - Rx First
   */
  __HAL_DMA_ENABLE(SpiHandle.hdmarx);
  __HAL_DMA_ENABLE(SpiHandle.hdmatx);
  
  return;
}
#endif

/**
 * @brief Receive header
 * @param  ReceiveEvent: Set the current receive event ongoing.
 *         DataHeader  : Data Header for Tx (not used today).
 *         DataLen     : Tx Data Len (not used today)
 * @retval None
 */
void ReceiveHeader(SPI_RECEIVE_EVENT_t ReceiveEvent, uint8_t * DataHeader, uint8_t DataLen)
{
  Spi_Receive_Event = ReceiveEvent;
      
  __HAL_WIFI_DMA_SET_MINC(SpiHandle.hdmatx);	/**< Configure DMA to send Tx packet */
  
  /*
   *  Set counter in both DMA
   */
  __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmatx, DataLen);
  __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmarx, HEADER_SIZE);
  
  /*
   *  Set memory address in both DMA
   */
  __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmarx, (uint32_t)Received_Header);
  __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmatx, (uint32_t)DataHeader);
  
  return;
}

/**
 * @brief Tx and Rx Transfer completed callbacks
 * @param  None
 * @retval None
 */
uint8_t fix_counter = 0;
#define DELAY_NEXT_HEADER_FETCH 1555555 /*MAGIC NUMBER!!!*/

void WiFi_DMA_RxCallback(void)
{
  uint8_t error_state;
  volatile uint32_t localloop;
  WiFi_Indication_t wind;
  static WiFi_Indication_t last_wind;

  #if DEBUG_PRINT
  uint8_t wifi_status;
  #endif
  
  __HAL_DMA_CLEAR_FLAG(SpiHandle.hdmarx, WIFI_SPI_RX_DMA_TC_FLAG);
  
  /**
   * Clear TX TCIF just in case
   */
  __HAL_DMA_CLEAR_FLAG(SpiHandle.hdmatx, WIFI_SPI_TX_DMA_TC_FLAG);
  
  switch (Spi_Receive_Event)
    {
      case SPI_RECEIVED_HEADER_FOR_TX:
      case SPI_RECEIVED_HEADER_FOR_RX:
        kind_of_event = (KIND_OF_EVENT_t)((Received_Header[1]>>4)&0xF);
        wind_no = Received_Header[2];
        wind = (WiFi_Indication_t)wind_no;
        sync = Received_Header[0];
        
        /*For the time being we are filtering out headers wth SYNC word as 0xB5/0xB7/0xFE/0x80/0xFF*/
        if(sync != 0x02 && ( (sync != 0x80 && sync != 0xFF && sync != 0xB5 && sync != 0xB6 && sync != 0xB7) 
                        && kind_of_event==SPI_INCOMING_DATA )) 
          kind_of_event = SPI_FRAME_ERROR;        
        
        switch(kind_of_event)
        {                      
          case  SPI_WIND_NOTIFICATION:
            payload_len = (Received_Header[4]<<8)|Received_Header[3];  
            #if DEBUG_PRINT
            wifi_status = (Received_Header[1])&0xF;
            #endif
            
            if(wind_no == 0x07) 
            {
                error_state = (Received_Header[4]<<8)|Received_Header[3];  
                #if DEBUG_PRINT
                printf("\r\n>> Configuration Failure no %d", error_state);
                #endif
            }
            #if DEBUG_PRINT
            printf("\r\n<<WIND %d, status %d\r\n", wind_no, wifi_status);
            #endif
            if (payload_len <= 0)
              {
                  /* Go for the next header after processing the WIND*/
                  Disable_SPI_CS();    
                  ProcessEndOfReceive(SPI_WIND_PACKET, wind);
              }
              else
              {
                  last_wind = wind;
                  payload_to_read = payload_len;
                  payload_read_len = 0;
                  SPI_Receive_Manager(SPI_READ_PAYLOAD);	/**< Read the WIND Payload */
              }
            break;
            
          case  SPI_INCOMING_DATA:
            payload_len = (Received_Header[4]<<8)|Received_Header[3];  
            error_state = Received_Header[2];//@TBD:Check if parsing is correct for Error!
            //Handle Data
            //Should be same as above switch case??
            if (payload_len <= 0  && (error_state == 0x00) /* || error_state == 0xFE || error_state== 0xFF ) && SPI_Tx_Transfer_Complete==WIFI_FALSE */)
              {
                    
                    //printf("\r\n<<AT+S.OK\r\n");                    
                    /* This is an OK event*/
                    Disable_SPI_CS();  
                    ProcessEndOfReceive(SPI_OK_PACKET, Invalid_Wind); 
                                        
                    switch(IO_status_flag.AT_event_processing)
                    {
                    case WIFI_HTTP_EVENT:
                      if(WiFi_Control_Variables.http_req_pending == WIFI_TRUE)
                      {
                        WiFi_Control_Variables.http_req_pending = WIFI_FALSE;
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                        //IO_status_flag.sock_read_ongoing = WIFI_TRUE;
                        WiFi_Counter_Variables.sock_total_count = 0;
                      }
                      break;
                    case WIFI_CLIENT_SOCKET_WRITE_EVENT:
                      WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                      break;
                    case WIFI_SCAN_EVENT:
                      WiFi_Counter_Variables.sock_total_count = 0;
                      scan_ignore = 0;
                      break;
                    case WIFI_NO_EVENT:
                      WiFi_Counter_Variables.sock_total_count = 0;
                      #ifdef SPI_VCOM                          
                        IO_status_flag.AT_vcom_processing = WIFI_NO_EVENT;
                        IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                      #endif
                      break;
                    default:
                      break;
                    }                   
              
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                    SPI_Tx_Transfer = WIFI_FALSE;//OK has arrived, so AT cmd is not ongoing now
                    
                    //In case there is follow-up action after this OK, do it here
                    if(WiFi_Control_Variables.afterOK_start_sock_read == WIFI_TRUE)
                    {
                        WiFi_Control_Variables.afterOK_start_sock_read = WIFI_FALSE;
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;//stop event de-queue
                        //Socket_Read(WiFi_Counter_Variables.Socket_Data_Length);
                        WiFi_Control_Variables.start_sock_read = WIFI_TRUE;
                        
                    }                     
                    else if(WiFi_Control_Variables.afterOK_enable_query == WIFI_TRUE)
                    { 
                        IO_status_flag.prevent_push_OK_event = WIFI_TRUE;//There's another OK coming (for the NULL command)
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;//stop event de-queue
                        //Socket_Pending_Data();                        
                        WiFi_Control_Variables.afterOK_enable_query = WIFI_FALSE;
                        WiFi_Control_Variables.enable_query = WIFI_TRUE;                        
                    }
                    else if(WiFi_Control_Variables.afterOK_start_sockd_read == WIFI_TRUE)
                    {
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;//stop event de-queue
                        WiFi_Control_Variables.afterOK_start_sockd_read = WIFI_FALSE;
                        //Socket_Server_Read(WiFi_Counter_Variables.Socket_Data_Length);
                        WiFi_Control_Variables.start_sockd_read = WIFI_TRUE;                        
                    }
                    else if(WiFi_Control_Variables.afterOK_enable_server_query == WIFI_TRUE)
                    {                        
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;//stop event de-queue
                        WiFi_Control_Variables.afterOK_enable_server_query = WIFI_FALSE;
                        //Server_Pending_Data();
                        WiFi_Control_Variables.enable_server_query = WIFI_TRUE;                        
                    }                      
                    
              }
            else if (payload_len > 0)
              {
#if DEBUG_PRINT
                  //printf("<<payload_len:%d\r\n",payload_len);
#endif
                  switch(IO_status_flag.AT_event_processing)
                  {
                  case WIFI_HTTP_EVENT:
                    http_packet_len = payload_len;
                    payload_to_read = payload_len;
                    payload_read_len = 0;
                    break;
                  case WIFI_CLIENT_SOCKET_READ_DATA:
                    //This is read using DMA, not socket.read (using DMA we can red only upto payload_len!
                    //@TBD: Not correct. Cannot read more than payload_len!!!!!!!
                    #if DEBUG_PRINT
                    printf("<<payload_len:%d\r\n",payload_len);
                    #endif
                    payload_to_read = payload_len;
                    payload_read_len = 0;
                    //printf("<<socket read data payload_len:%d\r\n",payload_len);
                    break;
                  default:
                    payload_to_read = payload_len;
                    payload_read_len = 0;
                    break;
                  }                  
                  
                    DisableEnable_SPI_CS(); 
                    SPI_Receive_Manager(SPI_READ_DATA_PAYLOAD);	/**< Read the Data Packet Payload */
              }
            //else if (payload_len == 0 && error_state!= 0x00 && error_state!= 0xFE && error_state!= 0xFF)//ignore 0xFE && 0xFF error number
            else if (payload_len == 0 && error_state!= 0x00)//ignore 0xFE && 0xFF error number
              {
                    //printf("\r\n<<Error Condition!!\r\n");                    
                    Disable_SPI_CS(); 
                    if(error_state== 0xFE || error_state== 0xFF)
                    {
                      #if DEBUG_PRINT
                      printf("\r\n<<corrupted OK packet!!\r\n");                    
                      #endif
                      #ifdef SPI_VCOM
                      ProcessEndOfReceive(SPI_IGNORE_PACKET, Invalid_Wind);
                      #else
                      ProcessEndOfReceive(SPI_IGNORE_PACKET, Invalid_Wind);
                      #endif
                      break;
                    }                    
                    
                    if(IO_status_flag.AT_event_processing == WIFI_CERT_WRITE_EVENT)
                    {
                      IO_status_flag.prevent_push_OK_event = WIFI_TRUE; //do not Q this OK as a proper OK is arriving.
                      //Process packet as OK
                      ProcessEndOfReceive(SPI_OK_PACKET, Invalid_Wind);
                    }
                    else
                      ProcessEndOfReceive(SPI_ERROR_PACKET, Invalid_Wind);
              }
            else
              {
                    /* Go for the next header if there is any*/
                    Disable_SPI_CS();   
                    ProcessEndOfReceive(SPI_DATA_PACKET, Invalid_Wind); 
              }
            break;  
            
          case  SPI_ERROR_NOTIFICATION:
            Disable_SPI_CS(); 
            ProcessEndOfReceive(SPI_ERROR_PACKET, Invalid_Wind);
            break;
            
          case  SPI_FRAME_ERROR:
            //printf("Frame dump\r\n");            
            Disable_SPI_CS(); 
            ProcessEndOfReceive(SPI_IGNORE_PACKET, Invalid_Wind);
            break;
            
          default:
            /* Release CS line */
            //printf("\r\n Unknown packet event %d!!\r\n", kind_of_event);
            Disable_SPI_CS();    
            ProcessEndOfReceive(SPI_IGNORE_PACKET, Invalid_Wind);
            break;
        }        
        memset(Received_Header, 0x00, strlen((char const*)Received_Header));
        break;
        
      case SPI_RECEIVE_PAYLOAD_DATA:
          //Data or AT Cmd Payload 
          /*Next a header should arrive if there is more data*/
          Disable_SPI_CS();
          ProcessEndOfReceive(SPI_DATA_PAYLOAD_PACKET, Invalid_Wind);

        break;
      
      case SPI_RECEIVE_INTERIM_PAYLOAD_DATA:   
        
        Process_Packet(SPI_DATA_INTERIM_PAYLOAD_PACKET, Invalid_Wind);//Set the events  
        memset(dataBuff, 0x00, strlen((char const*)dataBuff));
    
        break;
      
      case SPI_RECEIVE_END:
        /* Release CS line */
        /*Data is in dataBuff*/
        Disable_SPI_CS();       
        /*@TBD: In case of WIND:24 WiFi-Up Fix! 
        -> causes frame error when next header is fetched*/
        /*Temp Fix -> Delay next header fetch*/        
        if(wind_no == 24 && fix_counter==0) 
        { 
            fix_counter = 1;//This loop happens only once during WiFi-Up
            for (localloop = 0 ; localloop < DELAY_NEXT_HEADER_FETCH ; localloop++)
            {
              no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
            }
        }
        //Spi_Receive_Event = SPI_RECEIVED_HEADER_FOR_RX;
        /*Proceed with next header*/
        ProcessEndOfReceive(SPI_WIND_PAYLOAD_PACKET, last_wind);        
        break;
    
      default:
        break;
    }
  
}

#define DELAY_PART_PAYLOAD_FETCH 1555555//200000 /*MAGIC NUMBER!!!*/
/**
 * @brief  Resume the payload reception which is not finished yet
 * @param  None
 * @retval None
 */
void ResumePayloadReception(void)
{
    //volatile uint32_t localloop;
	
    //There is more data pending for this payload packet, so just fetch them
    DisableEnable_SPI_CS();
    //printf("\r\nresume_payload");
    switch(IO_status_flag.AT_event_processing)
        {
        case WIFI_HTTP_EVENT:
          //printf("\r\nResuming HTTP Payload (more than 512b)>>\r\n");
//          for (localloop = 0 ; localloop < DELAY_PART_PAYLOAD_FETCH ; localloop++)
//            {
//              *(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1;
//            }
          break;
        case WIFI_GPIO_EVENT:          
          break;          
        case WIFI_GCFG_EVENT:          
          break;
        case WIFI_SCAN_EVENT:
          break;
        default:
          break;
        }
    SPI_Receive_Manager(SPI_READ_DATA_PAYLOAD);//no header coming for this case
}    

/**
 * @brief  Resume the packet header reception
 * @param  None
 * @retval None
 */
void ResumePacketReception(void)
{
  volatile uint32_t localloop;
  memset(dataBuff, 0x00, strlen((char const*)dataBuff));
    
  Enable_SPI_Receiving_Path();
  
  if (HAL_GPIO_ReadPin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN) == GPIO_PIN_RESET)
  {
    __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
  } 
  else //line is low does not mean that there is no more data, as in cases handled below
  {
    switch(IO_status_flag.AT_event_processing)
      {
      case WIFI_GPIO_EVENT:                        
        break;
        
      case WIFI_GCFG_EVENT:                        
        break;
      
      case WIFI_HTTP_EVENT:
        //printf("\r\nResuming HTTP Payload Packet Headers>>\r\n");
        
        //Check:Fix for No OK frame after HTTP_Payload!!---->Start
        //Send a dummy NULL AT command to generate an OK frame, but sometimes this will also not work!
        Reset_AT_CMD_Buffer();  
        sprintf((char*)WiFi_AT_Cmd_Buff,"AT");  //Send a Null Command
        run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);              
        
        //IO_status_flag.AT_CMD_count++;
        //printf("\r\nIO_status_flag.AT_CMD_count++: %d\r\n",IO_status_flag.AT_CMD_count);

        //do not queue the OK for this?
        //IO_status_flag.prevent_push_OK_event = WIFI_TRUE;
        //Check:Fix for No OK frame after HTTP_Payload!!---->End
      break;
      
      case WIFI_SERVER_SOCKET_READ_EVENT:
      case WIFI_SERVER_SOCKET_READ_DATA:
      case WIFI_CLIENT_SOCKET_READ_EVENT:
      case WIFI_CLIENT_SOCKET_READ_DATA:
        for (localloop = 0 ; localloop < 10000 ; localloop++)
          {
               no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
          }
        //printf("\r\nresume_packet1...new packet header expected...");
        //But it does not come, so workaround-> generate SW IRQ
        __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
        break;
        
      case WIFI_SCAN_EVENT:
        for (localloop = 0 ; localloop < 10000 ; localloop++)//1555555
        {
          no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
        }
        __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
        break;
        
      case WIFI_NO_EVENT:
        #ifdef SPI_VCOM
        switch(IO_status_flag.AT_vcom_processing)
          {
            case WIFI_SCAN_EVENT:
              for (localloop = 0 ; localloop < 200 ; localloop++)//1555555
              {
                no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
              }
              __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
              break;
            
            default:
              break;
          }
        #endif
        break;
      default:
        break;
      }
  }
  
}    

        
/**
 * @brief  Close the receiver path
 * @param  None
 * @retval None
 */
void ReceiveClosure(void)
{
  /*
   *  Disable both DMA
   */
  __HAL_DMA_DISABLE(SpiHandle.hdmatx);
  __HAL_DMA_DISABLE(SpiHandle.hdmarx);
  
  /*
   * Check if a command is pending
   */
  //__disable_irq();
  //TODO://Disable here and Enable after Processing data
  //Disable_SPI_Receiving_Path();
  
  //__enable_irq();
  
  return;
}

/**
 * @brief  ProcessEndOfReceive
 * @param  kop : the kind of packet received
 *         wind_no : the WIND number if applicable
 * @retval None
 */
void ProcessEndOfReceive(KIND_OF_PACKET_t kop, WiFi_Indication_t wind_no)
{
  volatile uint32_t localloop;
  
  ReceiveClosure();

  switch(IO_status_flag.AT_event_processing)
      {
      case WIFI_GPIO_EVENT:        
        break;        
      case WIFI_GCFG_EVENT:        
        break;      
      case WIFI_HTTP_EVENT:        
        break;      
      
      case WIFI_SERVER_SOCKET_READ_DATA:
      case WIFI_CLIENT_SOCKET_READ_DATA:
        //Why do we need this delay: Somehow some sync issues, delay here to finish all user processing incase of Socket and
        //delay to receive the next packet as it might interrupt with the ongoing user process/activity?
        //TBD: SOLVE THIS SYNC ISSUE!!!
        for (localloop = 0 ; localloop < 10000 ; localloop++)//1555555
          {
            no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
          }
        //printf("\r\nresume_packet2");
        break;
        
      case WIFI_SCAN_EVENT:
        for (localloop = 0 ; localloop < 55000 ; localloop++)//50000 cycles sometimes works, sometimes doesn't. 50000 safe?
          {
            no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
          }
        __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
        break;
        
      case WIFI_NO_EVENT:
        #ifdef SPI_VCOM
          switch(IO_status_flag.AT_vcom_processing)
          {
            case WIFI_SCAN_EVENT:
              for (localloop = 0 ; localloop < 1000 ; localloop++)//1555555
              {
                no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
              }
              __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
              break;
            default:
              for (localloop = 0 ; localloop < 1000 ; localloop++)//1555555
              {
                no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
              }
              break;
          }
        #endif
        break;
        
      default:
        break;
          
      }  

  Process_Packet(kop, wind_no);//Set the events
  
  if(!spi_callback_in_progress)
  {
    ResumePacketReception();
  }
  return;
}

/**
 * @brief  Tx and Rx Transfer completed callbacks
 * @param  None
 * @retval None
 */
void WiFi_DMA_TxCallback(void)
{
  __HAL_DMA_CLEAR_FLAG(SpiHandle.hdmatx, WIFI_SPI_TX_DMA_TC_FLAG);
  
  switch (Spi_Transmit_Event)
  {
  case SPI_HEADER_TRANSMITTED:
    if(payload_size_to_transmit != 0)
    {
      SPI_Transmit_Manager(SPI_PAYLOAD_TRANSMIT);
    }
    else
    {
      TransmitClosure();      
    }
    break;
    
  case SPI_PAYLOAD_TRANSMITTED:    
      TransmitClosure();
    break;
    
  default:
    break;
  } 
  
  return;
}


/**
 * @brief  Set the SPI write parameters for DMA Tx
 * @param  header_data : pointer to header data if applicable
 *         payload_data : pointer to payload data if applicable
 *         header_size : header size
 *         payload_size : payload size
 * @retval None
 */
void WiFi_SPI_Write(uint8_t* header_data, uint8_t* payload_data, uint8_t header_size, uint8_t payload_size)
{  
  tx_header_data = header_data;
  tx_payload_data = payload_data;
  tx_header_size = header_size;
  tx_payload_size = payload_size;
  
  packet_cont = WIFI_FALSE;
  
  Disable_SPI_Receiving_Path();
  
  return;
}

/**
* @brief  Process_Packet
*         Process the SPI Packet
* @param  KIND_OF_PACKET_t: kind of packet
          wind_no: The wind number of the packet
* @retval None
*/
void Process_Packet(KIND_OF_PACKET_t kop, WiFi_Indication_t wind_no)
{ 
  static uint8_t index, chan_value, error_state;
  char * process_buffer_ptr = (char*)dataBuff; //stores the starting address of the payload.
  int rssi_value = 0;
  char SocketId_No[2];
  char databytes_No[4];
  char * pStr, *token;
  //volatile uint32_t localloop;
  static uint8_t sock_read_header_count=0;
  
  reset_event(&wifi_instances.wifi_event);
  
  if( !WiFi_Control_Variables.enable_receive_http_response /*&& !IO_status_flag.sock_read_ongoing*/)
      {
        switch(kop)
        {
        case SPI_WIND_PACKET:
        case SPI_WIND_PAYLOAD_PACKET:
            
          //if(kind_of_event==SPI_WIND_NOTIFICATION)
          {
              __disable_irq();
              Process_Wind_Payload(wind_no);
              __enable_irq();
              
              if(!WiFi_Control_Variables.prevent_push_WIFI_event)
                {
                    __disable_irq();
                    push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                     __enable_irq();
                    reset_event(&wifi_instances.wifi_event);
                }                  
              else if(!WiFi_Control_Variables.queue_wifi_wind_message)  //if we do not want to queue a WIND: message
                {
                    reset_event(&wifi_instances.wifi_event);
                    WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;
                    WiFi_Control_Variables.queue_wifi_wind_message = WIFI_TRUE;
                }
              
              if (!WiFi_Control_Variables.do_not_reset_push_WIFI_event) 
                WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_FALSE;

              if(WiFi_Control_Variables.enable_sock_read)
                WiFi_Control_Variables.Q_Contains_Message = WIFI_FALSE;
              else
                return;
          }
          break;
            
        case SPI_OK_PACKET:
          
          //Push a simple OK Event, if this is an OK event required to be pushed to Q
          if(IO_status_flag.prevent_push_OK_event)
              {       
                  //printf("\r\nOK not pushed!!!!!->>>");
                  //This OK is not to be handled, hence the pop action on OK completion to be done here                              
                  if(IO_status_flag.client_socket_close_ongoing) //OK received is of the sock close command
                    {
                        if(WiFi_Counter_Variables.no_of_open_client_sockets > 0)
                          WiFi_Counter_Variables.no_of_open_client_sockets--;
                        IO_status_flag.prevent_push_OK_event                         = WIFI_FALSE;
                        open_sockets[WiFi_Counter_Variables.remote_socket_closed_id] = WIFI_FALSE;
                        //socket ID for which OK is received.
                        WiFi_Counter_Variables.closed_socket_id                      = WiFi_Counter_Variables.remote_socket_closed_id;
                        IO_status_flag.client_socket_close_ongoing                   = WIFI_FALSE;

                        //User Callback not required in case if sock_close is called by User (Only in WIND:58)
                        if(WiFi_Control_Variables.enable_SockON_Server_Closed_Callback)
                          {
                            // User callback after successful socket Close
                            WiFi_Control_Variables.SockON_Server_Closed_Callback         = WIFI_TRUE;
                          }
                        else
                            WiFi_Control_Variables.enable_SockON_Server_Closed_Callback  = WIFI_TRUE;
                        WiFi_Control_Variables.stop_event_dequeue                        = WIFI_FALSE;
                    }
              }
          else
              {
                  //wifi_instances.wifi_event.ok_eval = WIFI_TRUE;
                  wifi_instances.wifi_event.event = WIFI_OK_EVENT;
                  __disable_irq();
                  push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                   __enable_irq();
                  reset_event(&wifi_instances.wifi_event);
              }
          if(sock_read_header_count!=0)
              {
                sock_read_header_count=0;
              }
          IO_status_flag.prevent_push_OK_event = WIFI_FALSE;
          
          break;
          
        case SPI_DATA_PACKET:
        case SPI_DATA_PAYLOAD_PACKET:
              /*We have a payload with some data in it either from AT cmd reply or a Data packet (NOT FROM A WIND)*/          
              if(SPI_Tx_Transfer)/*This is True while AT Cmd is ongoing*/
                    {
                      switch(IO_status_flag.AT_event_processing)
                      {
                      case WIFI_NO_EVENT:                        
                        
#ifdef SPI_VCOM
                        //Just copy the data into SPI VCOM buffer  
                        memcpy(SPI_VCOM_Buff+WiFi_Counter_Variables.sock_total_count, dataBuff, payload_read_len);
                        WiFi_Counter_Variables.sock_total_count = WiFi_Counter_Variables.sock_total_count + payload_read_len;
#endif         
                        break;
                      case WIFI_GPIO_EVENT:
                        // AT+S.GPIOR
                        token = strtok(process_buffer_ptr, ":");
                        //just use the 2nd and 3rd token
                        token = strtok(NULL, ":");
                        
                        WiFi_Counter_Variables.gpio_value = *token - '0';
                        token = strtok(NULL, ":");
                        WiFi_Counter_Variables.gpio_dir= *token - '0';    //out
                        
                        //No need to push event
                        break;
                        
                      case WIFI_GCFG_EVENT:
                        /*Now Check to which AT Cmd response this OK belongs to so that correct parsing can be done*/
                        if((pStr = (char *)(strstr((const char *)process_buffer_ptr,"="))) != NULL)
                           {
                              // AT command GCFG                    
                              //we need to copy only the value in get_cfg_value variable
                              memcpy(WiFi_Counter_Variables.get_cfg_value, pStr+1, (strlen(pStr)));
                              //No need to queue this since OK is following this which will be queued
                           }
                        break;
                      
                      case WIFI_SCAN_EVENT:
                        
                        if(scan_ignore) return;
                        if(sock_read_header_count==0)
                        {
                          //do nothing with this header, what is the use of this header?
                          sock_read_header_count++;
                          return;
                        }
                        
                        pStr = (char *) strstr((const char *)&dataBuff,"CHAN:");            
                        if(pStr != NULL)
                          {
                              databytes_No[0] = *(pStr + 6) ;
                              databytes_No[1] = *(pStr + 7) ;
                    
                              chan_value = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }
                        wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].channel_num = chan_value;
                        
                        pStr = (char *) strstr((const char *)&dataBuff,"RSSI:");            
                        if(pStr != NULL)
                          {
                              databytes_No[0] = *(pStr + 7) ;
                              databytes_No[1] = *(pStr + 8) ;
                    
                              rssi_value = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }

                        wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].rssi = -(rssi_value);

                        pStr = (char *) strstr((const char *)&dataBuff,"SSID:");
                        if(pStr != NULL)
                          {
                              index = 7;
                              while(*(pStr + index) != 0x27)
                                  {
                                      wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].ssid[index-7] = *(pStr + index);
                                      index++;
                                      if(index==35) break; //max ssid lenght is 30 characters
                                  }                                
                          }

                        pStr = (char *) strstr((const char *)&dataBuff,"WPA ");            
                        if(pStr != NULL)
                          {
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa = WIFI_TRUE;
                          } else
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa = WIFI_FALSE;
                      
                        pStr = (char *) strstr((const char *)&dataBuff,"WPA2 ");            
                        if(pStr != NULL)
                          {
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa2 = WIFI_TRUE;
                          } else
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wpa2 = WIFI_FALSE;
                      
                        pStr = (char *) strstr((const char *)&dataBuff,"WPS ");            
                        if(pStr != NULL)
                          {
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wps = WIFI_TRUE;
                          } else
                              wifi_scanned_list[WiFi_Counter_Variables.scanned_ssids].sec_type.wps = WIFI_FALSE;

                        WiFi_Counter_Variables.scanned_ssids++;//increment total_networks
                        if(WiFi_Counter_Variables.scanned_ssids==15) scan_ignore = 1;
                      
                        break;
                        
                      case WIFI_HTTP_EVENT:                                            
                        
                        //sock_total_count used as http_total_count
                        WiFi_Counter_Variables.sock_total_count = WiFi_Counter_Variables.sock_total_count + payload_read_len;
                        IO_status_flag.sock_read_ongoing = WIFI_TRUE;
                        
                        //Check total length incoming and if greater than 512, callback user with the data
                        if(WiFi_Counter_Variables.sock_total_count >= http_packet_len)
                        {   
                            WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                            IO_status_flag.sock_read_ongoing = WIFI_FALSE;      
                            
                            WiFi_Counter_Variables.UserDataBuff_index = payload_read_len;//total incoming bytes
                            //WiFi_Counter_Variables.chunk_size = payload_read_len;
                            memcpy(UserDataBuff, dataBuff, payload_read_len);
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                            packet_payload_data_available = WIFI_FALSE;
                            spi_callback_in_progress = WIFI_TRUE;                            
                            
                            http_packet_len = 0;
                            WiFi_Counter_Variables.sock_total_count = 0;
                            
                            WiFi_Counter_Variables.last_process_buffer_index = 5;
                            WiFi_Control_Variables.HTTP_Data_available=WIFI_TRUE;
                            
                        } else
                        {
                            WiFi_Counter_Variables.UserDataBuff_index = payload_read_len;//total incoming bytes
                            memcpy(UserDataBuff, dataBuff, payload_read_len);
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                            packet_payload_data_available = WIFI_FALSE;
                            spi_callback_in_progress = WIFI_TRUE;
                            
                            WiFi_Counter_Variables.last_process_buffer_index = 5;                            
                            WiFi_Control_Variables.HTTP_Data_available=WIFI_TRUE;
                        }
                        break;
                        
                      case WIFI_CLIENT_SOCKET_OPEN_EVENT:
                        // SOCKON ID (Open a client socket)
                        //Reply to SOCKON for SPWF04 -> AT-S.On:<IP Address>:0
                        
                        //for secure socket first header is: AT-S.LOADING:1:2\r\n
                        if(WiFi_Control_Variables.is_secure_socket && sock_read_header_count==0)//if this is a secure socket
                        {
                          //do nothing with this header, what is the use of this header?
                          IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_OPEN_EVENT;
                          WiFi_Control_Variables.is_secure_socket = WIFI_FALSE;
                          sock_read_header_count++;
                          return;
                        }
                        else
                        {
                          //second header with AT-S.On:<IP Address>:0
                          sock_read_header_count=0;
                        }
                        #if DEBUG_PRINT
                        printf("\r\n Socket ID recv \r\n");
                        #endif
                        token = strtok(process_buffer_ptr, ":");
                        //just use the second token
                        token = strtok(NULL, ":");
                        
                        SocketId_No[0]    = *token ;
                        
                        wifi_instances.wifi_event.socket_id = (SocketId_No[0] - '0');
                        wifi_instances.wifi_event.event     =  WIFI_SOCK_ID_EVENT;
                        __disable_irq();
                        push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                         __enable_irq();
                        reset_event(&wifi_instances.wifi_event);
                        
                        break;
                        
                      case WIFI_SERVER_SOCKET_OPEN_EVENT:
                        
                        //<socket_id>->just the number arrives             
                        SocketId_No[0]    = *(process_buffer_ptr + 0);
                        
                        wifi_instances.wifi_event.socket_id = (SocketId_No[0] - '0');
                        wifi_instances.wifi_event.event     =  WIFI_SOCK_SERVER_ID_EVENT;
                        __disable_irq();
                        push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                         __enable_irq();
                        reset_event(&wifi_instances.wifi_event);
                        break;
                        
                      case WIFI_CLIENT_SOCKET_QUERY_EVENT:
                        // DATALEN from SOCKQ, just the length
                        //Find the DataLength and do a socket read
                        databytes_No[0] = *(process_buffer_ptr + 0);
                        databytes_No[1] = *(process_buffer_ptr + 1);
                        databytes_No[2] = *(process_buffer_ptr + 2);
                        databytes_No[3] = *(process_buffer_ptr + 3);

                        if( databytes_No[1] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                          }
                        else if( databytes_No[2] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }
                        else if( databytes_No[3] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                          }
                        else //it's a 4-digit number
                          {
                            WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                          }
                        
                        if(WiFi_Counter_Variables.Socket_Data_Length != 0)
                          {
                            WiFi_Control_Variables.afterOK_start_sock_read = WIFI_TRUE;
                          }
                        else if(WiFi_Counter_Variables.Socket_Data_Length == 0)  //no data remaining to be read
                          {
                            if(WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.sockon_query_id])
                              {
                                // Q socket_close event for that socket for which sock_close command could not be processed earlier due to ERROR: pending data.
                                 if(open_sockets[WiFi_Counter_Variables.sockon_query_id])
                                  {
                                    Queue_Client_Close_Event(WiFi_Counter_Variables.sockon_query_id);
                                  }
                                WiFi_Counter_Variables.socket_close_pending[WiFi_Counter_Variables.sockon_query_id] = WIFI_FALSE;
                              }
                            WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;  //continue popping events if nothing to read
                          }
                        IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                        break;
                        
                      case WIFI_SERVER_SOCKET_QUERY_EVENT:
                        
                        //Find the DataLength and do a server socket read
                        databytes_No[0] = *(process_buffer_ptr + 0);
                        databytes_No[1] = *(process_buffer_ptr + 1);
                        databytes_No[2] = *(process_buffer_ptr + 2);
                        databytes_No[3] = *(process_buffer_ptr + 3);

                        if( databytes_No[1] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                          }
                        else if( databytes_No[2] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }
                        else if( databytes_No[3] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                          }
                        else //it's a 4-digit number
                          {
                            WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                          }
                        
                        if(WiFi_Counter_Variables.Socket_Data_Length != 0)
                          {
                            WiFi_Control_Variables.afterOK_start_sockd_read = WIFI_TRUE;
                            //WiFi_Control_Variables.start_sockd_read = WIFI_TRUE;
                            WiFi_Control_Variables.data_pending_sockD = WIFI_TRUE;//to make sure end of data is treated correctly
                          }
                        else if(WiFi_Counter_Variables.Socket_Data_Length == 0)  //no data remaining to be read
                          {
                            WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;  //continue popping events if nothing to read
                          }
                              
                        break;
                        
                      case WIFI_CLIENT_SOCKET_READ_EVENT:
                        //If this is first header on a sock read
                        if(sock_read_header_count==0)
                        {
                          //do nothing with this header, what is the use of this header?
                          IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_READ_EVENT;
                          sock_read_header_count++;
                          return;
                        }
                        else
                        {
                          //second header with <pending bytes>:<cumulated pending bytes>
                          IO_status_flag.AT_event_processing = WIFI_CLIENT_SOCKET_READ_DATA;
                          sock_read_header_count=0;
                        }
                        
                        token = strtok(process_buffer_ptr, ":");
                        //just use the second token
                        token = strtok(NULL, ":");
                        
                        //This is the Data Header from a Client SOCK Read -> the payload has the length
                        //New FW has structure e.g."7:7\r\n"->ends in \r\n and not NULL
                        databytes_No[0] = *(token + 0);
                        databytes_No[1] = *(token + 1);
                        databytes_No[2] = *(token + 2);
                        databytes_No[3] = *(token + 3);

                        if( databytes_No[1] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                          }
                        else if( databytes_No[2] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }
                        else if( databytes_No[3] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                          }
                        else //it's a 4-digit number
                          {
                            WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                          }
                        
                        break;
                                  
                      case WIFI_CLIENT_SOCKET_READ_DATA:
                        //This is the Raw Header from a Client SOCK Read
                        WiFi_Counter_Variables.sock_total_count += payload_read_len;//payload_len is the bytes read so far                                
                        
                        if(WiFi_Counter_Variables.sock_total_count >= WiFi_Counter_Variables.Socket_Data_Length)
                        {                        
                            //printf("\r\nReached SockON len");
                          
                            /*@TODO: Do not need to prevent OK push in case of server socket*/
                            IO_status_flag.prevent_push_OK_event = WIFI_TRUE; //prevent the qeueuing of the OK after this read operation
                            IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                            
                            WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                            IO_status_flag.sock_read_ongoing = WIFI_FALSE;      
                            
                            WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.Socket_Data_Length;//total incoming bytes
                            WiFi_Counter_Variables.chunk_size = payload_read_len;
                            memcpy(UserDataBuff, dataBuff, payload_read_len);
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                            packet_payload_data_available = WIFI_FALSE;
                            spi_callback_in_progress = WIFI_TRUE;
                            
                            //do we have more data?
                            WiFi_Control_Variables.afterOK_enable_query = WIFI_TRUE;
                            
                            /*This is a workaround for "NO OK received after last payload"*/
                            Reset_AT_CMD_Buffer();  
                            sprintf((char*)WiFi_AT_Cmd_Buff,"AT");  //Send a Null Command
                            run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                            /*This is a workaround for "NO OK received after last payload"*/
                            
                            WiFi_Counter_Variables.sock_total_count = 0;
                            WiFi_Counter_Variables.Socket_Data_Length = 0;
                            WiFi_Counter_Variables.SockON_Data_Length = 0;
                            
                            WiFi_Counter_Variables.last_process_buffer_index = 5;
                            
                            if(WiFi_Control_Variables.data_pending_sockD)
                              {
                                WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE; //callback to user with User Buffer pointer, in case of server socket
                              }
                            else
                              {
                                WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE; //set this to callback to user with User Buffer pointer
                              }
                            
                        } else
                          {
                              //printf("\r\nhere i am ");
                              WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.Socket_Data_Length;//total incoming bytes
                              WiFi_Counter_Variables.chunk_size = payload_read_len;
                              memcpy(UserDataBuff, dataBuff, payload_read_len);
                              WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                              packet_payload_data_available = WIFI_FALSE;
                              spi_callback_in_progress = WIFI_TRUE;
                              
                              WiFi_Counter_Variables.last_process_buffer_index = 5;
                              
                              if(WiFi_Control_Variables.data_pending_sockD)
                                {
                                  WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE; //callback to user with User Buffer pointer, in case of server socket
                                }
                              else
                                {
                                  WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE; //set this to callback to user with User Buffer pointer
                                }
                          }
                        
                        break;
                        
                      case WIFI_SERVER_SOCKET_READ_EVENT:      
                        //If this is first header on a sock read
                        if(sock_read_header_count==0)
                        {
                          //do nothing with this header, what is the use of this header?
                          IO_status_flag.AT_event_processing = WIFI_SERVER_SOCKET_READ_EVENT;
                          sock_read_header_count++;
                          return;
                        }
                        else
                        {
                          //second header with <pending bytes>:<cumulated pending bytes>
                          IO_status_flag.AT_event_processing = WIFI_SERVER_SOCKET_READ_DATA;
                          sock_read_header_count=0;
                        }
                        
                        token = strtok(process_buffer_ptr, ":");
                        //just use the second token
                        token = strtok(NULL, ":");
                        
                        //This is the Data Header from a Server SOCK Read -> the payload has the length
                        //New FW has structure e.g."7:7\r\n"->ends in \r\n and not NULL
                        databytes_No[0] = *(token + 0);
                        databytes_No[1] = *(token + 1);
                        databytes_No[2] = *(token + 2);
                        databytes_No[3] = *(token + 3);

                        if( databytes_No[1] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = databytes_No[0] - '0'; 
                          }
                        else if( databytes_No[2] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 10 ) + (databytes_No[1] - '0'));
                          }
                        else if( databytes_No[3] == 0x0D)
                          {
                            WiFi_Counter_Variables.Socket_Data_Length = (((databytes_No[0] - '0') * 100 ) + ((databytes_No[1] - '0') * 10 ) + (databytes_No[2] - '0'));
                          }
                        else //it's a 4-digit number
                          {
                            WiFi_Counter_Variables.Socket_Data_Length  = ((databytes_No[0] - '0') * 1000 ) + ((databytes_No[1] - '0') * 100 ) + ((databytes_No[2] - '0') * 10) + (databytes_No[3] - '0');
                          }
                        
                        break;
                        
                      case WIFI_SERVER_SOCKET_READ_DATA:
                          //This is the Raw Header from a Client SOCK Read
                          WiFi_Counter_Variables.sock_total_count = WiFi_Counter_Variables.sock_total_count + payload_read_len;//payload_len is the bytes read so far
                          
                          if(WiFi_Counter_Variables.sock_total_count >= WiFi_Counter_Variables.Socket_Data_Length)
                          { 
                              //printf("\r\nReached data len");
                              WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.Socket_Data_Length;//total incoming bytes
                              WiFi_Counter_Variables.chunk_size = payload_read_len;
                              memcpy(UserDataBuff, dataBuff, payload_read_len);
                              WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                              packet_payload_data_available = WIFI_FALSE;
                              spi_callback_in_progress = WIFI_TRUE;
                              
                              if(WiFi_Control_Variables.data_pending_sockD)
                                      {                    
                                        //do we have more data?
                                        WiFi_Control_Variables.afterOK_enable_server_query = WIFI_TRUE;//enable query after OK arrives
                                        //WiFi_Control_Variables.enable_server_query = WIFI_TRUE;
                                        WiFi_Control_Variables.enable_query = WIFI_FALSE;                                        
                                        WiFi_Counter_Variables.sockdon_id_user = WiFi_Counter_Variables.sockdon_query_id;
                                        IO_status_flag.prevent_push_OK_event = WIFI_TRUE; //prevent the qeueuing of the OK after this read operation
                                      }
                              
                              IO_status_flag.AT_event_processing = WIFI_NO_EVENT;                              
                              WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                              IO_status_flag.sock_read_ongoing = WIFI_FALSE;  
                              
                              /*This is a workaround for "NO OK received after last payload"*/
                              Reset_AT_CMD_Buffer();  
                              sprintf((char*)WiFi_AT_Cmd_Buff,"AT");  //Send a Null Command
                              run_spi_cmd((char*)WiFi_AT_Cmd_Buff, SPI_POLL);
                              /*This is a workaround for "NO OK received after last payload"*/
                            
                              WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
                              WiFi_Counter_Variables.sock_total_count = 0;
                              WiFi_Counter_Variables.Socket_Data_Length = 0;
                              WiFi_Counter_Variables.SockON_Data_Length = 0;                              
                              WiFi_Counter_Variables.last_process_buffer_index = 5;
                              
                              if(WiFi_Control_Variables.data_pending_sockD)
                                {
                                  WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE; //callback to user with User Buffer pointer, in case of server socket
                                }
                              else
                                {
                                  WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE; //set this to callback to user with User Buffer pointer
                                }
                              
                              WiFi_Control_Variables.data_pending_sockD = WIFI_FALSE;
                          }else
                          {
                              WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
                              WiFi_Counter_Variables.sockdon_id_user = WiFi_Counter_Variables.sockdon_query_id;
                              WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.Socket_Data_Length;//total incoming bytes
                              WiFi_Counter_Variables.chunk_size = payload_read_len;
                              memcpy(UserDataBuff, dataBuff, payload_read_len);
                              WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                              packet_payload_data_available = WIFI_FALSE;
                              spi_callback_in_progress = WIFI_TRUE;
                              
                              WiFi_Counter_Variables.last_process_buffer_index = 5;
                              
                              if(WiFi_Control_Variables.data_pending_sockD)
                                {
                                  WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE; //callback to user with User Buffer pointer, in case of server socket
                                }
                              else
                                {
                                  WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE; //set this to callback to user with User Buffer pointer
                                }
                          }
                          break;
                      
                      case WIFI_FW_UPDATE_EVENT:
                          
                          //Just print something to indicate progress of data download!
                          printf("*");
                          //printf(dataBuff);
                          fflush(stdout);
                          
                        break;
                        
                      default:
                        break;
                      }
                    }
          break;                  
          
        case SPI_DATA_INTERIM_PAYLOAD_PACKET:
                if(SPI_Tx_Transfer)/*This is True while AT Cmd is ongoing*/
                    {
                      switch(IO_status_flag.AT_event_processing)
                        {
                        case WIFI_HTTP_EVENT:
                          WiFi_Counter_Variables.sock_total_count += payload_read_len;
                          //now make the user callback call
                          WiFi_Counter_Variables.UserDataBuff_index = payload_read_len;
                          memcpy(UserDataBuff, dataBuff, payload_read_len);
                          WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                          packet_payload_data_available = WIFI_TRUE;
                          
                          WiFi_Control_Variables.HTTP_Data_available=WIFI_TRUE;
                          
                          break;
                          
                          case WIFI_SERVER_SOCKET_READ_DATA:
                          case WIFI_CLIENT_SOCKET_READ_DATA:
                            WiFi_Counter_Variables.sock_total_count += payload_read_len;
                        
                            //callback user with data
                            if(WiFi_Control_Variables.data_pending_sockD)
                                {                                  
                                  WiFi_Counter_Variables.sockdon_id_user = WiFi_Counter_Variables.sockdon_query_id;                                        
                                }
                            
                            WiFi_Counter_Variables.sockon_id_user = WiFi_Counter_Variables.sockon_query_id;
                            WiFi_Counter_Variables.message_size = WiFi_Counter_Variables.Socket_Data_Length;//total incoming bytes
                            WiFi_Counter_Variables.chunk_size = MAX_BUFFER_GLOBAL;
                            memcpy(UserDataBuff, dataBuff, payload_read_len);
                            WiFi_Control_Variables.stop_event_dequeue = WIFI_TRUE;             //Stop any more event de-queue
                            packet_payload_data_available = WIFI_TRUE;
                            if(WiFi_Control_Variables.data_pending_sockD)
                              {
                                WiFi_Control_Variables.Pending_SockD_Callback = WIFI_TRUE; //callback to user with User Buffer pointer, in case of server socket
                              }
                            else
                              {
                                WiFi_Control_Variables.Pending_SockON_Callback = WIFI_TRUE; //set this to callback to user with User Buffer pointer
                              }
                
                            break;     
                            
                        default:
                          break;
                        }
                    }
          break;
          
        case SPI_ERROR_PACKET:
          error_state = Received_Header[2];  

#if DEBUG_PRINT
          if(error_state > NUM_ERROR_STATE) 
            printf("\r\nError: Unknown Error, no. 0x%x!\r\n", error_state);
          else
            printf("\r\nError: %s\r\n", err_list[error_state]);
#endif         
#ifdef SPI_VCOM
          printf("\r\nError: %s\r\n", err_list[error_state]);
#endif
            switch(error_state)
            {
            case MISSING_ARGS:
            case VAR_NOT_FOUND:
            case TOO_MANY_ARGS:
            case INVALID_ARGS:
            case REQUEST:/*FAILURE*/
              if(SPI_Tx_Transfer)/*This is True while some User / AT Cmd is ongoing*/
              {                  
                  wifi_instances.wifi_event.event = WIFI_ERROR_EVENT;
                  __disable_irq();
                  push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                   __enable_irq();
                  reset_event(&wifi_instances.wifi_event);
                  if(WiFi_Control_Variables.stop_event_dequeue)
                    /*ERROR:Illegal Socket ID*/
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;//continue popping events if nothing to read
                  if(WiFi_Control_Variables.enable_timeout_timer)
                    {
                      WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                      WiFi_Counter_Variables.timeout_tick = 0;
                    }
                  SPI_Tx_Transfer = WIFI_FALSE;
              }
              break;
            case OPEN_SOCK:
            default:
                  wifi_instances.wifi_event.event = WIFI_ERROR_EVENT;
                  __disable_irq();
                  push_eventbuffer_queue(&wifi_instances.event_buff, wifi_instances.wifi_event);
                   __enable_irq();
                  reset_event(&wifi_instances.wifi_event);
                  if(WiFi_Control_Variables.stop_event_dequeue)
                    /*ERROR:Illegal Socket ID*/
                    WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;//continue popping events if nothing to read
                  if(SPI_Tx_Transfer && WiFi_Control_Variables.enable_sock_read != WIFI_TRUE)/*This is True while some User / AT Cmd is ongoing*/
                  {
                    SPI_Tx_Transfer = WIFI_FALSE;
                  }
                  if (WiFi_Control_Variables.enable_sock_read == WIFI_TRUE)
                  {       
                      WiFi_Control_Variables.enable_sock_read = WIFI_FALSE;
                  }
                  if(WiFi_Control_Variables.enable_timeout_timer)
                    {
                      WiFi_Control_Variables.enable_timeout_timer = WIFI_FALSE;
                      WiFi_Counter_Variables.timeout_tick = 0;
                    }
                  if(WiFi_Control_Variables.http_req_pending == WIFI_TRUE)
                      {
                        WiFi_Control_Variables.http_req_pending = WIFI_FALSE;
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                      }
                  
              break;
            }
            break;
            
        case SPI_IGNORE_PACKET:
            /*Do nothing with this packet*/
            break;
        default:
            break;
        }       
      }
}

/**
* @brief  Process_Wind_Payload
*         Process the WIND packet and prepare the event to be pushed to event Q as applicable
* @param  wind_no: The wind number of the packet
* @retval None
*/
void Process_Wind_Payload(WiFi_Indication_t wind_no)
{
  char * process_buffer_ptr = (char*)dataBuff; //stores the starting address of the payload.
  char * ptr_offset = (char*)0; 
  char databytes_No[4]; 
  char *token;
  
#if DEBUG_PRINT
  //printf(" \r\n");
  //printf((const char*)dataBuff);
  //fflush(stdout);
#endif
  int i;

  WiFi_Indication_t Wind_No = (WiFi_Indication_t)wind_no;
  wifi_instances.wifi_event.wind = Wind_No;
  wifi_instances.wifi_event.event = WIFI_WIND_EVENT;

  switch (Wind_No)
  {
      case SockON_Data_Pending: /*WIND:55*/
//          if(wind_55_in_Q)
//          {
//            printf("\r\n55 prvented!");
//          WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
//          break;// no need to push the second WIND55 in Q
//          }
          
          //wind_55_in_Q = WIFI_TRUE;//commenting this so that all WIND:55's are queued
          
          /*+WIND:55:Pending Data:<ServerID>:<ClientID>:<received_bytes>:<cumulated_bytes>*/             
          if(*(process_buffer_ptr + 0) == ':')//means it is a client socket WIND:55
          {
            /*Need to find out which socket ID has data pending*/
            databytes_No[0] = *(process_buffer_ptr + 1);

            wifi_instances.wifi_event.socket_id = (databytes_No[0] - '0'); //Max number of sockets is 8
            wifi_instances.wifi_event.server_id = 9;
          }
          else //it is a server socket ID
          {
            /*Need to find out which server socket ID has data pending*/
            databytes_No[0] = *(process_buffer_ptr + 0);//Server Socket ID
            databytes_No[1] = *(process_buffer_ptr + 2);//Client ID Connected to this socket
            wifi_instances.wifi_event.server_id = (databytes_No[0] - '0'); //Max number of server sockets is 3
            wifi_instances.wifi_event.socket_id = (databytes_No[1] - '0');//Each server socket can server upto 5 clients
          }
          break;

      case SockON_Server_Socket_Closed:
          /*<Client_ID>:<Closure_Reason>*/
          //Find the id of the socket closed          
          token = strtok(process_buffer_ptr, ":");//just use the 1st token
          databytes_No[0] = *token ;
          token = strtok(NULL, ":");
          databytes_No[1] = *token ;
          wifi_instances.wifi_event.socket_id = databytes_No[0] - '0'; //Max number of sockets is 8 (so single digit)
          //@TBD: CHECK!!!!
          //wifi_instances.wifi_event.closure_reason = databytes_No[1] - '0';
          break;                

      case WiFi__MiniAP_Associated:
          //Find out which client joined by parsing the WIND //+WIND:28
          ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"+WIND:28");
          for(i=17;i<=33;i++)
          WiFi_Counter_Variables.client_MAC_address[i-17] = *(ptr_offset + i) ;    
          IO_status_flag.WiFi_WIND_State = WiFiAPClientJoined;
          break;

      case WiFi_MiniAP_Disassociated:
          //Find out which client left by parsing the WIND //+WIND:72
          ptr_offset = (char *) strstr((const char *)process_buffer_ptr,"+WIND:72");
          for(i=17;i<=33;i++)
          WiFi_Counter_Variables.client_MAC_address[i-17] = *(ptr_offset + i) ;
          IO_status_flag.WiFi_WIND_State = WiFiAPClientLeft;
          break;
      case Incoming_socket_client:
          //<IP-Address>:<HeapSize?>:<ServerID?>:<ClientID?>
          token = strtok(process_buffer_ptr, ":");
          token = strtok(NULL, ":");
          token = strtok(NULL, ":");
          databytes_No[0] = *token ;
          token = strtok(NULL, ":");
          databytes_No[1] = *token ;
          wifi_instances.wifi_event.server_id = (databytes_No[0] - '0'); //Server ID. Max number of server sockets is 3
          wifi_instances.wifi_event.socket_id = (databytes_No[1] - '0');//This is the client ID connected to above server ID. Max 5 connections.
          //@TBD: Map IP Address to client number
          break;
      case Outgoing_socket_client:
          //Find out which client left on which server ID.
        //<Client-IP-Address>:<HeapSize?>:<ServerID?>:<ClientID?>:<Closure Reason>
          token = strtok(process_buffer_ptr, ":");
          token = strtok(NULL, ":");
          token = strtok(NULL, ":");
          databytes_No[0] = *token ;
          token = strtok(NULL, ":");
          databytes_No[1] = *token ;
          wifi_instances.wifi_event.server_id = (databytes_No[0] - '0'); //Server ID on which client ID was connected 
          wifi_instances.wifi_event.socket_id = (databytes_No[1] - '0');//This is the client ID which disconnected          
          break;
    
      case Remote_Configuration:
          wifi_connected = 0;
          WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
          WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
          break;
          
      case Going_Into_Standby:
          wifi_instances.wifi_event.event = WIFI_STANDBY_CONFIG_EVENT;
          break;
          
      case Resuming_From_Standby:
          wifi_instances.wifi_event.event = WIFI_RESUME_CONFIG_EVENT;
          break;
#if defined(SPWF04)
      case Configuration_Failure:  
          //Configuration Error Reason (1 - 10)
          databytes_No[0] = *(process_buffer_ptr + 0);
          databytes_No[1] = *(process_buffer_ptr + 1);//Max two digits
          
          //@TBD:What to do with this number now?
          break;
#endif          
      case NTP_Server_Delivery:
          //<Year>.<Month>.<Date>:<DayOfWeek>:<Hour>.<min>.<sec>
          token = strtok(process_buffer_ptr, ":");
          //@TBD:Keep the time? or set the time?
          break;
          
      // Queueing of following events not required.
      case Console_Active:           
      case Poweron:
      case WiFi_Reset:
      case Watchdog_Running:
      case Watchdog_Terminating:
      case CopyrightInfo:
      case WiFi_BSS_Regained:
      case WiFi_Signal_OK:
      case FW_update:
      case Encryption_key_Not_Recognized:
      case WiFi_Join:
      case WiFi_Scanning:
      case WiFi_Association_Successful:
      case WiFi_BSS_LOST:
      case WiFi_NETWORK_LOST:
      case WiFi_Unhandled_Event:
      case WiFi_UNHANDLED:
      case WiFi_MiniAP_Mode :
      case DOT11_AUTHILLEGAL:
      case Creating_PSK:      
      case WPA_Supplicant_Failed:
      case WPA_Handshake_Complete:
      case GPIO_line:
      case Wakeup:
      case Factory_debug:
      case Low_Power_Mode_Enabled:
      case Rejected_Found_Network:
          WiFi_Control_Variables.queue_wifi_wind_message = WIFI_FALSE;
          WiFi_Control_Variables.prevent_push_WIFI_event = WIFI_TRUE;
          break;

        //Queue these Events.
      case MallocFailed:                                          
          if(WiFi_Control_Variables.stop_event_dequeue)
            WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
          break;
      
      case WiFi_Up:
      case WPA_Terminated:
      case Heap_Too_Small:            
      case WiFi_Hardware_Dead:
      case Hard_Fault:
      case StackOverflow:                   
      case Error:
      case WiFi_PS_Mode_Failure:
      case WiFi_Signal_LOW:
      case JOINFAILED:
      case SCANBLEWUP:
      case SCANFAILED:
      case WiFi_Started_MiniAP_Mode:
      case Start_Failed :
      case WiFi_EXCEPTION :
      case WiFi_Hardware_Started :
      case Scan_Complete:
      case WiFi_UNHANDLED_IND:
      case WiFi_Powered_Down:
      case WiFi_Deauthentication:
      case WiFi_Disassociation:
      case RX_MGMT:
      case RX_DATA:
      case RX_UNK:
           break;
      default:
           break;
    }
  memset(process_buffer_ptr, 0x00, MAX_BUFFER_GLOBAL);

}

#endif

/*
E.g.
AT command: AT+S.SCAN=d,/scan.txt
1)	Command ID: 0x33
2)	Number of parameters: 0x02
First Parameter: 0x01,S
Second Parameter: 0x09,(/scan.txt)
3)	Full message payload length: 0x0D
Result. SPI Message: 0x02 0x00 0x0D 0x33 0x02 0x01 S 0x09 (/scan.txt)

AT+S.SCFG=console_enabled,0
0x02 0x00 0x0D 0x0A 0x02 0x0F (console_enabled) 0x01 0x00
*/
/* SPI DMA Transmit*/
void SPI_Send_AT_Command(int offset, int mode)
{
  tx_payload_data = WiFi_SPI_Packet;  
  
#if DEBUG_PRINT
  printf("\r\n>>AT\r\n");  
#endif
  payload_size_to_transmit = offset;  
  
  switch (mode)
  {
  case SPI_POLL:
    SPI_Transmit_Manager_Poll(tx_payload_data, payload_size_to_transmit);
    break;
  case SPI_DMA:
    SPI_Transmit_Manager(SPI_PAYLOAD_TRANSMIT);
    break;
  case SPI_IT:
    //not supported yet
    break;  
  }
}

/**
 * @brief  Manage the SPI transmit in polling mode
 * @param  None
 * @retval None
 */
void SPI_Transmit_Manager_Poll(uint8_t * Data, uint32_t DataLen)
{  
  //while(HAL_GPIO_ReadPin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN) == GPIO_PIN_RESET);//this doesn't look good while called from an IRQ context
  Disable_SPI_Receiving_Path();
  
  Enable_SPI_CS();
  
  if(HAL_SPI_Transmit(&SpiHandle, Data, DataLen, 1000)== HAL_OK)
  {
      TransmitClosure();
  }
}

/**
 * @brief  Manage the SPI transmit
 * @param  TransmitRequest: the transmit request
 * @retval None
 */
void SPI_Transmit_Manager(SPI_TRANSMIT_REQUEST_t TransmitRequest)
{  
  //while(HAL_GPIO_ReadPin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN) == GPIO_PIN_RESET);
  Disable_SPI_Receiving_Path();
  
  Enable_SPI_CS();//Enable nCS (Chip Select) 
//  for (localloop = 0 ; localloop < CS_PULSE_LENGTH1 ; localloop++)
//  {
//    *(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1;
//  }
  
  /*
   *  Disable both DMA
   */
  __HAL_DMA_DISABLE(SpiHandle.hdmatx);
  __HAL_DMA_DISABLE(SpiHandle.hdmarx);
  
  __HAL_DMA_DISABLE_IT(SpiHandle.hdmarx, DMA_IT_TC); /**< Disable Receive packet notification */
  
  __HAL_DMA_CLEAR_FLAG(SpiHandle.hdmatx, WIFI_SPI_TX_DMA_TC_FLAG); /**< Clear flag in DMA */
	HAL_NVIC_ClearPendingIRQ(WIFI_SPI_DMA_TX_IRQn); /**< Clear DMA pending bit in NVIC */
  __HAL_DMA_ENABLE_IT(SpiHandle.hdmatx, DMA_IT_TC);	/**< Enable Transmit packet notification */
  
  __HAL_WIFI_DMA_SET_MINC(SpiHandle.hdmatx); /**< Configure DMA to send Tx packet */
  
  switch (TransmitRequest)
  {
  case SPI_HEADER_TRANSMIT:
    Spi_Transmit_Event = SPI_HEADER_TRANSMITTED;
    
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmatx, tx_header_size); /**< Set counter in DMA TX */
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmatx, (uint32_t)tx_header_data); /**< Set memory address in DMA TX */
    break;
    
  case SPI_PAYLOAD_TRANSMIT:
    Spi_Transmit_Event = SPI_PAYLOAD_TRANSMITTED;
    
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmatx, payload_size_to_transmit); /**< Set counter in DMA TX */
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmatx, (uint32_t)tx_payload_data); /**< Set memory address in DMA TX */
    break;
  
  case SPI_WRITE_PAYLOAD_TRANSMIT:
    Spi_Transmit_Event = SPI_PAYLOAD_TRANSMITTED;
    
    __HAL_WIFI_DMA_SET_COUNTER(SpiHandle.hdmatx, WiFi_Counter_Variables.curr_DataLength); /**< Set counter in DMA TX */
    __HAL_WIFI_DMA_SET_MEMORY_ADDRESS(SpiHandle.hdmatx, (uint32_t)WiFi_Counter_Variables.curr_data); /**< Set memory address in DMA TX */
    break;
    
  default:
    break;
  }
  
  __HAL_DMA_ENABLE(SpiHandle.hdmatx); /**< Enable DMA TX */
}

/**
 * @brief  Close the transmit mechanism after packet has been sent
 *         Wait for the event to come back
 * @param  None
 * @retval None
 */
void TransmitClosure(void)
{
  Disable_SPI_CS();
  
  /*
   *  Disable both DMA
   */
  __HAL_DMA_DISABLE(SpiHandle.hdmatx);
  __HAL_DMA_DISABLE(SpiHandle.hdmarx);

  Enable_SPI_Receiving_Path();

  if (HAL_GPIO_ReadPin(WIFI_SPI_IRQ_PORT, WIFI_SPI_IRQ_PIN) == GPIO_PIN_RESET)
  {
    __HAL_GPIO_EXTI_GENERATE_SWIT(WIFI_SPI_IRQ_PIN);
  }
  
  SPI_Tx_Transfer = WIFI_TRUE;
  return;
}

/*Code for SPI IRQ*/

#ifdef SPI_IRQ_MODE
void SPI_Receive_Manager(SPI_RECEIVE_REQUEST_t ReceiveRequest)
{
  uint16_t byte_count;
  uint8_t i;
  //uint16_t payload_len;  
  
  switch (ReceiveRequest)
  {
  case SPI_READ_HEADER_FOR_RX:
    HAL_SPI_Receive_IT(&SpiHandle, Received_Header, HEADER_SIZE);
    //ReceiveHeader(SPI_RECEIVED_HEADER_FOR_RX, (uint8_t *)Read_Header_CMD, HEADER_SIZE);
    break;
    
  case SPI_READ_PAYLOAD:

    //byte_count = (Received_Header[4]<<8)|Received_Header[3];
    byte_count = payload_to_read;
    if (byte_count > (sizeof dataBuff))
    {
      byte_count = (sizeof dataBuff);
    }
    //payload_len = byte_count;
    payload_read_len = byte_count;

    Spi_Receive_Event = SPI_RECEIVE_END;/*For Wind Payload*/        
    HAL_SPI_Receive_IT(&SpiHandle, dataBuff, payload_read_len);    

    break;
  
  case SPI_READ_DATA_PAYLOAD:  
    
    //What happens when payload_to_read becomes > sizeof(dataBuff)?
    //we first read sizeof(datBuff) and repeat read till payload_to_read is complete
    byte_count = payload_to_read;
    
    if (byte_count > (sizeof dataBuff))
    {
      byte_count = (sizeof dataBuff);
      payload_to_read -= byte_count; //reduce payload_to_read by amount read(sizeof(dataBuff)) so far
      Spi_Receive_Event = SPI_RECEIVE_INTERIM_PAYLOAD_DATA;/*For Data Payload higher than dataBuff Size*/
    }
    else
    {
      payload_to_read = 0;
      Spi_Receive_Event = SPI_RECEIVE_PAYLOAD_DATA;/*For Data Payload*/
    }
    
    payload_read_len = byte_count;
    
    HAL_SPI_Receive_IT(&SpiHandle, dataBuff, payload_read_len);
    
    break;      

  default:
    break;
  }

  
  return;
}

/*SPI Interrupt handler*/
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  /*Empty Implementation*/
  uint8_t error_state;  
  volatile uint32_t localloop;
  WiFi_Indication_t wind;
  static WiFi_Indication_t last_wind;
  
  #if DEBUG_PRINT
  uint8_t wifi_status;
  #endif
  
  switch (Spi_Receive_Event)
    {
      case SPI_RECEIVED_HEADER_FOR_TX:
      case SPI_RECEIVED_HEADER_FOR_RX:
        kind_of_event = (KIND_OF_EVENT_t)((Received_Header[1]>>4)&0xF);
        wind_no = Received_Header[2];
        wind = (WiFi_Indication_t)wind_no;
        sync = Received_Header[0];
        
        /*For the time being we are filtering out headers wth SYNC word as 0x80 , 0xFF*/
        if(sync != 0x02 && ( (sync != 0x80 && sync != 0xFF) && kind_of_event==SPI_INCOMING_DATA )) 
          kind_of_event = SPI_FRAME_ERROR;        
        
        switch(kind_of_event)
        {                      
          case  SPI_WIND_NOTIFICATION:
            payload_len = (Received_Header[4]<<8)|Received_Header[3];  
            #if DEBUG_PRINT
            wifi_status = (Received_Header[1])&0xF;
            #endif
            
            if(wind_no == 0x07) 
            {
                error_state = (Received_Header[4]<<8)|Received_Header[3];  
                #if DEBUG_PRINT
                printf("\r\n>> Configuration Failure no %d", error_state);
                #endif
            }
            #if DEBUG_PRINT
            printf("\r\n<<WIND %d, status %d\r\n", wind_no, wifi_status);
            #endif
            
            if (payload_len <= 0)
              {
                  /* Go for the next header after processing the WIND*/
                  Disable_SPI_CS();    
                  ProcessEndOfReceive(SPI_WIND_PACKET, wind);
              }
              else
              {
                  last_wind = wind;
                  payload_to_read = payload_len;
                  payload_read_len = 0;
                  SPI_Receive_Manager(SPI_READ_PAYLOAD);	/**< Read the WIND Payload */
              }
            break;
            
          case  SPI_INCOMING_DATA:
            payload_len = (Received_Header[4]<<8)|Received_Header[3];  
            error_state = Received_Header[2];//@TBD:Check if parsing is correct for Error!
            //Handle Data
            //Should be same as above switch case??
            if (payload_len <= 0  && (error_state == 0x00) /*&& SPI_Tx_Transfer_Complete==WIFI_FALSE*/)
              {
                    
//                  printf("\r\n<<AT+S.OK\r\n");                    
                    /* This is an OK event*/
                    Disable_SPI_CS();
                    ProcessEndOfReceive(SPI_OK_PACKET, Invalid_Wind); 
                    switch(IO_status_flag.AT_event_processing)
                    {
                    case WIFI_FW_UPDATE_EVENT:
                      WiFi_Counter_Variables.sock_total_count = 0;
                      break;
                    case WIFI_HTTP_EVENT:
                      if(WiFi_Control_Variables.http_req_pending == WIFI_TRUE)
                      {
                        WiFi_Control_Variables.http_req_pending = WIFI_FALSE;
                        WiFi_Control_Variables.stop_event_dequeue = WIFI_FALSE;
                        IO_status_flag.sock_read_ongoing = WIFI_TRUE;
                        WiFi_Counter_Variables.sock_total_count = 0;
                      }
                      break;
                    case WIFI_CLIENT_SOCKET_WRITE_EVENT:
                      break;
                    default:
                      break;
                    }
                    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
                    SPI_Tx_Transfer = WIFI_FALSE;//OK has arrived, so AT cmd is not ongoing now
              }
            else if (payload_len > 0)
              {
                  //printf("\r\n>At cmd reply with Data!");
                  switch(IO_status_flag.AT_event_processing)
                  {
                  case WIFI_HTTP_EVENT:
                    http_packet_len = payload_len;
                    payload_to_read = payload_len;
                    payload_read_len = 0;
                    break;
                  case WIFI_CLIENT_SOCKET_READ_DATA:
                    //This is read using DMA, not socket.read (using DMA we can red only upto payload_len!
                    //@TBD: Not correct. Cannot read more than payload_len!!!!!!!
                    payload_to_read = payload_len;
                    payload_read_len = 0;
                    break;
                  default:
                    payload_to_read = payload_len;
                    payload_read_len = 0;
                    break;
                  }                  
                  //if(WiFi_Control_Variables.http_req_pending == WIFI_TRUE)
                    //SPI_Receive_Manager(SPI_READ_DATA_HTTP);
                  //else
                    DisableEnable_SPI_CS(); 
                    SPI_Receive_Manager(SPI_READ_DATA_PAYLOAD);	/**< Read the Data Packet Payload */
              }
            else if (payload_len == 0 && error_state!= 0x00)
              {
                    //printf("\r\n>>Error Condition!!\r\n");
                    Disable_SPI_CS(); 
                    ProcessEndOfReceive(SPI_ERROR_PACKET, Invalid_Wind);
              }
            else
              {
                    /* Go for the next header if there is any*/
                    Disable_SPI_CS();   
                    ProcessEndOfReceive(SPI_DATA_PACKET, Invalid_Wind); 
              }
            break;  
            
          case  SPI_ERROR_NOTIFICATION:
            Disable_SPI_CS(); 
            ProcessEndOfReceive(SPI_ERROR_PACKET, Invalid_Wind);
            break;
            
          case  SPI_FRAME_ERROR:
            //printf("Frame dump\r\n");            
            Disable_SPI_CS(); 
            ProcessEndOfReceive(SPI_IGNORE_PACKET, Invalid_Wind);
            break;
            
          default:
            /* Release CS line */
            //printf("\r\n Unknown packet event %d!!\r\n", kind_of_event);
            Disable_SPI_CS();    
            ProcessEndOfReceive(SPI_IGNORE_PACKET, Invalid_Wind);
            break;
        }        
        memset(Received_Header, 0x00, strlen((char const*)Received_Header));
        break;
        
      case SPI_RECEIVE_PAYLOAD_DATA:
        //Data or AT Cmd Payload
        /*If Data to be read is not finished, fetch remaining data*/
        
//        if(WiFi_Control_Variables.http_req_pending == WIFI_TRUE)
//        {
//          Disable_SPI_CS();          
//          ProcessEndOfReceive(SPI_DATA_PAYLOAD_PACKET, Invalid_Wind);          
//          break;
//        }   
          
          /*Next a header should arrive if there is more data*/
          Disable_SPI_CS();
          ProcessEndOfReceive(SPI_DATA_PAYLOAD_PACKET, Invalid_Wind);

        break;
      
      case SPI_RECEIVE_INTERIM_PAYLOAD_DATA:   
        
        Process_Packet(SPI_DATA_INTERIM_PAYLOAD_PACKET, Invalid_Wind);//Set the events  
        memset(dataBuff, 0x00, strlen((char const*)dataBuff));         
        
        //There is more data pending for this payload packet, so just fetch them
        DisableEnable_SPI_CS(); 
        SPI_Receive_Manager(SPI_READ_DATA_PAYLOAD);//no header coming for this case
        break;
      
      case SPI_RECEIVE_END:
        /* Release CS line */
        /*Data is in dataBuff*/
        Disable_SPI_CS();       
        /*@TBD: In case of WIND:24 WiFi-Up Fix! 
        -> causes frame error when next header is fetched*/
        /*Temp Fix -> Delay next header fetch*/        
        if(wind_no == 24 && fix_counter==0) 
        { 
            fix_counter = 1;//This loop happens only once during WiFi-Up
            for (localloop = 0 ; localloop < DELAY_NEXT_HEADER_FETCH ; localloop++)
            {
              no_op(*(volatile uint32_t*)DUMMY_RAM_ADDRESS_TO_READ1);
            }
        }
#ifdef SPI_IRQ_MODE
        Spi_Receive_Event = SPI_RECEIVED_HEADER_FOR_RX;
#endif
        /*Proceed with next header*/
        ProcessEndOfReceive(SPI_WIND_PAYLOAD_PACKET, last_wind);        
        break;
    
      default:
        break;
    }
}
#endif //SPI_IRQ_MODE
