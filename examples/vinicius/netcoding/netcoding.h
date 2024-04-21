#ifndef NET_CODING_H_
#define NET_CODING_H_

#include "buffer.h"
#include "hash_table.h"
#include "node.h"
#include "packet.h"

/* ------------------- CODING ----------------------------------------------- */
/**
 * @brief Stores a packet inside a node. If the packet is raw, it goes to the
 * raw_buffer and combination_buffer otherwise.
 *
 * @param node
 * @param packet
 * @return int 1 if the packet was stored and 0 otherwise.
 */
static int store_packet(netcoding_node* node, netcoding_packet* packet) {
    if(is_raw_packet(packet)) return push_packet(&node->raw_buffer, packet);
    return push_packet(&node->combination_buffer, packet);
}

static int should_combine_packet(netcoding_node* node) {
    return rand() % 100 < node->prob_to_combine;
}

/**
 * @brief Get the packet to combine. To do that it searches for a fitting packet
 * inside the combination_buffer. Reference `are_fitting_headers` to understand
 * more about fitting packets.
 *
 * @param node The node holding the packet buffer.
 * @param inbound_packet The packet to be complementary matched in the buffer.
 * @param packet_to_combine A pointer to hold the packet to combine.
 * @return int
 */
static int get_packet_to_combine(netcoding_node* node,
                                 netcoding_packet* inbound_packet,
                                 netcoding_packet* packet_to_combine) {
    // There is only one packet slot remaining
    if(get_header_num_packets(&inbound_packet->header)
       == NUM_COMBINATIONS - 1) {
        return pop_fitting_packet(
            &node->raw_buffer, &inbound_packet->header, packet_to_combine);
    }

    // This beign called before raw_buffer prioritizes combination_buffer. I
    // guess this will deliver more info in 1 packet, then the network will have
    // a bigger throughput gain with this
    int found_combined_fitting = pop_fitting_packet(
        &node->combination_buffer, &inbound_packet->header, packet_to_combine);

    if(!found_combined_fitting) {
        return pop_fitting_packet(
            &node->raw_buffer, &inbound_packet->header, packet_to_combine);
    }
    return found_combined_fitting;
}

static void xor_combine(char data1[PAYLOAD_SIZE],
                        char data2[PAYLOAD_SIZE],
                        char combined[PAYLOAD_SIZE]) {
    for(int i = 0; i < PAYLOAD_SIZE; i++) combined[i] = data1[i] ^ data2[i];
}

/**
 * @brief Combine two packets
 *
 * @param pck1 Packet of reference.
 * @param pck2 Packet to be merged.
 * @return netcoding_packet* The new combined packet.
 */
static netcoding_packet* combine_packets(netcoding_packet* pck1,
                                         netcoding_packet* pck2) {
    netcoding_packet* combined_packet =
        (netcoding_packet*)malloc(sizeof(netcoding_packet));

    combined_packet->header = merge_headers(&pck1->header, &pck2->header);
    xor_combine(pck1->body, pck2->body, combined_packet->body);

    return combined_packet;
}

/**
 * @brief Function that routes a packet according to network coding rules:
 * 1) With a probability P, the packet should be combined. With 1-P the packet
 * should be sent with no modifications.
 * 2) If P happens, the node will try to combine the received packet with any
 * previous stored packet. If there is no valid packet to combine, then store
 * the packet.
 *
 * @param node The node to route the packet.
 * @param packet The packet to be routed.
 * @return netcoding_packet
 */
static netcoding_packet* encode_packet(netcoding_node* node,
                                       netcoding_packet* packet) {
    if(  // TODO: packet->header.can_be_combined ||
        should_combine_packet(node)) {
        netcoding_packet packet_to_combine;
        if(get_packet_to_combine(node, packet, &packet_to_combine)) {
            printf("Combinou\n");
            return combine_packets(packet, &packet_to_combine);
        }

        if(store_packet(node, packet)) {
            printf("Armazenou\n");
        }
        return NULL;
    }
    return packet;
}

/* ------------------- DECODING --------------------------------------------- */
static void deep_copy(void* item, void** dest) {}

static size_t facade_hash_calculator(void* data) {
    return packet_hash((netcoding_packet*)data);
}

static void** array_allocator(int size) {
    return (void**)malloc(size * sizeof(netcoding_packet**));
}

static int is_header_collision(void* data1, void* data2) {
    netcoding_packet_header* h1 = (netcoding_packet_header*)data1;
    netcoding_packet_header* h2 = (netcoding_packet_header*)data2;
    return are_equivalent_headers(h1, h2);
}

static void fill_with_existing_packets(hash_table* map, netcoding_node* node) {
    linked_list_node* cur_node = node->raw_buffer.head;

    while(cur_node) {
        try_insert_item(map, cur_node->data);
        cur_node = cur_node->next;
    }

    cur_node = node->combination_buffer.head;
    while(cur_node) {
        try_insert_item(map, cur_node->data);
        cur_node = cur_node->next;
    }
}

static struct linked_list_t* decode_packet(netcoding_node* node,
                                           netcoding_packet* packet) {
    struct linked_list_t *decoded_packets = (struct linked_list_t*)malloc(
                             sizeof(struct linked_list_t)),
                         *packets_to_decode = (struct linked_list_t*)malloc(
                             sizeof(struct linked_list_t));
    // List with the next batch of packets to be decoded
    push_packet(packets_to_decode, packet);

    // Creates aa hashtable holding all the current packets in the node
    hash_table already_processed_packets =
        create_hash_table(NETCODING_WINDOW_SIZE * 3,
                          is_header_collision,
                          array_allocator,
                          facade_hash_calculator,
                          deep_copy);
    fill_with_existing_packets(&already_processed_packets, node);

    while(packets_to_decode->size) {
        linked_list_node* cur_node = packets_to_decode->head;
        while(cur_node) {
            cur_node = cur_node->next;
        }
    }

    return decoded_packets;
}

#endif /* NET_CODING_H_ */