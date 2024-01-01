#!/usr/bin/env perl

#   Copyright (c) MediaTek USA Inc., 2020-2023
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or (at
#   your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, see
#   <http://www.gnu.org/licenses/>.
#
#
# p4annotate.pm
#
#   This script runs "p4 annotate" for the specified file and formats the result
#   to match the diffcov(1) 'annotate' callback specification:
#      use p4annotate;
#      my $calllback = p4annotate->new([--md5] [--log logifle] [--verify]);
#      $callback->annotate(filename);
#
#   This utility is implemented so that it can be loaded as a Perl module such
#   that the callback can be executed without incurring an additional process
#   overhead - which appears to be large and hightly variable in our compute
#   farm environment.
#
#   It can also be called directly, as
#       p4annotate [--md5] [--log logfild] [--verify] filename

package p4annotate;

use strict;
use File::Basename;
use File::Spec;
use Getopt::Long qw(GetOptionsFromArray);
use Fcntl qw(:flock);
use annotateutil qw(get_modify_time not_in_repo call_annotate);

our @ISA       = qw(Exporter);
our @EXPORT_OK = qw(new);

use constant {
              LOG     => 0,
              VERIFY  => 1,
              LOGFILE => 2,
              SCRIPT  => 3,
};

sub printlog
{
    my ($self, $msg) = @_;
    my $fh = $self->[LOG];
    return unless $fh;
    flock($fh, LOCK_EX) or die('cannot lock ' . $self->[LOGFILE] . ": $!");
    print($fh $msg);
    flock($fh, LOCK_UN) or die('cannot unlock ' . $self->[LOGFILE] . ": $!");
}

sub new
{
    my $class  = shift;
    my @args   = @_;
    my $script = shift;    # this should be 'me'
                           #other arguments are as passed...
    my $logfile;
    my $verify = 0;    # if set, check that we merged local changes correctly
    my $exe    = basename($script ? $script : $0);

    if (exists($ENV{LOG_P4ANNOTATE})) {
        $logfile = $ENV{LOG_P4ANNOTATE};
    }
    my $help;
    if (!GetOptionsFromArray(\@_,
                             ("verify" => \$verify,
                              "log=s"  => \$logfile,
                              'help'   => \$help)) ||
        $help
    ) {
        print(STDERR "usage: $exe [--log logfile] [--verify] filename\n");
        exit($help ? 0 : 1) if $script eq $0;
        return 1;
    }

    my @notset;
    foreach my $var ("P4USER", "P4PORT", "P4CLIENT") {
        push(@notset, $var) unless exists($ENV{$var});
    }
    if (@notset) {
        die("$exe requires environment variable" .
            (1 < scalar(@notset) ? 's' : '') . ' ' .
            join(' ', @notset) . " to be set.");
    }

    my $self = [$logfile, $verify];
    $self->[SCRIPT] = $exe;
    bless $self, $class;
    if ($logfile) {
        open(LOGFILE, ">>", $logfile) or
            die("unable to open $logfile");

        $self->[LOG]     = \*LOGFILE;
        $self->[LOGFILE] = $logfile;
        $self->printlog("$exe " . join(" ", @args) . "\n");
    }
    return $self;
}

sub annotate
{
    my ($self, $pathname) = @_;
    defined($pathname) or die("expected filename");

    if (-e $pathname && -l $pathname) {
        $pathname = File::Spec->catfile(File::Basename::dirname($pathname),
                                        readlink($pathname));
        my @c;
        foreach my $component (split(m@/@, $pathname)) {
            next unless length($component);
            if ($component eq ".")  { next; }
            if ($component eq "..") { pop @c; next }
            push @c, $component;
        }
        $pathname = File::Spec->catfile(@c);
    }

    my $null = File::Spec->devnull();    # more portable
    my @lines;
    my $status;
    if (0 == system(
               "p4 files $pathname 2>$null|grep -qv -- '- no such file' >$null")
    ) {
        # this file is in p4..
        my $version;
        my $have = `p4 have $pathname`;
        if ($have =~ /#([0-9]+) - /) {
            $version = "#$1";
        } else {
            $version = '@head';
        }
        $self->printlog("  have $pathname:$version\n");

        my @annotated;
        # check if this file is open in the current sandbox...
        #  redirect stderr because p4 print "$path not opened on this client" if file not opened
        my $opened = `p4 opened $pathname 2>$null`;
        my %localAdd;
        my %localDelete;
        my ($localChangeList, $owner, $now);
        if ($opened =~ /edit (default change|change (\S+)) /) {
            $localChangeList = $2 ? $2 : 'default';

            $self->printlog("  local edit in CL $localChangeList\n");

            $owner = $ENV{P4USER};    # current user is responsible for changes
            $now   = get_modify_time($pathname)
                ;    # assume changes happened when file was liast modified

            # what is different in the local file vs the one we started with
            if (open(PIPE, "-|", "p4 diff $pathname")) {
                my $line = <PIPE>;    # eat first line
                die("unexpected content '$line'")
                    unless $line =~ m/^==== /;
                my ($action, $fromStart, $fromEnd, $toStart, $toEnd);
                while ($line = <PIPE>) {
                    chomp $line;
                    # Also remove CR from line-end
                    s/\015$//;
                    if ($line =~
                        m/^([0-9]+)(,([0-9]+))?([acd])([0-9]+)(,([0-9]+))?/) {
                        # change
                        $action    = $4;
                        $fromStart = $1;
                        $fromEnd   = $3 ? $3 : $1;
                        $toStart   = $5;
                        $toEnd     = $7 ? $7 : $5;
                    } elsif ($line =~ m/^> (.*)$/) {
                        $localAdd{$toStart++} = $1;
                    } elsif ($line =~ m/^< (.*)$/) {
                        $localDelete{$fromStart++} = $1;
                    } else {
                        die("unexpected line '$line'")
                            unless $line =~ m/^---$/;
                    }
                }
                close(PIPE) or die("unable to close p4 diff pipe: $!\n");
                if (0 != $?) {
                    $? & 0x7F &
                        die("p4 pipe died from signal ", ($? & 0x7F), "\n");
                    die("p4 pipe exited with error ", ($? >> 8), "\n");
                }
            } else {
                die("unable to open pipe to p4 diff $pathname");
            }
        }
        # -i: follow history across branches
        # -I: follow history across integrations
        #     (seem to be able to use -i or -I - but not both, together)
        # -u: print user name
        # -c: print changelist rather than file version ID
        # -q: quiet - suppress the 1-line header for each line
        my $annotateLineNo = 1;
        my $emitLineNo     = 1;
        if (open(HANDLE, "-|", "p4 annotate -Iucq $pathname$version")) {
            while (my $line = <HANDLE>) {

                if (exists $localDelete{$annotateLineNo++}) {
                    next;    # line was deleted .. skip it
                }
                while (exists $localAdd{$emitLineNo}) {
                    my $l = $localAdd{$emitLineNo};
                    push(@lines, [$l, $owner, undef, $now, $localChangeList]);
                    push(@annotated, $l) if ($self->[VERIFY]);
                    delete $localAdd{$emitLineNo};
                    ++$emitLineNo;
                }

                chomp $line;
                # Also remove CR from line-end
                s/\015$//;

                if ($line =~ m/([0-9]+):\s+(\S+)\s+([0-9\/]+)\s(.*)/) {
                    my $changelist = $1;
                    my $owner      = $2;
                    my $when       = $3;
                    my $text       = $4;
                    $owner =~ s/^.*<//;
                    $owner =~ s/>.*$//;
                    $when  =~ s:/:-:g;
                    $when  =~ s/$/T00:00:00-05:00/;
                    push(@lines, [$text, $owner, undef, $when, $changelist]);
                    push(@annotated, $text) if ($self->[VERIFY]);
                } else {
                    push(@lines, [$line, 'NONE', undef, 'NONE', 'NONE']);
                    push(@annotated, $line) if ($self->[VERIFY]);
                }
                ++$emitLineNo;
            }    # while (HANDLE)

            # now handle lines added at end of file
            die("lost track of lines")
                unless (0 == scalar(%localAdd) ||
                        exists($localAdd{$emitLineNo}));

            while (exists $localAdd{$emitLineNo}) {
                my $l = $localAdd{$emitLineNo};
                push(@lines, [$l, $owner, undef, $now, $localChangeList]);
                delete $localAdd{$emitLineNo};
                push(@annotated, $l) if ($self->[VERIFY]);
                ++$emitLineNo;
                die("lost track of lines")
                    unless (0 == scalar(%localAdd) ||
                            exists($localAdd{$emitLineNo}));
            }
            if ($self->[VERIFY]) {
                if (open(DEBUG, "<", $pathname)) {
                    my $lineNo = 0;
                    while (my $line = <DEBUG>) {
                        chomp($line);
                        my $a = $annotated[$lineNo];
                        die("mismatched annotation at $pathname:$lineNo: '$line' -> '$a'"
                        ) unless $line eq $a;
                        ++$lineNo;
                    }
                }
            }
            close(HANDLE) or die("unable to close p4 annotate pipe: $!\n");
            $status = $?;
        }
    } else {
        $self->printlog("  $pathname not in P4\n");
        not_in_repo($pathname, \@lines);
    }
    return ($status, \@lines);
}

unless (caller) {
    call_annotate("p4annotate", @ARGV);
}

1;

