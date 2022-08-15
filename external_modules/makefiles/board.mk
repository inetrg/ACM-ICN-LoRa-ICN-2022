DISABLE_MODULE += auto_init_nrf802154
ifeq (1,$(USE_ETHOS))
endif

## cpy from jose
FEATURES_REQUIRED += cpp # basic C++ support
FEATURES_REQUIRED += libstdcpp # libstdc++ support (for #include <cstdio>)

CXXEXFLAGS += -Wno-unused-parameter -Wno-pedantic -Wno-deprecated-copy -Wno-missing-field-initializers -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-unused-variable -Wno-reorder -Wno-address -Wno-sign-compare -Wno-unused-function -Wno-error
USEPKG += opendsme
USEMODULE += ztimer_msec
USEMODULE += gnrc_pktbuf_malloc
USEMODULE += cpp11-compat
USEMODULE += ieee802154
USEMODULE += od
USEMODULE += luid
USEMODULE += l2util
USEMODULE += gnrc
USEMODULE += checksum
USEMODULE += gnrc_netif
USEMODULE += gnrc_netif_events
USEMODULE += auto_init_gnrc_netif

USEMODULE += sx1276
USEMODULE += gnrc_netapi_callbacks
USEMODULE += event_thread
USEMODULE += event_periodic

# OpenDSME Configs
CFLAGS += -DCONFIG_DSME_PLATFORM_STATIC_GTS=1
CFLAGS += -DUSE_CAD=1
CFLAGS += -DCAP_REDUCTION=0
CFLAGS += -DCONFIG_DSME_MAC_SUPERFRAME_ORDER=3
CFLAGS += -DCONFIG_DSME_MAC_MULTISUPERFRAME_ORDER=5
CFLAGS += -DCONFIG_DSME_MAC_BEACON_ORDER=5

# RIOT Configs
CFLAGS += -DGNRC_PKTDUMP_STACKSIZE=700
CFLAGS += -DISR_STACKSIZE=1024
CFLAGS += -DEVENT_THREAD_MEDIUM_PRIO=THREAD_PRIORITY_MAIN+1
CFLAGS += -DCONFIG_IEEE802154_AUTO_ACK_DISABLE=1
CFLAGS += -DCONFIG_GNRC_PKTBUF_SIZE=512
CFLAGS += -DDEBUG_ASSERT_VERBOSE
CFLAGS += -DSX127X_STACKSIZE=2048
