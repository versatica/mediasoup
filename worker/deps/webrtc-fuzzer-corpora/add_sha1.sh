#!/bin/bash

if [ -z $1 ]; then
  echo "Usage ./add_sha1.sh corpus_folder"
  exit 1
fi

SAVEIFS=$IFS
IFS=$(echo -en "\n\b")
crpfiles=$(find $1 \( ! -regex '.*/\..*' \) -not -iname "*.md" -not -iname "*.sh" -not -iname "*README*" -not -iname "*LICENSE*" -not -iname "*COPYRIGHT*" -type f)
for file in $crpfiles; do
  sha1=$(sha1sum $file | awk '{print $1}')
  if [[ $file != *"$sha1"* ]]; then
    basedir=$(dirname $file)
    basefile=$(basename $file)
    name="${basefile%.*}"
    ext="${basefile##*.}"
    duplicates=$(find $basedir -type f -name "*$sha1*")
    if [ ! -z $duplicates ]; then echo "$file -> DUPLICATE $duplicates"; continue; fi
    if [[ $name == "$ext" ]]; then ext=""; else ext=".$ext"; fi
    mv -vn "$file" "$basedir/$name-$sha1$ext"
  else
    echo "$file -> OK"
  fi
done
IFS=$SAVEIFS
