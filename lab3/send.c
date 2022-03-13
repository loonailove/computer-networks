#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

size_t hamming_encode(uint8_t *buf, size_t len, uint8_t *enc) {
	for (size_t idx = 0; idx < len; idx++) {
		enc[idx * 2] = hamming_4to7(buf[idx] >> 4);
		enc[idx * 2 + 1] = hamming_4to7(buf[idx] & 0xf);
	}

	return len * 2;
}



int main(int argc,char** argv){
	init(HOST,PORT);

	/* Look in link_emulator/lib.h for the definition of msg */
	msg t;

	/* We set the payload */
	sprintf(t.payload, "Hello World of PC");
	t.len = strlen(t.payload) + 1;
	/* Add the checksum */
	t.sum = inet_csum((void *) t.payload, t.len);

	/* Encode the message with error correction codes */
	uint8_t enc[2 * (sizeof(msg))];
	hamming_encode((void *) &t, sizeof(msg), enc);

	/* Send the message */
	send_message(&enc);

	return 0;
}
