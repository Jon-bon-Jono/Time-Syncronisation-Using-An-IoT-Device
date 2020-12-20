//By Jonathan Williams
//Node maintains its local utc time in the global variable localutctime
//When it receives a timeseries packet it updates localutctime
//The localutctime ticks (increments by 1) every second using a ctimer utc_tick
//The node replies to timesync.py every 10 seconds using a ctimer utc_send
//The blinking pattern is determined by blink_freq (int between 1 and 10) and blink_colours (int between 0 and 2)
   //Every time a timeseries packet is received, if the number of blinks is a multiple of 10 the pattern is randomly changed using the utc time received as a seed. This ensures both nodes in the network randomly select the same pattern


#include <stdlib.h>
#include <stdio.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"
#include <string.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MAX_PAYLOAD_LEN 120

static struct uip_udp_conn *server_conn;


static struct ctimer send_timer;
static struct ctimer tick_timer;
static struct ctimer blink_timer;
static void tick_utc(void *v);
static void send_utc(void *v);
static void blink_utc(void *v);

//global for this nodes' local time
int localutctime = 0;
//every 2 seconds
int blink_freq = 2;
int num_blinks = 0;
//0-green, 1-red, 3-all
int blink_colours = 0;

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&resolv_process,&udp_server_process);


static void
tcpip_handler(void)
{
  if(uip_newdata()) {
    ((char *)uip_appdata)[uip_datalen()] = 0;
	//update global
	int* timestamp = (int*) uip_appdata;
	localutctime = *timestamp;
    //printf("Server received: %d -- \n", *timestamp);
	//every 10 blinks, change the pattern and frequency
	//uses timestamp as the seed
	if(num_blinks%10 == 0){
		leds_off(LEDS_ALL);			
		blink_freq = (*timestamp)%10+1;
		blink_colours = (*timestamp)%3;
		printf("BLINKING EVERY %d SECONDS, COLOUR %d\n",blink_freq,blink_colours);
	}
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  
#if UIP_CONF_ROUTER
  uip_ipaddr_t ipaddr;
#endif /* UIP_CONF_ROUTER */

  PROCESS_BEGIN();
  PRINTF("UDP server started\n\r");

#if RESOLV_CONF_SUPPORTS_MDNS
  resolv_set_hostname("contiki-udp-server");
#endif

#if UIP_CONF_ROUTER
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif /* UIP_CONF_ROUTER */

  //Create UDP socket and bind to port 3000
  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));
  printf("Blinking every %d seconds colour %d\n",blink_freq,blink_colours);
  //set ctimer processes
  ctimer_set(&send_timer,10*CLOCK_SECOND,send_utc,NULL);
  ctimer_set(&tick_timer,CLOCK_SECOND,tick_utc,NULL);
  ctimer_set(&blink_timer,CLOCK_SECOND,blink_utc,NULL);
  while(1) {
    PROCESS_YIELD();
	//Wait for tcipip event to occur
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}

//increment localutctime every second
static void tick_utc(void *v) {
  localutctime+=1;
  ctimer_reset(&tick_timer);
}
//blink every blink_freq seconds
static void blink_utc(void *v){
	if(localutctime % blink_freq==0){
		if(blink_colours==0){
			leds_toggle(LEDS_GREEN);
		}else if(blink_colours==1){
			leds_toggle(LEDS_RED);
		}else if(blink_colours==2){
			leds_toggle(LEDS_ALL);
		}
		num_blinks++;
	}
	ctimer_reset(&blink_timer);
}

//send UTC every 10 seconds
static void send_utc(void *v) {
  char buf[32];
  printf("Sending UTC:%d -- \n",localutctime);
  sprintf(buf, "%d", localutctime);
  uip_udp_packet_sendto(server_conn, buf, strlen(buf), &UIP_IP_BUF->srcipaddr, UIP_HTONS(3001));
  ctimer_reset(&send_timer);
}
/*---------------------------------------------------------------------------*/
