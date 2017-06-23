/**
 ******************************************************************************
 * @file    enum.c
 * @author  Central LAB
 * @version V2.1.0
 * @date    17-May-2016
 * @brief   Error Codes for SPWF04SA
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

#define NUM_ERROR_STATE 119

typedef enum
{
	NO_ERROR = 0,

	/* Index 1 to 9 */
	NO_CMD,
	MISSING_ARGS,
	ph3,							/* use some placeholder for unused error codes */
	VAR_NOT_FOUND,
	ph5,
	TOO_MANY_ARGS,
	INVALID_ARGS,
	ph8, ph9,

	/* Index 10 to 19 */
	ph10, ph11, ph12, ph13, ph14, ph15, ph16, ph17,
	RADIO_OFF,
	IO_DIR,

	/* Index 20 to 29 */
	GPIO_WRONG,
	GPIO_SLEEP,
	GPIO_WPS,
	GPIO_BLINK,
	GPIO_NCSS,
	VOLTAGE,
	FREQUENCY,
	DUTY_CYCLE,
	PWM_STOP,
	UNMOUNT,

	/* Index 30 to 39 */
	ph30, ph31,
	TOO_BIG,
	ZERO,
	ph34, ph35, ph36, ph37, ph38,
	PIN_WPS,

	/* Index 40 to 49 */
	SCAN_IN_PROGRESS,
	SCAN_FAIL,
	HW_BUSY,
	HW_STARTING,
	PWM_SETTING,
	ph45, ph46,
	ADC_EOC,
	ph48, ph49,

	/* Index 50 to 59 */
	FILESYSTEM,
	FILE_OPEN,
	FILE_SEEK,
	FILE_READ,
	FILE_CLOSE,
	FILE_RENAME,
	FILE_DELETE,
	DIR_OPEN,
	DIR_READ,
	VOLUME_UMOUNT,

	/* Index 60 to 69 */
	IP_READY,
	RENEW_NOT_ALLOWED,
	RENEW_FAILED,
	DNS_BUSY,
	DNS_START,
	DNS_FAIL,
	NTP_NOT_ALLOWED,
	ph67, ph68, ph69,

	/* Index 70 to 79 */
	ph70, ph71,
	CLOSE_SOCK,
	PORT_BUSY,
	OPEN_SOCK,
	TOO_MANY_SOCK,
	BAD_SOCK_ID,
	PENDING_DATA,
	NOT_CONN_SOCK,
	WRITE_SOCK,

	/* Index 80 to 89 */
	HTTP_INSTANCE,
	HTTP_BUSY,
	HTTP_RELEASE,
	ph83, ph84, ph85, ph86, ph87, ph88, ph89,

	/* Index 90 to 99 */
	ph90,
	LOW_MEMORY,
	ph92, ph93, ph94, ph95, ph96, ph97, ph98,
	SCAN_ABORT,

	/* Index 100 to 109 */
	READ_FLASH,
	WRITE_FLASH,
	ph102, ph103, ph104, ph105, ph106, ph107, ph108, ph109,

	/* Index 110 to 119 */
	MFG_TEST,
	REQUEST,
	ph112, ph113, ph114, ph115, ph116, ph117, ph118, ph119,
} ConsoleErr;

#define UNUSED_ERROR_STRING " "
#ifdef SPWF04
static const char *err_list[] = {
	/* Index 0: No error */
	"AT.OK",

	/* Index 1 to 9 */
	"Command not found",
	"Missing argument(s)",
	UNUSED_ERROR_STRING,
	"Variable not found",
	UNUSED_ERROR_STRING,
	"Too many argument(s)",
	"Invalid argument(s)",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,

	/* Index 10 to 19 */
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, 
  "WiFi BSS Regained",
	"WiFi Signal Low",
  UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	"Radio not running",
	"Direction must be 'in' or 'out'",

	/* Index 20 to 29 */
	"Invalid GPIO Num (0-15)",
	"Cannot use GPIO6 when sleep_enabled",
	"Cannot use GPIO7. Reserved for WPS feature",
	"Cannot use GPIO10 when blink_led",
	"Cannot use GPIO3 when spi_console",
	"Output voltage not allowed",
	"Frequency not supported",
	"Duty cycle not supported",
	"PWM not running",
	"Volume not mounted",

	/* Index 30 to 39 */
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	"Argument exceeds allowed size",
	"Argument evaluated to zero",
	"WiFi Scan Complete",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
  "WiFi Powered Down", 
  "HW in miniAP mode",
	"PIN needs to be 8 digits",

	/* Index 40 to 49 */
	"Scan in Progress",
	"Scan Failed",
	"Wait for Hardware Busy",
	"Wait for Hardware Starting",
	"Unable to complete PWM setting",
	UNUSED_ERROR_STRING, 
  "WPA Crunching PSK",
	"ADC conversion failed",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,

	/* Index 50 to 59 */
	"Unable to access RAM filesystem",
	"Unable to open file",
	"Unable to seek file",
	"Unable to read file",
	"Unable to close file",
	"Unable to rename file",
	"Unable to delete file",
	"Unable to open directory",
	"Unable to read directory",
	"Unable to unmount volume",

	/* Index 60 to 69 */
	"IP not ready to send",
	"Cannot renew IP address",
	"Failed to renew IP address",
	"DNS busy",
	"DNS start failed",
	"DNS address failure",
	"Cannot update DateTime",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,

	/* Index 70 to 79 */
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	"Closed socket",
	"Port already opened",
	"Failed to open socket",
	"Too many sockets",
	"Illegal Socket ID",
	"Pending data",
	"Socket not connected",
	"Write failed",

	/* Index 80 to 89 */
	"No valid HTTP Client Instance ID",
	"HTTP Client busy",
	"Failed to release HTTP Client Instance",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,

	/* Index 90 to 99 */
	UNUSED_ERROR_STRING,
	"Low Memory",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	"Scan Aborted",

	/* Index 100 to 109 */
	"Failed to read flash",
	"Failed to write flash",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,

	/* Index 110 to 119 */
	"Mfg test failed",
	"Request failed",
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
	UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING, UNUSED_ERROR_STRING,
};
#endif
