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

struct user_data {
	char username[64];
	char password[64];
	int is_logged_in; // flag pt login
};


int main(int argc,char** argv) {
	/* Don't modify this */
	init(HOST,PORT);

	/* ==== DEFINIRE ACK ==== */
	struct l3_msg ack;
	ack.hdr.type = 4;
	snprintf(ack.payload, sizeof(ack.payload), "%s", "ACK\n");
	ack.hdr.len = strlen(ack.payload) + 1;
	ack.hdr.sum = 0;
	ack.hdr.sum = htonl(simple_csum((void *) &ack, sizeof(struct l3_msg)));

	/* ==== DEFINIRE NACK ==== */
	struct l3_msg nack;
	nack.hdr.type = 5;
	snprintf(nack.payload, sizeof(nack.payload), "%s", "NACK\n");
	nack.hdr.len = strlen(nack.payload) + 1;
	nack.hdr.sum = 0;
	nack.hdr.sum = htonl(simple_csum((void *) &nack, sizeof(struct l3_msg)));

	/* ==== array cu toti utilizatorii inregistrati ==== */
	struct user_data db[100];
	int user_count = 0;
	printf("Server running\n");

	while(1) {
		struct l3_msg t;
		/* Receive the frame from the link */
		int len = link_recv(&t, sizeof(struct l3_msg));
		DIE(len < 0, "Receive message");

		/* We have to convert it to host order */
		uint32_t recv_sum = ntohl(t.hdr.sum);
		t.hdr.sum = 0;
		int sum_ok = (simple_csum((void *) &t, sizeof(struct l3_msg)) == recv_sum);
		/* TODO 2: Change to crc32 */

		/* Since we are sending messages with a payload of 1500 - sizeof(header), most of the times the bytes from
		* 30 - 1500 will be corrupted and thus when we are printing or string message "Hello world" we see no probems.
		* This will be visible when we will be sending a file */

		// printf("[RECV] len=%d; sum(%s)=0x%04hx; payload=\"%s\";\n", t.hdr.len, sum_ok ? "GOOD" : "BAD", recv_sum, t.payload);
		if (sum_ok == true) {
			// send an ACK to sender
			link_send(&ack, sizeof(struct l3_msg));
			// implement REGISTER, LOGIN, CONNECT

			if (t.hdr.type == 1) {
				char username_buff[64], pass_buff[64];
				int res = sscanf(t.payload, "%s %s", username_buff, pass_buff);
				int found = 0;
				for (int i = 0; i < user_count; i++) {
					if (strcmp(db[i].username, username_buff) == 0) {
						found = 1;

						printf("[RECV]: REGISTRED FAILED: user %s already exists\n", username_buff);
						
						/* trebuie sa trimitem acest mesaj si catre sender */

						struct l3_msg registered_failed;
						snprintf(registered_failed.payload, sizeof(registered_failed.payload), "[RECV] User already registered. Please LOGIN.\n");
						registered_failed.hdr.len = strlen(registered_failed.payload) + 1;
						registered_failed.hdr.sum = 0;
						registered_failed.hdr.sum = htonl(simple_csum((void *) &registered_failed, sizeof(struct l3_msg)));

						link_send(&registered_failed, sizeof(struct l3_msg));
						break;
					}
				}
				/* user doesn't exist */
				if (found == 0) {
					strcpy(db[user_count].username, username_buff);
					strcpy(db[user_count].password, pass_buff);
					db[user_count].is_logged_in = 0;

					printf("[RECV]: RESGISTERED ok: %s registered. %d users total.\n", db[user_count].username, user_count + 1);
					user_count++;

					struct l3_msg registered_ok;
					snprintf(registered_ok.payload, sizeof(registered_ok.payload), "[RECV] Registered successfully. You can now LOGIN.\n");
					registered_ok.hdr.len = strlen(registered_ok.payload) + 1;
					registered_ok.hdr.sum = 0;
					registered_ok.hdr.sum = htonl(simple_csum((void *) &registered_ok, sizeof(struct l3_msg)));
					
					link_send(&registered_ok, sizeof(struct l3_msg));
				}
			}

			if(t.hdr.type == 2) {
				char username_buff[64], pass_buff[64];
				int res = sscanf(t.payload, "%s %s", username_buff, pass_buff);
				int found = 0;
				for (int i = 0; i < user_count; i++) {
					if (strcmp(db[i].username, username_buff) == 0 && strcmp(db[i].password, pass_buff) == 0) {
						/* user exists */
						printf("[RECV]: LOGIN ok: user %s found\n", username_buff);
						found = 1;
						db[i].is_logged_in = 1;

						struct l3_msg login_ok;
						snprintf(login_ok.payload, sizeof(login_ok.payload), "[RECV] Login successful. You can now CONNECT.\n");
						login_ok.hdr.len = strlen(login_ok.payload) + 1;
						login_ok.hdr.sum = 0;
						login_ok.hdr.sum = htonl(simple_csum((void *) &login_ok, sizeof(struct l3_msg)));

						link_send(&login_ok, sizeof(struct l3_msg));
						break;
					}
				}

				if (found == 0) {
					printf("[RECV]: LOGIN failed: user %s not found\n", username_buff);

					struct l3_msg login_failed;
					snprintf(login_failed.payload, sizeof(login_failed.payload), "[RECV] User not found. Please REGISTER first.\n");
					login_failed.hdr.len = strlen(login_failed.payload) + 1;
					login_failed.hdr.sum = 0;
					login_failed.hdr.sum = htonl(simple_csum((void *) &login_failed, sizeof(struct l3_msg)));

					link_send(&login_failed, sizeof(struct l3_msg));
				}
			}

			if (t.hdr.type == 3) {
				printf("[RECV]: Connected to the internet!\n");

				struct l3_msg connect_ok;
				snprintf(connect_ok.payload, sizeof(connect_ok.payload), "[RECV] Connected to the internet!\n");
				connect_ok.hdr.len = strlen(connect_ok.payload) + 1;
				connect_ok.hdr.sum = 0;
				connect_ok.hdr.sum = htonl(simple_csum((void *) &connect_ok, sizeof(struct l3_msg)));

				link_send(&connect_ok, sizeof(struct l3_msg));
			}
		} else {
			printf("[RECV]: Sending NACK. Frame corrupted.\n");

			link_send(&nack, sizeof(struct l3_msg));
		}

		/* TODO 3.1: In a loop, recv a frame and check if the CRC is good */

		/* TODO 3.2: If the crc is bad, send a NACK frame */

		/* TODO 3.2: Otherwise, write the frame payload to a file recv.data */

		/* TODO 3.3: Adjust the corruption rate */

	}

	return 0;
}
