# OS2020 Final Project

**B06902016 林紹維 B06902020 唐  浩 B06902024 黃秉迦**
**B06902057 薛佳哲 B06902074 柯宏穎 B06902106 宋岩叡**

This version support bidirectional communication between master and slave.
The following instrctions are using our own script to excute.

## OS version and environment
- Arch linux, 4.14.184
- gcc 10.1

## Reqirements
- python>=3.6
- numpy
- tqdm

## Excution
```powershell
	pip install -r src/requirement.txt
	cd src
	sudo sh clean.sh
	sudo sh compile.sh
	cd ..
	sudo python <input_file dir> <output_file dir> [-mt master_type] [-st slave_type] [-t times] [-ml master_log] [-sl slave_log]
```
For example:
```powershell
	sudo python sample_input/sample_input_1 received/sample_input_1 -mt mmap -st fcntl -t 100 -ml ./master_log -sl ./slave_log
```
