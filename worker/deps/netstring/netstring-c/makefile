CFLAGS = -O2 -Wall
LDFLAGS= -lm 

.PHONY: test clean

all: testsuite

testsuite: testsuite.c netstring.o

test: testsuite
	./testsuite

clean:
	rm -f *.o testsuite
