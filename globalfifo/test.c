#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sys/select.h>


#include <string.h>

#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20
int main()
{
    int fd,num;
    char rd_ch[BUFFER_LEN];
    fd_set rfds,wfds;/*读写文件描述符集*/

    /*以非阻塞方式打开设备文件*/
    fd=open("/dev/globalfifo",O_RDONLY|O_NONBLOCK);
    if(fd!=-1)
        {
            /*fifo清0*/
            if(ioctl(fd,FIFO_CLEAR,0)<0)
                printf("ioctl command failed!!\n");

            while(1)
                {
                    FD_ZERO(&rfds);
                    FD_ZERO(&wfds);
                    FD_SET(fd,&rfds);
                    FD_SET(fd,&wfds);

                    select(fd+1,&rfds,&wfds,NULL,NULL);

                    /*数据可获得*/
                    if(FD_ISSET(fd,&rfds))
                        printf("poll monitor ；can be read !!\n");
                    /*数据可写入*/
                    if(FD_ISSET(fd,&wfds))
                        printf("poll monitor ；can be write!!\n");
                }
        }else 
        {
            printf("device open failure!!\n");
        }
    return 0;    

}
