#!/usr/bin/env python

from mininet.net import Mininet
from mininet.node import OVSController
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink
import time
import os

UID=os.environ.get('UID',None)
GID=os.environ.get('GID',None)

APPLICATION="/root/ccn-lite-extensions"
#APPLICATION="/root/RIOT/examples/ccn-lite-relay"

def setupCCNNode(node, start_ccn_lite=True):
    node.cmd('ip tuntap add dev tap0 mode tap')
    node.cmd('brctl addbr tapbr0')
    node.cmd('brctl addif tapbr0 tap0')
    node.cmd('brctl addif tapbr0 '+str(node.intf()))
    node.cmd('ifconfig tapbr0 up')
    node.cmd('ifconfig tap0 up')
    if (start_ccn_lite):
        node.cmd("rm /tmp/hosts/*"+str(node.intf()))
        node.popen(["socat" ,"-d", "-d", "-v", "pty,rawer,link=/tmp/hosts/v_"+str(node.intf()), "EXEC:\"make term BOARD=native\",pty,rawer"],cwd=APPLICATION)
        node.popen(["socat" ,"-d", "-d", "-v", "UNIX-LISTEN:/tmp/hosts/"+str(node.intf())+",fork", "/tmp/hosts/v_"+str(node.intf())],cwd=APPLICATION)
        if (UID and GID):
            node.cmd("sleep 0.3")
            node.cmd(["chown", "{}:{}".format(UID,GID), "/tmp/hosts/"+str(node.intf())])
            node.cmd(["chown", "{}:{}".format(UID,GID), "/tmp/hosts/v_"+str(node.intf())])

def emptyNet():

    "Create an empty network and add nodes to it."

    net = Mininet( controller=OVSController, link=TCLink )

    info( '*** Adding controller\n' )
    net.addController( 'c0' )

    info( '*** Adding hosts\n' )
    root = net.addHost( 'root', ip='10.123.123.1/32', inNamespace=False)
    h1 = net.addHost( 'h1', ip='10.0.0.2' )
    h2 = net.addHost( 'h2', ip='10.0.0.3' )

    info( '*** Adding switch\n' )
    s1 = net.addSwitch( 's1' )
    s2 = net.addSwitch( 's2' )
    s3 = net.addSwitch( 's3' )

    info( '*** Creating links\n' )
    intf = net.addLink( root, s1 ).intf1

    net.addLink(s1, s2, delay='20ms')
    net.addLink(s2, s3, delay='20ms')

    # enable this to set link losses
    # net.addLink( s1, s2, delay='20ms', loss=5 )
    # net.addLink( s2, s3, delay='20ms', loss=5 )

    net.addLink( root, s1 )
    net.addLink( h1, s2 )
    net.addLink( h2, s3 )

    info( '*** Starting network\n')
    net.start()

    root.cmd( 'route add -net 10.0.0.0/24 dev ' + str( intf ) )
    setupCCNNode(root,start_ccn_lite=False)
    setupCCNNode(h1,start_ccn_lite=True)
    setupCCNNode(h2,start_ccn_lite=True)

    info( '*** Running CLI\n' )
    CLI( net )

    info( '*** Stopping network' )
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    emptyNet()
