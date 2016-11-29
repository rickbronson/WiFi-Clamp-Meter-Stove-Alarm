//////////////////////////////////////////////////
// Simple NTP client for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
//////////////////////////////////////////////////

#define DEBUG
#include <esp8266.h>
#include <time.h>

#include "ntp.h"
#include "main.h"

// list of major public servers http://tf.nist.gov/tf-cgi/servers.cgi
int ntp_index = 0;
#define IP4_SZ 4
uint8 ntp_server[][IP4_SZ] = {
	{ 131, 107, 13, 100 }, // microsoft
	{ 208, 75, 89, 4 },  /* 0.us.pool.ntp.org */
	{ 64, 71, 128, 26 },
	{ 107, 170, 242, 27 },
	{ 173, 255, 246, 13 },
	}; 

static void (*time_cb)(struct tm *) = NULL;
static os_timer_t ntp_timeout;
static struct espconn *pCon = 0;

static void ICACHE_FLASH_ATTR ntp_udp_timeout(void *arg) {
	
	os_timer_disarm(&ntp_timeout);
	if (time_cb)
		time_cb(NULL);
	os_printf("Error: ntp timout\r\n");

	// clean up connection
	if (pCon) {
		espconn_delete(pCon);
		os_free(pCon->proto.udp);
		os_free(pCon);
		pCon = 0;
	}
}

static void ICACHE_FLASH_ATTR ntp_udp_recv(void *arg, char *pdata, unsigned short len) {
	
	struct tm *dt;
	time_t timestamp;
	ntp_t *ntp;

	os_timer_disarm(&ntp_timeout);

	// extract ntp time
	ntp = (ntp_t*)pdata;
	timestamp = ntp->trans_time[0] << 24 | ntp->trans_time[1] << 16 |ntp->trans_time[2] << 8 | ntp->trans_time[3];
	// convert to unix time
	timestamp -= 2208988800 + (GMT_TIME_SHIFT * 60 * 60);  /* adjust to PST, 8 hours from GMT */
	// create tm struct
	dt = gmtime(&timestamp);

	// do something with it, like setting an rtc
	if (time_cb)
		time_cb(dt);

	// clean up connection
	if (pCon) {
		espconn_delete(pCon);
		os_free(pCon->proto.udp);
		os_free(pCon);
		pCon = 0;
	}
}

void ICACHE_FLASH_ATTR ntp_get_time(void (*time_callback)(struct tm *))
	{
	ntp_t ntp;

	// set up the udp "connection"
	pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	time_cb = time_callback;
	pCon->type = ESPCONN_UDP;
	pCon->state = ESPCONN_NONE;
	pCon->proto.udp = (esp_udp*)os_zalloc(sizeof(esp_udp));
	pCon->proto.udp->local_port = espconn_port();
	pCon->proto.udp->remote_port = 123;
	os_memcpy(pCon->proto.udp->remote_ip, ntp_server[ntp_index++], IP4_SZ);
	debug("using %d.%d.%d.%d\n", pCon->proto.udp->remote_ip[0], pCon->proto.udp->remote_ip[1], pCon->proto.udp->remote_ip[2], pCon->proto.udp->remote_ip[3]);
	ntp_index = ntp_index % ARRAY_SIZE(ntp_server);
	// create a really simple ntp request packet
	os_memset(&ntp, 0, sizeof(ntp_t));
	ntp.options = 0b00100011; // leap = 0, version = 4, mode = 3 (client)

	// set timeout timer
	os_timer_disarm(&ntp_timeout);
	os_timer_setfn(&ntp_timeout, (os_timer_func_t*)ntp_udp_timeout, pCon);
	os_timer_arm(&ntp_timeout, NTP_TIMEOUT_MS, 0);

	// send the ntp request
	espconn_create(pCon);
	espconn_regist_recvcb(pCon, ntp_udp_recv);
	espconn_sent(pCon, (uint8*)&ntp, sizeof(ntp_t));
}
