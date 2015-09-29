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
static struct ctimer leds_off_timer_send;

/* Timer callback turns off the blue led */
static void timerCallback_turnOffLeds()
{
  leds_off(LEDS_BLUE);
}


static void
recv_bc(struct broadcast_conn *c, rimeaddr_t *from)
{
  char helloMessage[50];
   /* from the packet we have just received, read the data and write it into the
   * helloMessage character array we have defined above
   */
  packetbuf_copyto(helloMessage);

  printf("received '%s' from %d.%d, RSSI=%i\n", helloMessage, from->u8[0], from->u8[1], (int) packetbuf_attr(PACKETBUF_ATTR_RSSI));
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
    /* declare 50 bytes example helloMessage */
    char helloMessage[50];
    /* print the contents "NODE %u TEST STRING" with the node id into the helloMessage string */
    sprintf(helloMessage, "NODE %u TEST STRING", node_id);

    /* prepare the packet to be sent, copy the helloMessage into the packet. For this, we have to
     * give a pointer to the helloMessage byte array as first argument, then the size as the second
     * argument */
    packetbuf_copyfrom(helloMessage, sizeof(helloMessage));

    /* send the packet */
    broadcast_send(&bc);

    /* turn on blue led */
    leds_on(LEDS_BLUE);
    /* set the timer "leds_off_timer" to 1/16 second */
    ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 16, timerCallback_turnOffLeds, NULL);
    printf("sent broadcast message\n");
  }
  PROCESS_END();
}
AUTOSTART_PROCESSES(&broadcast_rssi_process);



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

