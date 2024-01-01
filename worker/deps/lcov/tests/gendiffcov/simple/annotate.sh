#!/bin/env perl

use strict;
use warnings;
use DateTime;
use POSIX qw(strftime);

sub get_modify_time($)
{
    my $filename = shift;
    my @stat     = stat $filename;
    my $tz       = strftime("%z", localtime($stat[9]));
    $tz =~ s/([0-9][0-9])$/:$1/;
    return strftime("%Y-%m-%dT%H:%M:%S", localtime($stat[9])) . $tz;
}

my $file      = $ARGV[0];
my $annotated = $file . ".annotated";
my $now       = DateTime->now();

if (open(HANDLE, '<', $annotated)) {
    while (my $line = <HANDLE>) {
        chomp $line;
        $line =~ s/\r//g;    # remove CR from line-end
        my ($commit, $who, $days, $text) = split(/\|/, $line, 4);
        my $duration = DateTime::Duration->new(days => $days);
        my $date     = $now - $duration;

        printf("%s|%s|%s|%s\n", $commit, $who, $date, $text);
    }
    close(HANDLE) or die("unable to close $annotated: $!");
} elsif (open(HANDLE, $file)) {
    my $mtime = get_modify_time($file);    # when was the file last modified?
    my $owner =
        getpwuid((stat($file))[4]);    # who does the filesystem think owns it?
    while (my $line = <HANDLE>) {
        chomp $line;
        # Also remove CR from line-end
        $line =~ s/\015$//;
        printf("%s|%s|%s|%s\n", "NONE", $owner, $mtime, $line);
    }
    close(HANDLE) or die("unable to close $annotated: $!");
} else {
    die("unable to open $file: $!");
}
