#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/switch_to.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/poll.h>


//-------------class_create,device_create------
#include <linux/device.h>

/*用udev机制自动添加设备节点*/
struct class *globalfifo_class;


#define GLOBALFIFO_SIZE    0x1000    /*全局内存最大4K字节*/
#define MEM_CLEAR 0x1  /*清0全局内存*/
#define GLOBALFIFO_MAJOR 666    /*预设的globalfifo的主设备号*/

#define DEVICE_NAME "globalfifo"

static int globalfifo_major = GLOBALFIFO_MAJOR;

/*globalfifo设备结构体*/
struct globalfifo_dev                                     
{                                                        
  struct cdev cdev; /*cdev结构体*/                       
  unsigned char mem[GLOBALFIFO_SIZE]; /*全局内存*/ 
  unsigned int current_len;/*fifo 有效数据长度*/
  struct semaphore sem;/*并发控制用的信号量*/
  wait_queue_head_t r_wait;/*阻塞读用的等待队列头*/
  wait_queue_head_t w_wait;/*阻塞写用的等待队列头*/
  struct fasync_struct *async_queue;
};

static int globalfifo_fasync(int fd, struct file *filp, int mode)
{
	struct globalfifo_dev *dev = filp->private_data;
	
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

struct globalfifo_dev *globalfifo_devp; /*设备结构体指针*/
/*文件打开函数*/
int globalfifo_open(struct inode *inode, struct file *filp)
{
  printk("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%##########open\n");
  /*将设备结构体指针赋值给文件私有数据指针*/
  struct globalfifo_dev *dev;
  dev=container_of(inode->i_cdev,struct globalfifo_dev,cdev);//通过结构体成员的指针找到对应结构体的指针
  filp->private_data = dev;
  return 0;
}
/*文件释放函数*/
int globalfifo_release(struct inode *inode, struct file *filp)
{
  globalfifo_fasync(-1, filp, 0);
  return 0;
}

/* ioctl设备控制函数 */
static long globalfifo_ioctl(struct file *filp, unsigned
  int cmd, unsigned long arg)
{
  printk("############################ioctl\n");
  struct globalfifo_dev *dev = filp->private_data;/*获得设备结构体指针*/

  switch (cmd)
  {
    case MEM_CLEAR:
      memset(dev->mem, 0, GLOBALFIFO_SIZE);  //mem数组名，即空间首地址   
      printk(KERN_INFO "globalfifo is set to zero\n");
      break;

    default:
      return  - EINVAL;
  }
  return 0;
}

/*读函数*/
static ssize_t globalfifo_read(struct file *filp, char __user *buf, size_t size,
  loff_t *ppos)
{
  printk("################################read");
  int ret = 0;
  struct globalfifo_dev *dev = filp->private_data; /*获得设备结构体指针*/
  DECLARE_WAITQUEUE(wait,current);/*define wait queue*/


  if(down_interruptible(&dev->sem))/*获得信号量*/
      {
      return -ERESTARTSYS;
      }

   add_wait_queue(&dev->r_wait,&wait);/*进入读等待队列头*/
   

  /*等待fifo非空并检测用户空间是否要求非阻塞访问*/
  while(dev->current_len==0)
      {
       if(filp->f_flags &O_NONBLOCK)
           {
               ret = -EAGAIN;
            goto out;
           }
       __set_current_state(TASK_INTERRUPTIBLE);/*改变进程状态为睡眠*/

       up(&dev->sem);/*释放信号量，防止死锁*/

       schedule();/*调度其他进程执行，开始睡眠*/
       
       if(signal_pending(current))/*如果是因为信号唤醒*/
           {
               ret = -ERESTARTSYS;/*重新执行系统调用*/
            goto out2;
           } else 

        if(down_interruptible(&dev->sem))
            {
                return -ERESTARTSYS;
            }
    }

  

  /*内核空间->用户空间*/
  if(size>dev->current_len)
      size=dev->current_len;
  if (copy_to_user(buf,dev->mem,size))
  {
    ret =  - EFAULT;   /* Bad address */
    goto out;
  }
  else
  {
    memcpy(dev->mem,dev->mem+size,size);/*fifo数据前移*/
    dev->current_len -=size;/*有效数据长度减少*/

    printk(KERN_INFO"read %d bytes(s),current_len:%d \n",size,dev->current_len);
    
    wake_up_interruptible(&dev->w_wait);/*唤醒写等待队列*/
    ret= size;
  }
  out:
      up(&dev->sem);/*释放信号量*/
  out2:
      remove_wait_queue(&dev->r_wait,&wait);
  __set_current_state(TASK_RUNNING);
  return ret;
}

/*写函数*/
static ssize_t globalfifo_write(struct file *filp, const char __user *buf,
  size_t size, loff_t *ppos)
{

printk("#####################################################write");
  int ret = 0;
  struct globalfifo_dev *dev = filp->private_data; /*获得设备结构体指针*/
  DECLARE_WAITQUEUE(wait,current);/*define wait queue*/


  if(down_interruptible(&dev->sem))/*获得信号量*/
      {
      return -ERESTARTSYS;
      }

   add_wait_queue(&dev->w_wait,&wait);/*进入写等待队列*/
   

  /*等待fifo非满并检测用户空间是否要求非阻塞访问*/
  while(dev->current_len==GLOBALFIFO_SIZE)
      {
       if(filp->f_flags &O_NONBLOCK)
           {
               ret = -EAGAIN;
            goto out;
           }
       __set_current_state(TASK_INTERRUPTIBLE);/*改变进程状态为睡眠*/

       up(&dev->sem);/*释放信号量，防止死锁*/

       schedule();/*调度其他进程执行，开始睡眠*/
       
       if(signal_pending(current))
           {
               ret = -ERESTARTSYS;
            goto out2;
           } else 

        if(down_interruptible(&dev->sem))
            {
                return -ERESTARTSYS;
            }
    }

  

  /*内核空间<-用户空间*/
  if(size>GLOBALFIFO_SIZE-dev->current_len)
      size=GLOBALFIFO_SIZE-dev->current_len;
  if (copy_from_user(dev->mem+dev->current_len,buf,size))
  {
    ret =  - EFAULT;   /* Bad address */
    goto out;
  }
  else
  {
    dev->current_len +=size;/*有效数据长度减少*/

    printk(KERN_INFO"write %d bytes(s),current_len:%d \n",size,dev->current_len);
    
    wake_up_interruptible(&dev->r_wait);/*唤醒写等待队列*/
    
    if(dev->async_queue)
    {
	kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
	printk(KERN_DEBUG "kill SIGIO\n");
    }    
    ret= size;
  }
  out:
      up(&dev->sem);/*释放信号量*/
  out2:
      remove_wait_queue(&dev->w_wait,&wait);
  __set_current_state(TASK_RUNNING);
  return ret;
}

static unsigned int globalfifo_poll(struct file *filp,poll_table *wait)
{
   printk("#################################poll");
    unsigned int mask=0;
    struct globalfifo_dev *dev=filp->private_data;
    if(down_interruptible(&dev->sem))
        {
            return -ERESTARTSYS;
        }

    poll_wait(filp,&dev->r_wait,wait);/*将对应的等待队列头添加到poll_table*/
    poll_wait(filp,&dev->w_wait,wait);
    /*fifo非空*/
    if(dev->current_len!=0)
        mask |=POLLIN|POLLRDNORM;/*标示数据可获得*/
    /*fifo非满*/
    if(dev->current_len!=GLOBALFIFO_SIZE)
        mask |=POLLOUT|POLLWRNORM;/*标示数据可获得*/

    up(&dev->sem);
    return mask;

}

/*文件操作结构体*/
static const struct file_operations globalfifo_fops =
{
  .owner = THIS_MODULE,
  .read = globalfifo_read,
  .write = globalfifo_write,
  .compat_ioctl = globalfifo_ioctl,
  .open = globalfifo_open,
  .fasync = globalfifo_fasync,
  .release = globalfifo_release,
  .poll = globalfifo_poll,
};

/*初始化并注册cdev*/
static void globalfifo_setup_cdev(struct globalfifo_dev *dev, int index)
{
  int err, devno = MKDEV(globalfifo_major, index);

  cdev_init(&dev->cdev, &globalfifo_fops);
  dev->cdev.owner = THIS_MODULE;
  dev->cdev.ops = &globalfifo_fops;
  err = cdev_add(&dev->cdev, devno, 1);
  if (err)
    printk(KERN_NOTICE "Error %d ", err);
}

/*设备驱动模块加载函数*/
int globalfifo_init(void)
{
  int result;
  dev_t devno = MKDEV(globalfifo_major, 0);

  /* 申请设备号*/
  if (globalfifo_major)
    result = register_chrdev_region(devno, 1, DEVICE_NAME);
  else  /* 动态申请设备号 */
  {
    result = alloc_chrdev_region(&devno, 0, 1,DEVICE_NAME);
    globalfifo_major = MAJOR(devno);
  }  
  if (result < 0)
    return result;
    
  /* 动态申请设备结构体的内存*/
  globalfifo_devp = kmalloc(sizeof(struct globalfifo_dev), GFP_KERNEL);//kmalloc()内核空间；malloc()用户空间。返回起始地址
  if (!globalfifo_devp)    /*申请失败*/
  {
    result =  - ENOMEM;
    goto fail_malloc;
  }
  memset(globalfifo_devp, 0, sizeof(struct globalfifo_dev));//把此内存空间清零
  
  globalfifo_setup_cdev(globalfifo_devp, 0);//注册设备

  /*udev机制可以自动添加设备节点，只需要添加xxx_class这个类，以及device_create()*/
  globalfifo_class = class_create(THIS_MODULE, "globalfifo_class");/*在sys目录下创建xx_class这个类,/sys/class/~*/
  device_create(globalfifo_class, NULL, globalfifo_devp->cdev.dev,  DEVICE_NAME, DEVICE_NAME);/*自动创建设备/dev/$DEVICE_NAME*/

  //init_MUTEX(&globalfifo_devp->sem);/*初始化信号量*/
  //sema_init(&globalfifo_devp->sem);
  sema_init(&globalfifo_devp->sem, 1);
  init_waitqueue_head(&globalfifo_devp->r_wait);/*初始化读写等待队列*/
  init_waitqueue_head(&globalfifo_devp->w_wait);
  
  printk("globalfifo driver installed!\n");
  printk("globalfifo_major is:%d\n",globalfifo_major);
  printk("the device name is %s\n",DEVICE_NAME);

  return 0;

  fail_malloc: unregister_chrdev_region(devno, 1);
  return result;
}

/*模块卸载函数*/
void globalfifo_exit(void)
{
  device_destroy(globalfifo_class, globalfifo_devp->cdev.dev);
  class_destroy(globalfifo_class);

  cdev_del(&globalfifo_devp->cdev);   /*注销cdev*/
  kfree(globalfifo_devp);     /*释放设备结构体内存*/
  unregister_chrdev_region(MKDEV(globalfifo_major, 0), 1); /*释放设备号*/
  printk("globalfifo driver uninstalled!\n");
}

MODULE_AUTHOR("mhb@SEU");
MODULE_LICENSE("Dual BSD/GPL");

module_param(globalfifo_major, int, S_IRUGO);

module_init(globalfifo_init);
module_exit(globalfifo_exit);
