
/* Philipp Hurni, University of Bern, August 2015 	    */
/* Exercises for sensor networks lecture                */


#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include "node-id.h" /* get the variable node_id that holds the own node id */
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "dev/cc2420_const.h"

#include "dev/radio.h"
/*---------------------------------------------------------------------------*/
PROCESS(radio_wake_process, "Radio Wake Process");
/*---------------------------------------------------------------------------*/

#define NETSTACK_RADIO NETSTACK_CONF_RADIO

#define T_AWAKE 	CLOCK_SECOND * 1
#define T_SLEEP 	CLOCK_SECOND * 4
#define T_WAIT_MAX 	CLOCK_SECOND * 1

static struct etimer timer;
static struct etimer wait_timer;

PROCESS_THREAD(radio_wake_process, ev, data)
{
	PROCESS_BEGIN();
	while(1) {
		NETSTACK_RADIO.on();
		leds_on(LEDS_BLUE);
		leds_off(LEDS_RED);
		etimer_set(&timer, T_AWAKE);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		etimer_set(&timer, T_SLEEP);
		if(!NETSTACK_RADIO.channel_clear()) {
			etimer_set(&wait_timer, T_WAIT_MAX);
			PROCESS_WAIT_EVENT_UNTIL(NETSTACK_RADIO.channel_clear() || etimer_expired(&wait_timer));
		}

		NETSTACK_RADIO.off();
		leds_off(LEDS_BLUE);
		leds_on(LEDS_RED);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
	}
	PROCESS_END();
}

/*** Static enumeration which holds the states

static enum {
	ON,
	WAITING,
	OFF
} state;

static struct ctimer timer;

//the event handler method

void radio_wake_eventhandler() {
	switch (state) {
		//based on the finite state model, implement the radio wake process using an event handler
		//HINT: setting a timer without blocking can be achieved using the function
		//      ctimer_set(&timer, TIMEINTERVAL, radio_wake_eventhandler, NULL);
	}
}

PROCESS(radio_wake_eventhandler_process, "Radio Wake Process based on event handler");
PROCESS_THREAD(radio_wake_eventhandler_process, ev, data)
{
	PROCESS_BEGIN();
	PROCESS_END();
}

*/

AUTOSTART_PROCESSES(&radio_wake_process);
