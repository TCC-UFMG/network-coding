#include <inttypes.h>
#include <stdint.h>
#include "../netcoding/netcoding.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "random.h"
#include "sys/log.h"
#include "sys/node-id.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
// static uint32_t rx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
    LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    rx_count++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data) {
    static struct etimer periodic_timer;
    static char str[32];
    uip_ipaddr_t dest_ipaddr;
    static uint32_t tx_count;

    create_netcoding_node(node_id);

    uip_ipaddr_t addr;
    uip_ip6addr(
        &addr, 0XFE80, 0, 0, 0, 512 + node_id, node_id, node_id, node_id);

    PROCESS_BEGIN();

    /* Initialize UDP connection */
    simple_udp_register(
        &udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);

    etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if(node_id == 5) {
            if(NETSTACK_ROUTING.node_is_reachable()
               && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
                uip_ip6addr(
                    &dest_ipaddr, 0xfd00, 0, 0, 0, 0X201, 0X1, 0X1, 0X1);

                /* Send to DAG root */
                tx_count = 0;
                LOG_INFO("Sending request %" PRIu32 " to ", tx_count);
                LOG_INFO_6ADDR(&dest_ipaddr);
                LOG_INFO_("\n");
                snprintf(str,
                         sizeof(str),
                         "%s-hello %" PRIu32 "",
                         PACKET_PREAMBLE,
                         tx_count);

                simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
            }
        }

        /* Add some jitter */
        etimer_set(&periodic_timer,
                   SEND_INTERVAL - CLOCK_SECOND
                       + (random_rand() % (2 * CLOCK_SECOND)));
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
