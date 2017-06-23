/**
  ******************************************************************************
  * @file    lwm2m-defs.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    11-April-2017
  * @brief   LWM2M and IPSO definitions
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
#ifndef LWM2M_DEFS_H_
#define LWM2M_DEFS_H_

/*****LWM2M and IPSO OBJECTS*****/
#define LWM2M_ENDPOINT_QUERY_NAME   "ep"
#define LWM2M_LIFETIME_QUERY_NAME   "lt"
#define LWM2M_SMS_NUMBER_QUERY_NAME "sms"
#define LWM2M_VERSION_QUERY_NAME    "lwm2m"
#define LWM2M_BINDING_QUERY_NAME    "b"

#define LWM2M_RD_RESOURCE         "rd"
#define LWM2M_RD_PATH              LWM2M_RD_RESOURCE"/"


/*OMA LWM2M V1.0 OBJECTS */
#define SECURITY                    0
#define SERVER                      1
#define ACCESS_CONTROL              2
#define DEVICE                      3
#define CONNECTIVITY_MONITORING     4
#define FIRMWARE                    5
#define LOCATION                    6
#define CONNECTIVITY_STATISTICS     7

/* SECURITY_ RESOURCES */

/* SERVER_ RESOURCES */
#define SHORT_SERVER_ID             0
#define LIFETIME                    1
#define DEFAULT_MIN_PERIOD          2
#define DEFAULT_MAX_PERIOD          3
#define DISABLE                     4
#define DISABLE_TIMEOUT             5
#define NOTIFICATION_STORING        6
#define BINDING                     7
#define REGISTRATION_UDPATE_TRIGGER 8

/* ACCESS_CONTROL_ RESOURCES */

/* DEVICE_ RESOURCES */
#define MANUFACTURER                0
#define MODEL_NUMBER                1
#define SERIAL_NUMBER               2
#define FIRMWARE_VERSION            3
#define REBOOT                      4
#define FACTORY_RESET               5
#define AVAILABLE_POWER_SOURCES     6
#define POWER_SOURCES_VOLTAGE       7
#define POWER_SOURCES_CURRENT       8
#define BATTERY_LEVEL               9
#define MEMORY_FREE                 10
#define ERROR_CODE                  11
#define RESET_ERROR_CODE            12
#define CURRENT_TIME                13
#define UTC_OFFSET                  14
#define TIMEZONE                    15
#define SUPPORTED_BINDING_AND_MODES 16
#define DEVICE_TYPE                 17
#define HARDWARE_VERSION            18
#define SOFTWARE_VERSION            19
#define BATTERY_STATUS              20
#define MEMORY_TOTAL                21
#define EXTDEVINFO                  22

/* CONNECTIVITY_MONITORING_ RESOURCES */

/* FIRMWARE_ RESOURCES */

/* LOCATION_ RESOURCES */

/* CONNECTIVITY_STATISTICS_ RESOURCES */

/*::::::::::::::::::::::::::::::::::::::::*/

/* IPSO SMART OBJECTS STARTER PACK */
#define IPSO_DIGITAL_INPUT          3200
#define IPSO_DIGITAL_OUTPUT         3201
#define IPSO_ANALOGUE_INPUT         3202
#define IPSO_ANALOGUE_OUTPUT        3203
#define IPSO_GENERIC_SENSOR         3300
#define IPSO_ILLUMINANCE_SENSOR     3301
#define IPSO_PRESENCE_SENSOR        3302
#define IPSO_TEMPERATURE_SENSOR     3303
#define IPSO_HUMIDITY_SENSOR        3304
#define IPSO_POWER_MANAGEMENT       3305
#define IPSO_ACTUATION              3306
#define IPSO_SET_POINT              3308
#define IPSO_LOAD_CONTROL           3310
#define IPSO_LIGHT_CONTROL          3311
#define IPSO_POWER_CONTROL          3312
#define IPSO_ACCELEROMETER          3313
#define IPSO_MAGNETOMETER           3314
#define IPSO_BAROMETER              3315

/* IPSO S.O. REUSABLE RESOURCES (to be completed) */
#define DIGITAL_INPUT_STATE            5500
#define DIGITAL_INPUT_COUNTER          5501
#define DIGITAL_INPUT_POLARITY         5502
#define DIGITAL_INPUT_DEBOUNCE_PERIOD  5503
#define DIGITAL_INPUT_EDGE_SELECTION   5504
#define DIGITAL_INPUT_COUNTER_RESET    5505
#define DIGITAL_OUTPUT_STATE           5550
#define DIGITAL_OUTPUT_COUNTER         5551
#define MIN_MEASURED_VALUE             5601
#define MAX_MEASURED_VALUE             5602
#define MIN_RANGE_VALUE                5603
#define MAX_RANGE_VALUE                5604
#define RESET_MIN_MAX_MEASURED_VALUES  5605
#define SENSOR_VALUE                   5700
#define UNITS                          5701
#define X_VALUE                        5702
#define Y_VALUE                        5703
#define Z_VALUE                        5704
#define COMPASS_DIRECTION              5705
#define COLOUR                         5706
#define APPLICATION_TYPE               5750
#define SENSOR_TYPE                    5750
#define CUMULATIVE_ACTIVE_POWER        5805
#define POWER_FACTOR                   5820
#define ON_OFF                         5850
#define DIMMER                         5851
#define ON_TIME                        5852
#define MULTI_STATE_OUTPUT             5853
#define BUSY_TO_CLEAR_DELAY            5903
#define CLEAR_TO_BUSY_DELAY            5904

/*Some convenient alias for our application   */
#define IPSO_BUTTON                    IPSO_DIGITAL_INPUT
#define BUTTON_COUNTER                 DIGITAL_INPUT_COUNTER

/*Strings for human readable data points */
#define TEMPERATURE_VALUE_STR          "Temperature"
#define TEMPERATURE_MIN_VALUE_STR      "MIN_Temperature"
#define TEMPERATURE_MAX_VALUE_STR      "MAX_Temperature"

#define BUTTON_COUNTER_STR             "Button_Counter"

#define ACCELERATION_X_STR             "Acceleration_X"
#define ACCELERATION_Y_STR             "Acceleration_Y"
#define ACCELERATION_Z_STR             "Acceleration_Z"
#define ACCELERATION_MIN_STR           "MIN_Acceleration"
#define ACCELERATION_MAX_STR           "MAX_Acceleration"

#define MAGNETOMETER_X_STR             "Magnetometer_X"
#define MAGNETOMETER_Y_STR             "Magnetometer_Y"
#define MAGNETOMETER_Z_STR             "Magnetometer_Z"
#define MAGNETOMETER_MIN_STR           "MIN_Magnetometer"
#define MAGNETOMETER_MAX_STR           "MAX_Magnetometer"

#define HUMIDITY_VALUE_STR             "Humidity"
#define HUMIDITY_MIN_VALUE_STR         "MIN_Humidity"
#define HUMIDITY_MAX_VALUE_STR         "MAX_Humidity"

#define PRESSURE_VALUE_STR             "Pressure"
#define PRESSURE_MIN_VALUE_STR         "MIN_Pressure"
#define PRESSURE_MAX_VALUE_STR         "MAX_Pressure"

#define PRESENCE_STATE_STR             "Presence"

#endif /*LWM2M_DEFS*/
