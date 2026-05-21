/*
 * Test Practic 2 - Aplicatie votare UDP cu START-STOP
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"

#define MAX_CLIENTS    32
#define MAX_CANDIDATES 32
#define TIMEOUT_US     250000   // 250ms

// ---- structuri ----

struct client_info {
    struct sockaddr_in addr;
    int port;           // portul sursa (folosit ca cheie de cautare)
    int id;             // ID secvential alocat de server: 0, 1, 2, ...
    int online;
    int has_voted;
    int expected_seq;   // seq asteptat de la acest client
    int server_seq;     // seq urmator trimis de server catre client
};

struct candidate_info {
    int client_id;
    int vote_count;
};

static struct client_info    clients[MAX_CLIENTS];
static int                   num_clients = 0;

static struct candidate_info candidates[MAX_CANDIDATES];
static int                   num_candidates = 0;

// ---- helpers ----

struct client_info *find_client_by_port(int port) {
    for (int i = 0; i < num_clients; i++)
        if (clients[i].port == port) return &clients[i];
    return NULL;
}

struct candidate_info *find_candidate(int client_id) {
    for (int i = 0; i < num_candidates; i++)
        if (candidates[i].client_id == client_id) return &candidates[i];
    return NULL;
}

// ---- START-STOP helpers ----

// Trimite un seq_udp si asteapta ACK, retransmite la timeout
void send_with_ack(int sockfd, struct seq_udp *pkt,
                   struct sockaddr_in *dest, socklen_t dlen) {
    struct timeval tv = {0, TIMEOUT_US};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        int rc = sendto(sockfd, pkt, sizeof(*pkt), 0,
                        (struct sockaddr *)dest, dlen);
        DIE(rc < 0, "sendto");

        int ack;
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        rc = recvfrom(sockfd, &ack, sizeof(ack), 0,
                      (struct sockaddr *)&from, &flen);

        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[SERVER] Timeout, retransmit seq=%d\n", pkt->seq);
                continue;
            }
            DIE(1, "recvfrom ack");
        }

        if (from.sin_port        == dest->sin_port &&
            from.sin_addr.s_addr == dest->sin_addr.s_addr &&
            ack == pkt->seq) {
            break;
        }
    }

    struct timeval notv = {0, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &notv, sizeof(notv));
}

// Primeste un seq_udp si trimite ACK; daca seq e gresit, re-ACK ultimul bun
int recv_with_ack(int sockfd, struct seq_udp *pkt,
                  struct sockaddr_in *src, socklen_t *slen,
                  int expected_seq) {
    while (1) {
        int rc = recvfrom(sockfd, pkt, sizeof(*pkt), 0,
                          (struct sockaddr *)src, slen);
        DIE(rc < 0, "recvfrom");

        int ack;
        if (pkt->seq == expected_seq) {
            ack = pkt->seq;
            sendto(sockfd, &ack, sizeof(ack), 0,
                   (struct sockaddr *)src, *slen);
            return 1;
        } else {
            ack = expected_seq - 1;
            sendto(sockfd, &ack, sizeof(ack), 0,
                   (struct sockaddr *)src, *slen);
        }
    }
}

// ---- server principal ----

void run_server(int sockfd) {
    struct pollfd poll_fds[2];
    poll_fds[0].fd     = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd     = sockfd;
    poll_fds[1].events = POLLIN;

    printf("[Server] PORNESTE\n");

    while (1) {
        int rc = poll(poll_fds, 2, -1);
        DIE(rc < 0, "poll");

        // ---- stdin: doar EXIT permis pe server ----
        if (poll_fds[0].revents & POLLIN) {
            char cmd[64];
            if (!fgets(cmd, sizeof(cmd), stdin)) continue;
            cmd[strcspn(cmd, "\r\n")] = 0;

            if (strcmp(cmd, "EXIT") == 0) {
                printf("[Server] SE INCHIDE\n");

                struct seq_udp pkt;
                memset(&pkt, 0, sizeof(pkt));
                pkt.type = RESP_CLOSING;
                snprintf(pkt.payload, sizeof(pkt.payload), "SE INCHIDE");
                pkt.len = strlen(pkt.payload) + 1;

                for (int i = 0; i < num_clients; i++) {
                    if (!clients[i].online) continue;
                    pkt.seq = clients[i].server_seq++;
                    send_with_ack(sockfd, &pkt,
                                  &clients[i].addr, sizeof(clients[i].addr));
                    clients[i].online = 0;
                }
                return;
            }
        }

        // ---- pachet de la un client ----
        if (poll_fds[1].revents & POLLIN) {
            struct seq_udp pkt;
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);

            // Peek ca sa aflam portul sursa fara sa consumam pachetul
            recvfrom(sockfd, &pkt, sizeof(pkt), MSG_PEEK,
                     (struct sockaddr *)&cli_addr, &cli_len);

            int src_port = ntohs(cli_addr.sin_port);
            struct client_info *cli = find_client_by_port(src_port);

            if (!cli) {
                // Client nou — ID = ordinea sosirii, incepand de la 0
                cli = &clients[num_clients];
                memset(cli, 0, sizeof(*cli));
                cli->addr         = cli_addr;
                cli->port         = src_port;
                cli->id           = num_clients;  // 0, 1, 2, ...
                cli->online       = 1;
                cli->has_voted    = 0;
                cli->expected_seq = 0;
                cli->server_seq   = 0;
                num_clients++;
                printf("[Client%d] PORNESTE\n", cli->id);
            } else if (!cli->online) {
                cli->online = 1;
                cli->addr   = cli_addr;
            }

            // Citim pachetul cu START-STOP
            recv_with_ack(sockfd, &pkt, &cli_addr, &cli_len,
                          cli->expected_seq);
            cli->expected_seq++;

            // Construim raspunsul
            struct seq_udp resp;
            memset(&resp, 0, sizeof(resp));
            resp.seq = cli->server_seq++;

            if (pkt.type == MSG_CONNECT) {
                resp.type = RESP_WELCOME;
                snprintf(resp.payload, sizeof(resp.payload),
                         "Bun venit. Ai ID-ul %d.", cli->id);

            } else if (pkt.type == MSG_CANDIDATE) {
                struct candidate_info *cand = find_candidate(cli->id);
                if (!cand) {
                    candidates[num_candidates].client_id  = cli->id;
                    candidates[num_candidates].vote_count = 0;
                    num_candidates++;
                    resp.type = RESP_CANDIDATE_OK;
                    snprintf(resp.payload, sizeof(resp.payload),
                             "Am adaugat candidatura pentru ID-ul %d.",
                             cli->id);
                } else {
                    resp.type = RESP_CANDIDATE_ALREADY;
                    snprintf(resp.payload, sizeof(resp.payload),
                             "Ai candidat deja. Ai %d voturi.",
                             cand->vote_count);
                }

            } else if (pkt.type == MSG_VOTE) {
                int target_id = atoi(pkt.payload);

                if (cli->has_voted) {
                    resp.type = RESP_VOTE_ALREADY;
                    snprintf(resp.payload, sizeof(resp.payload),
                             "Ai votat deja. Nu o mai poti face inca odata.");
                } else {
                    struct candidate_info *cand = find_candidate(target_id);
                    if (!cand) {
                        resp.type = RESP_VOTE_NO_CANDIDATE;
                        snprintf(resp.payload, sizeof(resp.payload),
                                 "Clientul %d nu a candidat.", target_id);
                    } else {
                        cand->vote_count++;
                        cli->has_voted = 1;
                        resp.type = RESP_VOTE_OK;
                        snprintf(resp.payload, sizeof(resp.payload),
                                 "Am adaugat un vot pentru clientul %d.",
                                 target_id);
                    }
                }

            } else if (pkt.type == MSG_LIST) {
                resp.type = RESP_LIST;
                int off = snprintf(resp.payload, sizeof(resp.payload),
                                   "CANDIDAT VOTE_COUNT\n");
                for (int i = 0; i < num_candidates; i++) {
                    off += snprintf(resp.payload + off,
                                    sizeof(resp.payload) - off,
                                    "%d %d\n",
                                    candidates[i].client_id,
                                    candidates[i].vote_count);
                }
                snprintf(resp.payload + off, sizeof(resp.payload) - off,
                         "SFARSIT");

            } else if (pkt.type == MSG_EXIT) {
                cli->online = 0;
                resp.type   = RESP_CLOSING;
                snprintf(resp.payload, sizeof(resp.payload), "SE INCHIDE");
                printf("[SERVER] Clientul %d s-a deconectat.\n", cli->id);
            }

            resp.len = strlen(resp.payload) + 1;
            send_with_ack(sockfd, &resp, &cli_addr, cli_len);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "port invalid");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd < 0, "socket");

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(port);

    rc = bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    DIE(rc < 0, "bind");

    run_server(sockfd);

    close(sockfd);
    return 0;
}
