#include <arpa/inet.h> /* ntoh, hton and inet_ functions */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
	/* We can iterate through rtable for (int i = 0; i < rtable_len; i++). Entries in
	 * the rtable are in network order already */
	return NULL;
}

struct mac_entry *get_mac_entry(uint32_t given_ip) {
	/* TODO 2.4: Iterate through the MAC table and search for an entry
	 * that matches given_ip. */

	/* We can iterate thrpigh the mac_table for (int i = 0; i 
	 * mac_table_len; i++) */
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
		/* We call get_packet to receive a packet. get_packet returns
		the interface it has received the data from. And writes to
		len the size of the packet. */
		interface = recv_from_all_links(packet, &packet_len);
		DIE(interface < 0, "get_message");
		printf("We have received a packet\n");
		
		/* Extract the Ethernet header from the packet. Since protocols are
		 * stacked, the first header is the ethernet header, the next header is
		 * at m.payload + sizeof(struct ether_header) */
		struct ether_header *eth_hdr = (struct ether_header *) packet;
		struct iphdr *ip_hdr = (struct iphdr *)(packet + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));

		/* Check if we got an IPv4 packet */
		if (eth_hdr->ether_type != htons(ETHERTYPE_IP)) {
			/* ether_type este primit de pe cablu (retea). trebuie sa comparam ETHERTYPE_IP (0x0800)
			care este in host order cu o data care este in network order
			*/
			printf("Ignored non-IPv4 packet\n");
			continue;
		}

		/* mandatoriu!!!! verificam checksum IP */
		uint16_t received_check = ip_hdr->check;
        ip_hdr->check = 0;
        if (ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)) != ntohs(received_check)) {
            printf("Checksum gresit, drop pachet\n");
            continue;
        }

		/* pt ICMP logic:
		1. verificam daca ip_hdr->protocol == 1 (ICMP)
		2. PT REQUEST (!), verificam 
			2.1. TTL > 1 => h0 -> router -> ttl-- -> h1 -> router -> h0
			2.2. TTL <= 1 => h0 -> router -> ttl = 46 -> h1 -> router -> type = 1 -> h0
		la intoarcere, forwardeaza raspunsul fara modificari
		*/

		if (ip_hdr->protocol == 1) {	// icmp
			if (icmp_hdr->type == 8) { // echo request

				if (ip_hdr->ttl > 1) {
					/* forwarding normal */
					ip_hdr->ttl--;

					/* trebuie sa recalculam checksum-ul pt header-ul ip 
					pt ca ttl s-a schimbat
					*/
					ip_hdr->check = 0;
					ip_hdr->check = htons(ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

					/* hardcodam MAC pt host 1
					Sursa: Router (interfata 1), Dest: Host 1
					*/
					get_interface_mac(1, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host1, 6);
					send_to_link(1, packet, packet_len);

					/* asteptam reply-ul de la host 1 pe interfata 1 */
					recv_from_all_links(packet, &packet_len);

					/* forward reply inapoi la host 0 pe interfata 0*/
					get_interface_mac(0, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host0, 6);
					send_to_link(0, packet, packet_len);
				} else {
					uint32_t original_saddr = ip_hdr->saddr;
					
					printf("original_saddr salvat: %s\n", inet_ntoa(*(struct in_addr*)&original_saddr));
					fflush(stdout);
					printf("TTL exceeded!\n");
					fflush(stdout);

					ip_hdr->daddr = original_saddr;
					printf("daddr setat\n"); fflush(stdout);
					
					ip_hdr->saddr = inet_addr("192.168.0.1");
					printf("saddr setat\n"); fflush(stdout);
					
					ip_hdr->ttl = 64;
					ip_hdr->protocol = 1;
					ip_hdr->check = 0;
					ip_hdr->check = htons(ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));
					printf("ip checksum recalculat\n"); fflush(stdout);

					icmp_hdr->type = 11;
					icmp_hdr->code = 0;
					icmp_hdr->id = 0;
					icmp_hdr->sequence = 0;

					int icmp_len = sizeof(struct icmphdr);
					icmp_hdr->checksum = 0;
					void *icmp_ptr = icmp_hdr;
					icmp_hdr->checksum = htons(ip_checksum((uint16_t *)icmp_ptr, icmp_len));
					printf("icmp checksum recalculat\n"); fflush(stdout);

					get_interface_mac(0, eth_hdr->ether_shost);
					memcpy(eth_hdr->ether_dhost, mac_host0, 6);
					printf("trimit pe interfata 0\n"); fflush(stdout);
					send_to_link(0, packet, packet_len);
					printf("trimis!\n"); fflush(stdout);
				}
			}
		}

	}
}
