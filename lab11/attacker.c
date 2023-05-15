#include "common.h"
#include "lib.h"
#include "protocols.h"
#include "utils.h"
#include "dh.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SRC_ALICE 0
#define SRC_BOB 1

#define ALICE_INTERFACE  0
#define BOB_INTERFACE    1

#define ALICE_IP  "10.10.10.2"
#define BOB_IP    "55.55.55.2"

static char ALICE_MAC[6] = "\x00\x00\xaa\xaa\xaa\xaa";
static char BOB_MAC[6] = "\x00\x00\xbb\xbb\xbb\xbb";

struct packet {
	size_t size;
	char payload[MAX_PACKET_LEN];
	int interface;
};

void handle_message(int source, char *data, size_t size)
{
	switch (source) {
	case SRC_ALICE:
		puts("ALICE wrote to BOB: ");
	break;
	case SRC_BOB:
		puts("BOB wrote to ALICE: ");
	break;
	default:
		printf("Unknown source: %d\n", source);
	}
	fflush(stdout);
	hex_dump(data, size);
}

void handle_encrypted_message(int source, char *data, size_t size)
{
	static int key_retrieved;
	static uint32_t key[KEY_SIZE / sizeof(uint32_t)];
	switch (source) {
	case SRC_ALICE:
		if (!key_retrieved) {
			// TODO 2. Steal the key from Alice in the first step
			key_retrieved = 1;
			return;
		}
		puts("ALICE wrote to BOB: ");
	break;
	case SRC_BOB:
		puts("BOB wrote to ALICE: ");
	break;
	default:
		printf("Unknown source: %d\n", source);
	}
	fflush(stdout);
	// TODO 2. Use the stolen key to decrypt the intercepted message
}

void handle_dh_message(int source, char *data, size_t size)
{
	static int exchange_done;
	static uint32_t secret;
	static uint32_t share;
	static uint32_t *key_alice;
	static uint32_t *key_bob;
	uint8_t *plaintext;
	uint8_t *ciphertext;

	switch (source) {
	case SRC_ALICE:
		if (!exchange_done) {
			// TODO 4.1 Intercept Alice's first message.
			// Create your own secret, calculate the shared value
			// and send it to Bob (by overwriting "data")
			// Calculate the secret key shared with Alice.

			return;
		} else {
			// TODO 4.3 Use the key for Alice to decrypt the
			// message and print it
			// Then use the key for Bob to encrypt it back and
			// send it to Bob (overwrite "data")
			puts("ALICE wrote to BOB: ");
		}
	break;
	case SRC_BOB:
		if (!exchange_done) {
			// TODO 4.2 Intercept Bob's message.
			// send your shared value to Alice (by overwriting "data")
			// Calculate the secret key shared with Bob.
			exchange_done = 1;
			return;
		} else {
			// TODO 4.3 Use the key for Bob to decrypt the
			// message and print it
			// Then use the key for Alice to encrypt it back and
			// send it to Bob (overwrite "data")
			puts("BOB wrote to ALICE: ");
		}
	break;
	default:
		printf("Unknown source: %d\n", source);
	}
}

void handle_tcp_data(int source, char *data, size_t size)
{
	static int read_size;
	static size_t msg_size;
	static size_t bytes_read;
	static char msg[MAX_MESSAGE_SIZE];
	
	/*
	 * The message format between Alice and Bob is to first send a size_t
	 * object with the size in bytes of what will be sent after.
	 *
	 * So we need to fully intercept the size and then the bytes (these may
	 * be split by TCP at the source, so we might need to inspect several
	 * different packets).
	 */
	if (read_size) {
		DIE(bytes_read + size > msg_size, "Malformed message!");
		memcpy(msg + bytes_read, data, size);
		bytes_read += size;
		/*handle_message(source, msg, msg_size);*/
		/*handle_encrypted_message(source, msg, msg_size);*/
		handle_dh_message(source, msg, msg_size);
		memcpy(data, msg, msg_size);
		if (bytes_read == msg_size) {
			// reset, for the next message
			msg_size = 0;
			bytes_read = 0;
			read_size = 0;
		}
	} else {
		DIE(bytes_read + size > sizeof(msg_size), "Malformed message!");
		memcpy(&msg_size + bytes_read, data, size);
		bytes_read += size;
		if (bytes_read == sizeof(msg_size)) {
			read_size = 1;
			bytes_read = 0;
		}
	}
}

int main(int argc, char *argv[])
{
	// Do not modify this line
	init();

	uint32_t alice_ip, bob_ip;
	uint32_t highest_tcp_seq_num[2] = {0, 0};
	int res;

	res = inet_pton(AF_INET, ALICE_IP, &alice_ip);
	DIE(res < 0, "inet_pton");
	res = inet_pton(AF_INET, BOB_IP, &bob_ip);
	DIE(res < 0, "inet_pton");
	

	/* The code below handles the forwarding between Bob and Alice. */
	while (1) {
		struct packet pkt;

		pkt.interface = recv_from_any_link(pkt.payload, &pkt.size);
		DIE(pkt.interface < 0, "recv_from_any_links");
		int out_interface = !pkt.interface;

		fprintf(stderr, "Got a packet from interface %d!\n",
				pkt.interface);

		struct ether_header *eth_hdr = (struct ether_header *)pkt.payload;

		if (ntohs(eth_hdr->ether_type) != ETHERTYPE_IP)
			continue;

		struct iphdr *ip_hdr = (struct iphdr *)(pkt.payload + sizeof(struct ether_header));
		struct in_addr src = {ip_hdr->saddr};
		struct in_addr dst = {ip_hdr->daddr};
		fprintf(stderr, "\tIt's an IP packet from %s", inet_ntoa(src));
		fprintf(stderr, " to %s!\n", inet_ntoa(dst));

		if (ip_hdr->protocol != IPTYPE_TCP)
			goto forward;

		struct tcp *tcp_hdr = (struct tcp *)(pkt.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

		size_t size = pkt.size - sizeof(struct ether_header)
			               - ip_hdr->ihl * 4
				       - tcp_hdr->tcp_hdr_len * 4;
		fprintf(stderr, "\tIt's a TCP packet from %u to %u!\n",
				ntohs(tcp_hdr->tcp_src),
				ntohs(tcp_hdr->tcp_dst));

		if (tcp_hdr->tcp_syn || tcp_hdr->tcp_fin || tcp_hdr->tcp_rst) {
			fprintf(stderr, "\tTCP control message, forwarding...\n");
			goto forward;
		}

		if (ntohl(tcp_hdr->tcp_seq_num) < highest_tcp_seq_num[pkt.interface]) {
			// our extra processing time may trigger
			// retransmissions; ignore those packets
			goto forward;
		} else {
			highest_tcp_seq_num[pkt.interface] = ntohl(tcp_hdr->tcp_seq_num);
		}

		char *data = (char *)(pkt.payload + sizeof(struct ether_header)
				+ ip_hdr->ihl * 4 + tcp_hdr->tcp_hdr_len * 4);
		if (size) // ignore empty ACKs and the like
			handle_tcp_data(pkt.interface, data, size);
forward:
		if (ip_hdr->protocol == IPTYPE_TCP) {
			// this is actually quite a bit extra than we need;
			tcp_hdr->tcp_chk = 0;
			char tcp_checksum_buffer[sizeof(struct message)];
			memcpy(tcp_checksum_buffer, &ip_hdr->saddr, 4);
			memcpy(tcp_checksum_buffer + 4, &ip_hdr->daddr, 4);
			tcp_checksum_buffer[8] = 0;
			tcp_checksum_buffer[9] = IPTYPE_TCP;
			uint16_t tcp_length = size + tcp_hdr->tcp_hdr_len * 4;
			uint16_t tcp_length_n = htons(tcp_length);
			memcpy(tcp_checksum_buffer + 10, &tcp_length_n, 2);
			memcpy(tcp_checksum_buffer + 12, tcp_hdr, tcp_length);
			tcp_hdr->tcp_chk = htons(checksum((uint16_t *)tcp_checksum_buffer, tcp_length + 12));
		}

		uint8_t mac[6];
		get_interface_mac(out_interface, mac);
		memcpy(eth_hdr->ether_shost, mac, 6);

		if (!out_interface)
			memcpy(eth_hdr->ether_dhost, ALICE_MAC, 6);
		else
			memcpy(eth_hdr->ether_dhost, BOB_MAC, 6);


		fprintf(stderr, "\tForwarding packet on interface %d\n", out_interface);
		res = send_to_link(out_interface, pkt.payload, pkt.size);
		DIE(res == -1, "send_to_link");
	}
}
