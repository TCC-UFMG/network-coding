#ifndef NET_CODING_UTILS_H_
#define NET_CODING_UTILS_H_

#include "net/ipv6/uip-sr.h"
#include "sys/log.h"

static void netcoding_log_format(const char* type,
                                 int node_id,
                                 int has_preamble,
                                 uip_ip6addr_t* addr) {
    printf("[%s| ID:%d | P:%d", type, node_id, has_preamble);
    if(has_preamble) {
        printf(" | ");
        log_6addr(&UIP_IP_BUF->srcipaddr);
    }
    else {
        printf("                  ");
    }
    printf("] ");
}

#endif /* NET_CODING_UTILS_H_ */