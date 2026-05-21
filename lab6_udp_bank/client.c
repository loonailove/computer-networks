/*
 * Aplicatie bancara simpla - UDP cu START-STOP
 * client.c - peste scheletul din laborator
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
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

#define TIMEOUT_US 1000000  // 1s

// Trimite un pachet si asteapta ACK cu retransmisie la timeout
// Exact ca in send_file_start_stop din schelet, adaptat pentru un singur pachet
void send_with_ack(int sockfd, struct seq_udp *pkt,
                   struct sockaddr_in *dest, socklen_t dlen) {
    struct timeval tv = {0, TIMEOUT_US};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        sendto(sockfd, pkt, sizeof(*pkt), 0, (struct sockaddr *)dest, dlen);

        int ack;
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        int rc = recvfrom(sockfd, &ack, sizeof(ack), 0,
                          (struct sockaddr *)&from, &flen);

        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[SERVER] Timeout, retransmit seq=%d\n", pkt->seq);
                continue;
            }
            DIE(1, "recvfrom ack");
        }

        // Verifica ca ACK-ul vine de la clientul corect si are seq corect
        if (from.sin_port == dest->sin_port &&
            from.sin_addr.s_addr == dest->sin_addr.s_addr &&
            ack == pkt->seq) {
            break;
        }
    }

    struct timeval notv = {0, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &notv, sizeof(notv));
}

// Primeste un pachet de la server cu START-STOP
// Verifica seq, trimite ACK, retransmite ACK daca primim duplicat
void recv_with_ack(int sockfd, struct seq_udp *pkt, int expected_seq) {
    // Timeout pe recv in caz ca serverul nu trimite
    struct timeval tv = {0, TIMEOUT_US};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        int rc = recvfrom(sockfd, pkt, sizeof(*pkt), 0, NULL, NULL);

        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            DIE(1, "recvfrom resp");
        }

        int ack;
        if (pkt->seq == expected_seq) {
            ack = pkt->seq;
            // send() merge pentru ca am facut connect() mai jos in main
            send(sockfd, &ack, sizeof(ack), 0);
            break;
        } else {
            // Duplicat de la server, re-ACK ultimul bun
            ack = expected_seq - 1;
            send(sockfd, &ack, sizeof(ack), 0);
        }
    }

    struct timeval notv = {0, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &notv, sizeof(notv));
}

void run_client(int sockfd, struct sockaddr_in *servaddr) {
    char buf[MAX_SIZE];
    int client_seq = 0;  // seq pentru pachetele trimise de client
    int server_seq = 0;  // seq asteptat de la server

    struct seq_udp pkt, resp;

    // Trimitem MSG_CONNECT - ne inregistram la server si primim ID-ul
    memset(&pkt, 0, sizeof(pkt));
    pkt.type = MSG_CONNECT;
    pkt.seq  = client_seq++;
    pkt.len  = 0;

    send_with_ack(sockfd, &pkt, servaddr, sizeof(*servaddr));
    recv_with_ack(sockfd, &resp, server_seq++);
    printf("%s\n", resp.payload);

    if (resp.type == RESP_CLOSING) return;

    // Bucla principala: citim de la stdin si trimitem comenzi
    while (1) {
        if (!fgets(buf, sizeof(buf), stdin)) break;
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;

        memset(&pkt, 0, sizeof(pkt));
        pkt.seq = client_seq++;

        if (strncmp(buf, "TRANSFER ", 9) == 0) {
            pkt.type = MSG_TRANSFER;
            // "TRANSFER ID X" -> trimitem "ID X" in payload
            strncpy(pkt.payload, buf + 9, sizeof(pkt.payload) - 1);
            pkt.len = strlen(pkt.payload) + 1;

        } else if (strcmp(buf, "LIST") == 0) {
            pkt.type = MSG_LIST;
            pkt.len  = 0;

        } else if (strcmp(buf, "EXIT") == 0) {
            pkt.type = MSG_EXIT;
            pkt.len  = 0;

            send_with_ack(sockfd, &pkt, servaddr, sizeof(*servaddr));
            recv_with_ack(sockfd, &resp, server_seq++);
            printf("%s\n", resp.payload);
            break;

        } else {
            printf("Comanda necunoscuta. Valide: TRANSFER ID X, LIST, EXIT\n");
            client_seq--;  // n-am trimis nimic
            continue;
        }

        send_with_ack(sockfd, &pkt, servaddr, sizeof(*servaddr));
        recv_with_ack(sockfd, &resp, server_seq++);

        if (resp.type == RESP_CLOSING) {
            printf("%s\n", resp.payload);
            break;
        }

        printf("%s\n", resp.payload);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_server> <port_server>\n", argv[0]);
        return 1;
    }

    uint16_t port;
    int rc = sscanf(argv[2], "%hu", &port);
    DIE(rc != 1, "port invalid");

    // Cream socket UDP - la fel ca in schelet
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd < 0, "socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    rc = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    DIE(rc <= 0, "inet_pton");

    // connect() pe UDP fixeaza portul local si ne permite sa folosim send/recv
    // fara sendto/recvfrom cu adresa de fiecare data
    rc = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    DIE(rc < 0, "connect");

    // Aflam portul local alocat de OS - acesta e ID-ul nostru
    struct sockaddr_in local;
    socklen_t llen = sizeof(local);
    getsockname(sockfd, (struct sockaddr *)&local, &llen);
    printf("[Client] PORNESTE\n");

    run_client(sockfd, &servaddr);

    close(sockfd);
    return 0;
}
