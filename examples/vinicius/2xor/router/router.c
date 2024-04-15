#include <stdio.h>
#include "../../netcoding/netcoding.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "contiki.h"
#include "log.h"
#include "sys/node-id.h"

#define UDP_ROUTER_PORT 3000
#define UDP_RECEIVER_PORT 3100
#define UDP_SENDER_PORT 3200

#define SEND_INTERVAL (4 * CLOCK_SECOND)

#define LOG_MODULE "Coding"
#define LOG_LEVEL LOG_LEVEL_INFO

static struct simple_udp_connection udp_connection;
static char buffer[PACKET_SIZE];
static uip_ipaddr_t addr;

/*---------------------------------------------------------------------------*/
PROCESS(udp_process, "UDP 2xor network coding router");
AUTOSTART_PROCESSES(&udp_process);
/*---------------------------------------------------------------------------*/
static void receiver(struct simple_udp_connection *c,
                     const uip_ipaddr_t *sender_addr,
                     uint16_t sender_port,
                     const uip_ipaddr_t *receiver_addr,
                     uint16_t receiver_port,
                     const uint8_t *data,
                     uint16_t datalen) {
    LOG_INFO("[Router]: Received response '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_process, ev, data) {
    uip_ipaddr_t dest_addr;

    create_netcoding_combinatory_routing_node(node_id);

    PROCESS_BEGIN();

    simple_udp_register(
        &udp_connection, UDP_ROUTER_PORT, NULL, UDP_RECEIVER_PORT, receiver);

    printf("PACKET SIZE = %d in router %d with ip ", (int)PACKET_SIZE, node_id);
    log_6addr(&uip_ds6_get_link_local(-1)->ipaddr);
    printf("\n");

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
