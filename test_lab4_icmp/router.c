/* teoretic e gresit, ar trb sa ajunga la host 1 si apoi sa moara la router. gen reply-ul 
lui host1 treb sa se transforme in time exceeded.
*/

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

		uint8_t chk_buf[MAX_LEN];
		
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
		uint16_t received_check = ntohs(ip_hdr->check);
		ip_hdr->check = 0;

		memcpy(chk_buf, ip_hdr, sizeof(struct iphdr));
		if (ip_checksum((uint16_t *)chk_buf, sizeof(struct iphdr)) != received_check) {
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
					memcpy(chk_buf, ip_hdr, sizeof(struct iphdr));
					ip_hdr->check = htons(ip_checksum((uint16_t *)chk_buf, sizeof(struct iphdr)));

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
					/* TTL <= 1: STRATEGIA HACK-ULUI */
					printf("TTL=1. Redirectionare via Host 1...\n");

					/* 1. Salvăm header-ul IP original (cel trimis de Host 0) */
					struct iphdr orig_ip;
					memcpy(&orig_ip, ip_hdr, sizeof(struct iphdr));

					/* 2. "Înviem" pachetul actual și îl trimitem la Host 1 */
					ip_hdr->ttl = 46;
					ip_hdr->check = 0;
					uint8_t tmp_ip[sizeof(struct iphdr)];
					memcpy(tmp_ip, ip_hdr, sizeof(struct iphdr));
					ip_hdr->check = htons(ip_checksum((uint16_t *)tmp_ip, sizeof(struct iphdr)));

					get_interface_mac(1, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host1, 6);
					send_to_link(1, packet, packet_len);

					/* 3. Așteptăm Reply-ul de la Host 1 */
					char recv_packet[MAX_LEN];
					int recv_len;
					recv_len = recv_from_all_links(recv_packet, &recv_len);

					/* 4. CONSTRUIM UN PACHET NOU (ICMP Time Exceeded) */
					char send_packet[MAX_LEN];
					memset(send_packet, 0, MAX_LEN);

					struct ether_header *new_eth = (struct ether_header *) send_packet;
					struct iphdr *new_ip = (struct iphdr *) (send_packet + sizeof(struct ether_header));
					struct icmphdr *new_icmp = (struct icmphdr *) (send_packet + sizeof(struct ether_header) + sizeof(struct iphdr));

					/* Ethernet */
					new_eth->ether_type = htons(ETHERTYPE_IP);
					get_interface_mac(0, new_eth->ether_shost);
					memcpy(new_eth->ether_dhost, mac_host0, 6);

					/* IP */
					new_ip->version = 4;
					new_ip->ihl = 5;
					new_ip->tos = 0;
					new_ip->protocol = 1; // ICMP
					new_ip->ttl = 64;
					new_ip->saddr = inet_addr("192.168.0.1"); // Router IP
					new_ip->daddr = orig_ip.saddr;           // Inapoi la Host 0
					
					/* Payload ICMP: Header ICMP(8) + Header IP Orig(20) + Primii 8 octeti din payload orig(8) */
					int icmp_payload_len = sizeof(struct iphdr) + 8;
					int total_icmp_len = sizeof(struct icmphdr) + icmp_payload_len;
					
					new_ip->tot_len = htons(sizeof(struct iphdr) + total_icmp_len);

					/* ICMP Header */
					new_icmp->type = 11; // Time Exceeded
					new_icmp->code = 0;
					new_icmp->id = 0;
					new_icmp->sequence = 0;

					/* ICMP Payload (Injectăm header-ul IP original al lui Host 0) */
					uint8_t *payload_dest = (uint8_t *)new_icmp + sizeof(struct icmphdr);
					memcpy(payload_dest, &orig_ip, sizeof(struct iphdr));
					
					/* Optional: punem si primii 8 octeti din payload-ul original (unde e ICMP Request-ul original) */
					struct icmphdr *orig_icmp_ptr = (struct icmphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));
					memcpy(payload_dest + sizeof(struct iphdr), orig_icmp_ptr, 8);

					/* Checksum-uri finale */
					new_icmp->checksum = 0;
					uint8_t tmp_icmp[MAX_LEN];
					memcpy(tmp_icmp, new_icmp, total_icmp_len);
					new_icmp->checksum = htons(ip_checksum((uint16_t *)tmp_icmp, total_icmp_len));

					new_ip->check = 0;
					uint8_t tmp_ip_final[sizeof(struct iphdr)];
					memcpy(tmp_ip_final, new_ip, sizeof(struct iphdr));
					new_ip->check = htons(ip_checksum((uint16_t *)tmp_ip_final, sizeof(struct iphdr)));

					/* Trimitem pachetul curat */
					int final_len = sizeof(struct ether_header) + ntohs(new_ip->tot_len);
					send_to_link(0, send_packet, final_len);

					printf("ICMP Time Exceeded (Failsafe Hack) trimis!\n");
				}
			}
		}
	}
}
