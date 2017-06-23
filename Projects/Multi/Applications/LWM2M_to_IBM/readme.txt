-------------------------------------------------------------------------------
-                    (C) COPYRIGHT 2017 STMicroelectronics                    -
- File:    readme.txt                                                         -
- Author:  ST Central Labs                                                    -
- Date:    19-Apr-2017                                                        -
- Version: V2.0.0                                                             -

-----------
LWM2M_to_IBM/readme.txt
-----------
This application connects a 6LoWPAN network that uses OMA LWM2M and IPSO Semantic over CoAP protocol to IBM Watson IoT cloud services, using Contiki OS, the Wi-Fi module (SPWF01SA) and MQTT Paho library. 

The application implements a gateway, acting as a simple OMA LWM2M server for the nodes in the 6LoWPAN network, and as a MQTT client for the IBM Cloud.

Since the OMA LWM2M protocol does not allow nodes to send data (except in the registration phase) to a server, a mechanism to start retrieving sensor information is needed. The LWM2M_to_IBM gateway can be pre-programmed to automatically retrieve some resources from the nodes that support these resources. This can be done using the Observe/Notification mechanism of LWM2M (and CoAP) or by polling the resource with periodic LWM2M Reads.
A code template has been provided for this, it can be found at the beginning of the file Src/lwm2m-resource-directory.c:

Syntax:
 TEMPLATE(template_name,
 			IPSO_RESOURCE_TYPE(OBJECT_ID, RESOURCE_ID, OPERATION_TYPE, ALTERNATIVE_NAME),
 			);

The gateway is by default pre-programmed with the following instructions:
TEMPLATE(template,
			IPSO_RESOURCE_FLOAT (IPSO_ACCELEROMETER, Z_VALUE, OPR_TO_OBSERVE, ACCELERATION_Z_STR),
			IPSO_RESOURCE_INT   (IPSO_BUTTON, BUTTON_COUNTER, OPR_TO_GET, BUTTON_COUNTER_STR),
			IPSO_RESOURCE_FLOAT (IPSO_TEMPERATURE_SENSOR, SENSOR_VALUE, OPR_TO_OBSERVE, TEMPERATURE_VALUE_STR),
			IPSO_RESOURCE_FLOAT (IPSO_TEMPERATURE_SENSOR, MAX_MEASURED_VALUE, OPR_TO_GET, TEMPERATURE_MAX_VALUE_STR),
			IPSO_RESOURCE_FLOAT (IPSO_HUMIDITY_SENSOR, SENSOR_VALUE, OPR_TO_OBSERVE, HUMIDITY_VALUE_STR),
			IPSO_RESOURCE_INT   (IPSO_PRESENCE_SENSOR, DIGITAL_INPUT_STATE, OPR_TO_OBSERVE, PRESENCE_STATE_STR),
		);

The actual macro definitions can be found in the file Inc/lwm2m-simple-server.h.
The LWM2M and IPSO related objects and resource definitions are included in the file Inc/lwm2m-defs.h, in compliance with the
“OMA Lightweight Machine to Machine Technical Specification V1.0” and “IPSO SmartObject Guideline - Smart Objects Starter Pack1.0” documents.

The rationale is that by adding in the template a line like
IPSO_RESOURCE_TYPE(OBJECT_ID, RESOURCE_ID, OPERATION_TYPE, ALTERNATIVE_NAME),
the LWM2M to IBM gateway automatically retrieves the URI "OBJECT_ID/INSTANCE_ID/RESOURCE_ID"

This is done for any object instance and for all the network nodes.
The type of resource is specified by IPSO_RESOURCE_TYPE:
 - IPSO_RESOURCE_FLOAT for floating point values (i.e. temperature)
 - IPSO_RESOURCE_INT for integer values (i.e. button counter)
 - IPSO_RESOURCE_STRING for text resources (i.e. manufacturer)

The type of retrieval is specified by OPERATION:
 - OPR_TO_GET for read operations (in loop)
 - OPR_TO_OBSERVE for observe/notifications
The ALTERNATIVE_NAME is an optional string to send the datapoints to IBM cloud in a more readable way compared to the LWM2M/IPSO semantic.
This behavior can be triggered by the USE_LWM2M_FORMAT_FOR_DATAPOINT_NAME macro, in the project settings.

This application relies on the Contiki OS and uses its programming model.
Three PROCESS_THREAD are defined:
 - "wifi_process" (in Src/main.c file) that periodically calls the WiFi_Process() function:
	+ Here the interface with the WiFi board and the IBM cloud by means of MQTT is implemented
 - "oma_lwm2m_light_server" (in Src/lwm2m-simple-server.c file)
	+ Here the Observe/Notification and periodic GETs calls are managed
- "command_process" (in Src/lwm2m-simple-server.c file)
	+ Here the commands received from cloud (if running in registered mode) are processed
The timers for these processes are defined in the respective files.
	
The LWM2M Resource Directory, where nodes register and their resources are discovered and processed by means of the TEMPLATE, is implemented in the Src/lwm2m-resource-directory.c file by means of the macro (using the Erbium REST framework for CoAP embedded in Contiki):
PARENT_RESOURCE(res_rd, NULL, NULL, post_handler, NULL, NULL);
All the node registration is handled starting from the post_handler() function there implemented.

The maximum number of supported nodes can be defined by means of the MAX_NODES_ macro defined (to 20 by default) in the Inc/project-conf.h file.

The Src/lwm2m-data.c implements the data structures for Observations, Periodic GETs and Commands Responses (if in registered mode). The maximum number of Observations can be set by means of the macro MAX_OBS_ and the maximum number of Periodic GETs by means of the macro MAX_GET_, both can be found in the Inc/lwm2m-simple-server.h file.
By default, both these macros are set to COAP_CONF_MAX_OBSERVEES that, in turn, is set in the Inc/project-conf.h file to be 
COAP_MAX_OBSERVERS_PER_NODE * MAX_NODES_ = 60 with the default (current) values. All these setting can be customized as per the user needs.

All the main data structure can be found in the Inc/lwm2m-simple-server.h file.

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
Once the LWM2M_to_IBM gateway is connected to the access point, the internet connectivity can be tested with one of our pre-compiled binaries to be flashed to a new STM32 Nucleo board.
The examples export different resources, based on the type of expansion boards stacked on the NUCLEO-F401RE.
The supported configurations are:
1. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5(/Utilities/Binary/LWM2M_to_IBM/Ipso_Example_Nosensors/): exports common resources like LEDs and user buttons.
2. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 + X-NUCLEOIKS01A1(/Utilities/Binary/LWM2M_to_IBM/Ipso_Example_Mems/): exports resources from the MEMS sensors expansion board (temperature, humidity, pressure, magnetometer and acceleration sensors).
3. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 + X-NUCLEO-6180XA1 (/Utilities/Binary/LWM2M_to_IBM/Ipso_Example_Proximity/): exports resources from the FlightSense expansion board (proximity sensor).
The nodes will attempt to connect to the LWM2M server running on the LWM2M_to_IBM gateway, that has an hard coded address of [aaaa::ff:fe00:1].
This address can be changed by means of the LWM2M_SERVER_ADDRESS_v6 macro at the beginning of Src/lwm2m-simple-server.c file.
    
_______________________________________________________________________________
- (C) COPYRIGHT 2017 STMicroelectronics                   ****END OF FILE**** -
