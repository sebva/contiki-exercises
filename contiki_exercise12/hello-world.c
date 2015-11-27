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
	  PROCESS_END();
}



/*---------------------------------------------------------------------------*/
