#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "link_emulator/lib.h"

static inline uint8_t hamming_4to7(uint8_t c) {
	// TODO: Implement hamming encoding for one nibble
}

static inline uint8_t hamming_7to4(uint8_t c) {
	// TODO: Implement hamming decoding for one nibble
	// TODO: Implement error correction
}

uint16_t inet_csum(uint8_t *buf, size_t len);

struct l3_msg_hdr {
	uint16_t len;
	uint16_t sum;
};

struct l3_msg {
	struct l3_msg_hdr hdr;
	char payload[sizeof(((msg *) NULL)->payload) / 2 - sizeof(struct l3_msg_hdr)];
};
