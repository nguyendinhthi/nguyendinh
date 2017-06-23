-------------------------------------------------------------------------------
-                    (C) COPYRIGHT 2017 STMicroelectronics                    -
- File:    FP-NET-6LPWIFI1_readme.txt                                         -
- Author:  ST Central Labs                                                    -
- Date:    19-Apr-2017                                                        -
- Version: V2.0.0                                                             -

-----------
FP-NET-6LPWIFI1_readme.txt
-----------
FP-NET-6LPWIFI1 is an STM32 ODE function pack which connects your IoT node in a 6LoWPAN wireless sensor network to the Internet via a Wi-Fi network. It saves you time and jumpstarts end-to-end IoT development by integrating the fundamental functions for connecting 6LoWPAN nodes to web-based infrastructure.
This software package implements application-level functions to bridge a 6LoWPAN with a Wi-Fi network (UDP protocol support only). Binary firmware for sensor nodes is provided to access resources like sensors and actuators on the 6LoWPAN nodes with the OMA Lightweight M2M protocol, using the provided WiFi-Bridge sample application to access the WSN.

For instructions on how to use the sample applications, see the files: 
 * Projects\Multi\Applications\WiFi-Bridge\readme.txt
 * Projects\Multi\Applications\LWM2M_to_IBM\readme.txt

For instruction on how to flash the pre-compiled binaries, see the file:
 * Utilities\Binary\readme.txt
---------
Contents
---------
This package contains:
 * FP-NET-6LPWIFI1_readme.txt ................ This readme.txt
   
 * Documentation\FP-NET-6LPWIFI1.chm ...... Documentation of the source code included in this package

 * Drivers\BSP\X-NUCLEO-IDW01M1
   ........................... BSP drivers for the X-NUCLEO-IDW01M1 WiFi expansion board

 * Drivers\BSP\X-NUCLEO-IDS01Ax
   ........................... BSP drivers for the X-NUCLEO-IDS01Ax SPIRIT1 expansion board

 * Drivers\BSP\Components\spirit1
   ........................... Component drivers for the SPSGRF-868 (or SPSGRF-915) SPIRIT1 sub-1GHz radio

 * Middlewares\ST\STM32_SPWF01SA
   ........................... WiFi-module middleware libraries

 * Middlewares\ST\STM32_Contiki_Library
   ........................... Porting of the Contiki OS to the STM32 platform

 * Middlewares\Third_Party\Contiki
   ........................... Contiki OS v3.x (see the release note for detailed information about the version)

 * Middlewares\Third_Party\MQTT-Paho
   ........................... Implementation of the MQTT Protocol
   
 * Projects\Multi\Applications\WiFi-Bridge
   * WiFi-Bridge  ............ Use it to create a bridge between a 6LoWPAN network and the Internet using Contiki OS 
                               and the Wi-Fi module (SPWF01SA). The application supports the UDP protocol and uses 
                               NAT64 technology to translate IPv6 packets to IPv4 packets.	

 * Projects\Multi\Applications\LWM2M_to_IBM
   * LWM2M_to_IBM ............ Use it to create a bridge between a OMA LWM2M/IPSO Smart Objects sensor network (using 6LoWPAN and SPIRIT1 radio)
                                  and the IBM Watson Cloud using Contiki OS and the Wi-Fi module (SPWF01SA). 

_______________________________________________________________________________
- (C) COPYRIGHT 2017 STMicroelectronics                   ****END OF FILE**** -
