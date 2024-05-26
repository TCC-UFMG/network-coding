#ifndef NET_CODING_H_
#define NET_CODING_H_

#include "buffer.h"
#include "hash_table.h"
#include "node.h"
#include "packet.h"

/**
 * @brief The receivers are the NUM_RECEIVERS-th first nodes.
 *
 */
#define NUM_RECEIVERS 1

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

    combined_packet->header = xor_merge_headers(&pck1->header, &pck2->header);
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
/**
 * @brief Function that copies the item into the hashmap slot
 *
 * @param item
 * @param dest
 */
static void deep_copy(void* item, void** dest) {
    //! Assumes the item will be freed
    netcoding_packet** packet_dest = (netcoding_packet**)dest;
    netcoding_packet* packet =
        (netcoding_packet*)malloc(sizeof(netcoding_packet));
    memcpy(packet, item, sizeof(netcoding_packet));
    *packet_dest = packet;

    //! Assumes item will be mantained allocated
    //     netcoding_packet** packet_dest = (netcoding_packet**)dest;
    //     *packet_dest = item;
}

/**
 * @brief The an facade to the hash calculator for a netcoding packet
 *
 * @param data
 * @return size_t
 */
static size_t facade_hash_calculator(void* data) {
    return packet_hash((netcoding_packet*)data);
}

/**
 * @brief Creates a new netcoding packet pointer array (where the content of the
 * hash table resides).
 *
 * @param size
 * @return void**
 */
static void** array_allocator(int size) {
    return (void**)malloc(size * sizeof(netcoding_packet**));
}

/**
 * @brief Detects collision on the hash map. Considers only the headers in order
 * to detect collision.
 *
 * @param data1
 * @param data2
 * @return int
 */
static int is_header_collision(void* data1, void* data2) {
    netcoding_packet* h1 = (netcoding_packet*)data1;
    netcoding_packet* h2 = (netcoding_packet*)data2;
    return are_equivalent_headers(&h1->header, &h2->header);
}

/**
 * @brief Inserts  all packets in the node's buffers into the hash map.
 *
 * @param map
 * @param node
 */
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

/**
 * @brief Function that realy decodes two packets using XOR logic.
 *
 * @param pck1
 * @param pck2
 * @return netcoding_packet*
 */
static netcoding_packet* resolve_packets(netcoding_packet* pck1,
                                         netcoding_packet* pck2) {
    netcoding_packet* combined_packet =
        (netcoding_packet*)malloc(sizeof(netcoding_packet));

    combined_packet->header = xor_merge_headers(&pck1->header, &pck2->header);
    xor_combine(pck1->body, pck2->body, combined_packet->body);

    return combined_packet;
}

/**
 * @brief Given an hash map and a packet to be decoded, iterates over the hash
 * map elements and try to generate not seen packets to be added to the output
 * list if it's of size 1 (has only one packet information) and insert it to the
 * packets_to_decode doesn't matter the size.
 *
 * @param map
 * @param packet_to_decode
 * @param packets_to_decode
 * @param output_list
 */
static void decode_packet_over_map(hash_table* map,
                                   netcoding_packet* packet_to_decode,
                                   struct linked_list_t* packets_to_decode,
                                   struct linked_list_t* output_list) {
    // Iterate for each already obtained packet in the hashmap
    for(int i = 0; i < map->capacity; i++) {
        if(map->keys[i]) {
            netcoding_packet* mapped_packet = (netcoding_packet*)map->items[i];

            netcoding_packet* resolved_packet =
                resolve_packets(packet_to_decode, mapped_packet);

            int header_size = get_header_num_packets(&resolved_packet->header);

            if(!header_size) continue;

            int did_insert = try_insert_item(map, resolved_packet);
            // This packet was not created yet
            if(did_insert == 1) {
                push_packet(packets_to_decode, resolved_packet);
                // If is a raw packet never found
                if(header_size == 1) {
                    push_packet(output_list, resolved_packet);
                }
            }
            else {
                free(resolved_packet);
            }
        }
    }
}

/**
 * @brief Given an node and the packet, returns a list with all the decoded
 * packets generated by this decodification.
 *
 * @param node
 * @param packet
 * @return struct linked_list_t*
 */
static struct linked_list_t* decode_packets(netcoding_node* node,
                                            netcoding_packet* packet) {
    struct linked_list_t *decoded_packets = (struct linked_list_t*)malloc(
                             sizeof(struct linked_list_t)),
                         *packets_to_decode = (struct linked_list_t*)malloc(
                             sizeof(struct linked_list_t)),
                         *next_packets_to_decode =
                             (struct linked_list_t*)malloc(
                                 sizeof(struct linked_list_t));
    start_list(decoded_packets);
    start_list(packets_to_decode);
    start_list(next_packets_to_decode);

    store_packet(node, packet);
    push_packet(decoded_packets, packet);
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
            netcoding_packet* cur_packet = (netcoding_packet*)cur_node->data;

            decode_packet_over_map(&already_processed_packets,
                                   cur_packet,
                                   next_packets_to_decode,
                                   decoded_packets);

            cur_node = cur_node->next;
        }

        // Set the next layer of packets as the current one
        clear_list(packets_to_decode);
        transfer_list(next_packets_to_decode, packets_to_decode);
        start_list(next_packets_to_decode);
    }

    free_list(packets_to_decode);
    free_list(next_packets_to_decode);
    clean_hash_table(&already_processed_packets);

    return decoded_packets;
}

#endif /* NET_CODING_H_ */