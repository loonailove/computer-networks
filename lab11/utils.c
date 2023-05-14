#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define HEX_LINESIZE 16

void hex_dump(const void * addr, size_t size)
{
    int i;
    unsigned char buff[HEX_LINESIZE+1];
    const unsigned char * pc = (const unsigned char *)addr;

    // Process every byte in the data.
    for (i = 0; i < size; i++) {
        // Multiple of HEX_LINESIZE means new or first line (with line offset).
        if ((i % HEX_LINESIZE) == 0) {
            // Only print previous-line ASCII buffer for lines beyond first.
            if (i != 0) printf("  %s\n", buff);
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And buffer a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % HEX_LINESIZE] = '.';
        else
            buff[i % HEX_LINESIZE] = pc[i];
        buff[(i % HEX_LINESIZE) + 1] = '\0';
    }

    // Pad out last line if not exactly HEX_LINESIZE characters.
    while ((i % HEX_LINESIZE) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII buffer.
    printf("  %s\n", buff);
}
