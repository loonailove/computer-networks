build: server client
CFLAGS = -Wall -Wextra -g -Wno-error=unused-variable
CINCLUDES = -I include/
all: server client

list.o: lib/list.c include/list.h

queue.o: lib/queue.c include/queue.h lib/list.o

server: server.c include/utils.h lib/queue.o lib/list.o
	gcc  $(CFLAGS) $(CINCLUDES) server.c list.o queue.o -o server

client: client.c include/utils.h lib/queue.o lib/list.o
	gcc  $(CFLAGS) $(CINCLUDES) client.c list.o queue.o -o client

.c.o:
	gcc $(CINCLUDES) $(CFLAGS) -g -c $?


.PHONY: clean run_client run_server random_file

# Construieste un fisier de 1MB (1024B*10^4)==1MB
random_file:
	dd if=/dev/urandom of=file.bin bs=1024 count=1000

clean:
	rm -f server client file.bin received_file.bin *.o include/*.h.gch
