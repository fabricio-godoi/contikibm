/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
//#include "lib/random.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "platform/cooja/dev/cooja-radio.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"

#include "platform/cooja/sys/node-id.h"
#include "apps/benchmark/benchmark.h"
#include "core/net/rpl/rpl.h"

#include "er-coap-engine.h"
/*---------------------------------------------------------------------------*/
/**
 * CoAP configurations
 */
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define LOCAL_PORT      UIP_HTONS(COAP_DEFAULT_PORT + 1)
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)

static coap_packet_t request[1]; /* This way the packet can be treated as pointer as usual. */

/* Example URIs that can be queried. */
#define NUMBER_OF_URLS 5
/* leading and ending slashes only for demo purposes, get cropped automatically when setting the Uri-Path */
char *service_urls[NUMBER_OF_URLS] =
{ ".well-known/core", "/actuators/toggle", "battery/", "error/in//path" , ""};


/* This function is will be passed to COAP_BLOCKING_REQUEST() to handle responses. */
void client_chunk_handler(void *response)
{
  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);

  // Do nothing, since nothing is returned

//  printf("|%.*s", len, (char *)chunk);
}

/*---------------------------------------------------------------------------*/
/**
 * RPL and Contiki configurations
 */

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_FULL // Enable RPL #L directives
#include "net/ip/uip-debug.h"

/** Define Benchmark debug messages */
//#define BM_DEBUG
#ifdef BM_DEBUG
#define BM_PRINTF(...) printf(__VA_ARGS__)
#define BM_PRINTF_ADDR64(x)	PRINT6ADDR(x)
#else
#define BM_PRINTF(...)
#define BM_PRINTF_ADDR64(x)
#endif

#ifndef PERIOD
#define PERIOD 1
#endif

#define START_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
//#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30

//static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

/* UNET network statistics */
struct netstat_t UNET_NodeStat;
char NodeStat_Ctrl = 1; // Default: enabled

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
/*---------------------------------------------------------------------------*/

// Benchmark variables
// Enable/Disable application communication
static uint8_t bm_en_comm = 0; // default: disabled
static uint8_t bm_num_of_nodes = 0;
static uint8_t bm_num_of_packets = 0;
static uint16_t bm_interval = 0;
static Benchmark_Control_Type bm_ctrl;
static Benchmark_Packet_Type bm_packet = { 0, 1, {BENCHMARK_MESSAGE} };
volatile uint32_t stick = 0;

/*---------------------------------------------------------------------------*/

// Create an event for the benchmark process
process_event_t benchmark_event;
PROCESS(coap_udp_client_process, "CoAP UDP client process");
AUTOSTART_PROCESSES(&coap_udp_client_process);

/*---------------------------------------------------------------------------*/
static void benchmark_parse_control(uint8_t * data){
	bm_ctrl.ctrl = data[0];
//	BM_PRINTF("client: Command 0x%02X received!\n",bm_ctrl.ctrl);
	if(bm_ctrl.c.start) { bm_en_comm = TRUE; stick = (uint32_t) clock_time(); }
	if(bm_ctrl.c.stop)  { bm_en_comm = FALSE; }
	if(bm_ctrl.c.stats) { NODESTAT_ENABLE();  }
	else 				{ NODESTAT_DISABLE(); }
	if(bm_ctrl.c.reset) { NODESTAT_RESET();   }
	if(bm_ctrl.c.get)   { printf("%02X\n",bm_ctrl.ctrl); }
	if(bm_ctrl.c.set){
		bm_num_of_nodes = data[1];
		bm_num_of_packets = data[2];
		bm_interval = (uint16_t)(data[3]<<7) + data[4]; // TODO need to get the last 2bits lost in communication method by JS
		BM_PRINTF("benchmark: Nodes: %d; Interval %d\n",bm_num_of_nodes,bm_interval);
//		if(getchar(&bm_num_of_nodes,0) == READ_BUFFER_OK){
//			printf("Number of motes: %u\n",bm_num_of_nodes);
//		}
	}
}
/*---------------------------------------------------------------------------*/
//static void send_packet(void *ptr) {
//
////			unet_send(&server,(uint8_t*)&bm_packet,BM_PACKET_SIZE,0);
//
//
//	bm_packet.tick = (uint32_t) clock_time() - stick;
////	uip_udp_packet_sendto(client_conn, &bm_packet, BM_PACKET_SIZE,
////			&server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
//
//
//    /* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
//    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
//    coap_set_header_uri_path(request, service_urls[4]);
//
//
//    coap_set_payload(request, &bm_packet, BM_PACKET_SIZE);
//
//    COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
//                          client_chunk_handler);
//
//
//	BM_PRINTF("benchmark: msg %d - tick %lu - clock %lu\n", bm_packet.msg_number, bm_packet.tick, stick);
//	NODESTAT_UPDATE(apptxed);
//	bm_packet.msg_number++;
//
//	if(bm_packet.msg_number > bm_num_of_packets){
//		// Reach number of messages, stop it all
//		bm_en_comm = FALSE;
//		printf(BENCHMARK_CLIENT_DONE);
//	}
//}
/*---------------------------------------------------------------------------*/
static void print_local_addresses(void) {
	int i;
	uint8_t state;

	BM_PRINTF("Client IPv6 addresses: ");
	for (i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if (uip_ds6_if.addr_list[i].isused
				&& (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
			BM_PRINTF_ADDR64(&uip_ds6_if.addr_list[i].ipaddr);
			BM_PRINTF("\n");
			/* hack to make address "final" */
			if (state == ADDR_TENTATIVE) {
				uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
			}
		}
	}
}
/*---------------------------------------------------------------------------*/
static void set_global_address(void) {
	uip_ipaddr_t ipaddr;

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	/* The choice of server address determines its 6LoPAN header compression.
	 * (Our address will be compressed Mode 3 since it is derived from our link-local address)
	 * Obviously the choice made here must also be selected in udp-server.c.
	 *
	 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the 6LowPAN protocol preferences,
	 * e.g. set Context 0 to aaaa::.  At present Wireshark copies Context/128 and then overwrites it.
	 * (Setting Context 0 to aaaa::1111:2222:3333:4444 will report a 16 bit compressed address of aaaa::1111:22ff:fe33:xxxx)
	 *
	 * Note the IPCMV6 checksum verification depends on the correct uncompressed addresses.
	 */

#if 0
	/* Mode 1 - 64 bits inline */
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
#elif 1
	/* Mode 2 - 16 bits inline */
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
	/* Mode 3 - derived from server link-local (MAC) address */
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_udp_client_process, ev, data) {
	static struct etimer periodic;
//	static struct etimer backoff_timer;
//	static clock_time_t send_interval;
	static clock_time_t send_time;
#if WITH_COMPOWER
	static int print = 0;
#endif

	PROCESS_BEGIN();

	// Allocate an event for the benchmark
	benchmark_event = process_alloc_event();

	PROCESS_PAUSE();

	/** Setup Contiki connection */

	srand(node_id);

	set_global_address();
	print_local_addresses();

//	/* new connection with remote host */
//	client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
//	if (client_conn == NULL) {
//		BM_PRINTF("No UDP connection available, exiting the process!\n");
//		PROCESS_EXIT();
//	}
//	udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

	/**
	 * Configure CoAP
	 */

//	SERVER_NODE(&server_ipaddr);

	/* receives all CoAP messages */
	coap_init_engine();

	//	  etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);

	/**
	 * Configure the Benchmark
	 */

//	BM_PRINTF("Created a connection with the server ");
//	BM_PRINTF_ADDR64(&client_conn->ripaddr);
//	BM_PRINTF(" local/remote port %u/%u\n", UIP_HTONS(client_conn->lport),
//			UIP_HTONS(client_conn->rport));

#if WITH_COMPOWER
	powertrace_sniff(POWERTRACE_ON);
#endif


	// Get the observer the number of nodes
	PROCESS_YIELD_UNTIL(ev == serial_line_event_message);
//	bm_ctrl.ctrl = ((uint8_t *) data)[0];
	benchmark_parse_control((uint8_t *) data);
	if(bm_ctrl.c.set) bm_num_of_nodes = ((uint8_t *) data)[1];

//	send_interval = (clock_time_t) bm_num_of_nodes * (BENCHMARK_INTERVAL);
	send_time = (clock_time_t) (bm_num_of_nodes - node_id) * (bm_interval/bm_num_of_nodes);

	// Wait until the a route was found
	etimer_set(&periodic, CLOCK_SECOND/2); // wait 500ms
	while(!uip_ds6_defrt_choose()){
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_TIMER);
		etimer_reset(&periodic);
	}
	etimer_stop(&periodic);

	// Notify the observer that is all ready to start
	// Wait observe response to actually initiate the process
	printf(BENCHMARK_CLIENT_CONNECTED);


/// START COAP DEBUG
//#define TOGGLE_INTERVAL 10
//	etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);

	// 0- not sending ; 1 - is sending
	char sending = 0;

	while (1) {
		PROCESS_YIELD();


#if 0 // DEBUG_COAP
		if(etimer_expired(&et)) {
		      printf("--Toggle timer--\n");

		      /* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
		      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
		      coap_set_header_uri_path(request, service_urls[4]);

		      const char msg[] = "Toggle!\0";

//		      coap_set_payload(request, (uint8_t *)msg, sizeof(msg) - 1);
			  coap_set_payload(request, &bm_packet, BM_PACKET_SIZE);

		      COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
		                            client_chunk_handler);

		      printf("\n--Done--\n");

		      etimer_reset(&et);
		}
#endif


		if (ev == serial_line_event_message) {
			benchmark_parse_control((uint8_t *) data);
			if(bm_ctrl.c.start){
				bm_packet.tick = 0;
				bm_packet.msg_number = 1;
			}
			if(bm_ctrl.c.set){
				send_time = (clock_time_t) (bm_num_of_nodes - node_id) * (bm_interval/bm_num_of_nodes);
			}
			// Just started
			if(bm_en_comm){
				etimer_set(&periodic, (clock_time_t)((CLOCK_SECOND*send_time)/1000));
			}
			else{
				etimer_stop(&periodic);
				sending = 0;
			}
		}
		else if(ev == PROCESS_EVENT_TIMER){
			if(etimer_expired(&periodic)) {
				etimer_stop(&periodic);
				sending = 1;
			}

#if old
			if(etimer_expired(&periodic)) {
				bm_packet.tick = (uint32_t) clock_time() - stick;

				/* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
				coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
				coap_set_header_uri_path(request, service_urls[4]);


				coap_set_payload(request, &bm_packet, BM_PACKET_SIZE);

				NODESTAT_UPDATE(apptxed);
				COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
									  client_chunk_handler);


				BM_PRINTF("benchmark: msg %d - tick %lu - clock %lu\n", bm_packet.msg_number, bm_packet.tick, stick);
				bm_packet.msg_number++;

				if(bm_packet.msg_number > bm_num_of_packets){
					// Reach number of messages, stop it all
					bm_en_comm = FALSE;
					printf(BENCHMARK_CLIENT_DONE);
					etimer_stop(&periodic);
				}
				else etimer_set(&periodic,(clock_time_t)((CLOCK_SECOND*bm_interval)/1000));
			}
#endif

		}

		while(sending){
			bm_packet.tick = (uint32_t) clock_time() - stick;

			/* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
			coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
			coap_set_header_uri_path(request, service_urls[4]);


			coap_set_payload(request, &bm_packet, BM_PACKET_SIZE);

			NODESTAT_UPDATE(apptxed);
			COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
								  client_chunk_handler);


			BM_PRINTF("benchmark: msg %d - tick %lu - clock %lu\n", bm_packet.msg_number, bm_packet.tick, stick);
			bm_packet.msg_number++;

			if(bm_packet.msg_number > bm_num_of_packets){
				// Reach number of messages, stop it all
				bm_en_comm = FALSE;
				printf(BENCHMARK_CLIENT_DONE);
				sending = 0;
			}
		}

		/**
		 * Total time = time_send + time_wait
		 * [<    time_send    > Send Packet <    time_wait    >]
		 * 0 -------------------------------------------------- send_interval
		 */
		if(!bm_en_comm){
			etimer_stop(&periodic);
		}


#if WITH_COMPOWER
		if (print == 0) {
			powertrace_print("#P");
		}
		if (++print == 3) {
			print = 0;
		}
#endif
	}

PROCESS_END();
}
/*---------------------------------------------------------------------------*/
