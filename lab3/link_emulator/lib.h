#ifndef LIB
#define LIB

#include <inttypes.h>

#define MAX_LEN 15

typedef struct {
  int len;
  char payload[MAX_LEN];
  uint16_t sum;
} msg;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const void* m);
int recv_message(void* r);

#endif
