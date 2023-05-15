/*
 * Protocoale de comunicații
 * Laborator 11 - Securitate
 * server.c
 */
#include "common.h"
#include "tea.h"
#include "utils.h"
#include "dh.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CONNECTIONS 32

void process_request(char *contents)
{
	char *ptr = contents;
	while (*ptr) {
		*ptr = toupper(*ptr);
		++ptr;
	}
}

void run_server(int sockfd) {
	int res;
	struct message msg;
	while (1) {
		res = recv_message(sockfd, &msg);
		if (res == 0) {
			puts("Client disconnected!");
			break;
		}

		DIE(msg.buffer[msg.size - 1] != '\0', "Non-string request!");
		printf("Client request: %s\n", msg.buffer);

		process_request(msg.buffer);
		send_message(sockfd, &msg);
	}
}

uint32_t *obtain_key_plain(int sockfd)
{
	// TODO 1. Get the key from Alice.
	return NULL;
}

uint32_t *obtain_key_dh(int sockfd)
{
	// TODO 3. Perform DH using the primitives in "include/dh.h". Use derive_key with the
	// secret as argument. We do this because the key and the secret don't necessarly have
	// the same size or structure.
	return NULL;
}

void run_secure_server(int sockfd)
{
	int res;
	struct message msg;

	// TODO 3. Comment this and uncomment the next line
	uint32_t *key = obtain_key_plain(sockfd);
	/*uint32_t *key = obtain_key_dh(sockfd);*/

	while (1) {
		// TODO 1. just like run_server, receive a message, process it,
		// then send a reply;
		// but the incoming message will be encrypted so we need to
		// decrypt and the outgoing reply needs to be encrypted as well.
		// use the primitives in "include/tea.h"
	}

	destroy_key(key);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
		return 1;
	}

	// Parsam port-ul ca un numar
	uint16_t port;
	int res = sscanf(argv[2], "%hu", &port);
	DIE(res != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru receptionarea conexiunilor
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd < 0, "socket");

	// Completăm in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	// Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
	// rulam de 2 ori rapid
	int enable = 1;
	res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	DIE(res < 0, "setsockopt(SO_REUSEADDR) failed");

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	res = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
	DIE(res <= 0, "inet_pton");

	// Asociem adresa serverului cu socketul creat folosind bind
	res = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(res < 0, "bind");

	res = listen(listenfd, 1);
	DIE(res < 0, "listen");

	int clientfd = accept(listenfd, NULL, NULL);
	DIE(clientfd == -1, "accept");

	// TODO 1. Comment this and uncomment the next line
	run_server(clientfd);
	/*run_secure_server(clientfd);*/

	close(clientfd);
	close(listenfd);

	return 0;
}
