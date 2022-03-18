#ifndef _LIB_H_
#define _LIB_H_

#include <stddef.h>
#include <stdint.h>

#define MAX_LEN 1500

typedef struct {
  int len;
  char payload[MAX_LEN];
} msg;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg *t);
int recv_message(msg* r);
int recv_message_timeout(msg *m, int timeout);
int link_send(void *buf, int len);
int link_recv(void *buf, int len);
int send_byte(uint8_t b);
uint8_t recv_byte(void);

#endif /* _LIB_H_ */
