CFLAGS = -O2 -std=c++11 -Wall
LDFLAGS= -lm

.PHONY: test clean

all: testsuite

testsuite: testsuite.cpp netstring.o

test: testsuite
	./testsuite

clean:
	rm -f *.o testsuite
