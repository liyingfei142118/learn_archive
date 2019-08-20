//testDev.c
//测试设备情况代码
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    int fd;
    int i;
    char data[256] = {0};
    int retval;

    fd = open("/dev/scull", O_RDWR);
    if (fd == 1) {
        perror("open error\n");
        exit(-1);
    }
    printf("open /dev/scull successfully\n");

    retval = write(fd, "1234567", 7);//你要写入的东西。
    if (retval == -1) {
        perror("write error\n");
        exit(-1);
    }
    retval = lseek(fd, 0, 0);
    if (retval == -1) {
        perror("lseek error\n");
        exit(-1);
    }
    retval = read(fd, data, 10);
    if (retval == -1) {
        perror("read error\n");
        exit(-1);
    }
    printf("read successfully: %s\n", data);
    close(fd);
    return 0;
}
