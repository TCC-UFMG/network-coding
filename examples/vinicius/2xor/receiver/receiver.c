#include <stdio.h>
#include "../../netcoding/netcoding.h"
#include "../../netcoding/utils.h"
#include "../../utils/benchmark.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "contiki.h"
#include "log.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "sys/node-id.h"

#define UDP_ROUTER_PORT 3000
#define UDP_RECEIVER_PORT 3100
#define UDP_SENDER_PORT 3200

#define SEND_INTERVAL (4 * CLOCK_SECOND)

#define LOG_MODULE "Coding"
#define LOG_LEVEL LOG_LEVEL_INFO

static struct simple_udp_connection udp_connection;

PROCESS(udp_server_process, "UDP 2xor network coding receiver");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void handle_decoded_packet(linked_list_node *node) {
    netcoding_packet *packet = (netcoding_packet *)node->data;
    uip_ip6addr_t sender_addr;
    uip_ip6addr(&sender_addr, 0xfd00, 0, 0, 0, 0X201, 0X0, 0X0, 0X0);

    netcoding_log_format("RECV-D", network_coding_node.id, 1, &sender_addr);

    print_packet_str(packet);
    printf("\n");

    //===========BENCHMARK======================================================
    char registry_str[REGISTRY_MAX_SIZE];
    create_received_registry(network_coding_node.id,
                             packet->header.holding_packets[0],
                             registry_str);
    add_benchmark_registry(registry_str);
}

static void print_headers(linked_list_node *node) {
    netcoding_packet *packet = (netcoding_packet *)node->data;
    printf("[");
    print_header(&packet->header);
    printf("],");
}

static void receiver_callback(struct simple_udp_connection *c,
                              const uip_ipaddr_t *sender_addr,
                              uint16_t sender_port,
                              const uip_ipaddr_t *receiver_addr,
                              uint16_t receiver_port,
                              const uint8_t *data,
                              uint16_t datalen) {
    static netcoding_packet packet;
    memcpy(&packet, data, PACKET_SIZE);

    netcoding_log_format(
        "RECV  ", network_coding_node.id, 1, (uip_ip6addr_t *)sender_addr);
    print_packet(&packet);
    printf("\n");

    struct linked_list_t *decoded_packets =
        decode_packets(&network_coding_node, &packet);

    if(decoded_packets->size) {
        printf("DECODED PACKETS:\n");
        iterate_over_list(decoded_packets, handle_decoded_packet);
    }

    free_list(decoded_packets);
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data) {
    create_netcoding_normal_routing_node(node_id);

    sprintf(benchmark_filename,
            "../../examples/vinicius/2xor/benchmarks/teste-1.csv");
    if(network_coding_node.id == 1) {
        create_benchmark_registry();
    }

    PROCESS_BEGIN();

    printf("PACKET SIZE = %d in node %d with ip ", (int)PACKET_SIZE, node_id);
    log_6addr(&uip_ds6_get_link_local(-1)->ipaddr);
    printf("\n");

    /* Initialize DAG root */
    NETSTACK_MAC.on();

    /* Initialize UDP connection */
    simple_udp_register(&udp_connection,
                        UDP_RECEIVER_PORT,
                        NULL,
                        UDP_SENDER_PORT,
                        receiver_callback);

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
