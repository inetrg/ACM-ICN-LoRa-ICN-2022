#!/bin/bash

: ${COMPILE:=1}
: ${FLASH:=1}
GW_SERIAL=$(make -C ../ccn-lite-gateway list-ttys | awk -F "'" '/ttyACM0/{print $2}')
NODE_SERIAL=$(make -C ../ccn-lite-gateway list-ttys | awk -F "'" '/ttyACM1/{print $2}')

## Compile
if [ $COMPILE -eq 1 ]; then
    BUILD_IN_DOCKER=1 BOARD=native make -C ../ccn-lite-extensions/ clean all -j4
    BUILD_IN_DOCKER=1 make -C ../ccn-lite-extensions/ clean all -j4
    BUILD_IN_DOCKER=1 make -C ../ccn-lite-gateway/ clean all -j4
fi

## Flash
if [ $FLASH -eq 1 ]; then
    DEBUG_ADAPTER_ID=$GW_SERIAL make -C ../ccn-lite-gateway/ flash-only
    DEBUG_ADAPTER_ID=$NODE_SERIAL make -C ../ccn-lite-extensions/ flash-only
fi
