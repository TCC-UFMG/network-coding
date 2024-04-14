#ifndef NET_CODING_H_
#define NET_CODING_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------- PACKET ----------------------------------------------- */
#define NUM_COMBINATIONS 2
#define PAYLOAD_SIZE 30
#define EMPTY_PACKET_ID UINT32_MAX
#define PACKET_PREAMBLE "preambulo"
#define PREAMBLE_SIZE (sizeof(PACKET_PREAMBLE) - 1)
#define HAS_PREAMBLE(str) (strncmp(str, PACKET_PREAMBLE, PREAMBLE_SIZE) == 0)

typedef struct netcoding_packet_header_t {
    uint32_t holding_packets[NUM_COMBINATIONS];
} netcoding_packet_header;

typedef struct netcoding_packet_t {
    char preamble[PREAMBLE_SIZE];
    netcoding_packet_header header;
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

typedef struct linked_list_node_t {
    void* data;
    struct linked_list_node_t* next;
} linked_list_node;

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

static int push_packet(packet_buffer* buffer, netcoding_packet* packet) {
    if(buffer->size == NETCODING_WINDOW_SIZE) return 0;

    linked_list_node* node =
        (linked_list_node*)malloc(sizeof(linked_list_node));
    netcoding_packet* cloned_packet =
        (netcoding_packet*)malloc(sizeof(netcoding_packet));
    *cloned_packet = *packet;
    node->data = cloned_packet;
    node->next = NULL;

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
#define COMBINATION_PERCENTAGE_RATE 30

typedef struct netcoding_node_t {
    int id;
    packet_buffer outbound_buffer;
} netcoding_node;

extern netcoding_node network_coding_node;

static inline void create_netcoding_node(int id) {
    // node.outbound_buffer = 1;
    network_coding_node.id = id;
}

/* ------------------- IMPLEMENTATION --------------------------------------- */
static int should_combine_packet() {
    return rand() % 100 < COMBINATION_PERCENTAGE_RATE;
}

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

static netcoding_packet combine_packets(netcoding_packet* pck1,
                                        netcoding_packet* pck2) {
    netcoding_packet combined_packet;

    combined_packet.header = merge_headers(&pck1->header, &pck2->header);
    xor_combine(pck1->body, pck2->body, combined_packet.body);

    return combined_packet;
}

static netcoding_packet route_packet(netcoding_node* node,
                                     netcoding_packet* packet) {
    netcoding_packet packet_to_combine;
    if(should_combine_packet()
       && get_packet_to_combine(node, packet, &packet_to_combine)) {
        return combine_packets(packet, &packet_to_combine);
    }
    else
        return *packet;
}

#endif /* NET_CODING_H_ */