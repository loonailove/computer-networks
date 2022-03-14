#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "link_emulator/lib.h"

static inline uint8_t hamming_4to7(uint8_t c) {
	// TODO 2: Implement hamming encoding for one nibble
}

static inline uint8_t hamming_7to4(uint8_t c) {
	// TODO 3: Implement hamming decoding for one nibble
	// TODO 4: Implement error correction
}

uint16_t inet_csum(uint8_t *buf, size_t len);

/* Layer 3 header */
struct l3_msg_hdr {
	uint16_t len;
	uint16_t sum;
};

/* Layer 3 frame */
struct l3_msg {
	struct l3_msg_hdr hdr;
	/* Data */
	char payload[sizeof(((msg *) NULL)->payload) / 2 - sizeof(struct l3_msg_hdr)];
};
