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

int main(int argc, char** argv)
{
	char method[20];
	char ip[20];
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;
	int file_num = atoi(argv[1]);
	int dev_fd;
	struct timeval start, end;
	int total_size = 0;
	
	strcpy(method, argv[argc - 2]);
	strcpy(ip, argv[argc - 1]);
	
	if((dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}

	gettimeofday(&start ,NULL);
	for(int n = 0; n < file_num; n ++){
		char file_name[50];
		char buf[BUF_SIZE];
		int ret, file_fd;// the fd for the device and the fd for the input file
		size_t file_size = 0, data_size = 0;

		strcpy(file_name, argv[n + 2]);

		if((file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}

		if(ioctl(dev_fd, IOCTL_CREATESOCK, ip) == -1)	//IOCTL_CREATESOCK : connect to master in the device
		{
			perror("ioclt create slave socket error\n");
			return 1;
		}

		write(1, "ioctl success\n", 14);
		
		switch(method[0])
		{
			case 'f'://fcntl : read()/write()
				do{
					ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
					//puts(buf);
					write(file_fd, buf, ret); //write to the input file
					file_size += ret;
				} while(ret > 0);
				//printf("%d finish\n", n);
				break;
			case 'm':
				while(1){
					ret = ioctl(dev_fd, IOCTL_MMAP);
					//printf("%d\n", ret);
					if(ret < 0){
						perror("slave ioctl mmap failed\n");
						return 2;
					}else if( ret == 0 ){
						file_size = data_size;
						break;
					}
					
					posix_fallocate(file_fd, data_size, ret);
					file_address = mmap(NULL, ret, PROT_WRITE, MAP_SHARED, file_fd, data_size);
					kernel_address = mmap(NULL, ret, PROT_READ, MAP_SHARED, dev_fd, 0);
					memcpy(file_address, kernel_address, ret);
					data_size += ret;
					munmap(file_address, ret);
					munmap(kernel_address, ret);
				}
				if(ioctl(dev_fd, IOCTL_DEFAULT, kernel_address) == -1){
					perror("slave ioctl print page decriptor failed");
					return 1;
				}
				break;
		}

		if(ioctl(dev_fd, IOCTL_EXIT) == -1)// end receiving data, close the connection
		{
			perror("ioclt client exits error\n");
			return 1;
		}
		
		close(file_fd);
		total_size += (int)file_size;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.001;
	printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, total_size);
	close(dev_fd);
	return 0;
}
