We write the implementation of a forwarding process of a router.

``router.c`` - the code that will run on the router device

``lib.h`` - available API for this lab. Check the utilities function we have
defined there.

``protocols.h`` - structures for the protocols that we will be using today.
Ethernet and IP.

We have the following API today:

```c
/* Receives a packet. Returns the interface it has been received from.
Write to len the total size of the packet (how many bytes in buf are written) */
/* Blocking function, blocks if there is no packet to be received. */
int recv_from_all_links(char *packet, int *len);

/* Sends a packet on an interface. */
int send_to_link(int interface, char *packet, int len);

/* Write to mac, a uint8_t mac[6] the MAC address of an interface */
int get_interface_mac(int interface, uint8_t *mac);

/* Returns the checksum of an IP header */
uint16_t ip_checksum(void* vdata, size_t length);
```

We use the following structures for the MAC table and route table:

```c
/* MAC Table Entry */
struct mac_entry {
	int32_t ip;
	uint8_t mac[6];
};

/* Route Table Entry */
struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
};
```

## Topology

```
h0 <------>(r-0)|--------|(r-1) <---------> h1
                |        |
                | Router |
                |        |
h3 <------>(r-3)|--------|(r-2) <---------> h2

r-1, r-2, r-3, r-4 interfaces
```

## IP adresses
```
host0 - 192.168.0.2
host1 - 192.168.1.2
host2 - 192.168.2.2
host3 - 192.168.3.2
```

## Usage

To compile the code
```
make
```

To start the topology:
```bash
sudo pkill ovs-test
sudo python3 topo.py
```

To start the `router`, in the `router` device, simply run `./router`.

## C++

Note, if you want to use C++, simply change the extension of the `router.cpp` 
and update the Makefile to use `g++`.
