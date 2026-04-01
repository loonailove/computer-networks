#include <arpa/inet.h> /* ntoh, hton and inet_ functions */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "lib.h"
#include "protocols.h"

/* Routing table */
struct route_table_entry *rtable;
int rtable_len;

/* Mac table */
struct mac_entry *mac_table;
int mac_table_len;

uint8_t mac_host0[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00};
uint8_t mac_host1[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};

/*
 Returns a pointer (eg. &rtable[i]) to the best matching route, or NULL if there
 is no matching route.
*/
struct route_table_entry *get_best_route(uint32_t ip_dest) {
	/* TODO 2.2: Implement the LPM algorithm */
	for (int i = 0; i < rtable_len; i++) {
		if ((ip_dest & rtable[i].mask) == (rtable[i].prefix & rtable[i].mask)) {
			return &rtable[i];
		}
	}
	return NULL;
}

struct mac_entry *get_mac_entry(uint32_t given_ip) {
	/* TODO 2.4: Iterate through the MAC table and search for an entry
	 * that matches given_ip. */
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
	DIE(rtable == NULL, "memory");

	mac_table = malloc(sizeof(struct  mac_entry) * 100);
	DIE(mac_table == NULL, "memory");
	
	/* Read the static routing table and the MAC table */
	rtable_len = read_rtable("rtable.txt", rtable);
	mac_table_len = read_mac_table(mac_table);

	while (1) {
		/* Receive a packet */
		interface = recv_from_all_links(packet, &packet_len);
		DIE(interface < 0, "get_message");
		printf("We have received a packet\n");
		
		/* Extract headers */
		struct ether_header *eth_hdr = (struct ether_header *) packet;
		struct iphdr *ip_hdr = (struct iphdr *)(packet + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));

		/* Check for IPv4 */
		if (eth_hdr->ether_type != htons(ETHERTYPE_IP)) {
			printf("Ignored non-IPv4 packet\n");
			continue;
		}

		/* Verify IP checksum */
		uint16_t received_check = ip_hdr->check;
		ip_hdr->check = 0;
		if (ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)) != ntohs(received_check)) {
			printf("Checksum gresit, drop pachet\n");
			continue;
		}

		/* Handle only ICMP packets */
		if (ip_hdr->protocol == 1) {	// icmp
			if (icmp_hdr->type == 8) { // echo request

				if (ip_hdr->ttl > 1) {
					/* -------- Normal forwarding to host1 -------- */
					ip_hdr->ttl--;

					/* Recompute IP checksum */
					ip_hdr->check = 0;
					ip_hdr->check = htons(ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

					/* Forward to host1: set MAC addresses */
					get_interface_mac(1, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host1, 6);
					send_to_link(1, packet, packet_len);

					/* Wait for reply from host1 */
					recv_from_all_links(packet, &packet_len);

					/* Forward reply back to host0 */
					get_interface_mac(0, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host0, 6);
					send_to_link(0, packet, packet_len);

				} else {
					/* TTL <= 1: send ICMP Time Exceeded */

					/* salvam header-ele originale inainte sa modificam pachetul */
					struct iphdr orig_ip;
					struct icmphdr orig_icmp;
					memcpy(&orig_ip, ip_hdr, sizeof(struct iphdr));
					memcpy(&orig_icmp, icmp_hdr, sizeof(struct icmphdr));

					/* IP header: sursa = router, destinatie = h0 */
					ip_hdr->saddr = inet_addr("192.168.0.1");
					ip_hdr->daddr = orig_ip.saddr;
					ip_hdr->ttl = 64;
					ip_hdr->protocol = 1;
					ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct icmphdr));

					/* recalculam checksum IP */
					ip_hdr->check = 0;
					ip_hdr->check = htons(ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

					/* ICMP header: Time Exceeded */
					icmp_hdr->type = 11;
					icmp_hdr->code = 0;
					icmp_hdr->id = 0;
					icmp_hdr->sequence = 0;

					/* payload: IP header original + ICMP header original */
					uint8_t *payload = (uint8_t *)icmp_hdr + sizeof(struct icmphdr);
					memcpy(payload, &orig_ip, sizeof(struct iphdr));
					memcpy(payload + sizeof(struct iphdr), &orig_icmp, sizeof(struct icmphdr));

					/* recalculam checksum ICMP peste tot (header + payload) */
					int icmp_total_len = sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct icmphdr);
					icmp_hdr->checksum = 0;
					uint8_t *icmp_bytes = (uint8_t *)icmp_hdr;
					icmp_hdr->checksum = htons(ip_checksum((uint16_t *)icmp_bytes, icmp_total_len));

					/* Ethernet header */
					get_interface_mac(0, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host0, 6);

					/* trimitem */
					int new_len = sizeof(struct ether_header) + ntohs(ip_hdr->tot_len);
					send_to_link(0, packet, new_len);
					printf("ICMP Time Exceeded sent\n");
				}
			}
		}
	}
}
