#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>

#define MAXSIZE 128
#define PORT 8313

struct seq_udp {
  uint32_t len;
  uint16_t seq;
  char payload[MAXSIZE];
};

#endif
