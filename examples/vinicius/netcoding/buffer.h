#ifndef NET_CODING_BUFFER_H_
#define NET_CODING_BUFFER_H_

#include <stdlib.h>
#include "packet.h"

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

static void start_list(struct linked_list_t* list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

static packet_buffer create_buffer() {
    packet_buffer buffer;
    start_list(&buffer);
    return buffer;
}

static int find_packet(packet_buffer* buffer,
                       netcoding_packet* original_packet,
                       netcoding_packet* output_packet) {
    linked_list_node* cur_node = buffer->head;

    while(cur_node) {
        netcoding_packet* packet = (netcoding_packet*)cur_node->data;

        if(are_equivalent_headers(&packet->header, &original_packet->header)) {
            output_packet = packet;
            return 1;
        }

        cur_node = cur_node->next;
    }
    return 0;
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
    netcoding_packet* _;
    if(find_packet(buffer, packet, _)) return 0;

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

static void free_list(struct linked_list_t* list) {
    linked_list_node* cur_node = list->head;

    while(cur_node) {
        linked_list_node* tmp = cur_node->next;
        free(cur_node->data);
        free(cur_node);
        cur_node = tmp;
    }
    free(list);
}

static void clear_list(struct linked_list_t* list) {
    linked_list_node* cur_node = list->head;

    while(cur_node) {
        linked_list_node* tmp = cur_node->next;
        free(cur_node->data);
        free(cur_node);
        cur_node = tmp;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

static void transfer_list(struct linked_list_t* from,
                          struct linked_list_t* to) {
    to->head = from->head;
    to->tail = from->tail;
    to->size = from->size;
}

static void reset_list(struct linked_list_t* list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

static void iterate_over_list(struct linked_list_t* list,
                              void (*iterator_callback)(linked_list_node*)) {
    linked_list_node* cur_node = list->head;

    while(cur_node) {
        iterator_callback(cur_node);

        cur_node = cur_node->next;
    }
}

#endif /* NET_CODING_BUFFER_H_ */