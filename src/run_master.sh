#! /usr/env/bin bash

file=""
strace=${3:-0}

for i in $(seq 1 $1);
do
	file="$file ../sample_input/sample_input_1/target_file_$i";
done

if [ $strace != 0 ];
then
	sudo strace ./user_program/master $1 $file $2
else
	sudo ./user_program/master $1 $file $2
fi
