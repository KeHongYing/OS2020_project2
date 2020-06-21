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

#define PAGE_SIZE 4096
#define BUF_SIZE 512
#define MMAP_SIZE 4096

#define IOCTL_CREATESOCK 0x12345677
#define IOCTL_MMAP 0x12345678
#define IOCTL_EXIT 0x12345679
#define IOCTL_OTHER 0x12345680

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
	
	strcpy(method, argv[argc - 1]);

	if((dev_fd = open("/dev/master_device", O_RDWR)) < 0)
		err_sys("failed to open /dev/master_device\n");

	if(ioctl(dev_fd, IOCTL_CREATESOCK) == -1) //IOCTL_CREATESOCK  : create socket and accept the connection from the slave
		err_sys("ioclt server create socket error\n");

	for(int i = 0; i < n; i ++){
		char buf[BUF_SIZE], file_name[50];
		int ret, file_fd;
		size_t offset = 0, file_size;

		strcpy(file_name, argv[i + 2]);
		gettimeofday(&start ,NULL);
		
		if((file_fd = open(file_name, O_RDWR)) < 0)
			err_sys("failed to open input file\n");

		if((file_size = get_filesize(file_name)) < 0)
			err_sys("failed to get filesize\n");
		
		write(dev_fd, &file_size, sizeof(size_t));
	
		switch(method[0])
		{
			case 'f': {//fcntl : read()/write()
				//fprintf(stderr, "in switch f\n");
				while(offset < file_size)
				{
					if((ret = read(file_fd, buf, sizeof(buf))) < 0) // read from the input file
						err_sys("read file error\n");

					write(dev_fd, buf, ret);//write to the the device
					offset += ret;
					//fprintf(stderr, buf);
				}
				break;
			}

			case 'm': {//mmap
				while(offset < file_size){
					// printf("offset: %ld\n", offset);
					size_t len = (file_size - offset < MMAP_SIZE) ? (file_size - offset) : MMAP_SIZE;
					file_address = mmap(NULL, len, PROT_READ, MAP_SHARED, file_fd, offset);
					kernel_address = mmap(NULL, len, PROT_WRITE, MAP_SHARED, dev_fd, offset);

					memcpy(kernel_address, file_address, len);
					offset += len;
							
					if(ioctl(dev_fd, IOCTL_MMAP, len) == -1)
						err_sys("master ioctl mmap failed\n");

					munmap(file_address, len);
					munmap(kernel_address, len);
				}
				
				ioctl(dev_fd, IOCTL_OTHER, (unsigned long)file_address);
				break;
			}
		}

		if(read(dev_fd, buf, sizeof(buf)) < 0)
			err_sys("master finish error\n");
		
		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.0001;
		printf("Transmission time: %lf ms, File size: %lu bytes\n", trans_time, file_size);

		close(file_fd);
	}

	if(ioctl(dev_fd, IOCTL_EXIT ) == -1) // end sending data, close the connection
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
