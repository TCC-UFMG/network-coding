#ifndef NET_CODING_NODE_H_
#define NET_CODING_NODE_H_

#include <stdlib.h>
#include "buffer.h"

/* ------------------- NODE ------------------------------------------------- */

/**
 * @brief The probability to combine a packet.
 *
 */
#define COMBINATION_PERCENTAGE_RATE 30

/**
 * @brief A node in the network that communicates in the network coding
 * protocol.
 *
 */
typedef struct netcoding_node_t {
    /**
     * @brief The node ID.
     *
     */
    int id;
    /**
     * @brief The probability to combine a packet.
     *
     */
    int prob_to_combine;
    /**
     * @brief The buffer with the modified packets waiting to be combined.
     *
     */
    packet_buffer combination_buffer;
    /**
     * @brief The buffer with the raw packets (the original packets, without
     * combinations).
     *
     */
    packet_buffer raw_buffer;
} netcoding_node;

extern netcoding_node network_coding_node;

static inline void create_netcoding_node(int id) {
    network_coding_node.id = id;
}

static inline void create_netcoding_combinatory_routing_node(int id) {
    create_netcoding_node(id);
    network_coding_node.prob_to_combine = COMBINATION_PERCENTAGE_RATE;
}

static inline void create_netcoding_normal_routing_node(int id) {
    create_netcoding_node(id);
    network_coding_node.prob_to_combine = 0;
}

#endif /* NET_CODING_NODE_H_ */