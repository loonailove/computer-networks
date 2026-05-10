#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "list.h"
#include "utils.h"

// Max size of the datagrams that we will be sending
#define CHUNKSIZE MAX_SIZE;
#define SENT_FILENAME "file.bin"
#define SERVER_IP "172.16.0.100"

#define TICK(X)                                                                \
  struct timespec X;                                                           \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X)

#define TOCK(X)                                                                \
  struct timespec X##_end;                                                     \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X##_end);                                \
  printf("Total time = %f seconds\n",                                          \
         (X##_end.tv_nsec - (X).tv_nsec) / 1000000000.0 +                      \
             (X##_end.tv_sec - (X).tv_sec))

list *window;

void send_file_start_stop(int sockfd, struct sockaddr_in server_address,
                          char *filename) {

  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;
  int seq = 0;

  while (1) {
    /* Reads a chunk of the file */
    struct seq_udp d;
    int n = read(fd, d.payload, sizeof(d.payload));
    DIE(n < 0, "read");

    d.len = n;
    d.seq = seq;

    // TODO 1.1: Send the datagram.
    while(1) {
      rc = sendto(sockfd, &d, sizeof(struct seq_udp) , 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));
      DIE(rc < 0, "sendto");

      // TODO 1.2: Wait for ACK before moving to the next datagram to send.
      // If timeout or wrong seq number, resend the datagram.
      int ack;
      rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);

      if (rc < 0) { // recvfrom failed
        /* If we are here, it means recvfrom failed */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          printf("[Timeout] Resending packet with seq %d...\n", seq);
          continue; 
        } else {
          DIE(1, "recvfrom error");
        }
      } else {
        /* We got something! Check if it's the correct ACK */
        if (ack == seq) {
          // Success! Break the inner loop to move to the next chunk
          break;
        }
        // If it's the wrong ACK, the loop continues and re-sends
      }
    }

    seq++;
    if (n == 0) // end of file
      break;
  }

  close(fd); }

void send_file_go_back_n(int sockfd, struct sockaddr_in server_address, char *filename) {
  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;

  // TODO 2.1: Setăm fereastra
  int window_size = 50; 
  
  // Citim tot fișierul și îl punem în listă
  int seq = 0;
  while (1) {
    struct seq_udp *d = malloc(sizeof(struct seq_udp));
    DIE(d == NULL, "malloc");

    int n = read(fd, d->payload, sizeof(d->payload));
    DIE(n < 0, "read");
    d->len = n;
    d->seq = seq;

    // Folosim funcția ta din list.h
    add_list_elem(window, d, sizeof(struct seq_udp), seq);
    
    if (n == 0) break; // Final de fișier
    seq++;
  }

  int last_seq = seq;
  int base_seq = 0;
  int next_to_send = 1;

  while (base_seq <= last_seq) {
    
    // Trimitem pachete noi până umplem fereastra
    while (next_to_send < base_seq + window_size && next_to_send <= last_seq) {
      
      // Căutăm pachetul manual în listă folosind 'head'
      list_entry *curr = window->head;
      struct seq_udp *p = NULL;

      while (curr != NULL) {
        if (curr->seq == next_to_send) {
          p = (struct seq_udp *)curr->info; // info este void* în structura ta
          break;
        }
        curr = curr->next;
      }

      if (p) {
        sendto(sockfd, p, sizeof(struct seq_udp), 0,
               (struct sockaddr *)&server_address, sizeof(server_address));
        next_to_send++;
      } else {
          // Dacă nu mai găsim pachetul în listă, ieșim din bucla de trimitere
          break;
      }
    }

    // Așteptăm ACK
    int ack;
    rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);

    if (rc < 0) {
      // TODO 2.3: Timeout -> Resetăm next_to_send (Go-Back-N)
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("[Timeout] GBN: Go Back to seq %d\n", base_seq);
        next_to_send = base_seq; 
      }
    } else {
      // TODO 2.2: ACK cumulativ
      if (ack >= base_seq) {
        // În acest lab, poți lăsa elementele în listă și doar să crești base_seq
        // Glisăm fereastra
        base_seq = ack + 1;
      }
    }
  }

  close(fd);
  printf("Transfer GBN terminat!\n");
}

void send_a_message(int sockfd, struct sockaddr_in server_address) {
  struct seq_udp d;
  strcpy(d.payload, "Hello world!");
  d.len = strlen("Hello world!");

  // Send a UDP datagram. Sendto is implemented in the kernel (network stack of
  // it), it basically creates a UDP datagram, sets the payload to the data we
  // specified in the buffer, and the completes the IP header and UDP header
  // using the sever_address info.
  int rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));

  DIE(rc < 0, "send");

  // Receive the ACK. recvfrom is blocking with the current parameters 
  int ack;
  rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
}

int main(void) {

  // We use this structure to store the server info. IP address and Port.
  // This will be written by the UDP implementation on recvfrom().
  struct sockaddr_in servaddr;
  int sockfd, rc;

  // for benchmarking
  TICK(TIME_A);

  // Our transmission window
  window = create_list();

  // Creating socket file descriptor. SOCK_DGRAM for UDP
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(sockfd < 0, "socket");

  // Set the timeout on the socket
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 250000; // 250ms

  rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
  DIE(rc < 0, "setsockopt");

  // Fill the information that will be put into the IP and UDP header to
  // identify the target process (via PORT) on a given host (via SEVER_IP)
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  inet_aton(SERVER_IP, &servaddr.sin_addr);

  // TODO: Read the demo function.
  // Implement and test (one at a time) each of the proposed versions for sending a
  // file.

  send_a_message(sockfd, servaddr);
  // send_file_start_stop(sockfd, servaddr, SENT_FILENAME);
  send_file_go_back_n(sockfd, servaddr, SENT_FILENAME);

  close(sockfd);

  free(window);

  // Print the runtime of the program
  TOCK(TIME_A);

  return 0;
}
