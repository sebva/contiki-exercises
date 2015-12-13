
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

#define NETSTACK_RADIO NETSTACK_CONF_RADIO

#define T_AWAKE 	CLOCK_SECOND * 1
#define T_SLEEP 	CLOCK_SECOND * 4
#define T_WAIT_MAX 	CLOCK_SECOND * 1


void radio_wake_eventhandler();

// Static enumeration which holds the states
static enum {
	ON,
	WAITING,
	OFF
} state;

static struct ctimer timer;

void switch_radio_on() {
    printf("Switching radio ON\n"),
    NETSTACK_RADIO.on();
    leds_on(LEDS_BLUE);
    leds_off(LEDS_RED);
}

void switch_radio_off() {
    printf("Switching radio OFF\n"),
    NETSTACK_RADIO.off();
    leds_on(LEDS_RED);
    leds_off(LEDS_BLUE);
}

void schedule(uint16_t interval) {
    ctimer_set(&timer, interval, radio_wake_eventhandler, NULL);
}

//the event handler method
void radio_wake_eventhandler() {
    printf("Enter eventhandler\n");
	switch (state) {
	case ON:
	    if(!NETSTACK_RADIO.channel_clear()) {
	        state = WAITING;
	        // Should change to OFF earlier if channel is clear earlier than the timer
	        schedule(T_WAIT_MAX);
	        break;
        }
        // no break
	case WAITING:
	    switch_radio_off();
	    state = OFF;
	    schedule(T_SLEEP);
	    break;
	case OFF:
	    switch_radio_on();
	    state = ON;
	    schedule(T_AWAKE);
	    break;
	}
}

PROCESS(radio_wake_eventhandler_process, "Radio Wake Process based on event handler");
PROCESS_THREAD(radio_wake_eventhandler_process, ev, data)
{
	PROCESS_BEGIN();
	state = OFF;
	printf("Start radio_wake_eventhandler_process process\n");
	ctimer_set(&timer, T_SLEEP, radio_wake_eventhandler, NULL);
	PROCESS_END();
}

AUTOSTART_PROCESSES(&radio_wake_eventhandler_process);
