#!/usr/bin/env python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Node
from mininet.log import setLogLevel, info
from mininet.cli import CLI
from mininet.link import TCLink
import sys

class LinuxRouter( Node ):
    "A Node with IP forwarding enabled."

    # pylint: disable=arguments-differ
    def config( self, **params ):
        super( LinuxRouter, self).config( **params )
        # Enable forwarding on the router
        self.cmd( 'sysctl net.ipv4.ip_forward=1' )

    def terminate( self ):
        self.cmd( 'sysctl net.ipv4.ip_forward=0' )
        super( LinuxRouter, self ).terminate()


class NetworkTopo( Topo ):
    "A LinuxRouter connecting three IP subnets"


    def build( self, **_opts ):

        defaultIP = '192.168.1.1/24'  # IP address for r0-eth1
        router = self.addNode( 'r0', cls=LinuxRouter, ip=defaultIP) 


        h1 = self.addHost( 'h1', ip='192.168.1.100/24',
                           defaultRoute='via 192.168.1.1' )

        h2 = self.addHost( 'h2', ip='172.16.0.100/12',
                           defaultRoute='via 172.16.0.1' )

        h3 = self.addHost( 'h3', ip='182.17.0.100/12',
                           defaultRoute='via 182.17.0.1' )

        # TODO: 10 Mbps, 1ms delay, 0 packet loss
        self.addLink( h1, router, intfName1='r0-eth1', bw=10, delay='1ms', loss=0,
                     params1={ 'ip' : '192.168.1.1/24' })  # for clarity

        # TODO: 10 Mbps, 5ms delay, 10% packet loss
        self.addLink( h2, router, intfName2='r0-eth2', bw=10, delay='1ms', loss=0,
                      params2={ 'ip' : '172.16.0.1/12' } )

        # TODO: 10 Mbps, 5ms delay, 10% packet loss
        self.addLink( h3, router, intfName2='r0-eth3', bw=10, delay='1ms', loss=0,
                      params2={ 'ip' : '182.17.0.1/12' } )


def run():
    "Test linux router"
    topo = NetworkTopo()
    net = Mininet( topo=topo, link=TCLink,
                   waitConnected=True, controller=None)  # controller is used by s1-s3
    net.start()

    net.startTerms()
    CLI( net )
    net.stop()


if __name__ == '__main__':
    setLogLevel('critical')
    run()
