/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include <time.h>
#include "cgi.h"
#include "io.h"
#include "main.h"


//cause I can't be bothered to write an ioGetLed()
int currAlarmState = 0;

//Template code for the alarm page.
int ICACHE_FLASH_ATTR tplAlarm(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}

#define BTNGPIO 0 /* GPIO0 */

static char *alarm_state_lut[] = { "STOVE_OFF", "ALARM_ON_BEEPING", "ALARM_ON_WAITING", "STOVE_ON", };

//Template code for the counter on the index page.
int ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[200];
	if (token==NULL) return HTTPD_CGI_DONE;

	if (os_strcmp(token, "status")==0) {
		os_sprintf(buff, "%s", getmytime());
	}
	if (os_strcmp(token, "alarmstate") == 0) {
		if (currAlarmState < ARRAY_SIZE(alarm_state_lut))
			os_strcpy(buff, alarm_state_lut[currAlarmState]);
		else
			os_strcpy(buff, "Unknown");
		}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
