/*
 * benchmark.h
 *
 *  Created on: Apr 20, 2017
 *      Author: user
 */

#ifndef BENCHMARK_H_
#define BENCHMARK_H_


/**
 * The approximation to get how much time each node need to send one packet
 * with network acknowledge:
 *      { [t1] [d1] [t2] [d2] [t3] [d3] }
 *      |<--            T            -->|
 *  T  - Time period to send one packet
 *  t1 - Packet transmission time (802.15.4 up to 250 Kbps);
 *  d1 - Delay to radio acknowledge (defined in radio datasheet);
 *  t2 - Radio acknowledge packet send time;
 *  d2 - Delay to network acknowledge (include time to process the packet);
 *  t3 - Network acknowledge time;
 *  d3 - Delay between packets, collision avoidance;
 *
 * e.g.: CC2520 with AUTOCRC and AUTOACK enabled
 * Packet = 128 B
 * t1 = (128*8)/(250*1024) = 4000 us
 * d1 = 192us
 * t2 = (3*8)/(250*1024) = 93.75 us
 * d2 = ~ 1000
 * t3 = (48*8)/(250*1024) = 1500us
 * d3 = 192 us (at least)
 * T  = 6977.75 us
 */

/**
 * Minimum transmission time between two motes with network acknowledge;
 * Value defined in milliseconds.
 * Mote1           Mote2
 *  | ->  Packet   -> |
 *  | <- Radio ACK <- |
 *  | <-  Net ACK  <- |
 */
#define BENCHMARK_MIN_DELAY 10 // Some error value included

/**
 * Define the longest path between clients and server
 * This value is determined by number of hops.
 *
 * e.g. (root) <- (1) <- (2) <- (3)
 * BENCHMARK_LONGEST_PATH is 3
 *
 * Observation.:
 *   This value can got from simulator,
 *   but in real environment this is hard to
 *   get without some application changes,
 *   by now this value is hard coded without
 *   possibility for dynamic mote alloc;
 */
#define BENCHMARK_LONGEST_PATH  32

/**
 * Benchmark interval to send packets
 * INTERVAL = LONGEST_PATH * MIN_DELAY * DELAY_BPKTS
 * e.g. = 12 * 10 * 10 = 1200 ms
 */
#define BENCHMARK_INTERVAL     (  \
	    BENCHMARK_LONGEST_PATH *  \
        BENCHMARK_MIN_DELAY    )


#if ((BENCHMARK_MIN_TX_TIME * BENCHMARK_LONGEST_PATH) > 0)
#error zero
#endif


/**
 * This defines the number of messages that each
 * client will send to the server
 * Default: 100
 */
#define BENCHMARK_NUMBER_OF_MESSAGES 100

/**
 * Configure the max app payload length, this value is dependent of
 * the network layer, so check what is the maximum network payload length
 * before set this.
 * unet deafult: 35 bytes -> 127 - 35 = 92
 * contiki ipv6 default: 40 bytes -> 127 - 40 = 87
 */
#define BENCHMARK_MSG_MAX_SIZE		80 //(spare for some possible overflows)

/**
 * Message that will be sent over network
 */
#define BENCHMARK_MESSAGE "DEFAULT BENCHMARK PAYLOAD\0"
#define BENCHMARK_MESSAGE_LENGTH    26

/**
 * Message that the client will sent to the controller
 * to inform that the parent has been set
 */
#define BENCHMARK_CLIENT_CONNECTED "BMCC_START\n"

/**
 * Message to inform that the client has sent
 * all the messages
 */
#define BENCHMARK_CLIENT_DONE	"BMCD_DONE\n"

/// Benchmark controls commands
enum{
	BMCC_NONE = 0,
	BMCC_START,
	BMCC_END,
	BMCC_RESET,
	BMCC_GET,
	BMCC_SET,

	// Do not remove, must be the last parameter
	BMCC_NUM_COMMANDS
};

typedef union{
	uint8_t ctrl;
	struct{
		uint8_t start:1;		//<! Start communication
		uint8_t stop:1;			//<! Stop communication
		uint8_t stats:1;		//<! Enable/Disable statistics
		uint8_t reset:1;		//<! Reset the statistics
		uint8_t shutdown:1;		//<! Disable any kind of network (not implmentend)
		uint8_t get:1;			//<! Get parameter (not implemented)
		uint8_t set:1;			//<! Set parameter (not implemented)
		uint8_t reserved:1;     //<! Reserved to ensure the usability of JavaScript
	}c;
}Benchmark_Control_Type;

typedef struct{
	uint32_t tick;
	uint16_t msg_number;
	uint8_t  message[BENCHMARK_MESSAGE_LENGTH];
}Benchmark_Packet_Type;
#define BM_PACKET_SIZE (4 + 2 + BENCHMARK_MESSAGE_LENGTH)

/**
 * Task parameters
 */
#define System_Time_StackSize      192
#define UNET_Benchmark_StackSize   736
#define Benchmark_Port		       222


/**
 * Statistics control
 */

#include <stdint.h>

typedef uint16_t netst_cnt_t;
struct netstat_t
{

	/// Application
	netst_cnt_t apptxed;      // application txed packets
	netst_cnt_t apprxed;      // application rxed packets

	/// Transport
	netst_cnt_t netapptx;	  // packets storage at network layer, ready to be sent
	netst_cnt_t netapprx;	  // packets received at network layer, ready to be used to application

	/// Network
	netst_cnt_t duplink;      // packets duplicates at link layer

	netst_cnt_t routed;       // routed packets
	netst_cnt_t dupnet;       // packets duplicates at net layer
	netst_cnt_t routdrop;     // packets dropped by routing buffer overflow

	/// Data Link
	netst_cnt_t txed;         // successfully transmitted packets
	netst_cnt_t rxed;         // received packets
	netst_cnt_t txfailed;     // transmission failures
	netst_cnt_t txnacked;	  // transmission not acked
	netst_cnt_t txmaxretries; // transmission max retries reached
	netst_cnt_t chkerr;       // checksum error

	netst_cnt_t overbuf;      // packets dropped by RX buffer overflow
	netst_cnt_t dropped;      // packets dropped by hops limit, route not available
	netst_cnt_t corrupted;    // application packet corrupted, does not have the size informed

	/// Physical
	netst_cnt_t radiotx;	  // packets sent in the radio
	netst_cnt_t radiorx;	  // packets recv in the radio
	netst_cnt_t radiocol;     // radio transmissions collisions
	netst_cnt_t radionack;    // radio transmission not acknowledged

	/// Removed because contiki doesn't use it
//	netst_cnt_t radioresets;  // radio reset

};

#define NODESTAT_ENABLE()   do{ extern char NodeStat_Ctrl; NodeStat_Ctrl=1;} while(0);
#define NODESTAT_DISABLE()	do{ extern char NodeStat_Ctrl; NodeStat_Ctrl=0;} while(0);
#define NODESTAT_RESET() 	do{ extern struct netstat_t UNET_NodeStat; memset(&UNET_NodeStat, 0x00, sizeof(UNET_NodeStat));} while (0);
#define NODESTAT_CHECK(x)   if((x) == (netst_cnt_t)(-1)) NODESTAT_RESET();
// Default is enabled
#define NODESTAT_UPDATE(x)  do {                                                \
                                extern struct netstat_t UNET_NodeStat;          \
                                extern char NodeStat_Ctrl;                      \
								if(NodeStat_Ctrl){                              \
                                    NODESTAT_CHECK(++UNET_NodeStat.x)           \
								}                                               \
							}while(0);

#define NODESTAT_ADD(x,y)	do {                                                \
								extern struct netstat_t UNET_NodeStat;          \
								extern char NodeStat_Ctrl;                      \
								if(NodeStat_Ctrl){                              \
									UNET_NodeStat.x += y;                       \
								}                                               \
							}while(0);




#endif /* BENCHMARK_H_ */
