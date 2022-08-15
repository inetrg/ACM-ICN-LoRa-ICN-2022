APPLICATION = ccn-lite-node

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT

# If no BOARD is found in the environment, use this default:
BOARD ?= nrf52840dk

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

include $(RIOTBASE)/../external_modules/makefiles/common.mk

ifeq (native,$(BOARD))
include $(RIOTBASE)/../external_modules/makefiles/native.mk
else
include $(RIOTBASE)/../external_modules/makefiles/board.mk
endif

include $(RIOTBASE)/Makefile.include
