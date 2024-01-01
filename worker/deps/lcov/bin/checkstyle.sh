#!/bin/bash
#
# checkstyle.sh FILENAME(S)
#
# Check specified files for coding style issues. The mode of checking is
# selected using the MODE environment variable:
#
# MODE:
#   diff    Report only issues on code changed since git HEAD (default)
#   full    Report all issues
#
# Note: Currently checking is restricted to Perl files

# Return absolute path
function realpath() {
	local path="$1"

	if [[ -d "$path" ]] ; then
		echo "$(cd "$path" && pwd)"
	else
		echo "$(cd "$(dirname "$path")" && pwd)/$(basename "$1")"
	fi
}

# Return path relative to TOOLDIR
function relpath() {
	local inpath="$1" outpath

	outpath="${1##$TOOLDIR/}"

	echo "$outpath"

	# Let caller know if path was inside TOOLDIR
	[[ "$inpath" != "$outpath" ]]
}

# Terminate with an error message
function die() {
	echo "Error: $*" >&2
	exit 1
}

# List source lines that violate the style guide rules. If an original file
# is supplied, only report new findings.
function report() {
	local file="$1" tidy="$2" origfile="${3:-}" origtidy="${4:-}"
	local diffopts mark origmark newmark lineno continuation newline
	local tag line

	# Mark offending lines. An offending line is a line in the original
	# file that was removed and replaced with a fixed line by the tidy
	# script.
	diffopts=(
		'--old-line-format'		'-%L'	# Mark changed lines
		'--new-line-format'		''	# Remove fixed lines
		'--unchanged-line-format'	' %L'	# Leave other lines
		'--minimal'
	)

	mark="$TEMPDIR/${file##*/}.marked"
	diff "${diffopts[@]}" "$file" "$tidy" >"$mark"

	origmark="$TEMPDIR/${file##*/}.origmarked"
	if [[ -n "$origfile" ]] ; then
		# Use original version as baseline
		diff "${diffopts[@]}" "$origfile" "$origtidy" >"$origmark"
	else
		# Use full file as baseline
		sed -e 's/^/ /g' "$file" >"$origmark"
	fi

	# Mark lines that are added compared to baseline
	diffopts=(
		'--old-line-format'		''	# Remove old lines
		'--new-line-format'		'+%L'	# Mark changed lines
		'--unchanged-line-format'	' %L'	# Leave other lines
		'--minimal'
	)

	newmark="$TEMPDIR/${file##*/}.newmarked"
	diff "${diffopts[@]}" "$origmark" "$mark" >"$newmark"

	if ! grep -q '^+-' "$newmark" ; then
		echo "$file has no obvious style issues"
		return 0
	fi

	if [[ -z "$origfile" ]] ; then
		echo "Listing all findings in $file:"
	else
		echo "Listing findings in $file since git ref $GITBASE:"
	fi
	echo

	# Display groups of newly introduced offending lines
	lineno=1
	continuation=0
	newline=0
	while read -r line ; do
		tag=${line:0:2}
		line=${line:2}
		if [[ "$tag" == "+-" ]] ; then
			# + means: introduced after baseline
			# - means: an offending line
			if [[ "$continuation" -eq 0 ]] ; then
				if [[ "$newline" -eq 1 ]] ; then
					echo
					newline=0
				fi
				echo "In $file line $lineno:"
				continuation=1
			fi
			echo "+${line//$'\t'/^I}"
		else
			if [[ "$continuation" -eq 1 ]] ; then
				newline=1
				continuation=0
			fi
		fi
		(( lineno++ ))
	done < "$newmark"

	echo
	echo "$file has style issues, please review (see also $tidy)." >&2

	return 1
}

TOOLDIR="$(realpath "$(dirname "$0")"/..)"

PERLTIDY="${PERLTIDY:-perltidy}"
PERLTIDYRC="${PERLTIDYRC:-$TOOLDIR/.perltidyrc}"

# HEAD is default baseline
GITBASE=${GITBASE:-HEAD}

# diff is default mode
MODE=${MODE:-diff}
#MODE=${MODE@L}

case "$MODE" in
"diff")	[[ ! -d "$TOOLDIR/.git" ]] && die "Not in a git repository" ;;
"full")	;;
*)	die "Unknown checking mode '$MODE'" ;;
esac

TEMPDIR=$(mktemp -d) || die "Could not create temporary directory"
trap "rm -rf '$TEMPDIR'" exit

RC=0
for FILE in "${@}" ; do
	BASE=${FILE##*/}
	TIDY="$FILE.tdy"

	if [[ "$MODE" == "diff" ]] ; then
		GITFILE="$TEMPDIR/$BASE.git"
		GITTIDY="$TEMPDIR/$BASE.git.tdy"

		RELFILE="$(relpath "$(realpath "$FILE")")" ||
			die "$FILE is outside of git repository"

		git show "$GITBASE:$RELFILE" >"$GITFILE" ||
			die "No version of $FILE found in git ref $GITBASE"

		"$PERLTIDY" --profile="$PERLTIDYRC" "$GITFILE" -o "$GITTIDY" ||
			die "Could not check git version of $FILE"

		"$PERLTIDY" --profile="$PERLTIDYRC" "$FILE" -o "$TIDY" ||
			die "Could not check $FILE"

		report "$FILE" "$TIDY" "$GITFILE" "$GITTIDY"
	else
		"$PERLTIDY" --profile="$PERLTIDYRC" "$FILE" -o "$TIDY" ||
			die "Could not check $FILE"

		report "$FILE" "$TIDY"
	fi

	if [[ $? -ne 0 ]] ; then
		RC=1
	else
		rm -f "$TIDY"
	fi
done

exit $RC
