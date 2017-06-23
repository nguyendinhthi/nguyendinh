/**
 ******************************************************************************
 * @file    spwf04_serial_to_spi.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    17-May-2016
 * @brief   Converts AT Cmd Strings to SPI Cmd structure
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enum.h"
#include "wifi_module.h"
#include "wifi_globals.h"

#define u8 unsigned char
#define CONSOLE_TypeDef void
#define SPWF_SPI_MSG_ID 0x02
#define SPI_WRITE_DELAY 100

//static ConsoleErr cmd_func(int id, char *args);
static ConsoleErr cmd_funcArgs(int id, char *args, char *data, int len, int mode);


struct console_cmd {
	char *cmd;
	char *args;
	char *help;
	ConsoleErr (*func)(int id, char *args, char *data, int len, int mode);
	u8 show;
	u8 id;
};

#define CONSOLE_CMD(__id, __c, __f, __s, __h)				{ .id = __id, .cmd = __c , .func = __f , .show = __s, .help = __h }
#define CONSOLE_CMD2(__id, __c, __f, __s, __a, __h)	{ .id = __id, .cmd = __c , .func = __f , .show = __s, .help = __h, .args = __a }

// FULL list console commands
static const struct console_cmd cmds[] = {
	/* General Utilities */
	CONSOLE_CMD (0x01, "AT",                cmd_funcArgs,	1, "Null command, always returns OK"),
	CONSOLE_CMD2(0x02, "AT+S.HELP",	        cmd_funcArgs,   1, "[=<command>]", "Display AT-Command information [on a specific command]"),

	CONSOLE_CMD (0x03, "AT+S.RESET",        cmd_funcArgs,	1, "Provide a software reset"),

	CONSOLE_CMD2(0x04, "AT+S.PMS",	        cmd_funcArgs,	1, "=<0|1|2|3>", "Enable active/powersave/stop/standby power modes"),
	CONSOLE_CMD2(0x05, "AT+S.STS",	        cmd_funcArgs,	1, "[=<sts_var>]", "Report [single] status/statistic"),
	CONSOLE_CMD (0x06, "AT+S.RTOSSTS",      cmd_funcArgs,	1, "Report status/statistics of RTOS"),
	CONSOLE_CMD (0x07, "AT+S.SSXSTS",	cmd_funcArgs,	1, "Report status/statistics of TCP/IP"),
	CONSOLE_CMD2(0x08, "AT+S.PYTHON",	cmd_funcArgs,	1, "[=<filename>]", "Enter interactive uPhyton shell"),

	/* Module Configuration */
	CONSOLE_CMD2(0x09, "AT+S.GCFG",		cmd_funcArgs,	1, "[=<cfg_var>]", "Report [single] configuration variables"),
	CONSOLE_CMD2(0x0A, "AT+S.SCFG",		cmd_funcArgs,	1, "=<key>,<value>", "Set configuration variable"),
	CONSOLE_CMD (0x0B, "AT+S.WCFG",		cmd_funcArgs,	1, "Save configuration variables to flash"),
	CONSOLE_CMD (0x0C, "AT+S.FCFG",		cmd_funcArgs,	1, "Restore factory configuration variables from flash"),
	CONSOLE_CMD2(0x0D, "AT+S.NVW",		cmd_funcArgs,	1, "=<magic>", "Save factory configuration variables to flash"),
	CONSOLE_CMD2(0x0E, "AT+S.MFGTEST",	cmd_funcArgs,	1, "=<magic>,<low|high|HSI|SYS>", "Perform Manufacturing Self-Tests"),

	/* STM32 Peripherals */
	CONSOLE_CMD2(0x11, "AT+S.TIME",		cmd_funcArgs,	1, "[=<time>]", "Get/Set current time in seconds"),
	CONSOLE_CMD (0x12, "AT+S.RANDOM",	cmd_funcArgs,	1, "Provide a new random number"),
	CONSOLE_CMD2(0x13, "AT+S.GPIOC",	cmd_funcArgs,	1, "=<num>,<in|out>,<r|f|b>", "Configure specified GPIO"),
	CONSOLE_CMD2(0x14, "AT+S.GPIOR",	cmd_funcArgs,	1, "=<num>", "Read specified GPIO"),
	CONSOLE_CMD2(0x15, "AT+S.GPIOW",	cmd_funcArgs,	1, "=<num>,<val>", "Write specified GPIO"),
	CONSOLE_CMD2(0x16, "AT+S.DAC",		cmd_funcArgs,	1, "=<0|value>", "Disable/Enable DAC on GPIO15"),
	CONSOLE_CMD (0x17, "AT+S.ADC",		cmd_funcArgs,	1, "Read ADC value on GPIO1"),
	CONSOLE_CMD2(0x18, "AT+S.PWM",		cmd_funcArgs,	1, "=frequency,duty_cycle", "Set PWM on GPIO2"),

	/* FileSystem Management*/
        CONSOLE_CMD (0x21, "AT+S.FSU",		cmd_funcArgs,	1, "Unmount/Erase Volume"),
	CONSOLE_CMD2(0x23, "AT+S.FSC",		cmd_funcArgs, 	1, "=<filename>,<datalen><CR><data>", "Create a file/Append to an existing file"),
	//CONSOLE_CMD2(0x22, "AT+S.FSA",	cmd_funcArgs,	1, "=<filename>,<datalen><CR><data>", "Append to an existing file"),
	CONSOLE_CMD2(0x25, "AT+S.FSD",		cmd_funcArgs,	1, "=<filename>", "Delete an existing file"),
	CONSOLE_CMD2(0x26, "AT+S.FSR",		cmd_funcArgs,	1, "=<old_filename>,<new_filename>", "Rename an existing file"),
	CONSOLE_CMD2(0x27, "AT+S.FSL",		cmd_funcArgs,   1, "[=<filename>]", "List existing filename(s)"),
	CONSOLE_CMD2(0x28, "AT+S.FSP",		cmd_funcArgs,	1, "=<filename>,<offset>,<length>", "Print the contents of an existing file"),
	CONSOLE_CMD2(0x29, "AT+S.HASH",	        cmd_funcArgs,	1, "=<functions,<filename>", "Compute Digest"),
	CONSOLE_CMD2(0x2A, "AT+S.WPAECERT",	cmd_funcArgs,	1, "=<ca|cert|key|auth|content>,<length|option>", "Configure/Get/Clean WPA-Enterprise certificates"),
	CONSOLE_CMD2(0x2B, "AT+S.TLSCERT",	cmd_funcArgs,	1, "=<ca|cert|key|auth|content>,<length|option>", "Configure/Get/Clean TLS certificates"),

	/* Network Management */
	//CONSOLE_CMD2(0x31, "AT+S.CMDREQ",	cmd_funcArgs,	1, "=<length><CR><data>", "Issue a radio command"),
	CONSOLE_CMD2(0x32, "AT+S.WIFI",		cmd_funcArgs,	1, "=<0|1>", "Disable/Enable WiFi radio"),
	CONSOLE_CMD2(0x33, "AT+S.SCAN",		cmd_funcArgs,	1, "=<d|s|m>,<filename>", "Perform a scan with/out filter on SSID/MAC"),
	CONSOLE_CMD2(0x34, "AT+S.SSIDTXT",      cmd_funcArgs,	1, "[=<ssidtxt>]", "Get/Set an ASCII SSID"),
	CONSOLE_CMD2(0x35, "AT+S.PEERS",	cmd_funcArgs,	1, "[=peer_number[,peer_var]]", "Dump contents of the peer table"),
	CONSOLE_CMD2(0x36, "AT+S.WPS",		cmd_funcArgs,	1, "[=<8-digit pin>]", "Initiate a WPS exchange"),
	CONSOLE_CMD (0x37, "AT+S.RENEW",	cmd_funcArgs,	1, "Send an IPv4 Address Renew Request"),
	CONSOLE_CMD (0x38, "AT+S.NTP",		cmd_funcArgs,	1, "Force an NTP Query"),
	CONSOLE_CMD2(0x39, "AT+S.PING",		cmd_funcArgs,	1, "=<count>,<size>,<hostname>,<IPv>", "Send pings to specified host"),
	CONSOLE_CMD2(0x41, "AT+S.SOCKON",	cmd_funcArgs,	1, "=<hostname>,<IPv>,<port>,<TCP local port>,<t|u>", "Open a socket client"),
	CONSOLE_CMD2(0x42, "AT+S.SOCKQ",	cmd_funcArgs,   1, "=<id>", "Query socket client for pending data"),
	CONSOLE_CMD2(0x43, "AT+S.SOCKC",	cmd_funcArgs,	1, "=<id>", "Close socket client"),
	CONSOLE_CMD2(0x44, "AT+S.SOCKW",	cmd_funcArgs,	1, "=<id>,<len>", "Write data to socket client"),
	CONSOLE_CMD2(0x45, "AT+S.SOCKR",	cmd_funcArgs,	1, "=<id>,<len>", "Read data from socket client"),
	CONSOLE_CMD (0x46, "AT+S.SOCKL",	cmd_funcArgs,	1, "List opened socket clients"),
	CONSOLE_CMD2(0x47, "AT+S.SOCKDON",	cmd_funcArgs,   1, "=<port>,<t|u|s1|s2>", "Open a socket server"),
	CONSOLE_CMD2(0x48, "AT+S.SOCKDQ",	cmd_funcArgs,	1, "=<sid>,<cid>", "Query socket server for pending data"),
	CONSOLE_CMD2(0x49, "AT+S.SOCKDC",	cmd_funcArgs,	1, "=<sid>[,<cid>]", "Close socket server [or disconnect TCP/TLS client]"),
	CONSOLE_CMD2(0x4A, "AT+S.SOCKDW",	cmd_funcArgs,	1, "=<sid>,<cid>,<len>", "Write data to socket server"),
	CONSOLE_CMD2(0x4B, "AT+S.SOCKDR",	cmd_funcArgs,	1, "=<sid>,<cid>,<len>", "Read data from socket server"),
	CONSOLE_CMD2(0x4C, "AT+S.SOCKDL",	cmd_funcArgs,	1, "[=<sid>]", "List bound socket clients [on a specific server]"),
	CONSOLE_CMD2(0x4F, "AT+S.SELFTEST",	cmd_funcArgs, 	1, "=<magic>,<ip>,<port>,<num>,<size>,<t|u>", "SelfTests"),
	CONSOLE_CMD2(0x51, "AT+S.TFTPGET",	cmd_funcArgs,	1, "=<hostname>,<IPv>,<filename>", "GET Request to specified TFTP Server"),
	CONSOLE_CMD2(0x52, "AT+S.TFTPPUT",	cmd_funcArgs,	1, "=<hostname>,<IPv>,<filename>", "PUT Request to specified TFTP Server"),
	CONSOLE_CMD2(0x53, "AT+S.SMTP",		cmd_funcArgs, 	1, "=<hostname>,<port>,<TLS>,<usn>,<pwd>,<heloID>,<address>,<to>,<cc>,<bcc>,<subject>,<body>,<attachment>", "Mail send to remote peer"),
	CONSOLE_CMD2(0x54, "AT+S.HTTPGET",	cmd_funcArgs, 	1, "=<hostname>,<path&queryopts>,<port>,<TLS>,<usn>,<pwd>,<filenameDown>,<filenameUp>", "GET Request to specified HTTP Server"),
	CONSOLE_CMD2(0x55, "AT+S.HTTPPOST",	cmd_funcArgs, 	1, "=<hostname>,<path&queryopts>,<formcontent>,<port>,<TLS>,<usn>,<pwd>,", "POST Request to specified HTTP Server"),
        CONSOLE_CMD2(0x56, "AT+S.FWUPDATE",	cmd_funcArgs, 	1, "=<hostname>,<path&queryopts>,<port>,<TLS>,<usn>,<pwd>", "Update firmware image from named host and path"),
	CONSOLE_CMD (0x59, "AT+S.INPUTSSI",	cmd_funcArgs,	1, "Fill buffer for Raw text input SSI"),
	CONSOLE_CMD2(0x5A, "AT+S.MQTTCONN",	cmd_funcArgs, 	1, "=<hostname>,<path>,<port>,<TLS>,<usn>,<pwd>,<ClientID>,<KeepAlive>,<Retry>,<LWT_QoS>,<LWT_Name>,<LWT_Message>", "Connect to specified MQTT Broker"),
	CONSOLE_CMD2(0x5B, "AT+S.MQTTSUB",	cmd_funcArgs, 	1, "=<id>,<topic>,<QoS>", "Subscribe topic to MQTT Broker"),
	CONSOLE_CMD2(0x5C, "AT+S.MQTTPUB",	cmd_funcArgs, 	1, "=<id>,<topic>,<QoS>,<retained>,<message>", "Publish a message to MQTT Broker"),
	CONSOLE_CMD2(0x5D, "AT+S.MQTTUNSUB",	cmd_funcArgs, 	1, "=<id>,<topic>", "Unsubscribe topic from MQTT Broker"),
	CONSOLE_CMD2(0x5E, "AT+S.MQTTDISC",	cmd_funcArgs, 	1, "=<id>", "Disconnect from MQTT Broker"),
	CONSOLE_CMD2(0x61, "AT+S.WSOCKON",	cmd_funcArgs,	1, "=<hostname>,<port>,<path>,<TLS>,<usn>,<pwd>,<origin>,<protocols>,<extensions>", "Open a web socket client"),
	CONSOLE_CMD2(0x62, "AT+S.WSOCKQ",	cmd_funcArgs,	1, "=<id>", "Query web socket client for pending data"),
	CONSOLE_CMD2(0x63, "AT+S.WSOCKC",	cmd_funcArgs,	1, "=<id>,<status>", "Close web socket client using a specific status code"),
	CONSOLE_CMD2(0x64, "AT+S.WSOCKW",	cmd_funcArgs,	1, "=<id>,<lastFrame>,<lastFrag>,<binary>,<len>", "Write data to web socket client"),
	CONSOLE_CMD2(0x65, "AT+S.WSOCKR",	cmd_funcArgs,	1, "=<id>,<len>", "Read data from web socket client"),
	CONSOLE_CMD (0x66, "AT+S.WSOCKL",       cmd_funcArgs,	1, "List opened web socket clients"),
	/* Ensure Retro-Compatibility */
	CONSOLE_CMD (0x00, "AT&V",		cmd_funcArgs,	0, "Report configuration variables"),
	CONSOLE_CMD (0x00, "AT&W",		cmd_funcArgs,	0, "Save configuration variables to flash"),
	CONSOLE_CMD (0x00, "AT&F",		cmd_funcArgs,	0, "Restore factory configuration variables from flash"),

	CONSOLE_CMD (0x00, NULL,		NULL,		0, NULL)
};

/**
**/
int get_cmd_id( char *str){
  int i = 0;
  
   for (i = 0 ; cmds[i].func != NULL ; i++) {
			if (!strncasecmp(cmds[i].cmd, str, 64)) {
				//cmd = (struct console_cmd *) &cmds[i];
				break;
			}
       }
      
      if (cmds[i].func == NULL){
        printf("Error: command not found");
        i = -1;
      }
      
      //printf("%d\r\n", i);
      return i;
}

// Run SPI command
int run_spi_cmd(char *str, int mode){
  // Split command stings and arguments
  char *args = strstr(str, "=");
  char *data = strstr(str, " ");
  int len = 0;
  
  if (data){
    *data = '\0';
    data += 1;
    len = strlen(data);
  }
    
  if (args){
    *args = '\0';
    args += 1;
  }
  
  int i = get_cmd_id(str);
  if (i>=0){
    cmds[i].func(i, args, data, len, mode);
  }
  
  return i;
}

static ConsoleErr cmd_funcArgs(int id, char *args, char *data, int len, int mode){ 
  
  char *tx = (char *)WiFi_SPI_Packet;
  uint8_t ll = 0;
  int offset = 0;
  int argc = 0;
  
  // Allocate buffer to store all data including message header
  //int tx_len = ((args != NULL) ? strlen(args) : 0) + 5;

  //Reset_AT_CMD_Buffer();  
  memset(tx, 0x0, sizeof WiFi_SPI_Packet);
  
  tx[0] = SPWF_SPI_MSG_ID;
  // tx[1,2] --> Full MSG length filled lather
  tx[1] = 0;
  tx[2] = 0x02;
  tx[3] = cmds[id].id; // Command id
  // tx[4] --> N arguments will be filled later
  tx[4] = 0;
  offset = 5;
  
  // Check if routine contains arguments
  if (args != NULL){
    // String contains comma separated arguments
    //printf("$$ %s\r\n", args);
    char *token = strtok(args, ",");
    
    while(token){
      //printf("%s\r\n", token);
      // Convert elemt into SPI field
        //sprintf(str, "%s", token);
        argc++;
        ll = strlen(token);
        if (strstr(token, "NULL") == NULL) {       
          //printf("Token len = %d\r\n", ll);          
          tx[offset++] = ll; 
          memcpy(tx+offset, token, ll);
          offset += ll;
        } else {
          ll = 1;
          tx[offset++] = 0;//In case of NULL string just fill it up as 0
        }

        token = strtok(NULL, ",");
    }
    if (data && len > 0)
    {
        //memcpy(tx+offset, data, len);
        offset += len;
    }

    // Set Payload length
    tx[1] = ((offset-3) & 0xFF00)>>8;
    tx[2] = ((offset-3) & 0x00FF);

    // Set N arguments
    //printf("Argc %d %x\r\n", argc, tx[3]);
    tx[4] = argc;
  }
  
#ifdef SPI_VCOM
  switch(tx[3])
  {
  case CMD_ID_SCAN:
    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
    IO_status_flag.AT_vcom_processing = WIFI_SCAN_EVENT;
    break;
  default:
    IO_status_flag.AT_vcom_processing = WIFI_NO_EVENT;
    IO_status_flag.AT_event_processing = WIFI_NO_EVENT;
    break;
  }
#endif  
  
  if (data && len > 0) {
    volatile uint8_t localloop;
    /*TIme needed to make sure internal DMA has been re-programmed?*/
    for (localloop = 0 ; localloop < SPI_WRITE_DELAY ; localloop++)
    {
      *(volatile uint32_t*)0x20000000;
    }
    SPI_Send_AT_Command(offset-len, mode);//send the SPI cmd packet
    //printf("Sending data: %d\r\n", len);
    //SPI_Transmit_Manager_Poll(data, len);//now send the data
  } else
    SPI_Send_AT_Command(offset, mode);


  return (ConsoleErr)0;
}
