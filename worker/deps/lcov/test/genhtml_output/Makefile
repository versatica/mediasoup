include ../common.mak

GENHTML_TEST := ./genhtml_test

TESTS := genhtml_output_zero genhtml_output_full genhtml_output_target \
	 genhtml_output_part1 genhtml_output_part2 genhtml_output_combined

test: $(TESTS)

genhtml_output_zero:
	@test_run genhtml_output_zero $(GENHTML) $(ZEROINFO) -o out_zero/

genhtml_output_full:
	@test_run genhtml_output_full $(GENHTML) $(FULLINFO) -o out_full/

genhtml_output_target:
	@test_run genhtml_output_target $(GENHTML) $(TARGETINFO) -o out_target/

genhtml_output_part1:
	@test_run genhtml_output_part1 $(GENHTML) $(PART1INFO) -o out_part1/

genhtml_output_part2:
	@test_run genhtml_output_part2 $(GENHTML) $(PART2INFO) -o out_part2/

genhtml_output_combined: genhtml_output_target
	@test_run genhtml_output_combined $(GENHTML_TEST) $(TARGETINFO) $(PART1INFO) $(PART2INFO)

clean:
	rm -rf out_*/

.PHONY: test $(TESTS) clean
