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

#define MAX_CONNECTIONS 32

int receive_and_send(int connfd1, int connfd2, size_t len) {
  int bytes_received;
  char buffer[len];

  // Primim exact len octeti de la connfd1
  bytes_received = recv_all(connfd1, buffer, len);
  // S-a inchis conexiunea
  if (bytes_received == 0) {
    return 0;
  }
  DIE(bytes_received < 0, "recv");

  // Trimitem mesajul catre connfd2
  int rc = send_all(connfd2, buffer, len);
  if (rc <= 0) {
    perror("send_all");
    return -1;
  }

  return bytes_received;
}

struct cli {
  int id;
  int msg_sent;
  int active; // 1 = ONLINE, 0 = OFFLINE
};

void run_chat_multi_server(int listenfd) {

  // cate un entry pt fiecare client + socket-ul listenfd + input tastatura (comanda EXIT)
  struct pollfd poll_fds[MAX_CONNECTIONS + 2];
  struct cli clients[MAX_CONNECTIONS];
  int num_sockets = 2, total_clients = 0;
  int rc;

  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    clients[i].active = 0;
    clients[i].msg_sent = 0;
  }

  struct chat_packet received_packet;

  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // multiplxare intre listen socket si citirea de la tastatura
  poll_fds[0].fd = listenfd;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = STDIN_FILENO;
  poll_fds[1].events = POLLIN;

  /*
    TODO 3: Adaugati un timerfd. Read-ul pe el se va debloca periodic, moment
    in care veti trimite anuntul promotional catre toti clientii.
  */

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

          clients[total_clients].id = newsockfd;
          clients[total_clients].active = 1;
          clients[total_clients].msg_sent = 0;
          total_clients++;

          printf("Noua conexiune de la %s, port %d, socket client %d\n",
                 inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
                 newsockfd);

          struct chat_packet welcome;
          snprintf(welcome.message, sizeof(welcome), "Bun venit! Ai ID-ul %d.", newsockfd);
          send_all(newsockfd, &welcome, sizeof(welcome));

        // primesc input de la tastatura (singura comanda valida: EXIT)
        } else if (poll_fds[i].fd == STDIN_FILENO) {
          char s_buf[MSG_MAXSIZE];
          
          if (!fgets(s_buf, sizeof(s_buf), stdin)) {
            break;
          }
          s_buf[strcspn(s_buf, "\n")] = 0;

          if(strcmp(s_buf, "EXIT") == 0) {
            printf("[SERVER] SE INCHIDE\n");
            for (int j = 2; j < num_sockets; j++) {
              // inchidem socket-urile pt. clienti
              close(poll_fds[j].fd);
              return;
            }
          }

        // am un client
        } else {
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc == 0) {
            printf("[SERVER] Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;

            for (int j = 0; j < total_clients; j++) {
              if (poll_fds[i].fd == clients[j].id) {
                clients[j].active = 0;
              }
            }
            // BCAST
          } else if (strncmp(received_packet.message, "BCAST", 5) == 0) {
            printf("BCAST %d: %s\n",
                   poll_fds[i].fd, received_packet.message + 6);
            /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */

            for (int k = 0; k < total_clients; k++) {
              if (clients[k].id == poll_fds[i].fd && clients[k].active == 1) {
                  clients[k].msg_sent++;
                  break;
              }
            }

            strcpy(received_packet.message, received_packet.message + 6);

            for (int j = 2; j < num_sockets; j++) {
              if (i != j) send_all(poll_fds[j].fd, &received_packet, sizeof(received_packet));
            }
            // PRIV!!
          } else if (strncmp(received_packet.message, "PRIV", 4) == 0) {
              // nu afisam in server
              // de la client i: PRIV Y mesaj

              int id_dest;
              char msg[MSG_MAXSIZE];

              if (sscanf(received_packet.message + 5, "%d %[^\n]", &id_dest, msg) < 2) {
                return; 
              }

              struct chat_packet priv_pkt;
              int found = 0;

              // cautam clientul destinatar in poll_fds (clientii online)
              for (int j = 2; j < num_sockets; j++) {
                if (poll_fds[j].fd == id_dest) {
                  snprintf(priv_pkt.message, sizeof(priv_pkt.message), "PRIV %d: %s", poll_fds[i].fd, msg);
                  send_all(poll_fds[j].fd, &priv_pkt, sizeof(priv_pkt));

                  found = 1;

                  // incrementam contorul de mesaje
                  for (int k = 0; k < total_clients; k++) {
                    if (clients[k].id == poll_fds[i].fd) {
                      clients[k].msg_sent++;
                      break;
                    }
                  }

                  break;
                }
              }

              if (!found) {
                // offline sau nu exista
                struct chat_packet err;
                memset(err.message, 0, sizeof(err.message));
                snprintf(err.message, sizeof(err.message), "Clientul %d nu este conectat", id_dest);
                send_all(poll_fds[i].fd, &err, sizeof(err));
              }
            // LIST !!
          } else if (strncmp(received_packet.message, "LIST", 4) == 0) {
              struct chat_packet list;
              memset(list.message, 0, sizeof(list.message));
              snprintf(list.message, sizeof(list.message), "ID_CLIENT  STARE  MESAJE\n");

              for (int i = 0; i < total_clients; i++) {
                char line[64];
                sprintf(line, "%d %s %d\n",
                      clients[i].id,
                      clients[i].active ? "ONLINE" : "OFFLINE",
                      clients[i].msg_sent);
                strcat(list.message, line);
              }

              strcat(list.message, "SFARSIT");
              send_all(poll_fds[i].fd, &list, sizeof(list));
            // primim EXIT!!!
          } else if (strncmp(received_packet.message, "EXIT", 4) == 0) {
              int id;

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

  // Parsam port-ul ca un numar
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

  /*
    TODO 2.1: Folositi implementarea cu multiplexare
  */
  run_chat_multi_server(listenfd);

  // Inchidem listenfd
  close(listenfd);

  return 0;
}