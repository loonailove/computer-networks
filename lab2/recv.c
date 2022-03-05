#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

#define DLE (char)0
#define STX (char)2
#define ETX (char)3

int main(int argc,char** argv){
  init(HOST,PORT);
  

  char c;

  /* Wait for the start of a frame */
  char c1,c2;
  do {
	c1 = recv_byte();
	c2 = recv_byte();
  } while(c1 != DLE || c2 != STX);
  
  printf("%d ## %d\n",c1, c2);
  c = recv_byte();
  printf("%c\n", c);

  c = recv_byte();
  printf("%c\n", c);

  c = recv_byte();
  printf("%c\n", c);

  c = recv_byte();
  printf("%c\n", c);

  c = recv_byte();
  printf("%c\n", c);

  c = recv_byte();
  printf("%c\n", c);

  printf("[RECEIVER] Finished transmission\n");
  return 0;
}
