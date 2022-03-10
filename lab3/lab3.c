#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lab3_pkthdr {
	size_t len;
	uint16_t sum;
};

static inline uint8_t hamming_4to7(uint8_t c) {
	// TODO: Hamming encode one nibble
}

static inline uint8_t hamming_7to4(uint8_t c) {
	// TODO: Hamming decode one nibble
}

size_t hamming_encode(uint8_t *buf, size_t len, uint8_t *enc) {
	for (size_t idx = 0; idx < len; idx++) {
		enc[idx * 2] = hamming_4to7(buf[idx] >> 4);
		enc[idx * 2 + 1] = hamming_4to7(buf[idx] & 0xf);
	}

	return len * 2;
}

size_t hamming_decode(uint8_t *enc, size_t len, uint8_t *buf) {
	for (size_t idx = 0; idx < len; idx++) {
		buf[idx] = hamming_7to4(enc[idx * 2]) << 4;
		buf[idx] |= hamming_7to4(enc[idx * 2 + 1]);
	}

	return len / 2;
}

uint16_t inet_csum(uint8_t *buf, size_t len) {
	// TODO: Implement the Internet Checksum
	// see: https://datatracker.ietf.org/doc/html/rfc1071
}

void usage() {
	printf("Usage ./lab3 send | ./lab3 recv\n");
}

int main(int argc, char **argv) {
	if (argc != 2) {
		usage();
		return -1;
	}

	bool send;

	if (strcmp(argv[1], "recv") == 0)
		send = false;

	else if (strcmp(argv[1], "send") == 0)
		send = true;

	else {
		usage();
		return -1;
	}

	if (send) {
		while (true) {
			uint8_t buf[1024];
			struct lab3_pkthdr *ph = (void *) buf;
			char *payload = (void *) buf + sizeof(struct lab3_pkthdr);

			if (fgets(payload, sizeof(buf) - sizeof(struct lab3_pkthdr), stdin) == NULL)
				return 0;

			ph->len = strlen(payload) - 1;
			ph->sum = inet_csum((void *) payload, ph->len);

			uint8_t enc[2 * (sizeof(struct lab3_pkthdr) + ph->len)];
			hamming_encode(buf, sizeof(enc) / 2, enc);
			if (fwrite(enc, 1, sizeof(enc), stdout) != sizeof(enc))
				return 0;

			fflush(stdout);
		}
	} else {
		while (true) {
			uint8_t enc_ph[sizeof(struct lab3_pkthdr) * 2];
			struct lab3_pkthdr ph;

			fread(enc_ph, 1, sizeof(enc_ph), stdin);
			if (feof(stdin))
				return 0;

			hamming_decode(enc_ph, sizeof(enc_ph), (void *) &ph);

			uint8_t *enc_payload = malloc(ph.len * 2);
			if (enc_payload == NULL)
				return -1;

			char *payload = malloc(ph.len);
			if (payload == NULL)
				return -1;

			fread(enc_payload, 1, ph.len * 2, stdin);
			if (feof(stdin))
				return 0;

			hamming_decode(enc_payload, ph.len * 2, (void *) payload);

			bool sum_ok = inet_csum((void *) payload, ph.len) == ph.sum;

			printf("len=%lu; sum(%s)=0x%04hx; payload=\"%s\";\n", ph.len, sum_ok ? "GOOD" : "BAD", ph.sum, payload);
		}
	}
}
