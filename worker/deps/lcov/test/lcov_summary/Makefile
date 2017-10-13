include ../common.mak

CHECK := ./check_counts
TESTS := lcov_summary_zero lcov_summary_full lcov_summary_target \
	 lcov_summary_part1 lcov_summary_part2 lcov_summary_concatenated \
	 lcov_summary_concatenated2

test: $(TESTS)

lcov_summary_zero:
	# Compare output of lcov --summary with generated counts
	test_run lcov_summary_zero $(CHECK) $(ZEROCOUNTS) $(ZEROINFO)
lcov_summary_full:
	# Compare output of lcov --summary with generated counts
	test_run lcov_summary_full $(CHECK) $(FULLCOUNTS) $(FULLINFO)
lcov_summary_target:
	# Compare output of lcov --summary with generated counts
	test_run lcov_summary_target $(CHECK) $(TARGETCOUNTS) $(TARGETINFO)
lcov_summary_part1:
	# Compare output of lcov --summary with generated counts
	test_run lcov_summary_part1 $(CHECK) $(PART1COUNTS) $(PART1INFO)
lcov_summary_part2:
	# Compare output of lcov --summary with generated counts
	test_run lcov_summary_part2 $(CHECK) $(PART2COUNTS) $(PART2INFO)
lcov_summary_concatenated:
	# Compare output of lcov --summary with generated counts for a
	# concatenated coverage data file
	cat $(TARGETINFO) $(TARGETINFO) > concatenated.info
	test_run lcov_summary_concatenated $(CHECK) $(TARGETCOUNTS) concatenated.info
lcov_summary_concatenated2:
	# Compare output of lcov --summary with generated counts for a
	# concatenated coverage data file (part1+part2=target)
	cat $(PART1INFO) $(PART2INFO) > concatenated2.info
	test_run lcov_summary_concatenated2 $(CHECK) $(TARGETCOUNTS) concatenated2.info



clean:
	rm -f *.info

.PHONY: test $(TESTS) clean
