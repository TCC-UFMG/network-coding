#ifndef BENCHMARK_NETCODING_H_
#define BENCHMARK_NETCODING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../sys/clock.h"

#define REGISTRY_MAX_SIZE 100

static char benchmark_filename[80];

void create_benchmark_registry() {
    FILE *file = fopen(benchmark_filename, "w");
    if(file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    char *msg = "Tempo, NodeID, MsgID\n";
    fwrite(msg, sizeof(char), strlen(msg), file);
    fclose(file);
}

/**
 * @brief Function to append a line to the end of the file.
 *
 * @param file
 * @param registry
 */
void add_benchmark_registry(const char *registry) {
    FILE *file = fopen(benchmark_filename, "a");
    if(file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    fwrite(registry, sizeof(char), strlen(registry), file);
    fclose(file);
}

void create_received_registry(int node_id,
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