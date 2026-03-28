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
#include <unistd.h>
#include <time.h>

#include "common.h"
#include "queue.h"
#include "utils.h"

#define TICK(X)                                                                \
  struct timespec X;                                                           \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X)

#define TOCK(X)                                                                \
  struct timespec X##_end;                                                     \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X##_end);                                \
  printf("Total time = %f seconds\n",                                          \
         (X##_end.tv_nsec - (X).tv_nsec) / 1000000000.0 +                      \
             (X##_end.tv_sec - (X).tv_sec))

/* Max size of the datagrams that we will be sending */
#define CHUNKSIZE 1024
#define SENT_FILENAME "file.bin"
#define SERVER_IP "172.16.0.100"

/* Queue we will use for datagrams */
queue datagram_queue;

/*
#define _XOPEN_SOURCE_EXTENDED 1
#include <sys/socket.h>

ssize_t sendto(int socket, const void *buffer, size_t length, int flags,
               const struct sockaddr *address, size_t address_len);

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
 */

void send_file_start_stop(int sockfd, struct sockaddr_in server_address, char *filename) {
  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;

  while (1) {
    struct seq_udp d;
    int n = read(fd, d.payload, sizeof(d.payload));
    DIE(n < 0, "read");
    d.len = n;

    /* Send the datagram */
    rc = sendto(sockfd, &d, sizeof(d), 0,
                (struct sockaddr *)&server_address, sizeof(server_address));
    DIE(rc < 0, "sendto");

    /* Wait for ACK */
    struct seq_udp ack;
    socklen_t addr_len = sizeof(server_address);
    rc = recvfrom(sockfd, &ack, sizeof(ack), 0,
                  (struct sockaddr *)&server_address, &addr_len);
    DIE(rc < 0, "recvfrom");

    if (n == 0) // Break after sending the 0-length EOF packet
      break;
  }
  close(fd);
}

void send_file_window(int sockfd, struct sockaddr_in server_address, char *filename) {
  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");

  /* TODO 2.1: Increase window size. 
     Optimal window = (Bandwidth * RTT) / PacketSize. Let's try 100 for now. */
  int window_size = 100; 

  /* 1.1 Read the entire file into the queue first */
  while (1) {
    struct seq_udp *d = malloc(sizeof(struct seq_udp));
    int n = read(fd, d->payload, sizeof(d->payload));
    DIE(n < 0, "read");
    d->len = n;
    queue_enq(datagram_queue, d);
    if (n == 0) break;
  }

  /* 2.2: Initial burst - Send the first 'window_size' packets */
  for (int i = 0; i < window_size && !queue_empty(datagram_queue); i++) {
    struct seq_udp *to_send = (struct seq_udp *)queue_deq(datagram_queue);
    sendto(sockfd, to_send, sizeof(struct seq_udp), 0,
           (struct sockaddr *)&server_address, sizeof(server_address));
    free(to_send); // Free after sending
  }

  /* 2.2: Slide the window. Every time an ACK comes in, send the next packet. */
  while (!queue_empty(datagram_queue)) {
    struct seq_udp ack;
    socklen_t addr_len = sizeof(server_address);
    
    // Wait for any ACK
    recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&server_address, &addr_len);

    // Send the next one to keep the "pipe" full
    struct seq_udp *next_pkt = (struct seq_udp *)queue_deq(datagram_queue);
    sendto(sockfd, next_pkt, sizeof(struct seq_udp), 0,
           (struct sockaddr *)&server_address, sizeof(server_address));
    free(next_pkt);
  }
  
  close(fd);
}

void send_a_message(int sockfd, struct sockaddr_in server_address) {
  struct seq_udp d;
  strcpy(d.payload, "Hello world!");
  d.len = strlen("Hello world!");

  /* Send a UDP datagram. Sendto is implemented in the kernel (network stack of
   * it), it basically creates a UDP datagram, sets the payload to the data we
   * specified in the buffer, and the completes the IP header and UDP header
   * using the sever_address info.*/
  int rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));

  DIE(rc < 0, "send");

  /* Receive the ACK. recvfrom is blocking with the current parameters */
  int ack;
  rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
}

int main(void) {

  /* We use this structure to store the server info. IP address and Port.
   * This will be written by the UDP implementation into the header */
  struct sockaddr_in servaddr;
  int sockfd, rc;

  // for benchmarking
  TICK(TIME_A);
  
  /* Queue that we will use when implementing sliding window */
  datagram_queue = queue_create();

  // Creating socket file descriptor. SOCK_DGRAM for UDP
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(sockfd < 0, "socket");

  // Fill the information that will be put into the IP and UDP header to
  // identify the target process (via PORT) on a given host (via SEVER_IP)
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  rc = inet_aton(SERVER_IP, &servaddr.sin_addr);
  DIE(rc == 0, "Invalid IP address for server");

  /* TODO: Read the demo function.
  Implement and test (one at a time) each of the proposed versions for sending a
  file. */
  send_a_message(sockfd, servaddr);
  // send_file_start_stop(sockfd, servaddr, SENT_FILENAME);
  // send_file_window(sockfd, servaddr, SENT_FILENAME);

  /* Print the runtime of the program */
  TOCK(TIME_A);
  
  close(sockfd);

  return 0;
}
