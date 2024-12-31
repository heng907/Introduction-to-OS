#!/bin/bash
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 /path/to/your/multilevelBF.so /path/to/your/main"
	exit 1
fi

for (( i=1; i<=6; i=i+1 )); do
	cp "test.txt.${i}" test.txt
	LD_PRELOAD=/home/heng/Desktop/os_hw4/multilevelBF.so ./main
	ret=$?
	if [ "$ret" -ne 0 ]; then
		echo "Execute Error"
		exit $ret
	fi
done