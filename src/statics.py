import numpy as np
import os
import argparse
from tqdm import tqdm

if __name__ == '__main__':

	parser = argparse.ArgumentParser()
	parser.add_argument('input_dir')
	parser.add_argument('output_dir')
	parser.add_argument('-mt', '--master_type', help = 'dafault: %(default)s', default = 'mmap')
	parser.add_argument('-st', '--slave_type', help = 'dafault: %(default)s', default = 'mmap')
	parser.add_argument('-t', '--time', help = 'dafault: %(default)s', default = 100, type = int)
	parser.add_argument('-ml', '--master_log', help = 'dafault: %(default)s', default = 'master_log')
	parser.add_argument('-sl', '--slave_log', help = 'dafault: %(default)s', default = 'slave_log')
	args = parser.parse_args()

	if os.path.exists(args.master_log):
		
		os.system('rm -f ' + args.master_log)
	
	if os.path.exists(args.slave_log):
		
		os.system('rm -f ' + args.slave_log)
	
	os.makedirs(args.output_dir, exist_ok = True)	
	
	files = os.listdir(args.input_dir)

	master_files = str(len(files)) + ' ' + ' '.join([os.path.join(args.input_dir, f) for f in files])
	slave_files = str(len(files)) + ' ' + ' '.join([os.path.join(args.output_dir, f) for f in files])
	master_exec = './src/user_program/master ' + master_files + ' ' + args.master_type + ' >> ' + args.master_log
	slave_exec = './src/user_program/slave ' + slave_files + ' ' + args.slave_type + ' 127.0.0.1 >> ' + args.slave_log
	total_exec = master_exec + ' & ' + slave_exec
	print(total_exec)

	for i in tqdm(range(args.time)):

		os.system(total_exec)
		os.system('diff ' + args.input_dir + ' ' + args.output_dir)

	with open(args.master_log, 'r') as f:
		
		lines = f.readlines()
		master_times = np.array([float(line.strip().split(' ')[2]) for line in lines])
		master_filesizes = np.array([int(line.strip().split(' ')[-2]) for line in lines])
	
	with open(args.slave_log, 'r') as f:
		
		lines = f.readlines()
		slave_times = np.array([float(line.strip().split(' ')[2]) for line in lines if 'ioctl success' not in line])
		slave_filesizes = np.array([int(line.strip().split(' ')[-2]) for line in lines if 'ioctl success' not in line])

	print('diff_mean(ms):', np.mean(np.abs(master_times - slave_times)))
	print('diff_std(ms):', np.std(np.abs(master_times - slave_times)))
	print('master_mean(ms):', np.mean(master_times))
	print('master_std(ms):', np.std(master_times))
	print('master_filesize(bytes):', np.mean(master_filesizes))
	print('master_average_filesize(bytes/ms):', np.mean(master_filesizes) / np.mean(master_times))
	print('slave_mean(ms):', np.mean(slave_times))
	print('slave_std(ms):', np.std(slave_times))
	print('slave_filesize(bytes):', np.mean(slave_filesizes))
	print('slave_average_filesize(bytes/ms):', np.mean(slave_filesizes) / np.mean(slave_times))
