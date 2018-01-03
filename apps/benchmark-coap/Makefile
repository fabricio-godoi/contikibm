CONTIKI_PROJECT = server client
all: $(CONTIKI_PROJECT)

#PROJECT_SOURCEFILES += statistics.c
#SOURCEDIRS += ../dev
#APPS=servreg-hack
CONTIKI=../..


# CoAP configuration
# linker optimizations
#SMALL=1
# REST Engine shall use Erbium CoAP implementation
APPS += er-coap
APPS += rest-engine

# automatically build RESTful resources
REST_RESOURCES_DIR=$(CONTIKI)/apps/benchmark-coap/resources
#REST_RESOURCES_DIR = ./resources
ifndef TARGET
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
ifeq ($(TARGET), native)
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c' ! -name 'res-plugtest*'))
endif
endif

PROJECTDIRS += $(REST_RESOURCES_DIR)
PROJECT_SOURCEFILES += $(REST_RESOURCES_FILES)

#ifdef WITH_COMPOWER
#APPS+=powertrace
#CFLAGS+= -DCONTIKIMAC_CONF_COMPOWER=1 -DWITH_COMPOWER=1 -DQUEUEBUF_CONF_NUM=4
#endif

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif
ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif


CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include


