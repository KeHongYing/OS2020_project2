#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include "../define.h"

size_t get_filesize(const char* filename);//get the size of the input file
void err_sys(const char *x);

int main (int argc, char* argv[])
{
	int dev_fd;// the fd for the device and the fd for the input file
	char method[20];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	int n = atoi(argv[1]);
	size_t total_size = 0;
	
	strcpy(method, argv[argc - 1]);

	if((dev_fd = open("/dev/master_device", O_RDWR)) < 0)
		err_sys("failed to open /dev/master_device\n");

	if(ioctl(dev_fd, IOCTL_CREATESOCK) == -1) //IOCTL_CREATESOCK  : create socket and accept the connection from the slave
		err_sys("ioclt server create socket error\n");
	
	gettimeofday(&start ,NULL);

	for(int i = 0; i < n; i ++){
		char buf[BUF_SIZE], file_name[128];
		int ret, file_fd;
		size_t offset = 0, file_size = 0;

		strcpy(file_name, argv[i + 2]);
		
		if((file_fd = open(file_name, O_RDWR)) < 0)
			err_sys("failed to open input file\n");

		if((file_size = get_filesize(file_name)) < 0)
			err_sys("failed to get filesize\n");
		
		write(dev_fd, &file_size, sizeof(size_t));
		total_size += file_size;
	
		switch(method[0])
		{
			case 'f': {//fcntl : read()/write()
				//fprintf(stderr, "in switch f\n");
				while(offset < file_size)
				{
					if((ret = read(file_fd, buf, BUF_SIZE)) < 0) // read from the input file
						err_sys("read file error\n");

					write(dev_fd, buf, ret);//write to the the device
					offset += ret;
				}
				break;
			}

			case 'm': {//mmap
				while(offset < file_size){
					size_t len = (file_size - offset < MMAP_SIZE) ? (file_size - offset) : MMAP_SIZE;
					file_address = mmap(NULL, len, PROT_READ, MAP_SHARED, file_fd, offset);
					kernel_address = mmap(NULL, len, PROT_WRITE, MAP_SHARED, dev_fd, offset);

					memcpy(kernel_address, file_address, len);
					offset += len;
							
					if(ioctl(dev_fd, IOCTL_MMAP, len) == -1)
						err_sys("master ioctl mmap failed\n");
				
					if(ioctl(dev_fd, IOCTL_DEFAULT, file_address) == -1)
						err_sys("master ioctl print page decriptor failed\n");

					munmap(file_address, len);
					munmap(kernel_address, len);
				}
				
				break;
			}
		}

		//int finish;
		//if(read(dev_fd, &finish, sizeof(int)) < 0)
		//	err_sys("master finish error\n");
		
		close(file_fd);
	}

	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.001;
	printf("Transmission time: %lf ms, File size: %lu bytes\n", trans_time, total_size);

	if(ioctl(dev_fd, IOCTL_EXIT) == -1) // end sending data, close the connection
		err_sys("ioclt server exits error\n");
	
	close(dev_fd);

	return 0;
}

size_t get_filesize(const char* filename)
{
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}

void err_sys(const char *x)
{
	perror(x);
	exit(1);
}
