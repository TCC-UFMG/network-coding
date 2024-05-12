all: $(CONTIKI_PROJECT)

CONTIKI=../../../..

MAKE_MAC = MAKE_MAC_TSCH

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.identify-target

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\" -I ../netcoding -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-uninitialized -Wno-maybe-uninitialized -Wno-memset-elt-size
CONTIKI_WITH_IPV6 = 1

include $(CONTIKI)/Makefile.include
