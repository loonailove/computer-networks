#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <unistd.h>
#include <stdint.h>

#define ETHERTYPE_IP     0x0800  /* IP protocol */
#define IPTYPE_TCP       0x06  /* TCP protocol */

/* Ethernet frame header*/
struct  ether_header {
    uint8_t  ether_dhost[6]; //adresa mac destinatie
    uint8_t  ether_shost[6]; //adresa mac sursa
    uint16_t ether_type;     // identificator protocol encapsulat
} __attribute__((packed));

/* IP Header */
struct iphdr {
    // this means that version uses 4 bits, and ihl 4 bits
    uint8_t    ihl:4, version:4;   // we use version = 4
    uint8_t    tos;      // we don't use this, set to 0
    uint16_t   tot_len;  // total length = ipheader + data
    uint16_t   id;       // id of this packet
    uint16_t   frag_off; // we don't use fragmentation, set to 0
    uint8_t    ttl;      // Time to Live -> to avoid loops, we will decrement
    uint8_t    protocol; // don't care
    uint16_t   check;    // checksum     -> Since we modify TTL,
    // we need to recompute the checksum
    uint32_t   saddr;    // source address
    uint32_t   daddr;    // the destination of the packet
} __attribute__((packed));

struct tcp {
        uint16_t tcp_src;
        uint16_t tcp_dst;
        uint32_t tcp_seq_num;
        uint32_t tcp_ack_num;
        uint8_t tcp_res1:4;
        uint8_t tcp_hdr_len:4;
        uint8_t tcp_fin:1;
        uint8_t tcp_syn:1;
        uint8_t tcp_rst:1;
        uint8_t tcp_psh:1;
        uint8_t tcp_ack:1;
        uint8_t tcp_urg:1;
        uint8_t tcp_res2:2;
        uint16_t tcp_win_size;
        uint16_t tcp_chk;
        uint16_t tcp_urg_ptr;
} __attribute__((packed));

#endif /* PROTOCOLS_H */
