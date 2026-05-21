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

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
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

void run_chat_server(int listenfd) {
  struct sockaddr_in client_addr1;
  struct sockaddr_in client_addr2;
  socklen_t clen1 = sizeof(client_addr1);
  socklen_t clen2 = sizeof(client_addr2);

  int connfd1 = -1;
  int connfd2 = -1;
  int rc;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, 2);
  DIE(rc < 0, "listen");

  // Acceptam doua conexiuni
  printf("Astept conectarea primului client...\n");
  connfd1 = accept(listenfd, (struct sockaddr *)&client_addr1, &clen1);
  DIE(connfd1 < 0, "accept");

  printf("Astept connectarea clientului 2...\n");
  connfd2 = accept(listenfd, (struct sockaddr *)&client_addr2, &clen2);
  DIE(connfd2 < 0, "accept");

  while (1) {
    printf("Primesc de la 1 si trimit catre 2...\n");
    int rc = receive_and_send(connfd1, connfd2, sizeof(struct chat_packet));
    if (rc <= 0) {
      break;
    }

    printf("Primesc de la 2 si trimit catre 1...\n");
    rc = receive_and_send(connfd2, connfd1, sizeof(struct chat_packet));
    if (rc <= 0) {
      break;
    }
  }

  // Inchidem conexiunile si socketii creati
  close(connfd1);
  close(connfd2);
}

// Structură pentru a reține starea fiecărui client
struct client_info {
    int fd;
    int msg_count;
    int active; // 1 dacă e conectat, 0 dacă a ieșit (pentru istoric LIST)
};

void run_chat_multi_server(int listenfd) {
  struct pollfd poll_fds[MAX_CONNECTIONS + 2];
  struct client_info clients[MAX_CONNECTIONS + 2];
  struct client_info history[MAX_CONNECTIONS];
  int total_clients_ever = 0;
  int num_sockets = 2;
  int rc;

  for(int i = 0; i < MAX_CONNECTIONS; i++) {
    clients[i].active = 0;
    clients[i].msg_count = 0;
  }

  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = listenfd;
  poll_fds[1].events = POLLIN;

  printf("[Server] PORNESTE\n");

  while (1) {
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_sockets; i++) {

      if (poll_fds[i].revents & POLLIN) {
        
        // 1. primim input de la tastatura server-ului
        // EXIT este singura comanda valida
        if (poll_fds[i].fd == STDIN_FILENO) {
          char s_buf[MSG_MAXSIZE];
          fgets(s_buf, sizeof(s_buf), stdin);
          s_buf[strcspn(s_buf, "\n")] = 0;

          if (strcmp(s_buf, "EXIT") == 0) {
            printf("[Server] SE INCHIDE\n");
            for (int j = 2; j < num_sockets; j++) {
              close(poll_fds[j].fd);
              return;
            }
          }
        }

        // 2. conexiune noua
        else if (poll_fds[i].fd == listenfd) {
          // daca socket-ul pe care primim un interrupt este chiar listenfd
          // acceptam o noua conexiune
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);

          int newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");
          
          poll_fds[num_sockets].fd = newsockfd;
          poll_fds[num_sockets].events = POLLIN;
          
          clients[num_sockets].fd = newsockfd;
          clients[num_sockets].active = 1;

          // exista deja clientul acesta in lista? (id)
          int found_index = -1;
          for (int k = 0; k < total_clients_ever; k++) {
              if (history[k].fd == newsockfd) {
                  found_index = k;
                  break;
              }
          }

          if (found_index != -1) {
              // Clientul a mai fost conectat, doar îl activăm la loc
              history[found_index].active = 1;
              // Păstrăm numărul de mesaje anterior (sau îl resetăm la 0, depinde de cerință)
              // history[found_index].msg_count = 0; 
              
              // Sincronizăm și structura locală clients[i] folosită în bucla poll
              clients[num_sockets] = history[found_index];
          } else {
              // Client nou, îl adăugăm în istoric
              history[total_clients_ever].fd = newsockfd;
              history[total_clients_ever].msg_count = 0;
              history[total_clients_ever].active = 1;
              
              clients[num_sockets] = history[total_clients_ever];
              total_clients_ever++;
          }

          // Trimitem ID-ul (care este chiar indexul sau socketul conform cerintei)
          struct chat_packet welcome;
          sprintf(welcome.message, "Bun venit. Ai ID-ul %d.", newsockfd);
          send_all(newsockfd, &welcome, sizeof(welcome));
          
          num_sockets++;

        // 3. am primit mesaj de la clienti
        } else {
          struct chat_packet pkt;
          int rc = recv_all(poll_fds[i].fd, &pkt, sizeof(pkt));

          // 3.1. am primit EXIT de la un client
          if (rc <= 0 || strcmp(pkt.message, "EXIT") == 0) {
            for (int k = 0; k < total_clients_ever; k++) {
                if (history[k].fd == poll_fds[i].fd) {
                    history[k].active = 0;
                    break;
                }
            }

            clients[i].active = 0;
            close(poll_fds[i].fd);

            // il scoatem din lista de clienti
            for (int j = i; j < num_sockets - 1; j++) {
                poll_fds[j] = poll_fds[j + 1];
                clients[j] = clients[j + 1];
            }
            num_sockets--;
            i--;
            // 3.2. BCAST sau PRIV sau LIST
          } else {
            for (int k = 0; k < total_clients_ever; k++) {
              if (history[k].fd == poll_fds[i].fd) {
                  history[k].msg_count++;
                  clients[i].msg_count = history[k].msg_count; // sincronizare (cu msg istoric)
                  break;
              }
            }
            
            if (strncmp(pkt.message, "BCAST", 5) == 0) {
              // Trimite la toți în afară de expeditor
              struct chat_packet bcast_pkt;
              sprintf(bcast_pkt.message, "BCAST %d: %s", poll_fds[i].fd, pkt.message + 6);
              for (int j = 2; j < num_sockets; j++) {
                if (i != j) send_all(poll_fds[j].fd, &bcast_pkt, sizeof(bcast_pkt));
              }
            } 
            else if (strncmp(pkt.message, "PRIV", 4) == 0) {
              int target_fd;
              char msg[MSG_MAXSIZE];
              sscanf(pkt.message + 5, "%d %s", &target_fd, msg);
              
              struct chat_packet priv_pkt;
              sprintf(priv_pkt.message, "PRIV %d: %s", poll_fds[i].fd, pkt.message + 7); // ajustat offset pt msg
              
              int found = 0;
              // !!! incepe de la j = 2 (sarim peste stdin si listenfd)
              for(int j = 2; j < num_sockets; j++) {
                  if(poll_fds[j].fd == target_fd) {
                      send_all(target_fd, &priv_pkt, sizeof(priv_pkt));
                      found = 1;
                      break;
                  }
              }
              if(!found) {
                  struct chat_packet err;
                  sprintf(err.message, "Clientul %d nu este conectat", target_fd);
                  send_all(poll_fds[i].fd, &err, sizeof(err));
              }
            }
            else if (strcmp(pkt.message, "LIST") == 0) {
              struct chat_packet list_pkt;
              memset(list_pkt.message, 0, sizeof(list_pkt.message));
              strcpy(list_pkt.message, "ID_CLIENT  STARE  MESAJE\n");

              for (int j = 0; j < total_clients_ever; j++) {
                  char line[64];
                  sprintf(line, "%d  %s  %d\n", 
                          history[j].fd, 
                          history[j].active ? "ONLINE" : "OFFLINE", 
                          history[j].msg_count);
                  strcat(list_pkt.message, line);
              }
              
              strcat(list_pkt.message, "SFARSIT");
              send_all(poll_fds[i].fd, &list_pkt, sizeof(list_pkt));
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
