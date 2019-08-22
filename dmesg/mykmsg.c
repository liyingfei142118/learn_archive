#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <asm/hardware.h>
#include <linux/proc_fs.h>

#define my_buff_len   1000          //环形缓冲区长度

static struct proc_dir_entry *my_entry;


/*    声明等待队列类型中断 mybuff_wait      */
static DECLARE_WAIT_QUEUE_HEAD(mybuff_wait);


static char my_buff[my_buff_len]; 
unsigned long R=0;                      //记录读的位置
unsigned long W=0;                    //记录写的位置
 
int read_buff(char  *p)         //p:指向要读出的地址
{
   if(R==W)          
             return 0;               //读失败
        *p=my_buff[R]; 
         R=(R+1)%my_buff_len;       //R++
        return  1;                   //读成功   
}

void write_buff(char c)          //c:等于要写入的内容
{    
        my_buff [W]=c;       
        W=(W+1)%my_buff_len;     //W++
        if(W==R)
            R=(R+1)%my_buff_len;     //R++
       wake_up_interruptible(&mybuff_wait);     //唤醒队列,因为R != W 
}

/*打印到my_buff[]环形缓冲区中*/
int myprintk(const char *fmt, ...)
{
       va_list args;
       int i,len;
       static char temporary_buff[my_buff_len];        //临时缓冲区
       va_start(args, fmt);
       len=vsnprintf(temporary_buff, INT_MAX, fmt, args);
       va_end(args);

        /*将临时缓冲区放入环形缓冲区中*/
       for(i=0;i<len;i++)       
       {
            write_buff(temporary_buff[i]);
       }
       return len;
}

static int mykmsg_open(struct inode *inode, struct file *file)
{
        return 0;
}  

static int mykmsg_read(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{
      int error = 0,i=0;
      char c;

        if((file->f_flags&O_NONBLOCK)&&(R==W))      //非阻塞情况下,且没有数据可读
            return  -EAGAIN;
      
       error = -EINVAL;

              if (!buf || !count )
                     goto out;
      
       error = wait_event_interruptible(mybuff_wait,(W!=R));
              if (error)
                     goto out;

       while (!error && (read_buff(&c)) && i < count) 
      {
        error = __put_user(c,buf);      //上传用户数据
        buf ++;
        i++;
      }

      if (!error)
               error = i;
out:
       return error;
}  

const struct file_operations mykmsg_ops = {
       .read              = mykmsg_read,
       .open         = mykmsg_open,
};
static int  mykmsg_init(void)
{
    my_entry = create_proc_entry("mykmsg", S_IRUSR, &proc_root);
   if (my_entry)
         my_entry->proc_fops = &mykmsg_ops;
    return 0;
}
static void mykmsg_exit(void)
{
        remove_proc_entry("mykmsg", &proc_root);  
}

module_init(mykmsg_init);
module_exit(mykmsg_exit); 
EXPORT_SYMBOL(myprintk);
MODULE_LICENSE("GPL");
