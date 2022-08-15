CFLAGS += -DUSE_LINKLAYER
CFLAGS += -DCCNL_UAPI_H_
CFLAGS += -DUSE_SUITE_NDNTLV
CFLAGS += -DNEEDS_PREFIX_MATCHING
CFLAGS += -DNEEDS_PACKET_CRAFTING

CFLAGS += -DCCNL_CACHE_SIZE=105
CFLAGS += -DCCNL_FACE_TIMEOUT=60
# CFLAGS += -DCCNL_FACE_TIMEOUT=15
# CFLAGS += -DNDN_DEFAULT_INTEREST_LIFETIME=10000
CFLAGS += -DCCNL_MAX_INTEREST_RETRANSMIT=0
CFLAGS += -DCCNL_INTEREST_RETRANS_TIMEOUT=2000
CFLAGS += -DCCNL_STACK_SIZE="THREAD_STACKSIZE_DEFAULT+512"

CFLAGS +=-DOMIT_PRINT_L2_INFO

USEMODULE += ps
USEMODULE += schedstatistics
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ztimer
USEMODULE += ztimer_msec
USEMODULE += ztimer_sec
# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
# USEMODULE += netdev_default # is above now
USEMODULE += auto_init_gnrc_netif
# This application dumps received packets to STDIO using the pktdump module
USEMODULE += gnrc_netapi_callbacks
USEMODULE += prng_xorshift

USEPKG += ccn-lite
USEMODULE += ccn-lite-helpers
EXTERNAL_MODULE_DIRS += $(RIOTBASE)/../external_modules

DOCKER_VOLUMES_AND_ENV += $(call docker_volume,$(RIOTBASE)/../openDSME,$(DOCKER_BUILD_ROOT)/openDSME)
DOCKER_VOLUMES_AND_ENV += $(call docker_volume,$(RIOTBASE)/../ccn-lite,$(DOCKER_BUILD_ROOT)/ccn-lite)
DOCKER_VOLUMES_AND_ENV += $(call docker_volume,$(RIOTBASE)/../external_modules,$(DOCKER_BUILD_ROOT)/external_modules)
