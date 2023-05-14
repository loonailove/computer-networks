# Protocoale de comunicații
# Laborator 11 - Securitate
# Makefile
CFLAGS = -Wall -g -Werror -Wno-error=unused-variable -Iinclude/

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 55.55.55.2

all: server client attacker

common.o: common.c tea.o utils.o

lib.o: lib.c

utils.o: utils.c

dh.o: dh.c

tea.o: tea.c utils.o

# Compileaza server.c
server: server.c common.o tea.o utils.o dh.o

# Compileaza client.c
client: client.c common.o tea.o utils.o dh.o

# Compileaza attacker.clang
attacker: attacker.c common.o lib.o tea.o utils.o dh.o

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server: server
	./server 0.0.0.0 ${PORT}

# Ruleaza clientul
run_client: client
	./client ${IP_SERVER} ${PORT}

run_attacker: attacker
	./attacker

clean:
	rm -rf server client attacker *.o *.dSYM
