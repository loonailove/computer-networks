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

struct cli {
  int id;
  int active;
  int msg_sent;
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

  /*
    TODO 3: Adaugati un timerfd. Read-ul pe el se va debloca periodic, moment
    in care veti trimite anuntul promotional catre toti clientii.
  */

  while (1) {
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_sockets; i++) {

      if (poll_fds[i].revents & POLLIN) {
        
        // 1. primim o cerere de conexiune
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

          // verificam daca acest client s-a mai conectat
          int found = 0;
          for (int k = 0; k < total_clients; k++) {
              if (clients[k].id == newsockfd) {
                  clients[k].active = 1;
                  found = 1;
                  break;
              }
          }
          if (!found) {
              clients[total_clients].id = newsockfd;
              clients[total_clients].active = 1;
              clients[total_clients].msg_sent = 0;
              total_clients++;
          }

          printf("[SERVER] Clientul %d s-a conectat.\n", newsockfd);

          struct chat_packet welcome;
          snprintf(welcome.message, sizeof(welcome.message), "Bine ai venit! ID-ul tau este %d.", newsockfd);

          rc = send_all(newsockfd, &welcome, sizeof(welcome));
          DIE(rc < 0, "send_all");
        
        // 2. primim un input de la tastatura (singura comanda valida este EXIT)
        } else if (poll_fds[i].fd == STDIN_FILENO) {
            char s_buf[MSG_MAXSIZE];

            if (!fgets(s_buf, sizeof(s_buf), stdin)) {
              break;
            }
            s_buf[strcspn(s_buf, "\n")] = '\0';

            if (strcmp(s_buf, "EXIT") == 0) {
              printf("[SERVER] Inchidem conexiunea...\n");

              for (int j = 2; j < num_sockets; j++) {
                close(poll_fds[j].fd);
              }

              return;
            }

        // 3. primim mesaje de la clienti
        // comenzi: BCAST, PRIV, LIST, EXIT
        } else {
          // Am primit date pe unul din socketii de client, asa ca le receptionam
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          // EXIT
          if (rc == 0) {
            printf("[SERVER] Clientul %d a inchis conexiunea\n", poll_fds[i].fd);
            close(poll_fds[i].fd);

            for (int k = 0; k < total_clients; k++) {
              if (poll_fds[i].fd == clients[k].id) {
                clients[k].active = 0;
              }
            }

            // Scoatem din multimea de citire socketul inchis
            for (int j = i; j < num_sockets - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_sockets--;
            i--;
          // LIST
          } else if (strncmp(received_packet.message, "LIST", 4) == 0) {
              struct chat_packet list;
              memset(list.message, 0, sizeof(list.message));
              snprintf(list.message, sizeof(list.message), "ID_CLIENT STATUS MESAJE\n");
              
              for (int j = 0; j < total_clients; j++) {
                char line[64];
                sprintf(line, "%d %s %d\n",
                        clients[j].id,
                        clients[j].active ? "ONLINE" : "OFFLINE",
                        clients[j].msg_sent);
                strcat(list.message, line);
              }

              strcat(list.message, "SFARSIT");

              rc = send_all(poll_fds[i].fd, &list, sizeof(list));
              DIE(rc < 0, "recv_all");
          // PRIV
          } else if (strncmp(received_packet.message, "PRIV", 4) == 0) {
            int id_dest;
            char msg[MSG_MAXSIZE];

            if(sscanf(received_packet.message + 5, "%d %s", &id_dest, msg) < 2) {
              return;
            } 

            struct chat_packet priv;
            int found = 0;

            for (int k = 2; k < num_sockets; k++) {
              if (poll_fds[k].fd == id_dest) {
                snprintf(priv.message, sizeof(priv.message), "PRIV %d: %s", poll_fds[i].fd, msg);
                send_all(poll_fds[k].fd, &priv, sizeof(priv));

                found = 1;

                for (int j = 0; j < total_clients; j++) {
                  if (clients[j].id == poll_fds[i].fd) {
                    clients[j].msg_sent++;
                    break;
                  }
                }

                break;
              }
            }

            if (!found) {
              struct chat_packet err;
              snprintf(err.message, sizeof(err), "Clientul %d nu este conectat", id_dest);
              send_all(poll_fds[i].fd, &err, sizeof(err));
            }
          // BCAST
          } else if (strncmp(received_packet.message, "BCAST", 5) == 0) {
            struct chat_packet msg;
            strcpy(msg.message, received_packet.message + 6);
            msg.len = strlen(msg.message) + 1;
            msg.message[strcspn(msg.message, "\n")] = '\0';

            for (int k = 0; k < total_clients; k++) {
              if (clients[k].id == poll_fds[i].fd) {
                clients[k].msg_sent++;
                break;
              }
            }

            for (int j = 2; j < num_sockets; j++) {
              if (i != j) {
                rc = send_all(poll_fds[j].fd, &msg, sizeof(msg));
                DIE(rc < 0, "recv_all");
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
  // run_chat_server(listenfd);
  run_chat_multi_server(listenfd);

  // Inchidem listenfd
  close(listenfd);

  return 0;
}
