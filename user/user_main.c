/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

/*
This is example code for the esphttpd library. It's a small-ish demo showing off 
the server, including WiFi connection management capabilities, some IO and
some pictures of cats.
*/

#define DEBUG
#define SPEEDUP_MODE 1  /* set to > 1 for speed up, usually by x3600 */
#include <esp8266.h>
#include <time.h>
#include <uart_hw.h>
#include <time.h>
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "stdout.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"
#include "cgiwebsocket.h"
#include "ntp.h"
#include "main.h"

char *getmytime(void)
	{
	struct MAIN_DATA *p_data = &main_data;

	return (p_data->buf);
	}

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}

struct MAIN_DATA main_data =
	{
	.resetCnt = 0,
	.loop_cntr = 0,
	.last_btn = 0,
	.ntp_delay = 2,
	.pRcvHead = main_data.RcvBuff,
	.pRcvTail = main_data.RcvBuff,
	.threshold = ADC_THRESHOLD,
	.adc_index = 0,
	};

/* callback from NTP, if dt=NULL then timeout occured */
static void time_cb(struct tm *dt)
	{
	struct MAIN_DATA *p_data = &main_data;

	if (dt)  /* if no timeout */
		{
		memcpy(&p_data->dt, dt, sizeof(*dt));

		if (!(p_data->flags & DOOR_FLAGS_GOT_NTP))  /* first time we got ntp date? */
			{
			p_data->flags |= DOOR_FLAGS_GOT_NTP;
			}
		}
	else  /* if timeout then schedule one later */
		p_data->ntp_delay = 2;  /* in sec's */
	}

static void stove_check(struct MAIN_DATA *p_data)  /* runs once every sec */
	{
	int cntr, start, head_dx, old_val, val, total, no_activity_cntr, cycles, event = 0;

  /* go over all samples, total = number of sec's over threshold of array */
	start = ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE;
	no_activity_cntr = cycles = old_val = 0;
	total = 0;  /* number of sec's there was current draw over 120 min's */
	head_dx = p_data->head_dx + BITS_PER_BYTE;  /* start with the oldest one */
	if (head_dx >= ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE)
		head_dx -= ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE;
	for (cntr = BITS_PER_BYTE; cntr < ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE; cntr++)
		{
		if (p_data->samplebuf[head_dx / BITS_PER_BYTE] & (1 << (head_dx % BITS_PER_BYTE)))
			val = 1;
		else
			val = 0;
		total += val;
		if ((val && !old_val) || (!val && old_val))
			{
			cycles++;  /* count half cycles */
			old_val = val;
			}
		if (val)
			no_activity_cntr = 0;
		else
			{
			if (++no_activity_cntr >= 30 * SECS_PER_MIN)  /* 30 min's with no activity? */
				{
				total = 0;  /* start over */
				cycles = 0;
				}
			}
		if (val && (start == ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE))
			start = cntr;  /* set start point */
		if (++head_dx >= ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE)
			head_dx = 0;
		}
	if ((total > (ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE) / (100 / ADC_PERCENT_THRESHOLD)) || /* over percent threshold? */
		cycles > MAX_HALF_CYCLES)  /* more than max cycles? */
		if (ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE - start > SECS_PER_HR * 1.5)  /* did the first occurance happen over 1.5 hours ago? */
			if (p_data->adc_result > p_data->threshold)  /* over threshold at this time? */
				event = 1;

#ifdef DEBUG_APP
	debug("per=%3d noact=%3d cycles=%3d start=%3d adc=%2d state=%d \r\n", 
		total * 100 / (ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE), no_activity_cntr, cycles, start, (int) p_data->adc_result, (int) p_data->adc_state);
#endif

	switch (p_data->adc_state)
		{
		default:  /* fall thru */
		case ADC_STATE_INIT:  /* initial state */
			if (event)
				{
				p_data->adc_state = ADC_STATE_ALARM_ON;
				p_data->adc_sec_timer = 0;
				}
			break;

		case ADC_STATE_ALARM_ON:  /* alarming, beeping N times */
			p_data->adc_sec_timer++;
			if (p_data->adc_sec_timer & 1)
	/* set, clear, enable, disable */
				gpio_output_set(0 << STOVE_ALARM_BEEP, 1 << STOVE_ALARM_BEEP, 1 << STOVE_ALARM_BEEP, 0);  /* set to low, sound alarm */
			else
				{
				gpio_output_set(1 << STOVE_ALARM_BEEP, 0 << STOVE_ALARM_BEEP, 1 << STOVE_ALARM_BEEP, 0);  /* set to high  */
				if (p_data->adc_sec_timer >= ADC_ALARM_ON_CYCLES * 2)
					{
					p_data->adc_state = ADC_STATE_WAITING;  /* go wait for awhile until alarming again */
					p_data->adc_sec_timer = 0;
					}
				}
			break;

		case ADC_STATE_WAITING:  /* we beeped, wait for wait time */
			if (++p_data->adc_sec_timer >= ADC_ALARM_ON_WAIT_SECS)
				{
				if (event)
					p_data->adc_state = ADC_STATE_ALARM_ON;
				else
					p_data->adc_state = ADC_STATE_INIT;
				p_data->adc_sec_timer = 0;
				}
			break;

		}
	if (!p_data->adc_state && p_data->adc_result > p_data->threshold)
		currAlarmState = 3;  /* show as stove on */
	else
		currAlarmState = p_data->adc_state;

	}


static char *mode_lut[] = { "NULL", "STA", "AP", "STA+AP", };

static void ICACHE_FLASH_ATTR main_loop(struct MAIN_DATA *p_data)
 {
 int btn, cntr, temp, adc_accum;
 int status = wifi_station_get_connect_status();
 struct rst_info *rst;

 gpio_output_set(0, 0, 0, 1 << BTNGPIO);
 btn = GPIO_INPUT_GET(BTNGPIO);  /* read switch AP button */
 PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);

	temp = system_adc_read();  /* take samples over time */
	p_data->adc_buf[p_data->adc_index++] = temp;
	if (p_data->adc_index >= TICKS_PER_SEC)
		p_data->adc_index = TICKS_PER_SEC - 1;  /* cap at max */
	if (temp > p_data->adc_max)
		p_data->adc_max = temp;
	p_data->sys_secs = system_get_time();  /* use SYS instead of timer, in microseconds */
	if (p_data->sys_secs - p_data->sys_secs_sav >= 1000000)
		{  /* only do this once a second */
			/* take an average once a sec */
		for (cntr = 0, temp = 0, adc_accum = 0; cntr < p_data->adc_index; cntr++)
			if (p_data->adc_buf[cntr] > p_data->adc_max * 2 / 3)
				{
				adc_accum += p_data->adc_buf[cntr];
				temp++;  /* count ones over 66% of max */
				}
		if (temp)
			p_data->adc_result = adc_accum / temp;  /* take average */
		else
			p_data->adc_result = 0;

		rst = system_get_rst_info();
		os_sprintf(p_data->buf, "loop=%4d, adc: result=%d over66=%d cnt=%d max=%d date=%d/%d/%d %d:%02d:%02d heap=%dk rssi=%d mode=%s boot=%d\r\n",
			p_data->loop_cntr++, (int) p_data->adc_result, temp, (int) p_data->adc_index, (int) p_data->adc_max,
			p_data->dt.tm_mon + 1, p_data->dt.tm_mday, p_data->dt.tm_year + 1900, p_data->dt.tm_hour, p_data->dt.tm_min, p_data->dt.tm_sec,
			system_get_free_heap_size() / 1000,
			wifi_station_get_rssi(),
			mode_lut[wifi_get_opmode()],
			rst->reason );
#ifdef DEBUG_APP
		debug("%s", p_data->buf);  /* debug only */
#endif

		p_data->adc_max = 0;
		p_data->adc_index = 0;  /* start over */
		if (p_data->adc_result > p_data->threshold)  /* if over then set a bit */
			p_data->samplebuf[p_data->head_dx / BITS_PER_BYTE] |= (1 << (p_data->head_dx % BITS_PER_BYTE));
		if (++p_data->head_dx >= ARRAY_SIZE(p_data->samplebuf) * BITS_PER_BYTE)
			p_data->head_dx = 0;  /* wrap head */
		if (p_data->head_dx % BITS_PER_BYTE == 0)  /* did we go to the next byte? */
			p_data->samplebuf[p_data->head_dx / BITS_PER_BYTE] = 0;  /* start over */

		stove_check(p_data);

		if (++p_data->dt.tm_sec >= SECS_PER_MIN)
			{  /* one minute passed */
			p_data->dt.tm_sec = 0;
			if (++p_data->dt.tm_min >= MINS_PER_HR)
				{  /* an hour has passed */
				p_data->dt.tm_min = 0;
				if (++p_data->dt.tm_hour >= HRS_PER_DAY)
					{  /* a day has passed */
					p_data->dt.tm_hour = 0;
					p_data->dt.tm_wday = (p_data->dt.tm_wday + 1) % 7;
					}
				}
			}
		p_data->sys_secs_sav += 1000000;
		}
	if (p_data->last_btn != btn)  /* button changed? */
		{
		if (!btn)  /* button pushed? */
			{
			memset(p_data->samplebuf, 0, sizeof(p_data->samplebuf));  /* start over */
			p_data->adc_state = ADC_STATE_INIT;
			}
		p_data->last_btn = btn;  /* save last button press */
		}
	if (status == STATION_GOT_IP && p_data->ntp_delay && --p_data->ntp_delay <= 0)
		ntp_get_time(&time_cb);  /* get the time */

	if (!btn) {
		p_data->resetCnt++;
	} else {
		if (p_data->resetCnt >= (5 * TICKS_PER_SEC)) { /* 5 sec pressed */
			wifi_station_disconnect();
			wifi_set_opmode(STATIONAP_MODE); /* reset to AP+STA mode */
			debug("Reset to AP mode. Restarting system...\n");
			system_restart();
		}
		p_data->resetCnt = 0;
	}
}

#ifdef ESPFS_POS
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_ESPFS,
	.fw1Pos=ESPFS_POS,
	.fw2Pos=0,
	.fwSize=ESPFS_SIZE,
};
#define INCLUDE_FLASH_FNS
#endif
#ifdef OTA_FLASH_SIZE_K
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
	.fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
};
#define INCLUDE_FLASH_FNS
#endif

/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.tpl"},
	{"/flash.bin", cgiReadFlash, NULL},
	{"/index.tpl", cgiEspFsTemplate, tplCounter},
	{"/flash/download", cgiReadFlash, NULL},
#ifdef INCLUDE_FLASH_FNS
	{"/flash/next", cgiGetFirmwareNext, &uploadParams},
	{"/flash/upload", cgiUploadFirmware, &uploadParams},
#endif
	{"/flash/reboot", cgiRebootFirmware, NULL},

	/* Routines to make the /wifi URL and everything beneath it work. */

/* Enable the line below to protect the WiFi configuration with an username/password combo. */
//	{"/wifi/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
	{"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},
	{"*", cgiEspFsHook, NULL}, /* Catch-all cgi function for the filesystem */
	{NULL, NULL, NULL}
};

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
	struct MAIN_DATA *p_data = &main_data;

	stdinoutInit(BIT_RATE_115200);
	ioInit();
	captdnsInit();

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif
	httpdInit(builtInUrls, 80);
	os_timer_disarm(&p_data->mainTimer);
	os_timer_setfn(&p_data->mainTimer, (os_timer_func_t *) main_loop, p_data);
	os_timer_arm(&p_data->mainTimer, 1000 / TICKS_PER_SEC, 1);
	p_data->sys_secs_sav = system_get_time();  /* use SYS instead of of timer */
	debug("\nReady\n");
}

void user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
}
