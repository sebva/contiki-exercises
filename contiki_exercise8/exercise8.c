#include "contiki.h"
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "node-id.h"
#include <stdio.h>


struct timeMessage {
	clock_time_t time;
	unsigned short originator;
};

static struct timeMessage tmReceived;
static struct timeMessage tmSent;

void print_temperature_binary_to_float(uint16_t temp) {
	printf("%d.%d", (temp / 10 - 396) / 10, (temp / 10 - 396) % 10);
}


static struct ctimer leds_off_timer_send;

/* Timer callback turns off the blue led */
static void timerCallback_turnOffLeds()
{
  leds_off(LEDS_BLUE);
}

#define RUNICAST_CHANNEL     140
#define RUNICAST_CHANNEL_II  141

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "RTT using Rime Unicast Primitive");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/

static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from);
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
static clock_time_t rtt;
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  printf("unicast message received from %d.%d\n", from->u8[0], from->u8[1]);
  /* turn on blue led */
  leds_on(LEDS_BLUE);
  /* set the timer "leds_off_timer" to 1/8 second */
  ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

  packetbuf_copyto(&tmReceived);

  printf("time received = %d clock ticks", (uint16_t)tmReceived.time);
  printf(" = %d secs ", (uint16_t)tmReceived.time / CLOCK_SECOND);
  printf("%d millis ", (1000L * ((uint16_t)tmReceived.time  % CLOCK_SECOND)) / CLOCK_SECOND);
  printf("originator = %d\n", tmReceived.originator);
  printf("rssi = %d\n", (int) packetbuf_attr(PACKETBUF_ATTR_RSSI));


  if( tmReceived.originator != node_id ){
	  rimeaddr_t addr;
	  packetbuf_copyfrom(&tmReceived, sizeof(tmReceived));

	  addr.u8[0] = (node_id % 2 == 0 ? node_id+1: node_id-1);
	  addr.u8[1] = 0;
	  if(!rimeaddr_cmp(&addr, &rimeaddr_node_addr)) {
		  unicast_send(&uc, &addr);
	  }
  }
  else {
      printf("original sender was ME!\n");
	  rtt = clock_time();
	  rtt -= tmReceived.time;
	  printf("round trip time (ms): %d\n", (1000L *(uint16_t)rtt) / CLOCK_SECOND);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, RUNICAST_CHANNEL, &unicast_callbacks);

  SENSORS_ACTIVATE(button_sensor);

  while(1){
	  PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

	  /* specify the address of the unicast */
	  rimeaddr_t addr;

	  tmSent.time = clock_time();
	  tmSent.originator = node_id;
	  packetbuf_copyfrom(&tmSent, sizeof(tmSent));


	  /* in case I am node 50, choose 51 as destination.*/
      if(node_id % 2 == 0) {
          addr.u8[0] = node_id + 1;
      }
      /* In case I am node 51, choose 50, etc */
      else {
          addr.u8[0] = node_id - 1;
      }
	  addr.u8[1] = 0;
	  if(!rimeaddr_cmp(&addr, &rimeaddr_node_addr)) {
		  unicast_send(&uc, &addr);
	  }
	  printf("sending packet to %u\n", addr.u8[0]);
  }

  SENSORS_DEACTIVATE(button_sensor);


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
