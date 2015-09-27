
/* Philipp Hurni, University of Bern, December 2013     */
/* Exercises for sensor networks lecture                */

#include <stdio.h> /* For printf() */
#include <string.h>
#include "dev/button-sensor.h" /* for the button sensor */
#include "dev/sht11-sensor.h" /* for the temperature sensor */
#include "node-id.h" /* get the variable node_id that holds the own node id */
#include "dev/leds.h"
/*---------------------------------------------------------------------------*/
PROCESS(button_press_process, "Button Press Process");
/*---------------------------------------------------------------------------*/

void print_temperature_binary_to_float(uint16_t temp) {
	printf("%d.%d\n", (temp / 10 - 396) / 10, (temp / 10 - 396) % 10);
}


PROCESS_THREAD(button_press_process, ev, data)
{
  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

    leds_on(LEDS_BLUE);

    printf("Temperature: ");
    SENSORS_ACTIVATE(sht11_sensor);
    print_temperature_binary_to_float(sht11_sensor.value(SHT11_SENSOR_TEMP));
    SENSORS_DEACTIVATE(sht11_sensor);

    leds_off(LEDS_BLUE);
  }

  PROCESS_END();
}


/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&button_press_process);
/*---------------------------------------------------------------------------*/


/* Exercise 2a: compile and run the program. press the button, observe what it does with
 * the serial interface
 */

/* Exercise 2b: alter the program such that whenever the button is pressed, the led
 * blinks and the string "button pressed" is printed
 */

/*
 * Exercise 2c (to be solved in ILIAS): read out the temperature from the temperature
 * sensor when the button is pressed. print the temperature to the serial interface by
 * passing the value read from the sensor to print_temperature_binary_to_float().
 *
 * Note: you have 1 node with environmental sensors, it will be 
 * either the node with the id 50, 60, 70, 80, 90, 100, 110, 120, 130, 140. you
 * need to flash this node which has the sensors with your program here
 */




