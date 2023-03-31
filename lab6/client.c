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
#include <sys/time.h>

#include "common.h"
#include "utils.h"
#include "list.h"

/* Max size of the datagrams that we will be sending */
#define CHUNKSIZE MAX_SIZE;
#define SENT_FILENAME "file.bin"
#define SERVER_IP "172.16.0.100"

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
    seq++;

    /* TODO 1.2: Send the datagram. */

    /* TODO 1.3: Wait for ACK before moving to the next datagram to send. If timeout, resend the datagram. */
    
    if (n == 0) // end of file
      break;

  }
}

void send_file_go_back_n(int sockfd, struct sockaddr_in server_address,
                      char *filename) {

  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;

  /* TODO 2.1: Increase window size to a value that optimally uses the link */
  int window_size = 5;
  window->max_seq = 5;

  int seq = 0;
  while (1) {
    /* TODO: 1.1 Read all the data of the and add it as datagrams in
     * datagram_queue */
    /* Reads the content of a file */
    struct seq_udp *d = malloc(sizeof(struct seq_udp));
    int n = read(fd, d->payload, sizeof(d->payload));
    DIE(n < 0, "read");
    d->len = n;
    d->seq = seq;

    add_list_elem(window, d, sizeof(struct seq_udp), seq);
    seq++;

    if (n == 0) // end of file
      break;
  }


  /* TODO 2.2: Send window_size  packets to the server to saturate the link */

  /* In a loop */

    /* TODO 2.2: On ACK remove from the list all the segments that have been ACKed
                and send the next new segments added to the window */

    /* TODO 2.3: On timeout on recv resend all the segments from the window*/
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

int main(int argc, char *argv[]) {

  /* We use this structure to store the server info. IP address and Port.
   * This will be written by the UDP implementation into the header */
  struct sockaddr_in servaddr;
  int sockfd, rc;

  /* Our transmission window*/
  window = create_list();

  // Creating socket file descriptor. SOCK_DGRAM for UDP
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(sockfd < 0, "socket");

  /* Set the timeout on the socket */
  struct timeval timeout;      
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
    
  rc = setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
              sizeof timeout);
  DIE(rc >= 0, "setsockopt");

  // Fill the information that will be put into the IP and UDP header to
  // identify the target process (via PORT) on a given host (via SEVER_IP)
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  inet_aton(SERVER_IP, &servaddr.sin_addr);

  /* TODO: Read the demo function.
  Implement and test (one at a time) each of the proposed versions for sending a
  file. */
  send_a_message(sockfd, servaddr);
  // send_file_start_stop(sockfd, servaddr, SENT_FILENAME);
  // send_file_window(sockfd, servaddr, SENT_FILENAME);

  close(sockfd);

  return 0;
}
