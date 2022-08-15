APPLICATION = ccn-lite-gateway

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT

# If no BOARD is found in the environment, use this default:
BOARD ?= nrf52840dk

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Include common settings, board support (DSME) and Ethos
include $(RIOTBASE)/../external_modules/makefiles/common.mk
include $(RIOTBASE)/../external_modules/makefiles/board.mk
include $(RIOTBASE)/../external_modules/makefiles/ethos.mk

include $(RIOTBASE)/Makefile.include
