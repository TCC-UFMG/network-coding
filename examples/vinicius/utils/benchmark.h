#ifndef BENCHMARK_NETCODING_H_
#define BENCHMARK_NETCODING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../os/sys/mutex.h"
#include "../../../sys/clock.h"
#include "pthread.h"

typedef struct benchmark_send_control_t {
    uint8_t rx_matrix[NUM_RECEIVERS][NUM_SENDERS];
    // NUM PACKETS RECEIVED
    long num_completed;
    pthread_mutex_t num_completed_mtx;
    // NUM PACKETS SENDED
    long num_packets_sended;
    pthread_mutex_t num_packets_count_mutex;
} benchmark_send_control;

extern benchmark_send_control *benchmark_tx_control;

#define REGISTRY_MAX_SIZE 100

static char benchmark_filename[80];

static void create_benchmark_registry() {
    FILE *file = fopen(benchmark_filename, "w");
    if(file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    char *msg = "Tempo, NodeID, MsgID\n";
    fwrite(msg, sizeof(char), strlen(msg), file);
    fclose(file);
    //=======================================================
    for(int i = 0; i < NUM_RECEIVERS; i++)
        for(int j = 0; j < NUM_SENDERS; j++)
            benchmark_tx_control->rx_matrix[i][j] = 0;
    pthread_mutex_init(&benchmark_tx_control->num_completed_mtx, NULL);
    benchmark_tx_control->num_completed = 0;
    //=======================================================
    benchmark_tx_control->num_packets_sended = 0;
    pthread_mutex_init(&benchmark_tx_control->num_packets_count_mutex, NULL);
}

/**
 * @brief Mark a message as received by a node. Returns the number of number of
 * completed messages.
 *
 * @param node_id
 * @param msg_id
 * @return long
 */
static long mark_msg_as_received(int node_id, int msg_id) {
    if(node_id >= NUM_RECEIVERS) exit(1);
    pthread_mutex_lock(&benchmark_tx_control->num_completed_mtx);
    // Safe region
    long already_received = ++benchmark_tx_control->num_completed;
    // Safe region
    pthread_mutex_unlock(&benchmark_tx_control->num_completed_mtx);
    // ! % NUM_SENDERS because I'm warranted that only  NUM_SENDERS messages
    // ! gonna be used, then I don't care about the order in the array, only
    // ! that they stay aways in the same column
    benchmark_tx_control->rx_matrix[node_id][msg_id % NUM_SENDERS] = 1;
    return already_received;
}

static int msg_was_received(int node_id, int msg_id) {
    if(node_id >= NUM_RECEIVERS) exit(1);
    return benchmark_tx_control->rx_matrix[node_id][msg_id % NUM_SENDERS];
}

/**
 * @brief Increments the global count of transmitted packets.
 *
 */
static void increment_packets_count() {
    pthread_mutex_lock(&benchmark_tx_control->num_packets_count_mutex);
    // Safe region
    benchmark_tx_control->num_packets_sended++;
    // Safe region
    pthread_mutex_unlock(&benchmark_tx_control->num_packets_count_mutex);
}

static long get_global_packet_count() {
    pthread_mutex_lock(&benchmark_tx_control->num_packets_count_mutex);
    // Safe region
    int count = benchmark_tx_control->num_packets_sended;
    // Safe region
    pthread_mutex_unlock(&benchmark_tx_control->num_packets_count_mutex);
    return count;
}

//=============================================================================

static void registers_heap_benchmark_control() {
    FILE *file =
        fopen("../../examples/vinicius/2xor/benchmarks/tmp-var.csv", "w");
    if(file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }

    benchmark_tx_control =
        (benchmark_send_control *)malloc(sizeof(benchmark_send_control));
    create_benchmark_registry();

    char buffer[20];
    sprintf(buffer, "%p", benchmark_tx_control);
    fwrite(buffer, sizeof(char), strlen(buffer), file);
    fclose(file);
}

static void get_heap_benchmark_control() {
    FILE *file;
    do {
        file =
            fopen("../../examples/vinicius/2xor/benchmarks/tmp-var.csv", "r");
    } while(file == NULL);

    char buffer[20];
    void *ptr;

    fread(buffer, sizeof(char), 14, file);
    fclose(file);
    sscanf(buffer, "%p", &ptr);

    benchmark_tx_control = (benchmark_send_control *)ptr;
}

//=============================================================================

/**
 * @brief Function to append a line to the end of the file.
 *
 * @param file
 * @param registry
 */
static void add_benchmark_registry(const char *registry) {
    FILE *file = fopen(benchmark_filename, "a");
    if(file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    fwrite(registry, sizeof(char), strlen(registry), file);
    fclose(file);
}

/**
 * @brief Writes a registry of received packet into a buffer.
 *
 * @param node_id
 * @param msg_id
 * @param output
 */
static void create_received_registry(int node_id,
                                     int msg_id,
                                     char output[REGISTRY_MAX_SIZE]) {
    clock_time_t time = clock_time();
    unsigned long seconds = (unsigned long)time / CLOCK_CONF_SECOND;
    unsigned int miliseconds = (double)(time & (CLOCK_CONF_SECOND - 1))
                               / CLOCK_CONF_SECOND
                               * 1000;  // the 7 last bits are ms
    sprintf(output, "%ld.%d,%d,%d\n", seconds, miliseconds, node_id, msg_id);
}

#endif