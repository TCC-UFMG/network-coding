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

/*---------------------------------------------------------------------------*/
PROCESS(udp_process, "UDP 2xor network coding sender");
AUTOSTART_PROCESSES(&udp_process);
/*---------------------------------------------------------------------------*/
static void receiver_callback(struct simple_udp_connection *c,
                              const uip_ipaddr_t *sender_addr,
                              uint16_t sender_port,
                              const uip_ipaddr_t *receiver_addr,
                              uint16_t receiver_port,
                              const uint8_t *data,
                              uint16_t datalen) {
    LOG_INFO("[Sender]: Received response '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_process, ev, data) {
    static struct etimer periodic_timer;
    uip_ipaddr_t dest_ipaddr;

    create_netcoding_normal_routing_node(node_id);

    uip_ip6addr(&dest_ipaddr, 0xfd00, 0, 0, 0, 0X201, 0X1, 0X1, 0X1);

    PROCESS_BEGIN();

    simple_udp_register(&udp_connection,
                        UDP_SENDER_PORT,
                        NULL,
                        UDP_RECEIVER_PORT,
                        receiver_callback);

    etimer_set(&periodic_timer, SEND_INTERVAL);

    printf("PACKET SIZE = %d in node %d with ip ", (int)PACKET_SIZE, node_id);
    log_6addr_compact(&uip_ds6_get_link_local(-1)->ipaddr);
    printf("\n");

    static int packet_id = 0;
    static netcoding_packet packet;
    char packet_message[PAYLOAD_SIZE];
    static char buffer[PACKET_SIZE];

    while(1) {
        sprintf(packet_message, "Message %d from %d", packet_id, node_id);
        packet = create_packet(packet_id, packet_message);
        memcpy(buffer, &packet, sizeof(packet));

        printf("Sending message %d to ", packet_id);
        log_6addr(&dest_ipaddr);
        printf("\n");

        simple_udp_sendto(&udp_connection, buffer, PACKET_SIZE, &dest_ipaddr);

        /*====================================================================*/
        // Make sure no packet is going to have and invalid id
        packet_id++;
        packet_id %= EMPTY_PACKET_ID;

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
