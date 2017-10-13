prog.info:

include ../../common.mak

prog.info: prog.gcda
	$(LCOV) -c -d . -o prog.info

prog.gcda: prog
	./prog || true

prog: prog.c
	$(CC) prog.c -o prog --coverage

clean:
	rm -f prog prog.gcda prog.gcno prog.info

.PHONY: all clean
