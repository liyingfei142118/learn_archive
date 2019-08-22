#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
 
#define CHAR_DEV_NAME "/dev/chr_dev"
 
int main()
{
	int ret;
	int fd;
	char buf[32];
 
	fd = open(CHAR_DEV_NAME, O_RDONLY | O_NDELAY);
	if(fd < 0)
	{
		printf("open failed!\n");
		return -1;
	}
	
	read(fd, buf, 32);
	close(fd);
	
	return 0;
}
