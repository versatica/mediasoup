#!/usr/bin/env perl
#
# Usage: fix.pl [FILE TYPE] [OPTIONS] FILE(s)
#
# Apply file-type specific fixups to the specified list of FILES. If no
# file type is specified, the type is automatically determined from file
# contents and name.
#
# FILE TYPE
#   --manpage           Specified files are man pages
#   --exec              Specified files are executable tools
#   --text              Specified files are text files
#   --spec              Specified files are RPM spec files
#
# OPTIONS
#   --help              Print this text, then exit
#   --verfile FILENAME  Write version information to FILENAME
#   --version VERSION   Use VERSION as version
#   --release RELEASE   Use RELEASE as release
#   --libdir PATH       Use PATH as library path
#   --bindir PATH       Use PATH as executable path
#   --scriptdir PATH    Use PATH as script path
#   --fixinterp         Replace /usr/bin/env interpreter references with values
#                       specified by LCOV_PERL_PATH and LCOV_PYTHON_PATH
#   --fixdate           Replace dates references with the value specified by
#                       SOURCE_DATA_EPOCH or the latest file modification time
#   --fixver            Replace version references with values specified by
#                       --version and --release
#   --fixlibdir         Replace library path references with value specified
#                       by --libdir
#   --fixbindir         Replace executable path references with value specified
#                       by --bindir
#   --fixscriptdir      Replace script path references with value specified by
#                       --scriptdir

use strict;
use warnings;

use Getopt::Long;

my ($opt_man, $opt_exec, $opt_text, $opt_spec,
    $opt_verfile, $opt_version, $opt_release, $opt_libdir,
    $opt_bindir, $opt_scriptdir, $opt_fixinterp, $opt_fixdate,
    $opt_fixver, $opt_fixlibdir, $opt_fixbindir, $opt_fixscriptdir);
my $verbose = $ENV{"V"};

sub get_file_info($)
{
    my ($filename) = @_;
    my ($sec, $min, $hour, $year, $month, $day);
    my @stat;

    die("Error: cannot stat $filename: $!\n") if (!-e $filename);

    @stat = stat($filename);
    my $epoch = int($ENV{SOURCE_DATE_EPOCH} || $stat[9]);
    $epoch = $stat[9] if $stat[9] < $epoch;
    ($sec, $min, $hour, $day, $month, $year) = gmtime($epoch);
    $year  += 1900;
    $month += 1;

    return (sprintf("%04d-%02d-%02d", $year, $month, $day),
            $epoch, sprintf("%o", $stat[2] & 07777));
}

sub update_man_page($$)
{
    my ($source, $date_string) = @_;

    if ($opt_fixver) {
        die("$0: Missing option --version\n") if (!defined($opt_version));

        $source =~ s/\"LCOV\s+\d+\.\d+\"/\"LCOV $opt_version\"/mg;
    }

    if ($opt_fixdate) {
        $date_string =~ s/-/\\-/g;
        $source      =~ s/\d\d\d\d\\\-\d\d\\\-\d\d/$date_string/mg;
    }

    if ($opt_fixscriptdir) {
        die("$0: Missing option --scriptdir\n") if (!defined($opt_scriptdir));
        $source =~ s/^(.ds\s+scriptdir\s).*$/$1$opt_scriptdir/mg;
    }

    return $source;
}

sub update_perl($)
{
    my ($source) = @_;
    my $path = $ENV{"LCOV_PERL_PATH"};

    if ($opt_fixver) {
        die("$0: Missing option --version\n") if (!defined($opt_version));
        die("$0: Missing option --release\n") if (!defined($opt_release));

        $source =~
            s/^(our\s+\$lcov_version\s*=).*$/$1 "LCOV version $opt_version-$opt_release";/mg;
    }

    if ($opt_fixinterp && defined($path) && $path ne "") {
        $source =~ s/^#!.*perl.*\n/#!$path\n/;
    }

    if ($opt_fixlibdir) {
        die("$0: Missing option --libdir\n") if (!defined($opt_libdir));

        $source =~ s/^use FindBin;\n//mg;
        $source =~ s/"\$FindBin::RealBin[\/.]+lib"/"$opt_libdir"/mg;
    }

    if ($opt_fixbindir) {
        die("$0: Missing option --bindir\n") if (!defined($opt_bindir));

        $source =~ s/^use FindBin;\n//mg;
        $source =~ s/"\$FindBin::RealBin"/"$opt_bindir"/mg;
    }

    if ($opt_fixscriptdir) {
        die("$0: Missing option --scriptdir\n") if (!defined($opt_scriptdir));

        $source =~ s/^use FindBin;\n//mg;
        $source =~ s/"\$FindBin::RealBin"/"$opt_scriptdir"/mg;
    }

    return $source;
}

sub update_python($)
{
    my ($source) = @_;
    my $path = $ENV{"LCOV_PYTHON_PATH"};

    if ($opt_fixinterp && defined($path) && $path ne "") {
        $source =~ s/^#!.*python.*\n/#!$path\n/;
    }

    return $source;
}

sub update_txt_file($$)
{
    my ($source, $date_string) = @_;

    if ($opt_fixdate) {
        $source =~ s/(Last\s+changes:\s+)\d\d\d\d-\d\d-\d\d/$1$date_string/mg;
    }

    return $source;
}

sub update_spec_file($)
{
    my ($source) = @_;

    if ($opt_fixver) {
        die("$0: Missing option --version\n") if (!defined($opt_version));
        die("$0: Missing option --release\n") if (!defined($opt_release));

        $source =~ s/^(Version:\s*)\d+\.\d+.*$/$1$opt_version/mg;
        $source =~ s/^(Release:\s*).*$/$1$opt_release/mg;
    }

    return $source;
}

sub write_version_file($$$)
{
    my ($filename, $version, $release) = @_;
    my $fd;

    die("$0: Missing option --version\n") if (!defined($version));
    die("$0: Missing option --release\n") if (!defined($release));

    open($fd, ">", $filename) or die("Error: cannot write $filename: $!\n");
    print($fd "VERSION=$version\n");
    print($fd "RELEASE=$release\n");
    close($fd);
}

sub guess_filetype($$)
{
    my ($filename, $data) = @_;

    return "exec"
        if ($data =~ /^#!/ ||
            $filename =~ /\.pl$/ ||
            $filename =~ /\.pm$/);
    return "manpage" if ($data =~ /^\.TH/m || $filename =~ /\.\d$/);
    return "spec" if ($data =~ /^%install/m || $filename =~ /\.spec$/);
    return "text" if ($data =~ /^[-=]+/m);

    return "";
}

sub usage()
{
    my ($fd, $do);

    open($fd, "<", $0) || return;
    while (my $line = <$fd>) {
        if (!$do && $line =~ /Usage/) {
            $do = 1;
        }
        if ($do) {
            last if ($line !~ s/^# ?//);
            print($line);
        }
    }
    close($fd);
}

sub main()
{
    my $opt_help;

    if (!GetOptions("help"         => \$opt_help,
                    "manpage"      => \$opt_man,
                    "exec"         => \$opt_exec,
                    "text"         => \$opt_text,
                    "spec"         => \$opt_spec,
                    "verfile=s"    => \$opt_verfile,
                    "version=s"    => \$opt_version,
                    "release=s"    => \$opt_release,
                    "libdir=s"     => \$opt_libdir,
                    "bindir=s"     => \$opt_bindir,
                    "scriptdir=s"  => \$opt_scriptdir,
                    "fixinterp"    => \$opt_fixinterp,
                    "fixdate"      => \$opt_fixdate,
                    "fixver"       => \$opt_fixver,
                    "fixlibdir"    => \$opt_fixlibdir,
                    "fixbindir"    => \$opt_fixbindir,
                    "fixscriptdir" => \$opt_fixscriptdir,
    )) {
        print(STDERR "Use $0 --help to get usage information.\n");
        exit(1);
    }

    if ($opt_help) {
        usage();
        exit(0);
    }

    if (defined($opt_verfile)) {
        print("Updating version file $opt_verfile\n") if ($verbose);
        write_version_file($opt_verfile, $opt_version, $opt_release);
    }

    for my $filename (@ARGV) {
        my ($fd, $source, $original, $guess);
        my @date = get_file_info($filename);

        next if (-d $filename);

        open($fd, "<", $filename) or die("Error: cannot open $filename\n");
        local ($/);
        $source = $original = <$fd>;
        close($fd);

        if (!$opt_man && !$opt_exec && !$opt_text && !$opt_spec) {
            $guess = guess_filetype($filename, $source);
        } else {
            $guess = "";
        }

        if ($opt_man || $guess eq "manpage") {
            print("Updating man page $filename\n") if ($verbose);
            $source = update_man_page($source, $date[0]);
        }
        if ($opt_exec || $guess eq "exec") {
            print("Updating bin tool $filename\n") if ($verbose);
            if ($filename =~ /\.pm$/ || $source =~ /^[^\n]*perl[^\n]*\n/) {
                $source = update_perl($source);
            } elsif ($filename =~ /\.py$/ ||
                     $source =~ /^[^\n]*python[^\n]*\n/) {
                $source = update_python($source);
            }
        }
        if ($opt_text || $guess eq "text") {
            print("Updating text file $filename\n") if ($verbose);
            $source = update_txt_file($source, $date[0]);
        }
        if ($opt_spec || $guess eq "spec") {
            print("Updating spec file $filename\n") if ($verbose);
            $source = update_spec_file($source);
        }

        if ($source ne $original) {
            open($fd, ">", "$filename.new") ||
                die("Error: cannot create $filename.new\n");
            print($fd $source);
            close($fd);

            chmod(oct($date[2]), "$filename.new") or
                die("Error: chmod failed for $filename\n");
            unlink($filename) or
                die("Error: cannot remove $filename\n");
            rename("$filename.new", "$filename") or
                die("Error: cannot rename $filename.new to $filename\n");
        }

        utime($date[1], $date[1], $filename) or
            warn("Warning: cannot update modification time for $filename\n");
    }
}

main();

exit(0);
