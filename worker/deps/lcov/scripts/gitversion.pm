#!/usr/bin/env perl
#   Copyright (c) MediaTek USA Inc., 2022-2023
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
# gitversion [--p4] [--md5] [--local-change] [--prefix path] pathname OR
# gitversion [--p4] [--md5] [--prefix path] --compare old_version new_version pathname
#
#   If the '--p4' flag is used:
#     we assume that the GIT repo is cloned from Perforce - and look for
#     the line in the generated commit log message which tells us the perforce
#     changelist ID that we actually want.
#   if the '--local-change' flag is used:
#     we assume that there may be local changes which are not committed to
#     the repo.  If flag is not set:  do not check for local change
#   If specified, 'path' is prependied to 'pathname' (as 'path/pathname')
#     before processing.

#   This is a sample script which uses git commands to determine
#   the version of the filename parameter.
#   Version information (if present) is used during ".info" file merging
#   to verify that the data the user is attempting to merge is for the same
#   source code/same version.
#   If the version is not the same - then line numbers, etc. may be different
#   and some very strange errors may occur.

package gitversion;

use strict;
use POSIX qw(strftime);
use File::Spec;
use Cwd qw(abs_path);
use File::Basename qw(dirname basename);
use Getopt::Long qw(GetOptionsFromArray);

use annotateutil qw(get_modify_time not_in_repo compute_md5);

our @ISA       = qw(Exporter);
our @EXPORT_OK = qw(new extract_version compare_version usage);

use constant {
              MD5                => 0,
              P4                 => 1,
              PREFIX             => 2,
              ALLOW_MISSING      => 3,
              CHECK_LOCAL_CHANGE => 4,
};

sub usage
{
    my $exe = shift;
    $exe = basename($exe);
    print(STDERR<<EOF);
usage: $exe --compare old_version new_version filename OR
       $exe [--md5] [--p4] [--prefix path] [--local-change] \
            [--allow-missing] filename
EOF
}

sub new
{
    my $class  = shift;
    my $script = shift;
    my @args   = @_;
    my $compare;
    my $use_md5;    # if set, append md5 checksum to the P4 version string
    my $mapp4;
    my $prefix;
    my $allow_missing;
    my $help;
    my $local_change;

    if (!GetOptionsFromArray(\@_,
                             "--compare"       => \$compare,
                             "--md5"           => \$use_md5,
                             "--p4"            => \$mapp4,
                             '--prefix:s'      => \$prefix,
                             '--allow-missing' => \$allow_missing,
                             '--local-change'  => \$local_change,
                             '--help'          => \$help) ||
        $help ||
        $compare && scalar(@ARGV) != 3
    ) {
        usage($script);
        exit(defined($help) ? 0 : 1) if ($script eq $0);
        return undef;
    }

    my $self = [$use_md5, $mapp4, $prefix, $allow_missing, $local_change];
    return bless $self, $class;
}

sub extract_version
{
    my ($self, $filename) = @_;

    if (!File::Spec->file_name_is_absolute($filename) &&
        defined($self->[PREFIX])) {
        $filename = File::Spec->catfile($self->[PREFIX], $filename);
    }

    unless (-e $filename) {
        if ($self->[ALLOW_MISSING]) {
            return '';    # empty string
        }
        die("Error: $filename does not exist - perhaps you need the '--allow-missing' flag"
        );
    }
    my $pathname = abs_path($filename);
    my $null     = File::Spec->devnull();

    my $dir  = dirname($pathname);
    my $file = basename($pathname);
    -d $dir or die("no such directory '$dir'");

    my $version;
    if (0 == system("cd $dir ; git rev-parse --show-toplevel >$null 2>&1")) {
        # in a git repo - use full SHA.
        my $log = `cd $dir ; git log --no-abbrev --oneline -1 $file 2>$null`;
        if (0 == $? &&
            $log =~ /^(\S+) /) {
            $version = $1;
            if ($self->[P4]) {
                if (open(GITLOG, '-|', "cd $dir ; git show -s $version")) {
                    while (my $l = <GITLOG>) {
                        # p4sync puts special comment in commit log.
                        #  pull the CL out of that.
                        if ($l =~ /git-p4:.+change = ([0-9]+)/) {
                            $version = "CL $1";
                            last;
                        }
                    }
                } else {
                    die("unable to execute 'git show -s $version': $!");
                }
                close(GITLOG) or die("unable to close");
                if (0 != $?) {
                    $? & 0x7F &
                        die("git show died from signal ", ($? & 0x7F), "\n");
                    die("git show exited with error ", ($? >> 8), "\n");
                }

            } else {
                $version = "SHA $version";
            }
            if ($self->[CHECK_LOCAL_CHANGE]) {
                my $diff = `cd $dir ; git diff $file 2>$null`;
                if ('' ne $diff) {
                    $version .= ' edited ' . get_modify_time($file);
                    $version .= ' md5:' . compute_md5($pathname)
                        if $self->[MD5];
                }
            }
        }
    }
    if (!$version) {
        # not in P4 - just print the modify time, so we have a prayer of
        #  noticing file differences
        $version = get_modify_time($pathname);
        $version .= ' md5:' . compute_md5($pathname)
            if ($self->[MD5]);
    }
    return $version;
}

sub compare_version
{
    my ($self, $new, $old, $filename) = @_;

    if ($self->[MD5] &&
        (   $old !~ /^SHA/ ||
            ($self->[P4] &&
                $old !~ /^CL/)) &&
        $old =~ / md5:(.+)$/
    ) {
        my $o = $1;
        if ($new =~ / md5:(.+)$/) {
            return ($o ne $1);
        }
        # otherwise:  'new' was not an MD5 signature - so fall through to exact match
    }
    return $old ne $new;    # just look for exact match
}

1;
