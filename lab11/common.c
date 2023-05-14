#include "common.h"
#include "tea.h"
#include "utils.h"

#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int send_all(int sockfd, const char *buf, size_t size)
{
	int bytes_sent = 0;
	int res;
	do {
		res = send(sockfd, buf + bytes_sent, size - bytes_sent, 0);
		DIE(res < 0, "send");
		bytes_sent += res;
	} while (res && bytes_sent < size);

	return bytes_sent;
}

int recv_all(int sockfd, char *buf, size_t size)
{
	int bytes_read = 0;
	int res;
	do {
		res = recv(sockfd, buf + bytes_read, size - bytes_read, 0);
		DIE(res < 0, "recv");
		bytes_read += res;
	} while (res && bytes_read < size);

	return bytes_read;
}

int send_message(int sockfd, const struct message *msg)
{
	int res;
	res = send_all(sockfd, (const char *)&msg->size, sizeof(msg->size));
	res += send_all(sockfd, msg->buffer, msg->size);

	return res;
}

int recv_message(int sockfd, struct message *msg)
{
	int res;
	res = recv_all(sockfd, (char *)&msg->size, sizeof(msg->size));
	res += recv_all(sockfd, msg->buffer, msg->size);

	return res;
}

int send_encrypted(int sockfd, const struct message *msg, const uint32_t *k)
{
	int res;
	struct message enc_msg;
	enc_msg.size = msg->size;

	uint8_t *cipher = (uint8_t *)encrypt((uint8_t *)msg->buffer,
			                     (uint32_t *)&enc_msg.size, k);
	memcpy(enc_msg.buffer, cipher, enc_msg.size);
	res = send_message(sockfd, &enc_msg);

	free(cipher);

	return res;
}

int recv_encrypted(int sockfd, struct message *msg, const uint32_t *k)
{
	int res;
	struct message enc_msg;

	res = recv_message(sockfd, &enc_msg);
	msg->size = enc_msg.size;

	uint8_t *plaintext = decrypt((uint8_t *)enc_msg.buffer,
			             (uint32_t *)&msg->size, k);
	memcpy(msg->buffer, plaintext, msg->size);
	free(plaintext);

	return res;
}
