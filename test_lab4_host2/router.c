#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

int main(int argc, char *argv[]) {
    int interface;
    char packet[MAX_LEN];
    int packet_len;

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

    while (1) {
        interface = recv_from_all_links(packet, &packet_len);
        DIE(interface < 0, "get_message");

        struct ether_header *eth_hdr = (struct ether_header *) packet;
        struct iphdr *ip_hdr = (struct iphdr *)(packet + sizeof(struct ether_header));
        struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));

        /* 1. Verificăm dacă e IPv4 */
        if (eth_hdr->ether_type != htons(0x0800)) {
            continue;
        }

        /* 2. Verificăm integritatea IP (Checksum) */
        uint16_t old_sum = ntohs(ip_hdr->check);
        ip_hdr->check = 0;
        if (ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)) != old_sum) {
            printf("[ROUTER] Checksum IP gresit. Drop.\n");
            continue;
        }
        /* restauram checksum-ul */
        ip_hdr->check = htons(old_sum);

        /* Definim adresele de control conform cerinței */
        uint32_t host1_ip = inet_addr("192.168.1.2");
		uint32_t host2_ip = inet_addr("192.168.2.2");

        /* CAZ 1: Destinat lui HOST 1 (Forwarding normal) */
        if (ip_hdr->daddr == host1_ip) {
            printf("[ROUTER] Forwarding catre Host 1...\n");

            if (ip_hdr->ttl <= 1) {
                printf("[ROUTER] TTL expirat. Drop.\n");
                continue;
            }

            ip_hdr->ttl--;
            ip_hdr->check = 0;
            ip_hdr->check = htons(ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

			/* gasim ruta */
			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
			/* gasim MAC-ul next hop din mac table */
			struct mac_entry *mac = get_mac_entry(best_route->next_hop);

			get_interface_mac(1, eth_hdr->ether_shost);  /* sursa = router interfata 1 */
			memcpy(eth_hdr->ether_dhost, mac->mac, 6);  /* dest = h1 */

            send_to_link(1, packet, packet_len);

		} else if (icmp_hdr->type == 0) {
            /* Echo Reply de la h1 -> trimitem inapoi la h0 */
			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
			struct mac_entry *mac = get_mac_entry(best_route->next_hop);

			get_interface_mac(1, eth_hdr->ether_shost);  /* sursa = router interfata 1 */
			memcpy(eth_hdr->ether_dhost, mac->mac, 6);  /* dest = h1 */

			send_to_link(1, packet, packet_len);

        /* CAZ 2: Destinat lui HOST 2 (Transformare în ICMP Time Exceeded) */
        } else if (ip_hdr->daddr == host2_ip) {
            printf("[ROUTER] Host 2 detectat. Transformare in Time Exceeded...\n");

            /* salvam header-ele originale */
            struct iphdr orig_ip;
            struct icmphdr orig_icmp;
            memcpy(&orig_ip, ip_hdr, sizeof(struct iphdr));
            memcpy(&orig_icmp, icmp_hdr, sizeof(struct icmphdr));

            /* modificam IP header */
            ip_hdr->saddr = orig_ip.daddr;  /* sursa = router (host2 ip) */
            ip_hdr->daddr = orig_ip.saddr;  /* destinatie = host1 (cel care a trimis) */
            ip_hdr->ttl = 64;
            ip_hdr->protocol = 1;
            ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8);
            ip_hdr->check = 0;
            ip_hdr->check = htons(ip_checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

            /* modificam ICMP header */
            icmp_hdr->type = 11;  /* Time Exceeded */
            icmp_hdr->code = 0;
            icmp_hdr->id = 0;
            icmp_hdr->seq = 0;

            /* payload: IP header original + primii 8 bytes din datele originale */
            uint8_t *payload = (uint8_t *)icmp_hdr + sizeof(struct icmphdr);
            memcpy(payload, &orig_ip, sizeof(struct iphdr));
            memcpy(payload + sizeof(struct iphdr), &orig_icmp, 8);

            /* checksum ICMP */
            int icmp_total_len = sizeof(struct icmphdr) + sizeof(struct iphdr) + 8;
            icmp_hdr->checksum = 0;
            uint8_t tmp[MAX_LEN];
            memcpy(tmp, icmp_hdr, icmp_total_len);
            icmp_hdr->checksum = htons(ip_checksum((uint16_t *)tmp, icmp_total_len));

            /* inversam MAC-urile */
            uint8_t aux_mac[6];
            memcpy(aux_mac, eth_hdr->ether_dhost, 6);
            memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
            memcpy(eth_hdr->ether_shost, aux_mac, 6);

            /* trimitem inapoi pe aceeasi interfata */
            int final_len = sizeof(struct ether_header) + ntohs(ip_hdr->tot_len);
            send_to_link(interface, packet, final_len);
        }
    }

    return 0;
}
