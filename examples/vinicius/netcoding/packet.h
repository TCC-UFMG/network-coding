#ifndef NET_CODING_PACKET_H_
#define NET_CODING_PACKET_H_

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
    printf("], Body: [");
    for(int i = 0; i < PAYLOAD_SIZE; i++) {
        printf("%d", packet->body[i]);
        if(i < PAYLOAD_SIZE - 1) printf(",");
    }
    printf("]");
}

static void print_packet_str(netcoding_packet* packet) {
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
 * @brief Get the header number of valid packets combined.
 *
 * @param header
 * @return int
 */
static int get_header_num_packets(netcoding_packet_header* header) {
    int num_pckt = 0;
    for(int i = 0; i < NUM_COMBINATIONS; i++) {
        if(header->holding_packets[i] == EMPTY_PACKET_ID) break;
        num_pckt++;
    }
    return num_pckt;
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
    int num_pckt_h1 = get_header_num_packets(inboud_header),
        num_pckt_h2 = get_header_num_packets(comparable_header);

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

static int are_equivalent_headers(netcoding_packet_header* inboud_header,
                                  netcoding_packet_header* comparable_header) {
    for(int i = 0; i < NUM_COMBINATIONS; i++) {
        int found = 0;
        for(int j = 0; j < NUM_COMBINATIONS; j++) {
            if(inboud_header->holding_packets[i]
               == comparable_header->holding_packets[j]) {
                found = 1;
                break;
            }
        }
        if(!found) return 0;
    }
    return 1;
}

/**
 * @brief Merge two headers, returning a new one with the union of both packets
 * holding_packets ids. Notice that here the XOR logic is applied, then if a
 * packet `A` appears in both headers, it will no appear in the resulting
 * header. This function assumes both headers agree with the rule 1 in the
 * `are_fitting_headers` function description.
 *
 * @param h1 The header 1.
 * @param h2 The header 2.
 * @return netcoding_packet_header A packet header with the union of both
 * headers packets ids.
 */
static netcoding_packet_header xor_merge_headers(netcoding_packet_header* h1,
                                                 netcoding_packet_header* h2) {
    netcoding_packet_header merged;
    for(int i = 0; i < NUM_COMBINATIONS; i++)
        merged.holding_packets[i] = EMPTY_PACKET_ID;

    // I'm considering it's impossible to each packet to have repeated packets
    int filled_index = 0, i = 0;
    while(h1->holding_packets[i] != EMPTY_PACKET_ID && i < NUM_COMBINATIONS) {
        int cur_item_count = 1, j = 0;

        while(h2->holding_packets[j] != EMPTY_PACKET_ID
              && j < NUM_COMBINATIONS) {
            if(h1->holding_packets[i] == h2->holding_packets[j]) {
                cur_item_count++;
            }
            j++;
        }
        // Even number of elements, then xor nullify this packet
        if(cur_item_count % 2) {
            merged.holding_packets[filled_index++] = h1->holding_packets[i];
        }

        i++;
    }

    // !TODO: Se um dia puder ter 2A+B por exemplo, isso aq vai ter q ser
    // alterado, pois se tiver  A em um pacote e 2A em outro, o resultado final
    // vai ter 2A (1 da primeira iteração e outro dessa abaixo).
    i = 0;
    while(h2->holding_packets[i] != EMPTY_PACKET_ID && i < NUM_COMBINATIONS) {
        int cur_item_count = 1, j = 0;

        while(h1->holding_packets[j] != EMPTY_PACKET_ID
              && j < NUM_COMBINATIONS) {
            if(h2->holding_packets[i] == h1->holding_packets[j]) {
                cur_item_count++;
            }
            j++;
        }
        // Even number of elements, then xor nullify this packet
        if(cur_item_count % 2) {
            merged.holding_packets[filled_index++] = h2->holding_packets[i];
        }

        i++;
    }

    return merged;
}

/**
 * @brief Verifies if a packet is raw (it's a original packet).
 *
 * @param packet
 * @return int
 */
static int is_raw_packet(netcoding_packet* packet) {
    return packet->header.holding_packets[1] == EMPTY_PACKET_ID;
}

static size_t packet_hash(netcoding_packet* packet) {
    size_t hash = 0;
    for(int i = 0; i < NUM_COMBINATIONS; i++)
        hash += packet->header.holding_packets[i];
    return hash;
}

#endif /* NET_CODING_PACKET_H_ */