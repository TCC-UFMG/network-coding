CONTIKI_PROJECT = udp-client udp-server
MAKE_MAC = MAKE_MAC_TSCH

all: $(CONTIKI_PROJECT)

CONTIKI=../../.
CFLAGS+= -DPROJECT_CONF_H=\"project-conf.h\" -Wno-unused-function

include $(CONTIKI)/Makefile.include
