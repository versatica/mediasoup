#!/usr/bin/env perl

#   Copyright (c) MediaTek USA Inc., 2021-2023
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
# criteria
#
#   This script is used as a genhtml "--criteria-script criteria" callback.
#   It is called by genhtml at each level of hierarchy - but ignores all but
#   the top level, and looks only at line coverage.
#
#   Format of the JSON input is:
#     {"line":{"found":10,"hit:2,"UNC":2,..},"function":{...},"branch":{}"
#   Only non-zero elements are included.
#   See the 'criteria-script' section in "man genhtml" for details.
#
#   The coverage criteria implemented here is "UNC + LBC + UIC == 0"
#   If the criterial is violated, then this script emits a single line message
#   to stdout and returns a non-zero exit code.
#
#   If passed the "--suppress" flag, this script will exit with status 0,
#   even if the coverage criteria is not met.
#     genhtml --criteria-script 'path/criteria --signoff' ....
#
#   It is not hard to envision much more complicated coverage criteria.
package criteria;

use strict;
use JSON;
use Getopt::Long qw(GetOptionsFromArray);

our @ISA       = qw(Exporter);
our @EXPORT_OK = qw(new extract_version compare_version usage);

use constant {SIGNOFF => 0,};

sub new
{
    my $class   = shift;
    my $signoff = 0;
    my $script  = shift;

    if (!GetOptionsFromArray(\@_, ('signoff' => \$signoff))) {
        print(STDERR "usage: name type json-string [--signoff]\n");
        exit(1) if ($script eq $0);
        return undef;
    }

    my $self = [$signoff];
    return bless $self, $class;
}

sub check_criteria
{
    my ($self, $name, $type, $json) = @_;

    my $fail = 0;
    my @messages;
    if ($type eq 'top') {
        # for the moment - only worry about the top-level coverage
        my $db = decode_json($json);

        if (exists($db->{'line'})) {
            # our criteria is LBC + UNC + UIC == 0
            my $sep    = '';
            my $sum    = 0;
            my $msg    = '';
            my $counts = '';
            my $lines  = $db->{'line'};
            foreach my $tla ('UNC', 'LBC', 'UIC') {
                $msg    .= $sep . $tla;
                $counts .= $sep;
                if (exists $lines->{$tla}) {
                    my $count = $lines->{$tla};
                    $sum += $count;
                    $counts .= "$count";
                } else {
                    $counts .= "0";
                }
                $sep = ' + ';
            }
            $fail = $sum != 0;
            push(@messages, $msg . " != 0: " . $counts . "\n")
                if $fail;
        }
    }

    return ($fail && !$self->[SIGNOFF], \@messages);
}

1;
