.PHONY: all

APP := lab3
OBJS := lab3.c.o

all: lab3

$(APP): $(OBJS)
	gcc -o $(@) $<

%.c.o: %.c
	gcc -Wall -Werror -c -o $@ $<

clean:
	rm -f *.o $(APP)
