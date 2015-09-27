
/* Philipp Hurni, University of Bern, December 2013 	*/
/* Exercises for sensor networks lecture                */


#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include "node-id.h" /* get the variable node_id that holds the own node id */
#include "dev/leds.h"
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello World Timer Process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  //static struct etimer et;
  //etimer_set(&et, CLOCK_SECOND*4);
  //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  printf("Hello, world\n");
  PROCESS_END();
}

AUTOSTART_PROCESSES(&hello_world_process); // autostart processes (can also be more than one)


/* Exercise 1a: compile the Contiki OS and flash the node attached to the PC via USB with
 * the above process it usually works best with flashing when you are the superuser
 *
 * $ cd <exercise1-path>
 * $ make
 * $ make exercise1.upload
 *
 * then run the following command
 * $ make login
 *
 * you can now communicate with the sensor node over the USB interface
 * restart the sensor node with the reset button until you see
 * "Hello World" printed onto the screen
 */

/* Exercise 1b: uncomment the three lines with the timer, compile and flash it to the sensor.
 * Observe what happens
 */

/* Exercise 1c: alter the program such that the node prints "Hello World" infinitely to the screen,
 * with the delay of 4 seconds in between each statement
 */

/* Exercise 1d: alter the program such that the node stops printing "Hello World" after 10 times
 */

/* Exercise 1e: alter the program such that the node turns on the blue led in each loop,
 * keeps it turned on for 1 second and then continues.
 */

/* Exercise 1f (to be solved in ILIAS): separate the led blinking logic from the "Hello World"
 * printing logic: use two different processes, one that a) infinitely turns on and off the blue
 * LED (blink time = 1s, repeat interval=4s) and one that b) infinitely prints "Hello World"
 * with an interval of 10s
 *
 * The two processes shall be started when the node boots. The first (a) shall start immediately,
 * the second (b) shall start 30s after bootup.
 */
