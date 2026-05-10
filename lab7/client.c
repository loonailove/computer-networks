#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

void run_client(int sockfd) {
  char buf[MSG_MAXSIZE + 1];
  struct chat_packet sent_packet;
  struct chat_packet recv_packet;

  /*
    TODO 2.2: Multiplexati intre citirea de la tastatura si primirea unui
    mesaj, ca sa nu mai fie impusa ordinea.

    Hint: server::run_multi_chat_server
  */
  struct pollfd pfds[2];
  pfds[0].fd = STDIN_FILENO;
  pfds[0].events = POLLIN;
  pfds[1].fd = sockfd;
  pfds[1].events = POLLIN;

  while (1) {
    int rc = poll(pfds, 2, -1);
    DIE(rc < 0, "poll");

    if (pfds[0].revents & POLLIN) {
      memset(buf, 0, MSG_MAXSIZE + 1);
      if (!fgets(buf, sizeof(buf), stdin) || isspace(buf[0])) {
        break;
      }

      sent_packet.len = strlen(buf) + 1;
      strcpy(sent_packet.message, buf);
      send_all(sockfd, &sent_packet, sizeof(sent_packet));
    }

    if (pfds[1].revents & POLLIN) {
      int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
      if (rc <= 0) {
        break;
      }
      printf("%s", recv_packet.message);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("\n Usage: %s <ip> <port>\n", argv[0]);
    return 1;
  }

  uint16_t port;
  int rc = sscanf(argv[2], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  run_client(sockfd);

  close(sockfd);

  return 0;
}
