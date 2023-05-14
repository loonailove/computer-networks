#!/usr/bin/env python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Node
from mininet.log import setLogLevel, info
from mininet.cli import CLI
from mininet.link import TCLink
import sys


class LinuxRouter(Node):
    "A Node with IP forwarding enabled."

    # pylint: disable=arguments-differ
    def config(self, **params):
        super(LinuxRouter, self).config(**params)
        # Disable forwarding on the router
        self.cmd("sysctl net.ipv4.ip_forward=0")
        for i in {0, 1}:
            self.cmd('ethtool iface r-eth{} --offload rx off tx off'.format(i))
            self.cmd('ethtool -K r-eth{} tx-checksum-ip-generic off'.format(i))

    def terminate(self):
        super(LinuxRouter, self).terminate()


class NetworkTopo(Topo):
    "A LinuxRouter connecting three IP subnets"
    def build(self, **_opts):
        router = self.addNode("r", cls=LinuxRouter,
                              ip="10.10.10.1/24")

        alice = self.addHost("alice", ip="10.10.10.2/24")

        bob = self.addHost("bob", ip="55.55.55.2/24")
        # bob.cmd("ip route add default via 55.55.55.1/24")

        #  10 Mbps, 1ms delay, 0 packet loss
        self.addLink(alice, router, intfName1="r-eth0", intfName2="r-eth0",
                     loss=0, params1={"ip" : "10.10.10.1/24"})

        #  10 Mbps, 1ms delay, 0% packet loss
        self.addLink(bob, router, intfName1="r-eth1", intfName2="r-eth1",
                     loss=0, params2={"ip" : "55.55.55.1/12"})


class NM(object):
    def __init__(self, net):
        self.net = net
        self.hosts = [self.net.get("alice"), self.net.get("bob")]

    def add_default_routes(self):
        self.hosts[0].cmd("ip route add default via 10.10.10.1")
        self.hosts[1].cmd("ip route add default via 55.55.55.1")

    def disable_unneeded(self):
        def disable_ipv6(host):
            host.cmd('sysctl -w net.ipv6.conf.all.disable_ipv6=1')
            host.cmd('sysctl -w net.ipv6.conf.default.disable_ipv6=1')

        def disable_nic_checksum(host, iface):
            host.cmd('ethtool iface r-eth{} --offload rx off tx off'.format(i))
            host.cmd('ethtool -K r-eth{} tx-checksum-ip-generic off'.format(i))

        for i, host in enumerate(self.hosts):
            disable_ipv6(host)
            disable_nic_checksum(host, i);

    def setup_macs(self):
        self.hosts[0].cmd("ifconfig r-eth0 hw ether 00:00:aa:aa:aa:aa")
        self.hosts[1].cmd("ifconfig r-eth1 hw ether 00:00:bb:bb:bb:bb")

    def setup(self):
        self.disable_unneeded()
        # self.setup_ifaces()
        self.setup_macs()
        # self.add_hosts_entries()
        self.add_default_routes()

def run():
    topo = NetworkTopo()
    net = Mininet(topo=topo, link=TCLink, waitConnected=True, controller=None)
    net.start()

    nm = NM(net)
    nm.setup()

    net.startTerms()
    CLI(net)
    net.stop()


if __name__ == "__main__":
    setLogLevel("critical")
    run()
