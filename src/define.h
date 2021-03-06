#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#define DEFAULT_PORT 2325

#define IOCTL_CREATESOCK 0x12345677
#define IOCTL_MMAP 0x12345678
#define IOCTL_EXIT 0x12345679
#define IOCTL_DEFAULT 0x12345680


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define BUF_SIZE 512
#define MMAP_SIZE (256 * 4096)

