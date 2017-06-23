-------------------------------------------------------------------------------
-                    (C) COPYRIGHT 2017 STMicroelectronics                    -
- File:    readme.txt                                                         -
- Author:  ST Central Labs                                                    -
- Date:    18-Apr-2017                                                        -
- Version: V2.0.0                                                             -

-----------
WiFi-Bridge/readme.txt
-----------
This application creates a bridge between a 6LoWPAN network and the Internet using Contiki OS and the Wi-Fi module (SPWF01SA). 
The application supports the UDP protocol and uses NAT64 technology to translate IPv6 packets to IPv4 packets. 
While the nodes connected to the Wi-Fi bridge issue direct requests to a specific server, it is actually the Wi-Fi bridge 
which connects the nodes to the requested server by transparently forwarding every packet from the 6LoWPAN network to the IPv4 network.

It is based on the ip64 feature embedded in Contiki OS, and on the ip64-router example (so the WITH_IP64 macro coming from Contiki OS
must be enabled).

The ip64-router example has been ported in the Projects/Multi/Applications/WiFi-Bridge/Src folder. It sets by default the 0xAAAA (on 64 bits) prefix for the WSN, this value can be changed in the ip64-router.c file.

The original Contiki "ip64" code has been modified by adding the SKIP_LIFETIME macro, that is set (to 1) by default in this project. When this feature is enabled, the "check_age()" function in the NAT/IP64 module is not called and the lifetime values are put to 0: no timeout is applied.
The rationale behind this is that we are addressing a UDP (usually, CoAP over UDP) based WSN, in which the nodes may sleep for a long time, so we don't want an open "connection"/data flow to be closed on a timeout basis. So the NAT/IP64 entries are discarded only if the socket they are bound to is explicitly closed, or reusing the oldest one if we need a new data flow.
Please note that the NAT/IP64 code is (as in the original Contiki OS) in Middlewares/Third_Party/Contiki/core/net/ip64/ and the files
that have been modified are:
 - ip64-addrmap.c
 - ip64.c
so be sure to preserve the modifications in case you need to update Contiki OS middleware.

The IPv4 side of the NAT/IP64 module is implemented, in this application, using the X-NUCLEO-IDW01M1 expansion board.
The IPv4 interface definition and behavior depends on the
Middlewares/ST/Contiki_STM32_Library/Inc/ip64-conf.h
configuration file. Following the Contiki OS conventions, by setting the macro MY_DRIVERS=1, the relevant part of the code is enabled in which the IPv4 interfaces are set as follows:

#define IP64_CONF_UIP_FALLBACK_INTERFACE_SLIP 0
#define IP64_CONF_UIP_FALLBACK_INTERFACE my_wifi_interface
#define IP64_CONF_INPUT                  my_wifi_interface_input
#define IP64_CONF_ETH_DRIVER             my_wifi_driver

These interfaces are implemented at application level in Projects/STM32F401RE-Nucleo/Applications/WiFi-Bridge folder (Src and Inc).

 - my_wifi_interface module: this is the simple implementation for the fallback interface in use. Here happens the IPv4<->IPv6 conversion.
	+ the "output()" method receives the IPv6 packets, converts them in IPv4 and calls the "IP64_ETH_DRIVER.output()" method that,
	as per the configuration reported above, is the "my_wifi_driver.output()";
	+ "my_wifi_interface_input()" function is called by my_wifi_driver module: it receives the IPv4 packets from the WiFi side,
	converts them in IPv6 and injects them into uIP6 calling "tcpip_input()";

 - my_wifi_driver module: this is the implementation of the actual network interface. For this application, this must hide the fact that
the WiFi expansion board offers a socket level API. For UDP this can be achieved by copying the UDP payload (possibly a CoAP packet) as
payload for a UDP socket on WiFi side, and vice versa.
	+ the "output()" method, as said above, is called by my_wifi_interface module. This part of code is in charge of handling
	the communication from Contiki to the WiFi protocol stack, and so of opening new sockets. As a consequence of this, it implements
	the connection between "ip64-addrmap" (of Contiki) and "my_wifi_map" modules. It calls "wifi_socket_client_write()" to
	actually pass the packets to the WiFi.
	+ the "ind_wifi_socket_data_received()" function (that has to be implemented in order to be notified of data arriving from the 
	WiFi module) handles the packets received from WiFi: it computes the relevant header sizes and puts the IPv4 packets in
	uIP stack, then (by calling "process_poll(&my_wifi_process)") it triggers IP64_CONF_INPUT() that, as per above configuration
	ad as we described, ends up in "my_wifi_interface_input()" function.

 - my_wifi_map module: following the rationale of the ip64-addrmap module, we have another mapping because of the socket level API
(instead of a regular network interface). Here the mapping between NAT64 and Sockets is implemented. The relevant structure is 
"my_wifimap_entry" defined in my_wifi_types.h file.

The last two modules described above, implement sockets reuse, to let any data flow coming from the WSN exit towards the IPv4 side, in line with the policy enabled by SKIP_LIFETIME for the ip64-addrmap module. This is done by calling "my_wifimap_free()" in order to free the oldest socket for the new data flow.
This is in line with a WSN scenario in which the nodes (more than the available sockets) periodically send out data to a server.
There is no (socket-)limitation to this number of clients, since the sockets are reused.

To summarize:
 - the maximum number of nodes/outbound connections is defined by IP64_ADDRMAP_CONF_ENTRIES or NUM_ENTRIES (ip64-addrmap.c) that
   defaults to 32.
 - the maximum number of bidirectional connections is defined by NUM_SOCKETS, that is set to his HARD limit of 8.


-----------
Board Settings 
-----------
Be sure to select the right X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 board in the code by setting the symbol X_NUCLEO_IDS01A4 for the 868MHz bandwidth, and X_NUCLEO_IDS01A5 for the 915MHz bandwidth.

-----------
Hardware and Software environment
-----------
  - This example runs on STM32 Nucleo devices with X-NUCLEO-IDS01A4 (or X-NUCLEO-IDS01A5) stacked on top of a X-NUCLEO-IDW01M1 expansion board
  - This example has been tested with STMicroelectronics:
    - NUCLEO-F401RE RevC board

-----------
How to use it?
-----------    
In order to make the program work, you must do the following:

 - WARNING: before opening the project with any toolchain be sure your folder
   installation path is not too in-depth since the toolchain may report errors
   after building.
   
 - WARNING: be sure to have a running wireless access point connected to the internet. Please also notice that WPA Enterprise encryption is not supported.
 
 - Open IAR toolchain and compile the project (see the release note for detailed information about the version).
   Alternatively you can use the Keil uVision toolchain (see the release note for detailed information about the version).
   Alternatively you can use the System Workbench for STM32 (see the release note for detailed information about the version).
 - Program the firmware on the STM32 Nucleo board: you can copy (or drag and drop) the binary file to the USB mass storage location created when you plug the STM32Nucleo 
   board to your PC. If the host is a Linux PC, the STM32 Nucleo F4 device can be found in the /media folder with the name "NUCLEO_F401RE". For example, if the created 
   mass storage location is "/media/NUCLEO_F401RE", then the command to program the board with a binary file named "my_firmware.bin" is simply: cp my_firmware.bin /media/NUCLEO_F401RE. 
   Alternatively, you can program the STM32 Nucleo board directly through one of the supported development toolchains.
 - Open a serial line monitor utility, select the serial port name to which the board is connected and configure it thus:
   + Baud Rate = 115200
   + Parity = None
   + Bits = 8
   + Stopbits = 1
 - Press the RESET (black) button on the STM32 Nucleo board and the user will be prompted to connect the WiFi Bridge to a wireless access point: 
   when asked keep the Blue User Button pressed to enter your credentials (SSID and password) or just wait to connect to the Wi-Fi network (if any) saved in Flash. If you enter new parameters, these will be saved in Flash and used as defaults when you reboot the board.
   
 - Alternatively, you can download the pre-built binaries in "Binary" 
   folder included in the distributed package. 
   
 - IMPORTANT NOTE: To avoid issues with USB connection (mandatory if you have USB 3.0), it is   
   suggested to update the ST-Link/V2 firmware for STM32 Nucleo boards to the latest version.
   Please refer to the readme.txt file in the Applications directory for details.

-----------
Connectivity test
-----------
Once the WiFi Bridge is connected to the access point, the internet connectivity can be tested with one of our pre-compiled binaries to be flashed to a new STM32 Nucleo board.
The examples export different resources, based on the type of expansion boards stacked on the NUCLEO-F401RE.
The supported configurations are:
1. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5(/Utilities/Binary/WiFi-Bridge/Ipso_Example_Nosensors/): exports common resources like LEDs and user buttons.
2. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 + X-NUCLEOIKS01A1(/Utilities/Binary/WiFi-Bridge/Ipso_Example_Mems/): exports resources from the MEMS sensors expansion board (temperature, humidity, pressure, magnetometer and acceleration sensors).
3. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 + X-NUCLEO-6180XA1 (/Utilities/Binary/WiFi-Bridge/Ipso_Example_Proximity/): exports resources from the FlightSense expansion board (proximity sensor).
The nodes will attempt to connect to a public online server (http://leshan.eclipse.org/), which is a Java implementation of a LWM2M server (see http://www.eclipse.org/leshan/ for further information).
    
_______________________________________________________________________________
- (C) COPYRIGHT 2017 STMicroelectronics                   ****END OF FILE**** -
