#include <unistd.h>
#include <stdint.h>

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP		0x0800	/* IP protocol */
#endif

struct  ether_header {
    uint8_t  ether_dhost[6];
    uint8_t  ether_shost[6];
    uint16_t ether_type;     // ETHERTYPE_IP
};

struct icmphdr {   /* are exact 8 octeti  */
    uint8_t type;      // (0 = Reply, 8 = Request, 11 = Time Exceeded)
    uint8_t code;
    uint16_t checksum; // checksum for the entire ICMP header
    
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

struct iphdr {
    uint8_t   ihl:4, version:4;// don't care
    uint8_t    tos;      // don't care
    uint16_t   tot_len;  // don't care <- in cazul asta, o sa fie 20 (IP header length) + 8 (ICMP header length)
    uint16_t   id;       // don't care
    uint16_t   frag_off; // don't care
    uint8_t    ttl;      // Time to Live -> to avoid loops, we will decrement
    uint8_t    protocol; // for ICMP, protocol = 1 (TCP = 6, UDP = 17)
    uint16_t   check;    // checksum     -> Since we modify TTL,
    // we need to recompute the checksum
    uint32_t   saddr;    // don't care
    uint32_t   daddr;    // the destination of the packet
};
