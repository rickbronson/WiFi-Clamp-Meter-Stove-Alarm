/*
Defs for the garage door opener
*/

#define STOVE_ALARM_BEEP 2  /* VCC -> AC_ON_ -> 220 ohm -> GPIO2 */
#define BTNGPIO 0  /* button to gnd w/10K pullup, keep GPIO0 for >5 seconds. The module will reboot into
its STA+AP mode.  */

#define AP_MODE_BTN 0  /* button to force back to AP mode */
#define BLUE_LED 1  /* blue LED on esp-12 on TX line */
#define GMT_TIME_SHIFT 8  /* number of hours to shift from GMT */

#define MAX_HALF_CYCLES 80
#define SECS_PER_MIN 60
#define MINS_PER_HR 60
#define HRS_PER_DAY 24
#define SECS_PER_HR (MINS_PER_HR * SECS_PER_MIN)

typedef enum {
	ADC_STATE_INIT,  /* initial state, ground zero */
	ADC_STATE_ALARM_ON,  /* we are alarming */
	ADC_STATE_WAITING,  /* waiting between alarms */
	}adc_state_t;

struct MAIN_DATA
	{
	char flags;
#define DOOR_FLAGS_GOT_NTP 1
	char ntp_delay;
	int resetCnt;
	int loop_cntr;
	int last_btn;
	int sys_secs_sav;
	int sys_secs;
	struct tm dt;
	ETSTimer mainTimer;
#define RCV_BUFF_SZ 100
	uint8 RcvBuff[RCV_BUFF_SZ];
	uint8 *pRcvHead;
	uint8 *pRcvTail;

	int total;  /* used for calculating percent */
#define TICKS_PER_SEC 59  /* speed at which main_loop runs, make undivisible by line freq */
	uint16_t adc_buf[TICKS_PER_SEC];
	uint32_t adc_index;
	uint32_t adc_max;  /* about 465-530 for 0W, 786-810 for 100W, 984-1018 : 1500W */
	uint32_t adc_result;
// #define DEBUG_APP
#ifdef DEBUG_APP
#define ADC_THRESHOLD 200  /* debug only */
#else
#define ADC_THRESHOLD 850  /* set for > 100 watts */
#endif
	uint32_t threshold;
	uint16_t adc_sec_timer;
#define ADC_ALARM_ON_WAIT_SECS (5 * SECS_PER_MIN)  /* 5 min's */
#define ADC_ALARM_ON_TIME 1 /* 1 sec */
#define ADC_ALARM_OFF_TIME 1 /* 1 sec */
#define ADC_ALARM_ON_CYCLES 7 /* 7 times */
	uint16_t head_dx;  /* head index */
#define BITS_PER_BYTE 8
	uint8_t samplebuf[2 * SECS_PER_HR / BITS_PER_BYTE];  /* each bit is equal to one sec, set for 2 hours */
#define ADC_PERCENT_THRESHOLD 7  /* percent of ticks threshold */ 
	adc_state_t  adc_state;
	char buf[200];
	};

extern struct MAIN_DATA main_data;
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

extern int currAlarmState;
char *getmytime(void);
