USEMODULE += stdio_ethos

# ethos baudrate can be configured from make command
ETHOS_BAUDRATE ?= 115200
CFLAGS += -DETHOS_BAUDRATE=$(ETHOS_BAUDRATE)

# make sure ethos and uhcpd are built
TERMDEPS += host-tools

# For local testing, run
#
#     $ cd dist/tools/ethos; sudo ./setup_network.sh riot0 2001:db8::0/64
#
#... in another shell and keep it running.
export TAP ?= tap0
TERMPROG = $(RIOTTOOLS)/ethos/ethos
TERMFLAGS = $(TAP) $(PORT)

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)

