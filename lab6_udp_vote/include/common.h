#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>

#define MAX_SIZE 1024

// Tipuri de mesaje client -> server
typedef enum {
    MSG_CONNECT,      // inregistrare la server
    MSG_VOTE,         // clientul voteaza (payload = ID candidat ca string)
    MSG_LIST,         // clientul cere lista candidatilor
    MSG_EXIT,         // clientul se deconecteaza
} msg_type;

// Tipuri de raspuns server -> client
typedef enum {
    RESP_WELCOME,           // "Bun venit. Ai ID-ul X."
    RESP_CANDIDATE_OK,      // "Am adaugat candidatura pentru ID-ul X."
    RESP_CANDIDATE_ALREADY, // "Ai candidat deja. Ai X voturi."
    RESP_VOTE_OK,           // "Am adaugat un vot pentru clientul X."
    RESP_VOTE_ALREADY,      // "Ai votat deja. Nu o mai poti face inca odata."
    RESP_VOTE_NO_CANDIDATE, // "Clientul X nu a candidat."
    RESP_LIST,              // tabelul CANDIDAT VOTE_COUNT
    RESP_CLOSING,           // server/client se inchide
} resp_type;

// Structura pachetului UDP cu numar de secventa
struct seq_udp {
    int seq;                // numarul de secventa (START-STOP)
    int type;               // msg_type sau resp_type
    int len;                // lungimea payload-ului
    char payload[MAX_SIZE];
};

#endif
