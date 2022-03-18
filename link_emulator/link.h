#ifndef _LINK_H_
#define _LINK_H_

#include "lib.h"

typedef struct {
  msg *m;
  unsigned long long finish_time;
} msg_in_flight;

#endif /* _LINK_H_ */
