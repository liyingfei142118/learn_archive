#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/stat.h>

//#define DEBUG

 #ifdef DEBUG
 #define MAX_LEN 100
#endif

  /*接受到异步读信号后的动作*/
  void input_handler(int signum)
  {
  #ifdef DEBUG
      char data[MAX_LEN];
      int len;
      /*读取并输出SIDIN_FILENO 上的输入*/
      len = read(fd, &data, MAX_LEN);
      data[len] = '\0';
      printf("input available:%s", data);
  
  #endif
      printf("receive a signal from globalfifo, signalnum:%d\n",signum);
  }
  
  int main(void)
  {
      int fd,oflags;
      fd = open("/dev/globalfifo", O_RDWR, S_IRUSR | S_IWUSR);
      if(fd != -1){
          /*启动信号驱动机制*/         /*设置信号处理函数*/
          signal(SIGIO, input_handler);/*让input_handler()函数处理SIGIO信号*/
          fcntl(fd, F_SETOWN, getpid()); /*通过F_SETOWN IO控制命令设置设备文件拥有者为本进程*/
          oflags = fcntl(fd, F_GETFL);/*F_GETFL 命令 取得fd设备文件状态标志*/
          fcntl(fd, F_SETFL, oflags | FASYNC); /*F_SETFL 命令设置设备文件支持FASYNC（异步通知模式）*/
          while(1){
              sleep(100);
          }
     }else{
        printf("device open failure\n");
    }
}
