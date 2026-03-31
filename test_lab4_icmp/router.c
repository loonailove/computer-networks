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
					/* -------- TTL <= 1: send ICMP Time Exceeded -------- */
					/* Create a new packet for the ICMP error message */
					char new_packet[MAX_LEN];
					struct ether_header *new_eth = (struct ether_header *) new_packet;
					struct iphdr *new_ip = (struct iphdr *)(new_packet + sizeof(struct ether_header));
					struct icmphdr *new_icmp = (struct icmphdr *)(new_packet + sizeof(struct ether_header) + sizeof(struct iphdr));
					void *payload = new_packet + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);

					/* Ethernet header */
					memcpy(new_eth->ether_dhost, mac_host0, 6);
					get_interface_mac(0, new_eth->ether_shost);
					new_eth->ether_type = htons(ETHERTYPE_IP);

					/* IP header for the error message */
					new_ip->ihl = 5;
					new_ip->version = 4;
					new_ip->tos = 0;
					new_ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct icmphdr));
					new_ip->id = htons(0);
					new_ip->frag_off = 0;
					new_ip->ttl = 64;
					new_ip->protocol = 1; // ICMP
					new_ip->saddr = inet_addr("192.168.0.1"); // router's IP on interface 0
					new_ip->daddr = ip_hdr->saddr; // original source
					new_ip->check = 0;
					new_ip->check = htons(ip_checksum((uint16_t *)new_ip, sizeof(struct iphdr)));

					/* ICMP header: Time Exceeded */
					new_icmp->type = 11;
					new_icmp->code = 0;
					new_icmp->checksum = 0;
					new_icmp->id = 0;      // not used for Time Exceeded
					new_icmp->sequence = 0; // not used for Time Exceeded

					/* Payload: original IP header + original ICMP header (first 8 bytes) */
					memcpy(payload, ip_hdr, sizeof(struct iphdr));
					memcpy(payload + sizeof(struct iphdr), icmp_hdr, sizeof(struct icmphdr));

					/* Compute ICMP checksum over the whole ICMP message (header + payload) */
					int icmp_total_len = sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct icmphdr);
					/* Use (void*) cast to avoid alignment warning from packed struct */
					void *icmp_ptr = (void *)new_icmp;
					new_icmp->checksum = htons(ip_checksum((uint16_t *)icmp_ptr, icmp_total_len));

					/* Send the error packet back to host0 on interface 0 */
					int new_packet_len = sizeof(struct ether_header) + ntohs(new_ip->tot_len);
					send_to_link(0, new_packet, new_packet_len);
					printf("ICMP Time Exceeded sent to host0\n");
				}
			}
		}
	}
}
