/* Philipp Hurni, University of Bern, January 2014                         */
/* Exercises for sensor networks lecture                                */
/* File adapted from $CONTIKI/examples/ipv6/sky-websense/sky-websense.c    */

#include "contiki.h"
#include "httpd-simple.h"
#include "dev/sht11-sensor.h"
#include "dev/light-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
#include <stdlib.h>

#define HISTORY 16
static int temperature[HISTORY];
static int light1[HISTORY];
static int sensors_pos;

/*---------------------------------------------------------------------------*/
static int get_light(void) {
return 10 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC) / 7;
}
/*---------------------------------------------------------------------------*/
static int get_temp(void) {
return ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
}

PROCESS(web_sense_process, "Sense Web Demo");
PROCESS(webserver_nogui_process, "Web server");
PROCESS_THREAD(webserver_nogui_process, ev, data) {
    PROCESS_BEGIN();
    httpd_init();

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
        httpd_appcall(data);
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
static const char *TOP =
"<html><head><title>Contiki Web Sense</title></head><body>\n";
static const char *BOTTOM = "</body></html>\n";
/*---------------------------------------------------------------------------*/
/* Only one single request at time */
static char buf[256];
static int blen;
#define ADD(...) do {                                                   \
        blen += snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);      \
} while(0)


static PT_THREAD(send_values(struct httpd_state *s)) {
    PSOCK_BEGIN(&s->sout);
    SEND_STRING(&s->sout, TOP);
    if (strncmp(s->filename, "/index", 6) == 0 || s->filename[1] == '\0') {
        /* Default page: show latest sensor values as text (does not
         require Internet connection to Google for charts). */
        blen = 0;
        ADD("<h1>Current readings</h1>\n"
        "Light: %u<br>"
        "Temperature: %u&deg; C", get_light(), get_temp());
        SEND_STRING(&s->sout, buf);
    } else if (s->filename[1] == '0') {
        /* Turn off leds */
        leds_off(LEDS_ALL);
        SEND_STRING(&s->sout, "Turned off leds!");
    } else if (s->filename[1] == '1') {
        /* Turn on leds */
        leds_on(LEDS_ALL);
        SEND_STRING(&s->sout, "Turned on leds!");
    }
    SEND_STRING(&s->sout, BOTTOM);
    PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_simple_script_t httpd_simple_get_script(const char *name) {
return send_values;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(web_sense_process, ev, data) {
    static struct etimer timer;
    PROCESS_BEGIN();

    sensors_pos = 0;
    etimer_set(&timer, CLOCK_SECOND * 2);
    SENSORS_ACTIVATE(light_sensor);
    SENSORS_ACTIVATE(sht11_sensor);

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
        etimer_reset(&timer);
        light1[sensors_pos] = get_light();
        temperature[sensors_pos] = get_temp();
        sensors_pos = (sensors_pos + 1) % HISTORY;
    }

    PROCESS_END();
}


/***************************************************************************/
/******* EXERCISE 9 ********************************************************/
/***************************************************************************/

// some macros
#ifndef MIN
#define MIN(a,b) ((a) < (b)? (a): (b))
#endif

#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTF(...) printf(__VA_ARGS__)
#define ABS(x)           (((x) < 0) ? -(x) : (x))

static struct psock ps;
static char indata[40];

static PT_THREAD(handle_connection(struct psock *p)) {
    PSOCK_BEGIN(p);
    // we use the PSOCK_READTO() function to read incoming data from the TCP connection until we get a newline character

    static struct etimer timer;
    etimer_set(&timer, CLOCK_SECOND * 2);

    static int lightval;
    lightval = get_light();
    static int old_lightval;

    static char light_string[12];

    while(1){
        PSOCK_WAIT_UNTIL(p, PSOCK_NEWDATA(p) || etimer_expired(&timer));

        old_lightval = lightval;
        lightval = get_light();

        if(PSOCK_NEWDATA(p)) {
            PSOCK_READTO(p, '\n');
            leds_blink();

            // light command
            if (strncmp(indata, "light", 5) == 0) {
                sprintf(light_string, "light %u\n", lightval);
                PSOCK_SEND_STR(p, light_string);
            }
            else {
                PSOCK_SEND_STR(p, "no such command: ");
                PSOCK_SEND(p, indata, PSOCK_DATALEN(p));
            }
            memset(indata, 0, 40);
        }
        else {
            etimer_reset(&timer);
            printf("timer ran out...light=%u\n", lightval);

            if (abs(old_lightval - lightval) >= 80) {
                printf("Intruder!");
                PSOCK_SEND_STR(p, "lightvaluechange\n");
            }
        }
    }
    PSOCK_CLOSE(p);
    PSOCK_END(p);
}


// The TCP Server Process
PROCESS(tcp_process, "TCP server");
PROCESS_THREAD(tcp_process, ev, data) {
    PROCESS_BEGIN();

    tcp_listen(UIP_HTONS(100));

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
        if (uip_connected()) {
            leds_on(LEDS_GREEN);
            PSOCK_INIT(&ps, indata, sizeof(indata));
            printf("Connection established from ");
            PRINT6ADDR(&uip_conn->ripaddr);
            printf("%u\n", uip_ntohs(uip_conn->rport));
        }

        // loop until the connection is aborted, closed, or times out.
        while (!(uip_aborted() || uip_closed() || uip_timedout())) {
            // We wait until we get a TCP/IP event
            PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
            // We call the handle_connection() protothread that we defined above
            handle_connection(&ps);
        }

        if(uip_closed()) {
            printf("Connection closed\n");
            leds_off(LEDS_GREEN);
        }
        if(uip_aborted()) {
            printf("Connection aborted\n");
            leds_off(LEDS_GREEN);
        }
        if(uip_timedout()) {
            printf("Connection timed out\n");
            leds_off(LEDS_GREEN);
        }

    }
    PROCESS_END();
}

AUTOSTART_PROCESSES(&web_sense_process, &webserver_nogui_process, &tcp_process);


/*---------------------------------------------------------------------------*/
