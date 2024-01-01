use strict;

while (<>) {
  if (/LF:(\d+)$/) {
    print("DA:71,1\nDA:74,0\nLF:", $1 + 2, "\n");
  } elsif (/LH:(\d+)$/) {
    print("LH:", $1 + 1, "\n");
  } else {
    print;
  }
}
