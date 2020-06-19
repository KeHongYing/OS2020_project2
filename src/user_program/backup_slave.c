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

#define PAGE_SIZE 4096
#define BUF_SIZE 512
#define MMAP_SIZE 4096

#define IOCTL_MMAP 0x12345678
#define IOCTL_PRINT 0x66254114

int main (int argc, char** argv)
{
	char method[20];
	char ip[20];
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;
	int file_num = atoi(argv[1]);
	int dev_fd;
	
	strcpy(method, argv[argc - 2]);
	strcpy(ip, argv[argc - 1]);
	
	if((dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}

	for(int n = 0; n < file_num; n ++){
		char file_name[50];
		char buf[BUF_SIZE];
		int file_fd;// the fd for the device and the fd for the input file
		size_t ret, file_size = 0, data_size = 0;
		struct timeval start;
		struct timeval end;

		strcpy(file_name, argv[n + 2]);

		gettimeofday(&start ,NULL);
		if((file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}

		if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
		{
			perror("ioclt create slave socket error\n");
			return 1;
		}

		write(1, "ioctl success\n", 14);
		if(read(dev_fd, &file_size, sizeof(unsigned long long int)) < 0){
			perror("get file size error\n");
			return 1;
		}

		printf("file size %d\n", file_size);
		
		switch(method[0])
		{
			case 'f'://fcntl : read()/write()
				do
				{
					ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
					puts(buf);
					write(file_fd, buf, ret); //write to the input file
					data_size += ret;
				} while(data_size < file_size);
				printf("%d finish\n", n);
				break;
			case 'm':
				while(data_size < file_size){
					ret = ioctl(dev_fd, sizeof(buf));
					if(ret < 0){
						perror("slave ioctl mmap failed\n");
						return 1;
					}
					
					posix_fallocate(file_fd, data_size, ret);
					file_address = mmap(NULL, ret, PROT_WRITE, MAP_SHARED, file_fd, data_size);
					kernel_address = mmap(NULL, ret, PROT_READ, MAP_SHARED, dev_fd, 0);
					memcpy(file_address, kernel_address, ret);
					data_size += ret;
				}
				break;
		}
		puts("fuck");

		if(ioctl(dev_fd, IOCTL_PRINT) == -1){
			perror("slave ioctl print page decriptor failed");
			return 1;
		}

		if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
		{
			perror("ioclt client exits error\n");
			return 1;
		}
		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.0001;
		printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size / 8);


		close(file_fd);
	}
	close(dev_fd);
	return 0;
}
