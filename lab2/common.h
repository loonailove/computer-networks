#define DLE (char)0
#define STX (char)2
#define ETX (char)3

/* Atributul este folosit pentru a anunta compilatorul sa nu alinieze structura */
/* DELIM | DATE | DELIM */
struct __attribute__((packed)) Frame {
    char frame_delim_start[2]; /* DLE STX */

    /* TODO 2: Add source and destination */
    int source;
    int destination;

    char payload[100];  /* Datele pe care vrem sa le transmitem */
    char frame_delim_end[2]; /* DLE ETX */
};
