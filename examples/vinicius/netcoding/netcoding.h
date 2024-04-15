#ifndef NET_CODING_H_
#define NET_CODING_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------- PACKET ----------------------------------------------- */
/**
 * @brief Max number of combinations per packet.
 *
 */
#define NUM_COMBINATIONS 2
/**
 * @brief Size of the packet payload.
 *
 */
#define PAYLOAD_SIZE 30
/**
 * @brief Indicates whether this is a invalid packet ID or not.
 *
 */
#define EMPTY_PACKET_ID UINT32_MAX
#define PACKET_PREAMBLE "preambulo"
#define PREAMBLE_SIZE (sizeof(PACKET_PREAMBLE) - 1)
#define HAS_PREAMBLE(str) (strncmp(str, PACKET_PREAMBLE, PREAMBLE_SIZE) == 0)

/**
 * @brief Header of the packet.
 *
 */
typedef struct netcoding_packet_header_t {
    /**
     * @brief The array with all packet ids that have been combined in order to
     * generate the resulting packet holding this header.
     *
     */
    uint32_t holding_packets[NUM_COMBINATIONS];
} netcoding_packet_header;

/**
 * @brief A network coding packet.
 *
 */
typedef struct netcoding_packet_t {
    /**
     * @brief Preamble to help to identify a network coding packet in the Cooja
     * environment.
     *
     */
    char preamble[PREAMBLE_SIZE];
    /**
     * @brief The packet header holding all the packets ids that have been
     * combined.
     *
     */
    netcoding_packet_header header;
    /**
     * @brief The packet payload resulting from the combination of all the
     * headers packets.
     *
     */
    char body[PAYLOAD_SIZE];
} netcoding_packet;

#define PACKET_SIZE sizeof(netcoding_packet)

static void print_header(netcoding_packet_header* header) {
    for(int i = 0; i < NUM_COMBINATIONS; i++) {
        uint32_t origin = header->holding_packets[i];
        origin == EMPTY_PACKET_ID ? printf("-1") : printf("%u", origin);
        if(i < NUM_COMBINATIONS - 1) printf(", ");
    }
}

static void print_packet(netcoding_packet* packet) {
    printf("Header: [");
    print_header(&packet->header);
    printf("], Body: \"%s\"", packet->body);
}

static netcoding_packet create_packet(uint32_t packet_id, const char* message) {
    netcoding_packet packet;
    strcpy(packet.preamble, PACKET_PREAMBLE);
    memset(packet.header.holding_packets,
           EMPTY_PACKET_ID,
           sizeof(packet.header.holding_packets));
    packet.header.holding_packets[0] = packet_id;

    size_t str_len = strlen(message);
    if(str_len >= PAYLOAD_SIZE) {
        printf("%zu is bigger then payload size %d", str_len, PAYLOAD_SIZE);
        strcpy(packet.body, "INVALID");
    }
    else {
        strcpy(packet.body, message);
    }

    return packet;
}

/**
 * @brief Verifies if two headers fit themselfs. Two headers fit themselfs if:
 * 1) The number of packets that these two hold do not add up more than
 * NUM_COMBINATIONS (max number of packets per header).
 * 2) The sets of packets ids being hold have to be disjunctive, i.e., there
 * must be no repeated packet id between both of the headers.
 *
 * @param inboud_header The reference header.
 * @param comparable_header The header to be compared.
 * @return int 1 if both are fitting headers and 0 otherwise.
 */
static int are_fitting_headers(netcoding_packet_header* inboud_header,
                               netcoding_packet_header* comparable_header) {
    int num_pckt_h1 = 0, num_pckt_h2 = 0;
    for(int i = 0; i < NUM_COMBINATIONS; i++) {
        if(inboud_header->holding_packets[i] != EMPTY_PACKET_ID) num_pckt_h1++;
        if(comparable_header->holding_packets[i] != EMPTY_PACKET_ID)
            num_pckt_h2++;
    }
    // If the number of packets on h2 would make the final header with more
    // packets then NUM_COMBINATIONS
    if(NUM_COMBINATIONS - num_pckt_h1 < num_pckt_h2) return 0;

    // Verifies if there are repeated packets on the headers (can only fit
    // headers if they are disjunctive)
    for(int i = 0; i < num_pckt_h1; i++) {
        for(int j = 0; j < num_pckt_h2; j++) {
            if(inboud_header->holding_packets[i]
               == comparable_header->holding_packets[j])
                return 0;
        }
    }

    return 1;
}

/**
 * @brief Merge two headers, returning a new one with the union of both packets
 * holding_packets ids. This function assumes both headers agree with the rule
 * 1 in the `are_fitting_headers` function description.
 *
 * @param h1 The header 1.
 * @param h2 The header 2.
 * @return netcoding_packet_header A packet header with the union of both
 * headers packets ids.
 */
static netcoding_packet_header merge_headers(netcoding_packet_header* h1,
                                             netcoding_packet_header* h2) {
    netcoding_packet_header merged;

    int filled_index = 0, i = 0;
    while(h1->holding_packets[i] != EMPTY_PACKET_ID)
        merged.holding_packets[filled_index++] = h1->holding_packets[i++];

    i = 0;
    while(h2->holding_packets[i] != EMPTY_PACKET_ID)
        merged.holding_packets[filled_index++] = h2->holding_packets[i++];

    return merged;
}

/* ------------------- CIRCULAR BUFFER -------------------------------------- */
#define NETCODING_WINDOW_SIZE 8

/**
 * @brief A generic linked list node.
 *
 */
typedef struct linked_list_node_t {
    void* data;
    struct linked_list_node_t* next;
} linked_list_node;

/**
 * @brief A packet buffer with limited size.
 *
 */
typedef struct linked_list_t {
    linked_list_node* head;
    linked_list_node* tail;
    int size;
} packet_buffer;

static packet_buffer create_buffer() {
    packet_buffer buffer;
    buffer.head = NULL;
    buffer.size = 0;
    return buffer;
}

/**
 * @brief Searches in a packet buffer for a fitting packet to the input one.
 *
 * @param buffer The packet buffer to be scanned.
 * @param original_header The packet we want to find another one fitting it.
 * @param output_packet The pointer to store the result packet.
 * @return int 1 if a packet was found and removed and 0 otherwise.
 */
static int pop_fitting_packet(packet_buffer* buffer,
                              netcoding_packet_header* original_header,
                              netcoding_packet* output_packet) {
    if(!buffer->size) return 0;

    linked_list_node* cur_node = buffer->head;
    linked_list_node* last_node = NULL;

    while(cur_node) {
        netcoding_packet* packet = (netcoding_packet*)cur_node->data;

        if(are_fitting_headers(original_header, &packet->header)) {
            *output_packet = *packet;

            // It's head node
            if(last_node == NULL) buffer->head = cur_node->next;
            // It's tail node
            else if(cur_node->next == NULL)
                buffer->tail = last_node;
            // It's middle node
            else
                last_node->next = cur_node->next;

            buffer->size--;

            free(cur_node->data);
            free(cur_node);
            return 1;
        }

        last_node = cur_node;
        cur_node = cur_node->next;
    }

    return 0;
}

/**
 * @brief Adds a packet into a packet buffer.
 *
 * @param buffer The packet buffer to be incremented.
 * @param packet The packet to be added.
 * @return int 1 if a packet was added and 0 otherwise.
 */
static int push_packet(packet_buffer* buffer, netcoding_packet* packet) {
    if(buffer->size == NETCODING_WINDOW_SIZE) return 0;

    linked_list_node* node =
        (linked_list_node*)malloc(sizeof(linked_list_node));
    netcoding_packet* cloned_packet =
        (netcoding_packet*)malloc(sizeof(netcoding_packet));
    // !Since I'm not here to make a performant code implementation, I just want
    // !to return as value in all situations. Since the buffer needs to use
    // !pointers, just copy the whole data.
    *cloned_packet = *packet;
    node->data = cloned_packet;
    node->next = NULL;

    // Is empty buffer
    if(buffer->head == NULL) {
        buffer->head = node;
        buffer->tail = node;
    }
    else {
        buffer->tail->next = node;
        buffer->tail = node;
    }

    buffer->size++;
    return 1;
}

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
     * @brief The buffer with the packets waiting to be combined.
     *
     */
    packet_buffer outbound_buffer;
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

/* ------------------- IMPLEMENTATION --------------------------------------- */
/**
 * @brief
 *
 * @param node
 * @param packet
 */
static void store_packet(netcoding_node* node, netcoding_packet* packet) {}

static int should_combine_packet(netcoding_node* node) {
    return rand() % 100 < node->prob_to_combine;
}

/**
 * @brief Get the packet to combine. To do that it searches for a fitting packet
 * inside the outbound_buffer. Reference `are_fitting_headers` to understand
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
    return pop_fitting_packet(
        &node->outbound_buffer, &inbound_packet->header, packet_to_combine);
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
static netcoding_packet* route_packet(netcoding_node* node,
                                      netcoding_packet* packet) {
    if(should_combine_packet(node)) {
        netcoding_packet packet_to_combine;
        if(get_packet_to_combine(node, packet, &packet_to_combine)) {
            return combine_packets(packet, &packet_to_combine);
        }

        // TODO: store_packet(node, packet);
        return NULL;
    }
    return packet;
}

#endif /* NET_CODING_H_ */