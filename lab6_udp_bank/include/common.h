#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>

#define MAX_SIZE 1024
 
// Tipuri de mesaje client -> server
typedef enum {
    MSG_CONNECT,
    MSG_TRANSFER,
    MSG_LIST,
    MSG_EXIT,
} msg_type;
 
// Tipuri de raspuns server -> client
typedef enum {
    RESP_WELCOME,
    RESP_TRANSFER_OK,
    RESP_TRANSFER_LOW_BALANCE,
    RESP_TRANSFER_NO_ACCOUNT,
    RESP_LIST,
    RESP_CLOSING,
} resp_type;
 
// Structura pachetului UDP cu numar de secventa
// Extindem struct seq_udp din schelet cu campul type
struct seq_udp {
    int seq;               // numarul de secventa (START-STOP)
    int type;              // msg_type sau resp_type
    int len;               // lungimea payload-ului
    char payload[MAX_SIZE];
};

#endif
