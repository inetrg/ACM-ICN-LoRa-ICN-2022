#!/bin/bash

EXP_VANILLAA1="vanillaa1"
EXP_VANILLAB1="vanillab1"
EXP_VANILLAC1="vanillac1"
EXP_VANILLAA2="vanillaa2"
EXP_VANILLAB2="vanillab2"
EXP_VANILLAC2="vanillac2"
EXP_CUSTODIAN="cust"
EXP_PUSH="push"

if [ "$#" -lt 2 ]; then
      echo "specify experiment: <$EXP_VANILLAA1 | $EXP_VANILLAB1 | $EXP_VANILLAC1 | $EXP_VANILLAA2 | $EXP_VANILLAB2 | $EXP_VANILLAC2 | $EXP_CUSTODIAN | $EXP_PUSH> <num transactions>"
      exit 1
else
    echo "Run experiment $1 with $2 transactions"
    CUR_EXP=$1
    EXP_REPS=$2
fi


# Configs and variables
CONSUMER=/tmp/consumer
FORWARDER=/tmp/fwd
GATEWAY=/tmp/gw
NODE=/tmp/node

## Functions
function reboot ()
{
    echo "reboot" > $1
    read -t 0.5 INPUT
}

# Return address of first interface!
function get_addr () {
    echo "ifconfig" | socat - $1 | awk -F " " '/Iface/{printf "%s ", $4} END{printf "\n"}'
}

function get_ifaces () {
    echo "ifconfig" | socat - $1 | awk -F " " '/Iface/{printf "%s ", $2} END{printf "\n"}'
}

function info () {
    echo "INFO " $1
}

function flush () {
    read -t 0.5 FLUSH < $1
}

function exp () {
    python -c "import numpy as np;print(np.random.exponential($1, 1)[0])"
}

function randy_uniform () {
    python -c "import numpy as np;print(np.random.uniform($1, $2, 1)[0])"
}

function join_network_up() {
    # $1: GW port
    # $2: GW interface
    # $3: Node port
    # $4: Node interface

    info "Set up coordinator"
    echo "ifconfig $2 pan_coord" > $1
    echo "ifconfig $2 up" > $1
    sleep 4
    info "Set up node"
    echo "ifconfig $4 up" > $3
    return 0
}

function join_network_assoc() {
    info "Wait for association..."
    sleep 70
    echo "TIMER ENDED. check for node state"
    LINK_UP=$(echo "ifconfig" | socat - $3 | awk '/Link: up/')
    if [ -n  "$LINK_UP" ]; then
        return 0
    fi
    return 1
}

function print_fibs() {
    sleep 0.5
    echo "ccnl_fib" > $CONSUMER
    echo "ccnl_fib" > $FORWARDER
    echo "ccnl_fib" > $GATEWAY
    echo "ccnl_fib" > $NODE
    sleep 0.5
}

function set_fibs_downlink() {
    # use applications default prefix /xid
    sleep 0.5
    echo "ccnl_fib add /xid $FORWARDER_ADDR" > $CONSUMER
    sleep 0.5
    echo "ccnl_fib add /xid $GATEWAY_ADDR_ETHOS" > $FORWARDER
    sleep 0.5
    echo "ccnl_fib add /xid $NODE_ADDR $GATEWAY_PID_DSME" > $GATEWAY
    sleep 0.5
}

function set_fibs_uplink() {
    # use applications default prefix /xid
    sleep 0.5
    echo "ccnl_fib add /RNP $GATEWAY_ADDR_DSME" > $NODE
    sleep 0.5
    echo "ccnl_fib add /RNP $FORWARDER_ADDR $GATEWAY_PID_ETHOS" > $GATEWAY
    sleep 0.5
    echo "ccnl_fib add /RNP $CONSUMER_ADDR" > $FORWARDER
    sleep 0.5
}

function init_vanilla_case_a1() {
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "prod" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 4 1000 4" > $FORWARDER
    sleep 0.5
    echo "params 4 1000 4" > $CONSUMER
    sleep 0.5
}

function init_vanilla_case_b1() {
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "prod" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 4 1000 4" > $FORWARDER
    sleep 0.5
    echo "params 64 15000 4" > $CONSUMER
    sleep 0.5
}

function init_vanilla_case_c1() {
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "prod" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 64 1000 4" > $FORWARDER
    sleep 0.5
    echo "params 64 15000 4" > $CONSUMER
    sleep 0.5
}

function init_vanilla_case_a2() {
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "prod" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 4 0 0" > $FORWARDER
    sleep 0.5
    echo "params 4 1000 4" > $CONSUMER
    sleep 0.5
}

function init_vanilla_case_b2() {
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "prod" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 4 0 0" > $FORWARDER
    sleep 0.5
    echo "params 64 15000 4" > $CONSUMER
    sleep 0.5
}

function init_vanilla_case_c2() {
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "prod" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $NODE
    sleep 0.5
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 60 0 0" > $FORWARDER
    sleep 0.5
    echo "params 64 15000 4" > $CONSUMER
    sleep 0.5
}

function init_custodian_case() {
    # with downlink route, we assume node was registeres and save skip that step here
    set_fibs_downlink
    print_fibs
    # set node local producer. no argument uses default ID /xid
    echo "setnw" > $CONSUMER
    echo "setnw" > $FORWARDER
    echo "setgw" > $GATEWAY
    echo "prod" > $NODE
    sleep 0.5
    # not crucial because data is sent back "immediately"
    echo "params 64 0 0" > $NODE
    sleep 0.5
    # increase interest lifetime on gw to extrem case of ~ 2+MSF
    # one retransmit after > 1MSF so that
    echo "params 64 0 0" > $GATEWAY
    sleep 0.5
    # set "default" on other nodes. 4s lifetime, 1sec retrans timer, 4 retransmits
    echo "params 4 1000 4" > $FORWARDER
    sleep 0.5
    echo "params 4 1000 4" > $CONSUMER
    sleep 0.5
}

function init_push_case() {
    set_fibs_uplink
    # set node local producer. no argument uses default ID /xid
    echo "setnodefx" > $GATEWAY
    echo "setondata" > $GATEWAY
    echo "setnodefx" > $FORWARDER
    echo "setsinkfx" > $CONSUMER
    sleep 0.5
    # inode doesnt transmit intrest here anyway
    echo "params 4 0 0" > $NODE
    sleep 0.5
    # for now, disable interest retrans because we did not implement final
    # data ACK to terminate initial indication interest
    # there is no loss on "the interenet" -> not critical
    echo "params 4 1000 4" > $GATEWAY
    sleep 0.5
    echo "params 4 1000 4" > $FORWARDER
    sleep 0.5
    echo "params 4 1000 4" > $CONSUMER


    # temporats disbale GTS to do reg with CAP
    echo "ifconfig $NODE_PID -gts" > $NODE
    echo "ifconfig $GATEWAY_PID_DSME -gts" > $GATEWAY
    # the node uses /xid to register by default. it doesn't need to know about the /RNP prefix used in the internet, fot trigger reflexive forwarding
    echo "reg $GATEWAY_ADDR_DSME" > $NODE
    sleep 30
    # enable GTS again
    echo "ifconfig $NODE_PID gts" > $NODE
    echo "ifconfig $GATEWAY_PID_DSME gts" > $GATEWAY
    sleep 1
    print_fibs
}

function print_ps() {
    echo "ps" > $NODE
    echo "ps" > $GATEWAY
}

## Setup sockets
socat /tmp/hosts/h2-eth0 pty,rawer,link=$CONSUMER &
socat /tmp/hosts/h1-eth0 pty,rawer,link=$FORWARDER &
# Reset border router
make -C ../ccn-lite-gateway/ reset
socat pty,rawer,link=$GATEWAY EXEC:'make -C ../ccn-lite-gateway/ term' &
socat pty,rawer,link=$NODE open:/dev/ttyACM1,b115200,echo=0,rawer &
sleep 0.5

# Send reboot command
info "Rebooting..."
reboot $CONSUMER
reboot $FORWARDER
reboot $NODE

echo "done!"

CONSUMER_PID="$(get_ifaces $CONSUMER)"
echo "CONSUMER_PID: $CONSUMER_PID"
FORWARDER_PID="$(get_ifaces $FORWARDER)"
echo "FORWARDER_PID: $FORWARDER_PID"
GW_PID="$(get_ifaces $GATEWAY)"
echo "GATEWAY_PID_DSME: $GATEWAY_PID_DSME"
GATEWAY_PID_DSME="$(echo $GW_PID | awk -F " " '{print $1}')"
echo "GATEWAY_PID_DSME: $GATEWAY_PID_DSME"
GATEWAY_PID_ETHOS="$(echo $GW_PID | awk -F " " '{print $2}')"
echo "GATEWAY_PID_ETHOS: $GATEWAY_PID_ETHOS"
NODE_PID="$(get_ifaces $NODE)"
echo "NODE_PID: $NODE_PID"


sleep 0.5

join_network_up $GATEWAY $GATEWAY_PID_DSME $NODE $NODE_PID

CONSUMER_ADDR="$(get_addr $CONSUMER)"
FORWARDER_ADDR="$(get_addr $FORWARDER)"
GW_ADDRESSES=$(get_addr $GATEWAY)
GATEWAY_ADDR_DSME="$(echo $GW_ADDRESSES | awk -F " " '{print $1}')"
GATEWAY_ADDR_ETHOS="$(echo $GW_ADDRESSES | awk -F " " '{print $2}')"
NODE_ADDR="$(get_addr $NODE)"

echo "CONSUMER_ADDR: $CONSUMER_ADDR"
echo "FORWARDER_ADDR: $FORWARDER_ADDR"
echo "GATEWAY_ADDR_DSME: $GATEWAY_ADDR_DSME"
echo "GATEWAY_ADDR_ETHOS: $GATEWAY_ADDR_ETHOS"
echo "NODE_ADDR: $NODE_ADDR"

if [ -z "$CONSUMER_ADDR" ] || [ -z "$FORWARDER_ADDR" ] || [ -z "$GATEWAY_ADDR_DSME" ] || [ -z "$GATEWAY_ADDR_ETHOS" ] || [ -z "$NODE_ADDR" ]; then
    info "One address variable empty. Exit...."
    # Kill socat
    kill $(ps -o pid=) > /dev/null
    kill $$
    exit 1
fi

sleep 0.5

join_network_assoc $GATEWAY $GATEWAY_PID_DSME $NODE $NODE_PID
if [ $? -eq 0 ]; then
    info "Successfully joined the network!"
else
    info "Failed to join the network!"
    kill $(ps -o pid=) > /dev/null
    kill $$
    exit 1
fi

info "START OF MEASUREMENTS"
socat - $NODE | ts "%.s;NODE;" &
socat - $GATEWAY | ts "%.s;GATEWAY;" &
socat - $FORWARDER | ts "%.s;FORWARDER;" &
socat - $CONSUMER | ts "%.s;CONSUMER;" &

sleep 0.5

## Set static GTS slots. ATTENTION! these are the hard coded PIDs of the interfaces
echo "ifconfig $NODE_PID gts" > $NODE
echo "ifconfig $GATEWAY_PID_DSME gts" > $GATEWAY

# The format of the "gts" commands is:
# `gts <neighbour_addr> <tx> <superframe_id> <slot_id> <channel_id>`
# Where:
#    neighbour_addr: The neighbour device to associate a slot with
#    tx: 1 if the GTS is TX. 0 otherwise
#    superframe_id: The superframe ID in range {0..(2^(MO-SO))-1}
#    slot_id: The slot ID in range 0-6 if superframe_id == 1. Otherwise, values
#             from 8-14 if CAP_REDUCTION=false and 0-14 if CAP_REDUCTION=true.
#    channel_id: The channel ID in range 0-15. This DOES NOT map to a IEEE 802.15.4
#                channel but rather a frequency slot that might do channel hopping
#                or channel adaptation.
#
# For effects of the experiment, we set a TX slot from gateway to node on the first
# GTS and a TX slot from node to gateway on the seconds GTS.

# Set Gateway->Node link
echo "gts $NODE_ADDR 1 0 0 0" > $GATEWAY
echo "gts $GATEWAY_ADDR_DSME 0 0 0 0" > $NODE

# Set Node->Gateway link
echo "gts $GATEWAY_ADDR_DSME 1 0 1 0" > $NODE
echo "gts $NODE_ADDR 0 0 1 0" > $GATEWAY

############### Add CCN experiment logic from here #############################


#Add a delay before sending data to the gateway again...
sleep 0.5

#Again...
sleep 0.5

# test: set params to 30 interest t/o and no retransmissions
# echo "params 30 0 0" > $CONSUMER
# echo "params 30 0 0" > $FORWARDER
# echo "params 30 0 0" > $GATEWAY
# echo "params 30 0 0" > $NODE

sleep 0.5
echo "$CUR_EXP"

if [ "$CUR_EXP" == "$EXP_VANILLAA1" ]; then
    echo "Start vanilla scenario A1"
    init_vanilla_case_a1
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "vanilla A1 case done"
fi

if [ "$CUR_EXP" == "$EXP_VANILLAB1" ]; then
    echo "Start vanilla scenario B1"
    init_vanilla_case_b1
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "vanilla B1 case done"
fi

if [ "$CUR_EXP" == "$EXP_VANILLAC1" ]; then
    echo "Start vanilla scenario C1"
    init_vanilla_case_c1
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "vanilla C1 case done"
fi

if [ "$CUR_EXP" == "$EXP_VANILLAA2" ]; then
    echo "Start vanilla scenario A2"
    init_vanilla_case_a2
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "vanilla A2 case done"
fi

if [ "$CUR_EXP" == "$EXP_VANILLAB2" ]; then
    echo "Start vanilla scenario B2"
    init_vanilla_case_b2
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "vanilla B2 case done"
fi

if [ "$CUR_EXP" == "$EXP_VANILLAC2" ]; then
    echo "Start vanilla scenario C2"
    init_vanilla_case_c2
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "vanilla C2 case done"
fi

if [ "$CUR_EXP" == "$EXP_CUSTODIAN" ]; then
    echo "Start gw custodian scenario"
    init_custodian_case
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        print_ps
        echo "ccnl_int /xid/$i" > $CONSUMER
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "custodian case done"
fi

if [ "$CUR_EXP" == "$EXP_PUSH" ]; then
    echo "Start push scenario"
    # # this waits for L3 node registration internally
    init_push_case
    sleep 3
    for i in `seq 1 $EXP_REPS`; do
        # # the node does not know aobut the RNP prefix for reflexive interests
        print_ps
        echo "txpush /xid/$i abc" > $NODE
        # TIME=$(exp 60)
        TIME=$(randy_uniform 50 70)
        echo "Sleep for $TIME"
        sleep $TIME
    done
    echo "push case done"
fi

print_ps
sleep 30

info "Cleanup"
info "All processed killed!"
# Kill socat
kill $(ps -o pid=) > /dev/null
kill $$
exit 0
