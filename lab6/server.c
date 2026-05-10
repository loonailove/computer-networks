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

#define SAVED_FILENAME "received_file.bin"

int recv_seq_udp(int sockfd, struct seq_udp *seq_packet, int expected_seq) {
  struct sockaddr_in client_addr;
  socklen_t clen = sizeof(client_addr);

  // Primim segmentul (apel blocant)
  int rc = recvfrom(sockfd, seq_packet, sizeof(struct seq_udp), 0,
                    (struct sockaddr *)&client_addr, &clen);
  
  if (rc < 0) return -1; // Eroare la receptie

  // TODO: Check if the sequence number is the expected one.
  if (seq_packet->seq == expected_seq) {
    /* TODO: If we got the expected packet send ACK for it */
    int ack = seq_packet->seq;
    
    printf("[Server] Got expected seq %d, sending ACK.\n", ack);
    
    sendto(sockfd, &ack, sizeof(ack), 0, 
           (struct sockaddr *)&client_addr, clen);

    // Returnăm numărul de bytes reali de date ca să știm cât scriem în fișier
    return seq_packet->len;

  } else {
    /* TODO: If segment is not the expected one, send ACK for the last 
       well received packet (expected_seq - 1) */
    int last_ack = expected_seq - 1;
    
    printf("[Server] Wrong seq! Got %d, expected %d. Sending ACK for %d.\n", 
            seq_packet->seq, expected_seq, last_ack);

    sendto(sockfd, &last_ack, sizeof(last_ack), 0, 
           (struct sockaddr *)&client_addr, clen);

    // Returnăm -1 pentru a-i spune funcției apelante să NU scrie în fișier
    return -1;
  }
}

void recv_a_file(int sockfd, char *filename) {
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  DIE(fd < 0, "open");
  struct seq_udp p;
  int expected_seq = 0;
  int rc;

  while (1) {
    rc = recv_seq_udp(sockfd, &p, expected_seq);

    if (rc == -1) {
      // Pachet greșit, nu facem nimic, așteptăm retransmisia
      continue;
    }

    // Dacă rc >= 0, înseamnă că e pachetul corect
    if (p.len == 0) break; // Final de fișier

    write(fd, p.payload, p.len);
    expected_seq++; // Incrementăm DOAR dacă pachetul a fost cel bun
  }
  close(fd);
}

void recv_a_message(int sockfd) {
  // Receive a datagram and send an ACK
  // The info of the who sent the datagram (PORT and IP)
  struct sockaddr_in client_addr;
  struct seq_udp p;
  socklen_t clen = sizeof(client_addr);
  int rc = recvfrom(sockfd, &p, sizeof(struct seq_udp), 0,
                    (struct sockaddr *)&client_addr, &clen);

  // We know it's a string so we print it
  printf("[Server] Received: %s\n", p.payload);

  int ack = 0;
  // Sending ACK. We model ACK as datagrams with only an int of value 0.
  rc = sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr,
              clen);
  DIE(rc < 0, "send");
}

int main(void) {
  int sockfd;
  struct sockaddr_in servaddr;

  // Creating socket file descriptor
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Make ports reusable, in case we run this really fast two times in a row
  int enable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  // Fill the details on what destination port should the
  // datagrams have to be sent to our process.
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET; // IPv4
  // 0.0.0.0, basically match any IP
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(PORT);

  // Bind the socket with the server address. The OS networking
  // implementation will redirect to us the contents of all UDP
  // datagrams that have our port as destination
  int rc = bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
  DIE(rc < 0, "bind failed");

  // TODO 1.0: Study the code. Uncoment this to receive a file chuck by chuck
  // and save it locally
  recv_a_message(sockfd);
  recv_a_file(sockfd, SAVED_FILENAME);

  return 0;
}
