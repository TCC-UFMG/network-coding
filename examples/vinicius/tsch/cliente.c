/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
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
 */
/**
 * \file
 *         A RPL+TSCH node able to act as either a simple node (6ln),
 *         DAG Root (6dr) or DAG Root with security (6dr-sec)
 *         Press use button at startup to configure.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "random.h"
#include "sys/log.h"
#include "sys/node-id.h"

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#define LOG_MODULE "AAAAAAAAAAA"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&node_process);
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
PROCESS_THREAD(node_process, ev, data) {
    static struct etimer periodic_timer;
    static char str[32];
    uip_ipaddr_t dest_ipaddr;
    static uint32_t tx_count;

    PROCESS_BEGIN();

    NETSTACK_MAC.on();

    /* Initialize UDP connection */
    simple_udp_register(
        &udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);

    etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if(NETSTACK_ROUTING.node_is_reachable()
           && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
            /* Send to DAG root */
            tx_count = 0;
            LOG_INFO("Sending request %" PRIu32 " to ", tx_count);
            LOG_INFO_6ADDR(&dest_ipaddr);
            LOG_INFO_("\n");
            snprintf(str,
                     sizeof(str),
                     "%s-hello %" PRIu32 "",
                     "preambulo",
                     tx_count);

            simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
        }

        /* Add some jitter */
        etimer_set(&periodic_timer,
                   SEND_INTERVAL - CLOCK_SECOND
                       + (random_rand() % (2 * CLOCK_SECOND)));
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
