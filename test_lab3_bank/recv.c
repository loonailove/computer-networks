#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "common.h"
#include "link_emulator/lib.h"
#include "include/utils.h"

/**
 * You can change these to communicate with another colleague.
 * There are several factors that could stop this from working over the
 * internet, but if you're on the same network it should work.
 * Just fill in their IP here and make sure that you use the same port.
 */
#define HOST "127.0.0.1"
#define PORT 10001

struct bank_account {
	char name[30];
	uint32_t amount;
};


int main(int argc,char** argv) {
	/* Don't modify this */
	init(HOST,PORT);

	struct l3_msg t;

	/* ==== DEFINIRE ACK ==== */
	struct l3_msg ack;
	strncpy(ack.payload, "ACK", MAX_PAYLOAD);
	ack.hdr.len = strlen(ack.payload);
	ack.hdr.sum = 0;
	ack.hdr.sum = crc32((void *)&ack, ack.hdr.len);

	/* ==== DEFINIRE NACK ==== */
	struct l3_msg nack;
	strncpy(nack.payload, "NACK", MAX_PAYLOAD);
	nack.hdr.len = strlen(nack.payload);
	nack.hdr.sum = 0;
	nack.hdr.sum = crc32((void *)&nack, nack.hdr.len);

	/* vom permite 50 de conturi deschise
	(it's a small bank!)
	*/
	struct bank_account accs[50];
	int cnt = 0;

	while(1) {
		link_recv((void *)&t, sizeof(struct l3_msg));

		if (t.hdr.len == 0 && t.hdr.sum == 0) {
			memset(&t, 0, sizeof(struct l3_msg));
			printf("[RECV] Incheiere conexiune...\n");
			break;
		}

		/* am primit pachetul */
		/* trebuie sa verificam ca a ajuns integru */

		uint32_t og_sum = t.hdr.sum;
		t.hdr.sum = 0;

		uint32_t computed = crc32((void *)&t, t.hdr.len);
		
		t.hdr.sum = og_sum;

		if (computed != og_sum) {
			printf("[RECV] Checksum BAD, sending NACK... %u, %u\n", t.hdr.sum, computed);
			link_send((void *)&nack, sizeof(struct l3_msg));
			continue;
		} else {
			printf("[RECV] Checksum matches, sending ACK...\n");
			link_send((void *)&ack, sizeof(struct l3_msg));
		}

		char command[256] = {0};
		char arg1[256] = {0}, arg2[256] = {0};

		int match = sscanf(t.payload, "%s %s %s", command, arg1, arg2);

		if (match == 2) {
			// OPEN | BALANCE
			if (strncmp(command, "OPEN", strlen("OPEN")) == 0) {
				strncpy(accs[cnt].name, arg1, strlen(arg1));
				accs[cnt].amount = 0;
				cnt++;

				printf("[RECV] Account opened successfully. You can now DEPOSIT and CHECK BALANCE.\n");
			} else if (strncmp(command, "BALANCE", strlen("BALANCE")) == 0) {
				uint32_t found = 0;
				for (int i = 0; i < cnt && found == 0; i++) {
					if (strncmp(accs[i].name, arg1, strlen(arg1)) == 0) {
						found = 1;
						printf("[RECV] Balance: '%s' has '%d'\n", accs[i].name, accs[i].amount);
						continue;
					}
				}
				if (found == 0) {
					printf("[RECV] Account not found. Please OPEN first.\n");
					continue;
				}
			}
		} else if (match == 3) {
			// DEPOSIT
			uint32_t found = 0;
			for (int i = 0; i < cnt && found == 0; i++) {
				if(strncmp(accs[i].name, arg1, strlen(arg1)) == 0) {
					found = 1;
					accs[i].amount += atoi(arg2);
					printf("[RECV] Succesfully added %s to '%s'\n", arg2, arg1);

					break;
				}
			}

			if (found == 0) {
				printf("[RECV] Account not found. Please OPEN first.\n");
			}
		}
	}

	return 0;
}
