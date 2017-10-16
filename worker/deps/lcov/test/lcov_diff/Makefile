include ../common.mak

test:
	test_run lcov_diff_apply ./diff_test

clean:
	make -C old clean
	make -C new clean
	rm -f *.info diff
