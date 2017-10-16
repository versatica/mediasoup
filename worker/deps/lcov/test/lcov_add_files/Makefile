include ../common.mak

ADDTEST := ./add_test

TESTS := lcov_add_zero lcov_add_zero2 lcov_add_full lcov_add_full2 \
	 lcov_add_part lcov_add_part2 lcov_add_concatenated4


test: $(TESTS)

lcov_add_zero:
	# Add single zero coverage file - output should be same as input
	test_run lcov_add_zero $(ADDTEST) 1 "$(ZEROINFO)" "$(ZEROINFO)"

lcov_add_zero2:
	# Add two zero coverage files - output should be same as input
	test_run lcov_add_zero2 $(ADDTEST) 1 "$(ZEROINFO)" "$(ZEROINFO)" "$(ZEROINFO)"

lcov_add_full:
	# Add single 100% coverage file - output should be same as input
	test_run lcov_add_full $(ADDTEST) 1 "$(FULLINFO)" "$(FULLINFO)"

lcov_add_full2:
	# Add two 100% coverage file and reduce counts to 1/2 - output should
	# be same as input
	test_run lcov_add_full2 $(ADDTEST) 0.5 "$(FULLINFO)" "$(FULLINFO)" "$(FULLINFO)"

lcov_add_part:
	# Add single coverage file with random coverage rate - output should
	# be same as input
	test_run lcov_add_part $(ADDTEST) 1 "$(PART1INFO)" "$(PART1INFO)"

lcov_add_part2:
	# Add two coverage files that were split from target file - output
	# should be same as target file
	test_run lcov_add_part2 $(ADDTEST) 1 "$(TARGETINFO)" "$(PART1INFO)" "$(PART2INFO)"

lcov_add_concatenated4:
	# Add coverage file that consists of 4 concatenation of target files 
	# and reduce counts to 1/4 - output should be the same as input
	cat $(TARGETINFO) $(TARGETINFO) $(TARGETINFO) $(TARGETINFO) >concatenated.info
	test_run lcov_add_concatenated4 $(ADDTEST) 0.25 $(TARGETINFO) concatenated.info

clean:
	rm -f *.info

.PHONY: test $(TESTS) clean
