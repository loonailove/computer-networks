#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "link_emulator/lib.h"

#define ACK "11"
#define NACK "12"

uint8_t simple_csum(uint8_t *buf, size_t len);

uint32_t crc32(uint8_t *buf, size_t len);

/* Layer 3 header */
struct l3_msg_hdr {
	uint16_t len;
	uint32_t sum;
} __attribute__((packed));

/* Layer 3 frame */
struct l3_msg {
	struct l3_msg_hdr hdr;
	/* Data */

	/* Note: MTU = 1500 => we can send 1500 - header size bytes of data */
	/* TODO 3.3: Update this to a higher value based on the MTU */
	char payload[25];
} __attribute__((packed));
