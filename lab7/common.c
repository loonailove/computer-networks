#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  /*
    while(bytes_remaining) {
      TODO: Make the magic happen
    }
  */
  while(bytes_remaining > 0) {
    int rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
    
    // Dacă rc este 0, conexiunea a fost închisă. 
    // Dacă este < 0, a apărut o eroare.
    if (rc <= 0) {
      return rc;
    }
    
    bytes_received += rc;
    bytes_remaining -= rc;
  }

  /*
    TODO: Returnam exact cati octeti am citit
  */
  
  //return recv(sockfd, buffer, len, 0);
  return bytes_received;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  /*
    while(bytes_remaining) {
      TODO: Make the magic happen
    }
  */
  while(bytes_remaining > 0) {
    int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
    
    if (rc <= 0) {
      return rc;
    }
    
    bytes_sent += rc;
    bytes_remaining -= rc;
  }
  /*
    TODO: Returnam exact cati octeti am trimis
  */
  //return send(sockfd, buffer, len, 0);
  return bytes_sent;
}