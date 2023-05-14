#ifndef COMMON_H
#define COMMON_H

#include "tea.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MESSAGE_SIZE 256
// extra space for encryption padding
#define MESSAGE_BUFFER_SIZE (MAX_MESSAGE_SIZE + MAX_PAD_SIZE)

struct message {
	size_t size;
	char buffer[MESSAGE_BUFFER_SIZE];
};

int send_message(int sockfd, const struct message *msg);
int recv_message(int sockfd, struct message *msg);

int send_encrypted(int sockfd, const struct message *msg, const uint32_t *k);
int recv_encrypted(int sockfd, struct message *msg, const uint32_t *k);

#endif /* COMMON_H */
