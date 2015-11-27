/* Philipp Hurni, University of Bern, January 2014 	                    */
/* Exercises for sensor networks lecture                                */
/* File adapted from $CONTIKI/examples/antelope/shell/shell-db.c		*/


#include <stdio.h>
#include "contiki.h"
#include "dev/serial-line.h"
#include "dev/sht11-sensor.h"
#include "dev/light-sensor.h"
#include "antelope.h"


/*---------------------------------------------------------------------------*/
static int get_light(void) {
return 10 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC) / 7;
}
/*---------------------------------------------------------------------------*/
static int get_temp(void) {
return ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
}

static int light;
static int temp;
static db_handle_t handleSense;
static db_handle_t handle;
static db_result_t result;

#define INTERVAL 10

PROCESS(sense_process, "Sense Process");
PROCESS_THREAD(sense_process, ev, data) {
	static struct etimer timer;
	PROCESS_BEGIN();

	etimer_set(&timer, CLOCK_SECOND * INTERVAL);
	SENSORS_ACTIVATE(light_sensor);
	SENSORS_ACTIVATE(sht11_sensor);
	static int i=INTERVAL;
	static char query[50];

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
		etimer_reset(&timer);

		sprintf(query, "INSERT (%d, %d, %d) INTO samples;", i, get_light(), get_temp());
		result = db_query(&handle, query);
		printf("Inserting sensor values: %s\n", db_get_result_message(result));

		i+=INTERVAL;
	}

	PROCESS_END();
}


PROCESS(db_shell, "DB shell");
PROCESS_THREAD(db_shell, ev, data)
{
  static tuple_id_t matching;
  static tuple_id_t processed;
  static int i;

  PROCESS_BEGIN();

  db_init();

  printf("Initializing DB\n");
  result = db_query(&handle, "REMOVE RELATION samples;");
  printf("Query 1: %s\n", db_get_result_message(result));
  result = db_query(&handle, "CREATE RELATION samples;");
  printf("Query 2: %s\n", db_get_result_message(result));
  result = db_query(&handle, "CREATE ATTRIBUTE time DOMAIN INT IN samples;");
  printf("Query 3: %s\n", db_get_result_message(result));
  result = db_query(&handle, "CREATE ATTRIBUTE light DOMAIN INT IN samples;");
  printf("Query 4: %s\n", db_get_result_message(result));
  result = db_query(&handle, "CREATE ATTRIBUTE temp  DOMAIN INT IN samples;");
  printf("Query 5: %s\n", db_get_result_message(result));
  result = db_query(&handle, "CREATE INDEX samples.time TYPE INLINE;");
  printf("Query 6: %s\n", db_get_result_message(result));

  printf("Ready to process queries\n");


  for(;;) {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);

    result = db_query(&handle, data);
    if(DB_ERROR(result)) {
      printf("Query \"%s\" failed: %s\n",
             (char *)data, db_get_result_message(result));
      db_free(&handle);
      continue;
    }

    if(!db_processing(&handle)) {
      printf("OK\n");
      continue;
    }

    db_print_header(&handle);

    matching = 0;
    processed = 0;

    while(db_processing(&handle)) {
      PROCESS_PAUSE();
      result = db_process(&handle);
      switch(result) {
      case DB_GOT_ROW:
        /* The processed tuple matched the condition in the query. */
        matching++;
        processed++;
        db_print_tuple(&handle);
        break;
      case DB_OK:
        /* A tuple was processed, but did not match the condition. */
        processed++;
        continue;
      case DB_FINISHED:
        /* The processing has finished. Wait for a new command. */
        printf("[%ld tuples returned; %ld tuples processed]\n",
               (long)matching, (long)processed);
        printf("OK\n");
      default:
        if(DB_ERROR(result)) {
          printf("Processing error: %s\n", db_get_result_message(result));      
        }
        db_free(&handle);
        break;
      }
    }
  }

  PROCESS_END();
}
AUTOSTART_PROCESSES(&db_shell, &sense_process);

/* Exercise 10: flash the program to your nodes and observe what happens
 * The program starts two protothreads. One is a database client, the other is a process with
 * a timer firing each 10 seconds
 *
 * In the process PROCESS_THREAD(db_shell, ev, data), a database schema is initialized.
 * A table with attributes time, light and temp is created.
 *
 * Start by inserting a tuple here by hand, entering the following:
 *
 * INSERT (10, 170, 28) INTO samples;
 * INSERT (20, 180, 25) INTO samples;
 *
 * Then, display the database contents
 *
 * SELECT time, light, temp from samples;
 *
 * Your tasks:
 *
 * a) In the process PROCESS(sense_process, "Sense Process"), the timer fires each 10s.
 *
 * In this code, add an insert statement such that the sensor node saves each 10s the current value of the
 * variable 'i' (raw timestamp), the light and temperature value into the database.
 *
 * Test your code by checking whether the database contents grows
 *
 * Paste your code into ILIAS. We will test it
 *
 * b) Find the maximum and minimum values of the database after a couple of minutes using SQL statements
 * Paste those statements into ILIAS
 *
 */
