#!/usr/bin/env bash

current_dir=${PWD##*/}

if [ "${current_dir}" != "worker" ] ; then
	echo ">>> [ERROR] $(basename $0) must be called from mediasoup/worker/ directory" >&2
	exit 1
fi

if ! type "flint++" &> /dev/null; then
	echo ">>> [ERROR] flint++ command not found" >&2
	echo ">>> [INFO] get it at https://github.com/L2Program/FlintPlusPlus and try again"
	exit 1
fi

echo ">>> [INFO] running flint++ in src/ folder ..."
cd src/
flint++ --recursive --verbose ./
cd ..

echo
echo ">>> [INFO] running flint++ in include/ folder ..."
cd include/
flint++ --recursive --verbose ./
cd ..
