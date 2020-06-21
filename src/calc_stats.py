import argparse
import numpy as np
from statistics import median

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--input', '-i', help = 'Path of input file')
	args = parser.parse_args()
	
	with open(args.input, 'r') as input_file:
		lines = input_file.readlines()
	
	times = []
	for line in lines:
		if line[0] == 'i':
			continue
		else:
			tokens = line.split()
			time = float(tokens[2])
			times.append(time)
	
	time_mean = np.mean(times)
	time_std = np.std(times)
	for idx, time in enumerate(times):
		if abs(time - time_mean) > (2 * time_std):
			times.pop(idx)
	
	print('Num of valid data: {}'.format(len(times)))
	print('Average: {}'.format(np.mean(times)))
	print('Standard Deviation: {}'.format(np.std(times)))
			
			
