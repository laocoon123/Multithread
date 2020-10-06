.phony all:
all: p2

p2: mts.c
	gcc mts.c -pthread -o mts

.PHONY clean:
clean:
	-rm -rf *.o *.exe

