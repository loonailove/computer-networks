/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 10

int number;

struct cli {
  int id;
  int status;
  int msg_cnt;
};

void run_chat_multi_server(int listenfd) {

  struct pollfd poll_fds[MAX_CONNECTIONS + 2];
  struct cli clients[MAX_CONNECTIONS];
  int num_sockets = 2, total_clients = 0;
  int rc;

  struct chat_packet received_packet;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
  // multimea poll_fds
  poll_fds[0].fd = listenfd;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = STDIN_FILENO;
  poll_fds[1].events = POLLIN;

  while (1) {
    // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_sockets; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == listenfd) {
          // Am primit o cerere de conexiune pe socketul de listen, pe care
          // o acceptam
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          const int newsockfd =
              accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          // Adaugam noul socket intors de accept() la multimea descriptorilor
          // de citire
          poll_fds[num_sockets].fd = newsockfd;
          poll_fds[num_sockets].events = POLLIN;
          num_sockets++;

          printf("[SERVER] Clientul %d s-a conectat\n",
                 newsockfd);

          clients[total_clients].id = newsockfd;
          clients[total_clients].status = 1;
          clients[total_clients].msg_cnt = 0;
          total_clients++;
        } else {
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc == 0) {
            for (int k = 0; k < total_clients; k++) {
              if (clients[k].id == poll_fds[i].fd) {
                clients[k].status = 0;
              }
            }

            printf("[SERVER] Clientul %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;

          // SET
          } else if (strncmp(received_packet.message, "set", 3) == 0) {
            struct chat_packet pkt;
            memset(&pkt, 0, sizeof(pkt));

            if (poll_fds[i].fd == clients[0].id) {
              sscanf(received_packet.message, "set %d", &number);
              snprintf(pkt.message, sizeof(pkt.message), "[SERVER] Ai setat nr. cu succes!");
              pkt.len = strlen(pkt.message) + 1;

              send_all(poll_fds[i].fd, &pkt, sizeof(pkt));

              for (int k = 0; k < total_clients; k++) {
                if (clients[k].status == 1 && clients[k].id != poll_fds[i].fd) {
                  snprintf(pkt.message, sizeof(pkt.message), "[SERVER] Numarul a fost setat. Oare il poti ghici?");
                  pkt.len = strlen(pkt.message) + 1;

                  send_all(poll_fds[k].fd, &pkt, sizeof(pkt));
                }
              }

            } else {
              snprintf(pkt.message, sizeof(pkt.message), "[SERVER] PROHIBITED. Nu ai voie sa efectuezi aceasta actiune!");
              pkt.len = strlen(pkt.message) + 1;

              send_all(poll_fds[i].fd, &pkt, sizeof(pkt));
            }
          } else if (strncmp(received_packet.message, "guess", 5) == 0) {
              struct chat_packet pkt;
              memset(&pkt, 0, sizeof(pkt));

              if (poll_fds[i].fd == clients[0].id) {
                snprintf(pkt.message, sizeof(pkt.message), "[SERVER] PROHIBITED. Nu ai voie sa efectuezi aceasta actiune.");
                pkt.len = strlen(pkt.message) + 1;

                send_all(poll_fds[i].fd, &pkt, sizeof(pkt));
              } else {
                  int guess;
                  sscanf(received_packet.message, "guess %d", &guess);

                  if (guess == number) {
                    snprintf(pkt.message, sizeof(pkt.message), "[SERVER] Ai ghicit!");
                    pkt.len = strlen(pkt.message) + 1;

                    send_all(poll_fds[i].fd, &pkt, sizeof(pkt));

                    // deconectam pe toata lumea, si noi dam exit
                    for (int k = 0; k < total_clients; k++) {
                        if (clients[k].status == 1) {
                            struct chat_packet end_pkt;
                            memset(&end_pkt, 0, sizeof(end_pkt));

                            snprintf(end_pkt.message, sizeof(end_pkt.message),
                                    "[SERVER] Joc terminat. Cineva a ghicit numarul!");

                            end_pkt.len = strlen(end_pkt.message) + 1;

                            send_all(clients[k].id, &end_pkt, sizeof(end_pkt));
                        }
                    }

                    break;
                    
                  } else {
                    snprintf(pkt.message, sizeof(pkt.message), "[SERVER] Mai incearca!!");
                    pkt.len = strlen(pkt.message) + 1;

                    send_all(poll_fds[i].fd, &pkt, sizeof(pkt));
                  }
              }
          }
        }
      }
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

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listenfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  // rulam de 2 ori rapid
  // Vezi https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
  const int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Asociem adresa serverului cu socketul creat folosind bind
  rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind");

  run_chat_multi_server(listenfd);

  // Inchidem listenfd
  close(listenfd);

  return 0;
}
