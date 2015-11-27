
#include "contiki.h"
#include "shell.h"
#include "serial-shell.h"
#include "cfs/cfs.h"
#include "dev/watchdog.h"
#include "contiki-net.h"
#include "dev/cc2420.h"
#include "dev/leds.h"
#include "dev/light.h"
#include "dev/sht11.h"
#include "dev/battery-sensor.h"
#include "node-id.h"
#include "lib/checkpoint.h"
#include "net/rime/timesynch.h"
#include "loader/elfloader.h"
#include <stdio.h>
#include <string.h>

#define RUNICAST_CHANNEL SHELL_RIME_CHANNEL_DOWNLOAD
#define RUCB_CHANNEL (SHELL_RIME_CHANNEL_DOWNLOAD+1)
#define MAX_RETRANSMISSIONS 8
#define MAX_FILENAME_SIZE 20
static struct runicast_conn runicast;
static struct rucb_conn rucb;
static uint8_t downloading;
static uint8_t req_seq_counter;
static uint8_t req_last_seq;
static int fd;
static const char* sendfile_filename;
static void recv_runicast(struct runicast_conn *c, rimeaddr_t *from, uint8_t seqno);
static void sent_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions);
static void timedout_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions);
static void request_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno);
static void request_sent(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions);
static void request_timedout(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions);


static const struct runicast_callbacks sendCommandCallbacks = {recv_runicast, sent_runicast, timedout_runicast};
static const struct runicast_callbacks runicast_callbacks = {request_recv, request_sent, request_timedout};

static struct runicast_conn runicastSendCommand;
#define RUNICAST_CHANNEL_SENDCOMMAND  100

int (*keep)(void) = etimer_expired;
// struct which is used to initiate file transmission
struct fileOrigStruct {
	char name[20];
	unsigned short originator;
};
static struct fileOrigStruct filenameOriginatorSent;
static struct fileOrigStruct filenameOriginatorRecv;

static const char *filename_download;

/*---------------------------------------------------------------------------*/
static void write_chunk(struct rucb_conn *c, int offset, int flag, char *data, int datalen);
static int read_chunk(struct rucb_conn *c, int offset, char *to, int maxsize);
static void timedout(struct rucb_conn *c);
static const struct rucb_callbacks rucb_call = { write_chunk, read_chunk, timedout };

void runReceivedProgram() {

	printf("runReceivedProgram\n");
	printf("originator: %u\n", filenameOriginatorRecv.originator);
	printf("filename: %s\n", filename_download);

	fd = cfs_open(filename_download, CFS_READ | CFS_WRITE);
	if(fd < 0) {
	    printf("exec: could not open %s\n", filename_download);
	}
	else {
	    int ret;
	    char *print, *symbol;
	    ret = elfloader_load(fd);
	    cfs_close(fd);
	    symbol = "";
	    switch(ret) {
	    case ELFLOADER_OK:
	       print = "OK";
	       break;
	    case ELFLOADER_BAD_ELF_HEADER:
	       print = "Bad ELF header";
	       break;
	    case ELFLOADER_NO_SYMTAB:
	       print = "No symbol table";
	       break;
	    case ELFLOADER_NO_STRTAB:
	       print = "No string table";
	       break;
	    case ELFLOADER_NO_TEXT:
	       print = "No text segment";
	       break;
	    case ELFLOADER_SYMBOL_NOT_FOUND:
	       print = "Symbol not found: ";
	       break;
	    case ELFLOADER_SEGMENT_NOT_FOUND:
	       print = "Segment not found: ";
	       break;
	    case ELFLOADER_NO_STARTPOINT:
	       print = "No starting point";
	       break;
	    default:
	       print = "Unknown return code from the ELF loader (internal bug)";
	       break;
	    }

	    if(ret == ELFLOADER_OK) {
	       int i;
	       for(i = 0; elfloader_autostart_processes[i] != NULL; ++i) {
	          printf("exec: starting process %s\n", elfloader_autostart_processes[i]->name);
	       }
	       autostart_start(elfloader_autostart_processes);
	    }
	}
}

/****************** Process to wait for download ********************/

PROCESS(download_and_execute_process, "Download process");
PROCESS_THREAD(download_and_execute_process, ev, data)
{
    PROCESS_BEGIN();
    // turn on blue leds while downloading, then
    leds_on(LEDS_BLUE);
    PROCESS_WAIT_UNTIL(!runicast_is_transmitting(&runicast) && !downloading);
    leds_off(LEDS_BLUE);

    printf("Downloaded Code!\n");
    printf("originator: %u\n", filenameOriginatorRecv.originator);
    printf("filename: %s\n", filename_download);

    runReceivedProgram();

    PROCESS_END();
}




static void recv_runicast(struct runicast_conn *c, rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n", from->u8[0], from->u8[1], seqno);
  packetbuf_copyto(&filenameOriginatorRecv);
  printf("originator: %u\n", filenameOriginatorRecv.originator);
  printf("filename: %s\n", filenameOriginatorRecv.name);

  // request file from initiator
  rucb_open(&rucb, RUCB_CHANNEL, &rucb_call);
  packetbuf_clear();
  *((uint8_t *)packetbuf_dataptr()) = ++req_seq_counter;
  memcpy(((char *)packetbuf_dataptr()) + 1, filenameOriginatorRecv.name, strlen(filenameOriginatorRecv.name)+1);
  packetbuf_set_datalen(strlen(filenameOriginatorRecv.name) + 2);
  rimeaddr_t addr;
  addr.u8[0] = filenameOriginatorRecv.originator;
  addr.u8[1] = 0;
  filename_download = filenameOriginatorRecv.name;
  fd = cfs_open(filename_download, CFS_WRITE );
  runicast_send(&runicast, &addr, MAX_RETRANSMISSIONS);
  downloading = 1;
  process_start(&download_and_execute_process, NULL);

}

static void sent_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
static void timedout_runicast(struct runicast_conn *c, rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}


/*---------------------------------------------------------------------------*/
PROCESS(shell_sendfile_process, "sendfile");
SHELL_COMMAND(sendfile_command, "sendfile", "sendfile <node addr> <filename>: send file to remote node", &shell_sendfile_process);
/*---------------------------------------------------------------------------*/

void shell_sendfile_init(){
	shell_register_command(&sendfile_command);
	runicast_open(&runicastSendCommand, RUNICAST_CHANNEL_SENDCOMMAND, &sendCommandCallbacks);
}

PROCESS_THREAD(shell_sendfile_process, ev, data)
{
	const char *nextptr;
	static rimeaddr_t addr;
	int len;
	char buf[32];
	PROCESS_BEGIN();
	/* Parse node addr */
	addr.u8[0] = shell_strtolong(data, &nextptr);
	if(nextptr == data || *nextptr != '.') {
	    printf("sendfile <node addr> <filename>: need node address\n");
	    PROCESS_EXIT();
	}
	++nextptr;
	addr.u8[1] = shell_strtolong(nextptr, &nextptr);
	printf("\nnode address: %d.%d\n", addr.u8[0], addr.u8[1]);

	while(nextptr[0] == ' ') nextptr++;
	len = strlen(nextptr);
	//snprintf(buf, sizeof(buf), "%d.%d", addr.u8[0], addr.u8[1]);
	if(len > MAX_FILENAME_SIZE) {
	    snprintf(buf, sizeof(buf), "%d", len);
	    printf("filename too large: ", buf);
	    PROCESS_EXIT();
	}
	sendfile_filename = nextptr;
	printf("filename: %s\n", sendfile_filename);
	sprintf(filenameOriginatorSent.name, "%s", sendfile_filename);
	filenameOriginatorSent.originator = node_id;
	packetbuf_copyfrom(&filenameOriginatorSent, sizeof(filenameOriginatorSent));
	runicast_send(&runicastSendCommand, &addr, MAX_RETRANSMISSIONS);
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static void write_chunk(struct rucb_conn *c, int offset, int flag, char *data, int datalen)
{
  //printf("write_chunk offset=%d, length=%d, filename=%s\n", offset, datalen, filename_download);
  if(datalen > 0) {
    cfs_write(fd, data, datalen);
  }
  if(flag == RUCB_FLAG_LASTCHUNK) {
	//printf("last chunk!\n");
    downloading = 0;
    cfs_close(fd);
    process_poll(&download_and_execute_process);
  }
}
/*---------------------------------------------------------------------------*/
static int read_chunk(struct rucb_conn *c, int offset, char *to, int maxsize)
{
  int ret;
  if(fd < 0) {
    /* No file, send EOF */
    leds_off(LEDS_GREEN);
    return 0;
  }
  cfs_seek(fd, offset, CFS_SEEK_SET);
  ret = cfs_read(fd, to, maxsize);
  if(ret < maxsize) {
    cfs_close(fd);
    fd = -1;
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static void timedout(struct rucb_conn *c) { }
/*---------------------------------------------------------------------------*/
static void request_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  const char *filename;
  uint8_t seq;
  if(packetbuf_datalen() < 2) {
    printf("download: bad filename request (null)\n");
    return;
  }
  seq = ((uint8_t *)packetbuf_dataptr())[0];
  if(seq == req_last_seq) {
    printf("download: ignoring duplicate request\n");
    return;
  }
  req_last_seq = seq;
  filename = ((char *)packetbuf_dataptr()) + 1;

  printf("file requested: '%s'\n", filename);
  /* Initiate file transfer */
  leds_on(LEDS_GREEN);
  if(fd >= 0) {
    cfs_close(fd);
  }
  fd = cfs_open(filename, CFS_READ);
  if(fd < 0) {
    printf("download: bad filename request (no read access): %s\n", filename);
  }

  rucb_close(&rucb);
  rucb_open(&rucb, RUCB_CHANNEL, &rucb_call);
  rucb_send(&rucb, from);

}
/*---------------------------------------------------------------------------*/
static void request_sent(struct runicast_conn *c, const rimeaddr_t *to,
	     uint8_t retransmissions) { }
/*---------------------------------------------------------------------------*/
static void request_timedout(struct runicast_conn *c, const rimeaddr_t *to,
		 uint8_t retransmissions)
{
  downloading = 0;
}
/*---------------------------------------------------------------------------*/
PROCESS(shell_download_server_process, "Download Server");
PROCESS_THREAD(shell_download_server_process, ev, data)
{
  PROCESS_EXITHANDLER( runicast_close(&runicast); rucb_close(&rucb); );
  PROCESS_BEGIN();
  runicast_open(&runicast, RUNICAST_CHANNEL, &runicast_callbacks);
  PROCESS_WAIT_UNTIL(0);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void download_init(void)
{
  req_seq_counter = 0;
  req_last_seq = -1;
  fd = -1;
  downloading = 0;
  process_start(&shell_download_server_process, NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS(sky_shell_process, "Sky Contiki Shell Process");
AUTOSTART_PROCESSES(&sky_shell_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sky_shell_process, ev, data)
{
  PROCESS_BEGIN();
  serial_shell_init();
  shell_file_init();
  shell_coffee_init();
  shell_exec_init();
  download_init();
  shell_sendfile_init();
  shell_base64_init();
  PROCESS_END();
}


