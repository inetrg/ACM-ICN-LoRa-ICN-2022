Basic Usage
===========

## Device configuration
Use standard `ifconfig` flags to configure the DSME interface:
```
ifconfig <if_id> (-)<option>
```

The following options are available:
- `pan_coord`: Enable/disable PAN coordinator role (a.k.a "Gateway").
- `ack_req`: Enable/disable ACK request
- `gts`: Enable/disable GTS transmission

E.g to enable PAN coordinator role and disable both ACK request and GTS
transmissions on interface `3`:
```
ifconfig 3 pan_coord
ifconfig 3 -ack_req
ifconfig 3 -gts
```

## Trigger association
Run:
```
ifconfig <if_id> up
```

If device is PAN coordinator, the node associates instantaneously. Otherwise,
the device will scan for beacons and perform association.

Use the `ifconfig` command to get the Link status.
```
Iface  4  HWaddr: BD:3F  Link: up 
          L2-PDU:127  
```

TAP configuration
=================

If you compile this application for boards (only nrf52840dk + LoRa shield currently) it will include one *ethos* network interface that can connect to RIOT native. Therefore, you must:

- Open the the shell with sudo: `sudo BOARD=nrf52840dk make term`. This creates a tap interface named `riot0`.
- In an other terminal, set the link up: `sudo ip link set riot0 up`.
- And link it to the tap bridge initiatey by RIOT tapsetup: `sudo ip link set riot0 master tapbr0`.


ICN Scenarios
=============

## Vanilla
An Internet node makes a standard Interest request to a data item on the Endnode tht produces that data on demand. PITs and retransmissions are likely to timeout on the first try.

Note, downstream FIB entries must be applied beforehand, using `ccnl_fib add <prefix> <MAC address>`. The prefix can be changed with commands (see below) or is `/xid` by default.

Set up the experiments by applying the following shell commands:
Endnodes:`prod [optional id]`
Gateway: -
Forwarders: -
Internet Endpoint: `ccnl_int ...`

## Vanilla adjusted timing
Like vanilla, but the timeouts are increased.

Like above, but adjust the timing parameters.

`params <interest t/o [s]> <retrans t/o [ms] <num retrans>`

## Gateway Terminate
Similar to vanilla, but the Gateway terminates the request. On incoming Interest, three situations can occur on the Gateway:
- The Endnode is not registered, and the Gateway answers with a NACK.
- The Endnode is registered. The Gateway forwards the Interest and directly answers the Internet request with a WAIT item which contains the estimates time for data arrival.
- The data item was retrieved already and the Interest request ist served from teh CS.

Note, downstream FIB entries must be applied beforehand, using `ccnl_fib add <prefix> <MAC address>`. 
The prefix can be changed with commands (see below) or is `/xid` by default.

Endnodes: `prod [optional id]`
Forwarders: `setnw`
Gateway: `setgw`
Internet Endpoint: `setnw`, `ccnl_int ...`

## Reflexive Indication
An Endnode that is registered at the Gateway can push data items to the Gateway cache. The Gateway then sends an indicating Interest to the data sink in the Internet. On forwarding, temporary downstream FIB entries are established. The data sink reacts with a reflexive Interest on the reverse path. This will be answered From the Gateway cache, since the content was placed there beforehand. Data returns to the sink and removes the temporary FIB entries. Note, a final data packet from Internet to Gateway is not yet in place.

Note, upstream FIB entries must be applied beforehand, using `ccnl_fib add <prefix> <MAC address>`. 
The prefix is /RNP by default.
Here, the Endnode will already have a FIB entry for the Gateway, from its registration.

Gateway: `setnodefx`, `setondata`
Forwarders: `setnodefx`
Internet Endpoint: `setsinkfx`
Endnodes: `reg <GW address> [optional pfx]`, `txpush [optional pfx] [optional data]`