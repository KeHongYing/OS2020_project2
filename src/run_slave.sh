#! /usr/env/bin bash

file=""
strace=${3:-0}

for i in $(seq 1 $1);
do
	file="$file ../received/target_file_$i";
done

if [ $strace != 0 ];
then
	sudo strace ./user_program/slave $1 $file $2 127.0.0.1
else
	sudo ./user_program/slave $1 $file $2 127.0.0.1
fi
