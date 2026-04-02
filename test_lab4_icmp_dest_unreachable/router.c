#include <arpa/inet.h> /* ntoh, hton and inet_ functions */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lib.h"
#include "protocols.h"

/* Routing table */
struct route_table_entry *rtable;
int rtable_len;

/* Mac table */
struct mac_entry *mac_table;
int mac_table_len;

/*
 Returns a pointer (eg. &rtable[i]) to the best matching route, or NULL if there
 is no matching route.
*/
// LPM
struct route_table_entry *get_best_route(uint32_t ip_dest) {
    struct route_table_entry *best = NULL;
    for (int i = 0; i < rtable_len; i++) {
        if ((ip_dest & rtable[i].mask) == (rtable[i].prefix & rtable[i].mask)) {
			/* le comparam in host order */
			/* vrem masca cea mai mare (specifica) */
            if (best == NULL || ntohl(rtable[i].mask) > ntohl(best->mask)) {
                best = &rtable[i];
            }
        }
    }
    return best;
}

struct mac_entry *get_mac_entry(uint32_t given_ip) {
    for (int i = 0; i < mac_table_len; i++) {
        if (mac_table[i].ip == given_ip) {
            return &mac_table[i];
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
	int interface;
	char packet[MAX_LEN];
	int packet_len;

	/* Don't touch this */
	init();

	/* Code to allocate the MAC and route tables */
	rtable = malloc(sizeof(struct route_table_entry) * 100);
	/* DIE is a macro for sanity checks */
	DIE(rtable == NULL, "memory");

	mac_table = malloc(sizeof(struct  mac_entry) * 100);
	DIE(mac_table == NULL, "memory");
	
	/* Read the static routing table and the MAC table */
	rtable_len = read_rtable("rtable.txt", rtable);
	mac_table_len = read_mac_table(mac_table);

	/* "event loop" - router-ul "asculta" incontinuu pt pachete noi */
	while(1) {
		interface = recv_from_all_links(packet, &packet_len);
		if (interface < 0) {
			printf("Eroare la primirea pachetului!! Nu ar trebui sa vezi asta!\n");
			return -1;
		}

		/* extragem headerele */
		struct ether_header *eth_hdr = (struct ether_header *)packet;
		struct iphdr *ip_hdr = (struct iphdr *)(packet + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + sizeof(struct iphdr) + sizeof(struct ether_header));

		/* verificam protocolul IPv4 (in ether_type din ethernet header) => 0x0800
		si sa fie ICMP (protocol in header IP) => 1!!
		*/

		if (eth_hdr->ether_type != htons(0x0800)) {
			printf("Not an IPv4 packet.\n");
			continue;
		}

		/* verific integritatea header-ului IP */

		uint16_t og_ip_check = ntohs(ip_hdr->check);
		ip_hdr->check = 0;

		uint8_t ip_buf[sizeof(struct iphdr)];
        memcpy(ip_buf, ip_hdr, sizeof(struct iphdr));
        ip_hdr->check = ip_checksum((uint16_t *)ip_buf, sizeof(struct iphdr));

		if (ip_hdr->check != og_ip_check) {
			printf("Packet CORRUPTED. Resending...\n");
			continue;
		}

		ip_hdr->check = htons(og_ip_check);

		if (ip_hdr->protocol != 1) {
			printf("Not an ICMP packet.\n");
			continue;
		}

		/* acum verificam intregritatea header-ului ICMP */
		// uint8_t tmp[MAX_LEN];
		// memcpy(tmp, icmp_hdr, sizeof(struct icmphdr));

		int icmp_len = ntohs(ip_hdr->tot_len) - sizeof(struct iphdr);
		uint16_t og_icmp_check = ntohs(icmp_hdr->check);
		icmp_hdr->check = 0;

		uint8_t temp[MAX_LEN]; 
        memset(temp, 0, MAX_LEN);
        memcpy(temp, icmp_hdr, icmp_len); // Copiem tot pachetul ICMP aici

		icmp_hdr->check = ip_checksum((uint16_t *)temp, icmp_len);

		if (icmp_hdr->check != og_icmp_check) {
			printf("Packet CORRUPTED. Resending...\n");
			continue;
		}

		icmp_hdr->check = htons(og_icmp_check);

		/* acum, router-ul alege daca sa forwardeze pachetul catre  
		trebuie sa faca o decizie:
		- destination unreachable (random)
		- forwardeaza catre host1
		*/

		if(rand() % 2) {
			/* icmp destination unreachable */
			printf("ICMP Destination Unreachable\n");

			/*
			aici trebuie sa fac un pachet nou
			original are shost = h0, dhost = router
			trebuie sa le inversez

			la ICMP header => type = 3 (cred?) pt destination unreachable
			code = 0
			recalculam checksum

			la IP header => schimb saddr (h0 -> router), si daddr(h1 -> h0)
			protocol e la fel
			ttl il fac la 46
			recalculez checksum

			trimit pe aceeasi interfata de pe care am primit pachetul

			^asta in alt pachet, plus payload treb. sa fie ip header-ul original si primii 8 octeti
			din icmp header (rfc 729 standard)
			*/

			char new_packet[MAX_LEN];
			// zerooooo
			memset(new_packet, 0, MAX_LEN);

			/* salvam headerele ip si icmphdr originale */
			struct iphdr orig_ip;
			struct icmphdr orig_icmp;
			memcpy(&orig_ip, ip_hdr, sizeof(struct iphdr));
			memcpy(&orig_icmp, icmp_hdr, sizeof(struct icmphdr));

			struct ether_header *new_eth_hdr = (struct ether_header *)new_packet;
			struct iphdr *new_ip_hdr = (struct iphdr *)(new_packet + sizeof(struct ether_header));
			struct icmphdr *new_icmp_hdr = (struct icmphdr *)(new_packet + sizeof(struct ether_header) + sizeof(struct iphdr));
			char *payload = (char *)(new_packet + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr));

			get_interface_mac(interface, new_eth_hdr->ether_shost);
			memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
			new_eth_hdr->ether_type = htons(0x0800);

			new_ip_hdr->version = 4;
			new_ip_hdr->ihl = 5;
			new_ip_hdr->tos = 0;
			new_ip_hdr->ttl = 64;
			new_ip_hdr->protocol = 1;
			new_ip_hdr->saddr = ip_hdr->daddr; // h0 -> router
			new_ip_hdr->daddr = orig_ip.saddr;
			// new ip header + new icmp header + old ip header + first 8 bytes of old icmp header
			new_ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8);
			new_ip_hdr->check = 0;
			new_ip_hdr->check = htons(ip_checksum((uint16_t *)new_ip_hdr, sizeof(struct iphdr)));

			/* ICMP!!*/
			new_icmp_hdr->type = 3;
			new_icmp_hdr->code = 0;
			new_icmp_hdr->id = 0;
			new_icmp_hdr->seq = 0;

			memcpy(payload, &orig_ip, sizeof(struct iphdr));	// old ip header
			memcpy(payload + sizeof(struct iphdr), &orig_icmp, 8); // first 8 bytes of old icmp header

			int icmp_total_len = sizeof(struct icmphdr) + sizeof(struct iphdr) + 8;
			new_icmp_hdr->check = 0;
			uint8_t tmp[MAX_LEN];
			memcpy(tmp, new_icmp_hdr, icmp_total_len);
			new_icmp_hdr->check = htons(ip_checksum((uint16_t *)tmp, icmp_total_len));

			int final_len = sizeof(struct ether_header) + ntohs(new_ip_hdr->tot_len);
			send_to_link(interface, new_packet, final_len);

		} else {
			/* normal forwarding */
			printf("Forwarding normal\n");

			/*
			verificam ttl (ttl > 1)
			decrementam ttl
			cautam in tabela de rutare (lpm)
			recalculam checksum-ul (ip)
			rezolutia adresei MAC - setam mac-urile sursa si destinatie (ethernet)
			*/

			if (ip_hdr->ttl <= 1) {
				printf("TTL expirat. DROP.\n");
				continue;
			}

			ip_hdr->ttl--;
			ip_hdr->check = 0;
		
			uint8_t ip_resum[sizeof(struct iphdr)];
            memcpy(ip_resum, ip_hdr, sizeof(struct iphdr));

            ip_hdr->check = ip_checksum((uint16_t *)ip_resum, sizeof(struct iphdr));

			/*
			ip_hdr->saddr
			ip_hdr->daddr

			eth_hdr->ether_dhost -> MAC of ip_hdr->daddr
			eth_hdr->ether_shost -> MAC of current interface
			*/

			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
			if (!best_route) {
				// ICMP Destination Unreachable
				printf("No route found!\n");
				continue;
			}

			struct mac_entry *mac = get_mac_entry(best_route->next_hop);
			if (mac == NULL) {
				printf("No MAC found, drop\n");
				continue;
			}

			get_interface_mac(best_route->interface, eth_hdr->ether_shost);
			memcpy(eth_hdr->ether_dhost, mac->mac, 6);

			send_to_link(best_route->interface, packet, packet_len);
		}
	}
}
