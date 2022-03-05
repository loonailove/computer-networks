#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

/* We don't touch this */
#define HOST "127.0.0.1"
#define PORT 10000

#define DLE (char)0
#define STX (char)2
#define ETX (char)3

int main(int argc,char** argv){
	init(HOST,PORT);

	/* Send Hello */
	send_byte(DLE);
	send_byte(STX);
	send_byte('H');
	send_byte('e');
	send_byte('l');
	send_byte('l');
	send_byte('o');
	send_byte('!');
	
	return 0;
}
