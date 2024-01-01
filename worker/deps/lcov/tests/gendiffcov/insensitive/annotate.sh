#!/usr/bin/env perl

# A bit hacky - to test that case insensitivity is working properly
#  in all the scripts:
# look for a file matching the input argument name (case insensitive)
# Then nmunge the return data to further scramble the name

use strict;
use warnings;
use DateTime;
use POSIX qw(strftime);
use File::Spec;

sub get_modify_time($)
{
    my $filename = shift;
    my @stat     = stat $filename;
    my $tz       = strftime("%z", localtime($stat[9]));
    $tz =~ s/([0-9][0-9])$/:$1/;
    return strftime("%Y-%m-%dT%H:%M:%S", localtime($stat[9])) . $tz;
}

my $file = $ARGV[0];

my ($volume, $dir, $f) = File::Spec->splitpath($file);
$dir = File::Spec->canonpath($dir);
my @stack;
OUTER: while ($dir &&
              !-d $dir) {
    push(@stack, $f);
    ($volume, $dir, $f) = File::Spec->splitpath($dir);
    $dir = File::Spec->canonpath($dir);

    if (opendir(my $d, $dir)) {
        foreach my $name (readdir($d)) {
            if ($name =~ /$f/i) {
                push(@stack, $name);
                last OUTER;
            }
        }
    }
}
my $path = $dir;
-d $dir or die("$dir is not a directory");
while (1 < scalar(@stack)) {
    my $f = pop(@stack);
    opendir(my $d, $path) or die("cannot read dir $path");
    foreach my $name (readdir($d)) {
        if ($name =~ /$f/i) {
            $path = File::Spec->catdir($path, $name);
            last;
        }
    }
}

die("I can't handle that path munging: $file") unless (-d $path);
die("I don't seem to have a filename")         unless 1 >= scalar(@stack);
$f = pop(@stack)
    if @stack;    # remaining element should be the filename we are looking for

my $annotated = File::Spec->catfile($path, $f . ".annotated");
opendir my $d, $path or die("cannot read $path");
foreach my $name (readdir($d)) {
    if ($name =~ /$f\.annotated/i) {
        $annotated = File::Spec->catfile($path, $name);
    } elsif ($name =~ /$f/i) {    # case insensitive match
        $file = File::Spec->catfile($path, $name);
    }
}

my $now = DateTime->now();

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
