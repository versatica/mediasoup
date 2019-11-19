#!/usr/bin/env python3
import glob
import subprocess
import os

reportdir = "reports/"
fuzzer = "./fuzzer_connect_multi_verbose"

class bcolors:
	HEADER = '\033[95m'
	OKBLUE = '\033[94m'
	OKGREEN = '\033[92m'
	WARNING = '\033[93m'
	FAIL = '\033[91m'
	ENDC = '\033[0m'
	BOLD = '\033[1m'
	UNDERLINE = '\033[4m'


print("Testing crashfiles")

FNULL = open(os.devnull, "w")
crashfiles = []
crashfiles.extend(glob.glob("crash-*"))
crashfiles.extend(glob.glob("timeout-*"))
crashfiles.extend(glob.glob("leak-*"))

if not os.path.exists(reportdir):
	os.makedirs(reportdir)

num_files = len(crashfiles)
filecounter = 1
for filename in crashfiles:
	filename_report = '{}{}{}'.format(reportdir, filename, '.report')
	reportfile = open(filename_report, "w")
	fuzzer_retval = subprocess.call([fuzzer, "-timeout=6", filename], stdout=reportfile, stderr=reportfile)
	if fuzzer_retval == 0:
		print(bcolors.FAIL, "[", filecounter, "/", num_files, "]", filename,"- not reproducable", bcolors.ENDC)
		reportfile.close()
		os.remove(filename_report)
	else:
		print(bcolors.OKGREEN, "[", filecounter, "/", num_files, "]", filename, "- reproducable", bcolors.ENDC)
		reportfile.write("\n>> HEXDUMP <<\n\n")
		reportfile.flush()
		subprocess.call(["hexdump", "-Cv", filename], stdout=reportfile)

	filecounter = filecounter + 1
