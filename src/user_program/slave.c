#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include "../define.h"

void err_sys(const char *x);

int main(int argc, char** argv)
{
	char method[20];
	char ip[20];
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;
	int file_num = atoi(argv[1]);
	int dev_fd;
	size_t total_size = 0;
	struct timeval start;
	struct timeval end;
	
	strcpy(method, argv[argc - 2]);
	strcpy(ip, argv[argc - 1]);
	
	if((dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
		err_sys("failed to open /dev/slave_device\n");
		
	if(ioctl(dev_fd, IOCTL_CREATESOCK, ip) == -1)	//0x12345677 : connect to master in the device
		err_sys("ioclt create slave socket error\n");

	write(1, "ioctl success\n", 14);
	
	gettimeofday(&start, NULL);

	for(int n = 0; n < file_num; n ++){
		char file_name[128] = {}, buf[BUF_SIZE] = {};
		int ret, file_fd; // the fd for the device and the fd for the input file
		size_t file_size = 0, data_size = 0;

		strcpy(file_name, argv[n + 2]);

		if((file_fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
			err_sys("failed to open input file\n");

		if((ret = read(dev_fd, &file_size, sizeof(size_t))) < 0)
			err_sys("get file size error\n");

		total_size += file_size;
		
		switch(method[0])
		{
			case 'f'://fcntl : read()/write()
				while(data_size < file_size){
					if((ret = read(dev_fd, buf, file_size - data_size < BUF_SIZE ? file_size - data_size : BUF_SIZE)) < 0) // read from the the device
						err_sys("read file error\n");
					write(file_fd, buf, ret); //write to the input file
					data_size += ret;
				}

				break;
			case 'm': {
				while(data_size < file_size){
					if((ret = ioctl(dev_fd, IOCTL_MMAP, file_size - data_size < MMAP_SIZE ? file_size - data_size : MMAP_SIZE)) < 0)
						err_sys("slave ioctl mmap failed\n");
					
					posix_fallocate(file_fd, data_size, ret);
					file_address = mmap(NULL, ret, PROT_WRITE, MAP_SHARED, file_fd, data_size);
					kernel_address = mmap(NULL, ret, PROT_READ, MAP_SHARED, dev_fd, 0);
					memcpy(file_address, kernel_address, ret);
					data_size += ret;

					munmap(file_address, ret);
					munmap(kernel_address, ret);
				}

				if(ioctl(dev_fd, IOCTL_DEFAULT, kernel_address) == -1)
					err_sys("slave ioctl print page decriptor failed\n");

				break;
			}
		}

		//write(dev_fd, &n, sizeof(int));
		
		close(file_fd);
	}

	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.001;
	printf("Transmission time: %lf ms, File size: %lu bytes\n", trans_time, total_size);

	if(ioctl(dev_fd, IOCTL_EXIT) == -1)// end receiving data, close the connection
		err_sys("ioclt client exits error\n");

	close(dev_fd);
	return 0;
}

void err_sys(const char *x)
{
	perror(x);
	exit(1);
}
