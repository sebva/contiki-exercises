#include "contiki.h"
#include <stdio.h> /* For printf() */
#include <string.h>
#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/light.h"
#include "node-id.h" /* get a pointer to the own node id */
#include "dev/sht11-sensor.h"

struct temperatureMessage {
	char messageString[50];
	uint16_t temperature;
};

#define BROADCAST_CHANNEL 128

void print_temperature_binary_to_float(uint16_t temp) {
	printf("%d.%d", (temp / 10 - 396) / 10, (temp / 10 - 396) % 10);
}


static void timerCallback_turnOffLeds();
static void recv_bc(struct broadcast_conn *c, rimeaddr_t *from);
static struct ctimer leds_off_timer_send;

/* Timer callback turns off the blue led */
static void timerCallback_turnOffLeds()
{
  leds_off(LEDS_BLUE);
}


/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks broadcast_call = {recv_bc};
static struct broadcast_conn bc;
/*---------------------------------------------------------------------------*/
/* one timer struct declaration/instantiation */
static struct etimer et;
/* one temperatureMessage struct declaration/instantiation  */
static struct temperatureMessage tm;
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_rssi_process, "Broadcast RSSI");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_rssi_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&bc);)
  PROCESS_BEGIN();

  SENSORS_ACTIVATE(sht11_sensor);

    printf("Starting broadcast\n");
  /* every broadcast has to be initiated with broadcast_open, where the broadcast_conn struct
   * are defined and the pointers to the callback, the function which has to be defined that is
   * going to be called when RECEIVING the broadcast
   */
  broadcast_open(&bc, BROADCAST_CHANNEL, &broadcast_call);
  /* Broadcast every 4 seconds */
  etimer_set(&et, 4*CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);

    // Temperature of 0 means no sensor, checked using node_id an even number
    uint16_t temperature = (node_id % 2 == 0) ? sht11_sensor.value(SHT11_SENSOR_TEMP) : 0;

    // Send a different message, depending on the existence of a temperature sensor
    if (node_id % 2 == 0)
      sprintf(tm.messageString, "Here is node %u, sending the temperature", node_id);
    else
      sprintf(tm.messageString, "Here is node %u, I don't have a sensor", node_id);

    // Save the temperature into the struct
    tm.temperature = temperature;

    // Copy the temperature struct in the packet buffer
    packetbuf_copyfrom(&tm, sizeof(tm));

    /* send the packet */
    broadcast_send(&bc);

    /* turn on blue led */
    leds_on(LEDS_BLUE);
    /* set the timer "leds_off_timer" to 1 second */
    ctimer_set(&leds_off_timer_send, CLOCK_SECOND, timerCallback_turnOffLeds, NULL);
    printf("sent broadcast message\n");
  }

  SENSORS_DEACTIVATE(sht11_sensor);

  PROCESS_END();
}
AUTOSTART_PROCESSES(&broadcast_rssi_process);

static void
recv_bc(struct broadcast_conn *c, rimeaddr_t *from)
{
  // Read the incoming data into the static temperatureMessage struct
  packetbuf_copyto(&tm);

  // Temperature = 0 means that we received a message from the sensor without the sht11 sensor
  if (tm.temperature != 0)
  {
    printf("Temperature: ");
    print_temperature_binary_to_float(tm.temperature);
    printf("\n");
  }
  printf("received '%s' from %d.%d, RSSI=%i\n", tm.messageString, from->u8[0], from->u8[1], (int) packetbuf_attr(PACKETBUF_ATTR_RSSI));
}

/* Exercise 3a: flash the program to your nodes and observe what happens. Read the code and understand it thoroughly.
 */

/* Exercise 3b (to be solved in ILIAS): for the node with the temperature sensor, read out the temperature value
 * in each loop. Then, send it to the other node using the broadcast function. Use the defined struct "temperatureMessage".
 * Learn how to define and instantiate structs in C on the internet. Replace the payload "helloMessage" entirely with
 * an instance of the struct temperatureMessage defined on line 11
 *
 * struct temperatureMessage {
 *	char messageString[50];
 *	uint16_t temperature;
 * };
 *
 * containing the string (messageString) and the current temperature (temperature).
 * Attention: watch out how you write into the temperatureMessage.messageString field in the C programming language...
 *
 * When received by the other node, let the receiver print the transmitted temperature to the serial port
 * AND let it blink with the blue LED for 1 second!
 *
 *
 */

