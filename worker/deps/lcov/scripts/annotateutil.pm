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
# annotateutil.pm:  some common utilities used by sample 'annotate' scripts
#
package annotateutil;

use strict;
use POSIX qw(strftime);

our @ISA       = qw(Exporter);
our @EXPORT_OK = qw(get_modify_time compute_md5 not_in_repo
                    call_annotate call_get_version);

sub get_modify_time($)
{
    my $filename = shift;
    my @stat     = stat $filename;
    my $tz       = strftime("%z", localtime($stat[9]));
    $tz =~ s/([0-9][0-9])$/:\1/;
    return strftime("%Y-%m-%dT%H:%M:%S", localtime($stat[9])) . $tz;
}

sub not_in_repo
{
    my ($pathname, $lines) = @_;
    my $context = '';
    eval { $context = MessageContext::context(); };
    my $mtime = get_modify_time($pathname);   # when was the file last modified?
        # who does the filesystem think owns it?
    my $owner = getpwuid((stat($pathname))[4]);

    open(HANDLE, $pathname) or die("unable to open '$pathname'$context: $!");
    while (my $line = <HANDLE>) {
        chomp $line;
        # Also remove CR from line-end
        s/\015$//;

        push(@$lines, [$line, $owner, undef, $mtime, "NONE"]);
    }
    close(HANDLE) or die("unable to close '$pathname'$context");
}

sub compute_md5
{
    my $filename = shift;
    die("$filename not found") unless -e $filename;
    my $null = File::Spec->devnull();
    my $md5  = `md5sum $filename 2>$null`;
    $md5 =~ /^(\S+)/;
    return $1;
}

sub call_annotate
{
    my $cb = shift;
    my $class;
    my $filename = pop;
    eval { $class = $cb->new(@_); };
    die("$cb construction error: $@") if $@;
    my ($status, $list) = $class->annotate($filename);
    foreach my $line (@$list) {
        my ($text, $abbrev, $full, $when, $cl) = @$line;
        print("$cl|$abbrev", $full ? ";$full" : '', "|$when|$text\n");
    }
    exit $status;
}

sub call_get_version
{
    my $cb = shift;
    my $class;
    my $filename = pop;
    eval { $class = $cb->new(@_); };
    die("$cb construction error: $@") if $@;
    my $v = $class->extract_version($filename);
    print($v, "\n");
    exit 0;
}

1;
