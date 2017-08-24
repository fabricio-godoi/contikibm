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
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "net/rime/rimestats.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "apps/benchmark/benchmark.h"


#include "net/ip/uip-debug.h"
#define DEBUG DEBUG_ANNOTATE

/** Define Benchmark debug messages */
//#define BM_DEBUG
#ifdef BM_DEBUG
#define BM_PRINTF(...) printf(__VA_ARGS__)
#define BM_PRINTF_ADDR64(x)	PRINT6ADDR(x)
#else
#define BM_PRINTF(...)
#define BM_PRINTF_ADDR64(x)
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define UDP_EXAMPLE_ID  190

//#define SERVER_REPLY 1

static struct uip_udp_conn *server_conn;




PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
/* Benchmark variables */
struct netstat_t UNET_NodeStat;
char NodeStat_Ctrl = 1; // Default: enabled
static Benchmark_Control_Type bm_ctrl;
static uint8_t bm_en_comm = 0; // default: disabled
static uint8_t bm_num_of_nodes = 0;
static uint8_t bm_num_of_packets = 0;
static uint16_t bm_interval = 0;
static uint16_t bm_pkts_recv[255]; // individual packets received count per client
static uint32_t stick = 0; // system tick
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
		bm_interval = (uint16_t)(data[3]<<7) + data[4];
		BM_PRINTF("benchmark: Nodes: %d; Interval %d\n",bm_num_of_nodes,bm_interval);
//		if(getchar(&bm_num_of_nodes,0) == READ_BUFFER_OK){
//			printf("Number of motes: %u\n",bm_num_of_nodes);
//		}
	}
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(state == ADDR_TENTATIVE || state == ADDR_PREFERRED) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  uip_ipaddr_t ipaddr;
  struct uip_ds6_addr *root_if;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

//  SENSORS_ACTIVATE(button_sensor);


#if UIP_CONF_ROUTER
/* The choice of server address determines its 6LoPAN header compression.
 * Obviously the choice made here must also be selected in udp-client.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the 6LowPAN protocol preferences,
 * e.g. set Context 0 to aaaa::.  At present Wireshark copies Context/128 and then overwrites it.
 * (Setting Context 0 to aaaa::1111:2222:3333:4444 will report a 16 bit compressed address of aaaa::1111:22ff:fe33:xxxx)
 * Note Wireshark's IPCMV6 checksum verification depends on the correct uncompressed addresses.
 */
 
#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
/* Mode 3 - derived from link local (MAC) address */
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
#endif

  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  root_if = uip_ds6_addr_lookup(&ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
    uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &ipaddr, 64);
    PRINTF("created a new RPL dag\n");
  } else {
    PRINTF("failed to create a new RPL DAG\n");
  }
#endif /* UIP_CONF_ROUTER */
  
  print_local_addresses();

  /* The data sink runs with a 100% duty cycle in order to ensure high 
     packet reception rates. */
  NETSTACK_MAC.off(1);

  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  if(server_conn == NULL) {
    BM_PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

  BM_PRINTF("Created a server connection with remote address ");
  BM_PRINTF_ADDR64(&server_conn->ripaddr);
  BM_PRINTF(" local/remote port %u/%u\n", UIP_HTONS(server_conn->lport),
         UIP_HTONS(server_conn->rport));


  // Benchmark
  Benchmark_Packet_Type bm_packet;

  // Inform observer with necessary information
#if OF_IS_MRHOF == 1
  printf("server: contiki mrhof\n");
#else
  printf("server: contiki of0\n");
#endif


  // Get from observer the number of nodes
  PROCESS_YIELD_UNTIL(ev == serial_line_event_message);
//  bm_ctrl.ctrl = ((uint8_t *) data)[0];
  benchmark_parse_control((uint8_t *) data);
//  if(bm_ctrl.c.set) bm_num_of_nodes = ((uint8_t *) data)[1];

  uint32_t tick;
  static uint16_t message_table[128];
  uint8_t from, i;
  for(i=0;i<128;i++) message_table[i] = 0;
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      if(uip_newdata()) {
    	// Does not check if it's duplicated, just inform that has packet arrived
	    NODESTAT_UPDATE(apprxed);


    	  // hop count
    	  // rpl_get_instance(***).min_hoprankinc ??
    	  // hops = uip_ds6_if.cur_hop_limit - UIP_IP_BUF->ttl + 1;
//        bm_packet = (char *)uip_appdata;
        tick = (uint32_t) clock_time() - stick;
        memcpy(&bm_packet,(char *)uip_appdata, BM_PACKET_SIZE);


		/**
		 * 32768 / 128 = 256
		 * 128 / 32768 = 0.00390625 - cada tick
		 */
//  	  	 delay =  ((uint32_t)(tick-bm_packet.tick)*1000*CLOCK_SECOND/RTIMER_CONF_SECOND),
        // Do nothing, just clear the buffer
  	    from = UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1];
  	    BM_PRINTF("msg %d: %d %s\n",from,bm_packet.msg_number,bm_packet.message);
  	    if(bm_packet.msg_number > message_table[from]){
  	  	    BM_PRINTF("benchmark: msg: %u; table: %u; %s; from: %d\n",
  	  	    		bm_packet.msg_number,
  	  	    		message_table[from],
  	  	    		bm_packet.message, from);

  	    	message_table[from] = bm_packet.msg_number;
  	    	// This ensure the number of packets arrived
  	    	bm_pkts_recv[from]++;

  	    	if(strcmp(bm_packet.message, BENCHMARK_MESSAGE) !=0 )	NODESTAT_UPDATE(corrupted);
  	    }
      }


    } else if(ev == serial_line_event_message){
//     	bm_ctrl.ctrl = ((char *)data)[0];
     	benchmark_parse_control((uint8_t *) data);
     	if(bm_ctrl.c.reset){
			for(i=0;i<=bm_num_of_nodes;i++){
				bm_pkts_recv[i] = 0;
				message_table[i] = 0;
			}
		}

    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


#if 0
static void notify_all() {
	for (r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
		if (r->length >= longestmatch &&
		uip_ipaddr_prefixcmp(addr, &r->ipaddr, r->length)) {
			longestmatch = r->length;
			found_route = r;
			/* check if total match - e.g. all 128 bits do match */
			if (longestmatch == 128) {
				break;
			}
		}
	}
}
#endif
