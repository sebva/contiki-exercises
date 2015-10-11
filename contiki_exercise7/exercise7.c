

#include "contiki.h"
#include "shell.h"
#include "serial-shell.h"
#include "net/rime.h"
#include "dev/leds.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
PROCESS(shell_blink_process, "blink");
SHELL_COMMAND(blink_command, "blink", "blink [num]: blink blue led [num] times", &shell_blink_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_blink_process, ev, data)
{
  static int num;
  const char *nextptr;
  static struct etimer et;

  PROCESS_BEGIN();
  if(data != NULL) {
    num = shell_strtolong(data, &nextptr);
    printf("number entered %u\n", num);
    while(num > 0) {
       	leds_on(LEDS_BLUE);
    	etimer_set(&et, CLOCK_SECOND/2);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    	leds_off(LEDS_BLUE);
    	etimer_set(&et, CLOCK_SECOND/2);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    	num--;
    }
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* UNICAST STUFF                                                             */
/*---------------------------------------------------------------------------*/
// First: a command to set the receiver of the unicast
PROCESS(shell_set_unicast_receiver_process, "set_unicast_receiver");
SHELL_COMMAND(set_unicast_receiver_command, "rcv", "rcv: set unicast receiver [target id]", &shell_set_unicast_receiver_process);
static uint16_t unicast_receiver_id;
#define UNICAST_CHANNEL 100
PROCESS_THREAD(shell_set_unicast_receiver_process, ev, data)
{
  const char *nextptr;
  PROCESS_BEGIN();
  if(data != NULL) {
	unicast_receiver_id = shell_strtolong(data, &nextptr);
    printf("id entered %u -> set unicast receiver_id\n", unicast_receiver_id);
  }
  PROCESS_END();
}

/* Timer callback turns off the blue led */
static void timerCallback_turnOffLeds()
{
  leds_off(LEDS_BLUE);
}
static struct ctimer leds_off_timer_send;

/* the receive unicast callbacks */
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from);
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  printf("unicast message received from %d\n", from->u8[0]);
  /* turn on blue led */
  leds_on(LEDS_BLUE);
  ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);

  /*********************/
  /* MISSING CODE HERE */
  /*********************/

}

// the command to send a unicast message
PROCESS(shell_unicast_process, "unicast message");
SHELL_COMMAND(unicast_command, "unicast", "unicast [message]", &shell_unicast_process);
PROCESS_THREAD(shell_unicast_process, ev, data)
{
	static int len;
	PROCESS_BEGIN();
	/* Get the length of the command line, excluding a terminating NUL character. */
	len = strlen((char *)data);
	char copiedData[len+1];
	memcpy(copiedData, data, len);
	copiedData[len]=0;

	/* Check the length of the command line to see that it is small enough to fit in a packet */
	if(len > PACKETBUF_SIZE - 32) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", len);
		shell_output_str(&unicast_command, "command line too large: ", buf);
	}
	else {
		printf("entered string '%s'\n", copiedData);

		/* prepare the packet to be sent, copy the helloMessage into the packet. For this, we have to
		 * give a pointer to the copiedData byte array as first argument, then the size as the second argument*/

		/*********************/
		/* MISSING CODE HERE */
		/*********************/

		printf("sent packet to node with id %u\n", unicast_receiver_id);
	}
	PROCESS_END();
}


/*---------------------------------------------------------------------------*/
/* BROADCAST STUFF                                                           */
/*---------------------------------------------------------------------------*/
// the broadcast channel
#define BROADCAST_CHANNEL 128
// the broadcast receive callback
static void recv_bc(struct broadcast_conn *c, rimeaddr_t *from)
{
	printf("received broadcast from nodeid=%u\n", from->u8[0]);
	leds_on(LEDS_BLUE);
	ctimer_set(&leds_off_timer_send, CLOCK_SECOND / 8, timerCallback_turnOffLeds, NULL);
	char receivedString[packetbuf_datalen()];
	packetbuf_copyto(receivedString);
	printf("message: '%s'\n\n", receivedString);
}

static const struct broadcast_callbacks broadcast_call = {recv_bc};
static struct broadcast_conn bc;
// the broadcast shell command
PROCESS(shell_broadcast_process, "broadcast message");
SHELL_COMMAND(broadcast_command, "broadcast", "broadcast [message]", &shell_broadcast_process);
PROCESS_THREAD(shell_broadcast_process, ev, data)
{
	PROCESS_BEGIN();
	static int len;

	/* Get the length of the command line, excluding a terminating NUL character. */
	len = strlen((char *)data);
	char copiedData[len+1];
	memcpy(copiedData, data, len);
	copiedData[len]=0;

	/* Check the length of the command line to see that it is small enough to fit into a packet */
	if(len > PACKETBUF_SIZE - 32) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", len);
		shell_output_str(&broadcast_command, "command line too large: ", buf);
	}
	else {
		printf("entered string '%s'\n", copiedData);
		/* prepare the packet to be sent, copy the copiedData array into the packet. For this, we have to
		 * give a pointer to the copiedData byte array as first argument, then the size as the second
		 * argument */
		packetbuf_clear();
		packetbuf_copyfrom(copiedData, sizeof(copiedData));
		/* send the packet using broadcast*/
		broadcast_send(&bc);
		printf("sent broadcast message\n");
	}
	PROCESS_END();
}


/* EXERCISE 7 PROCESS (TO BE STARTED WITH AUTOSTART_PROCESS */

PROCESS(exercise_7_process, "Exercise 7 - Using Shell Commands");
PROCESS_THREAD(exercise_7_process, ev, data)
{
  PROCESS_BEGIN();
  broadcast_open(&bc, BROADCAST_CHANNEL, &broadcast_call);

  serial_shell_init();
  shell_register_command(&blink_command);
  shell_register_command(&broadcast_command);
  shell_register_command(&unicast_command);
  shell_register_command(&set_unicast_receiver_command);
  PROCESS_END();
}
AUTOSTART_PROCESSES(&exercise_7_process);



/*  Exercise 7: Revisiting the Broadcast and Unicast with command line shell interface
 *
 *  a) in the code above, we set up the command line interface and register and implement
 *  some commands. compile the code and type "make login". Then, type "blink 10" and observe
 *  what happens. Check in the code where this is implemented.
 *
 *  b) type "help" after typing "make login". This displays you the list of commands which is
 *  registered. check the code in this file to see where which command is defined, implemented
 *  and registered
 *
 *  c) the command "broadcast" can be run from the shell. The command takes the input string from
 *  the interface and puts it into a broadcast packet. It then sends this over the radio.
 *  However, the second node does not get it. Why?
 *
 *  d) The command "rcv" sets the receipient of the unicast command. The "unicast" command then
 *  takes the string argument, and shall send it to the unicast receipient. Hoewever, you should complete
 *  this program, using the code from exercises1-6 from the lectures of the last weeks.
 *
 */
