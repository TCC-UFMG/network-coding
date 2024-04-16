#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#undef UIP_CONF_UDP_CHECKSUMS
#define UIP_CONF_UDP_CHECKSUMS 0

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0
/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and
 * saves energy. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

/* Logging */
// #define LOG_CONF_LEVEL_RPL LOG_LEVEL_WARN
// #define LOG_CONF_LEVEL_TCPIP LOG_LEVEL_WARN
// #define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_WARN
// #define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_WARN
// #define LOG_CONF_LEVEL_MAC LOG_LEVEL_WARN
/* Do not enable LOG_CONF_LEVEL_FRAMER on SimpleLink,
   that will cause it to print from an interrupt context. */
#ifndef CONTIKI_TARGET_SIMPLELINK
#define LOG_CONF_LEVEL_FRAMER LOG_LEVEL_WARN
#endif
#define TSCH_LOG_CONF_PER_SLOT 0

#endif /* PROJECT_CONF_H_ */