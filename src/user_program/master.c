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

#define PAGE_SIZE 4096
#define BUF_SIZE 512
#define MMAP_SIZE 4096

#define IOCTL_CREATESOCK 0x12345677
#define IOCTL_MMAP 0x12345678
#define IOCTL_EXIT 0x12345679
#define IOCTL_OTHER 0x12345680

size_t get_filesize(const char* filename);//get the size of the input file

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, offset, tmp;
	char file_name[50], method[20];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	int n = atoi(argv[1]);
	
	strcpy(method, argv[argc - 1]);

	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
	{
		perror("failed to open /dev/master_device\n");
		return 1;
	}

	for(int i = 0; i < n; i++) {
	
		offset = 0;
		strcpy(file_name, argv[2 + i]);
		gettimeofday(&start ,NULL);
		
		if( (file_fd = open (file_name, O_RDWR)) < 0 )
		{
			perror("failed to open input file\n");
			return 1;
		}

		if( (file_size = get_filesize(file_name)) < 0)
		{
			perror("failed to get filesize\n");
			return 1;
		}

		if(ioctl(dev_fd, IOCTL_CREATESOCK ) == -1) //IOCTL_CREATESOCK  : create socket and accept the connection from the slave
		{
			perror("ioclt server create socket error\n");
			return 1;
		}
	
		switch(method[0])
		{
			case 'f': //fcntl : read()/write()
				printf("in switch f\n");
				do
				{
					ret = read(file_fd, buf, sizeof(buf)); // read from the input file
					write(dev_fd, buf, ret);//write to the the device
				}while(ret > 0);
				puts(buf);
				break;

			case 'm': //mmap
				while(offset < file_size) {
					
					// printf("offset: %ld\n", offset);
					size_t len = (file_size - offset < MMAP_SIZE) ? (file_size - offset) : MMAP_SIZE;
					file_address = mmap(NULL, len, PROT_READ, MAP_SHARED, file_fd, offset);
					kernel_address = mmap(NULL, len, PROT_WRITE, MAP_SHARED, dev_fd, offset);

					memcpy(kernel_address, file_address, len);
					offset += len;
							
					if(ioctl(dev_fd, IOCTL_MMAP, len) == -1){
						perror("master ioctl mmap failed\n");
						return 1;
					}

					munmap(file_address, MMAP_SIZE);
					munmap(kernel_address, MMAP_SIZE);
				}
				
				ioctl(dev_fd, IOCTL_OTHER, (unsigned long)file_address);
				break;
		}

		if(ioctl(dev_fd, IOCTL_EXIT ) == -1) // end sending data, close the connection
		{
			perror("ioclt server exits error\n");
			return 1;
		}
		
		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
		printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size / 8);

		close(file_fd);
	
	}
	
	close(dev_fd);

	return 0;
}

size_t get_filesize(const char* filename)
{
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}
