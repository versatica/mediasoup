#!/usr/bin/env bash
#
# Usage: copy_dates.sh SOURCE TARGET
#
# For each file found in SOURCE, set the modification time of the copy of that
# file in TARGET to either the time of the latest Git commit (if SOURCE contains
# a Git repository and the file was not modified after the last commit), or the
# modification time of the original file.

SOURCE="$1"
TARGET="$2"

if [ -z "$SOURCE" -o -z "$TARGET" ] ; then
	echo "Usage: $0 SOURCE TARGET" >&2
	exit 1
fi

[ -d "$SOURCE/.git" ] ; NOGIT=$?

echo "Copying modification/commit times from $SOURCE to $TARGET"

cd "$SOURCE" || exit 1
find * -type f | while read FILENAME ; do
	[ ! -e "$TARGET/$FILENAME" ] && continue

	# Copy modification time
	touch -m "$TARGET/$FILENAME" -r "$FILENAME"

	[ $NOGIT -eq 1 ] && continue				# No Git
	git diff --quiet -- "$FILENAME" || continue		# Modified
	git diff --quiet --cached -- "$FILENAME" || continue	# Modified

	# Apply modification time from Git commit time
	TIME=$(git log --pretty=format:%cd -n 1 --date=iso -- "$FILENAME")
	[ -n "$TIME" ] && touch -m "$TARGET/$FILENAME" --date "$TIME"
done
