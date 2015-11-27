#include "contiki.h"
#include "dev/leds.h"
#include "shell.h"
#include "serial-shell.h"
#include <stdio.h> /* For printf() */
#include <string.h>
#include "node-id.h"

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
	  PROCESS_BEGIN();
	  printf("Hello, world\n");
	  static struct etimer timer_blink;
	  static int i;
	  // Blink pattern every second
	  etimer_set(&timer_blink, CLOCK_SECOND);

	  for (i = 0; i < 20; i++)
	  {
	      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_blink));
	      etimer_reset(&timer_blink);
	      if (i % 2 == 0)
	          leds_on(LEDS_BLUE);
	      else
	          leds_off(LEDS_BLUE);
	  }
	  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
