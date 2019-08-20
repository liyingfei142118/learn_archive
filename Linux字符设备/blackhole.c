//blackhole.c
//驱动代码
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

int scull_major = 0;
int scull_minor = 0;
struct cdev cdev;

#define MAX_SIZE 10
size_t size = 0;
char store[MAX_SIZE];

int scull_open(struct inode *inode, struct file *filp) {
    /* trim to 0 the length of the device if open was write-only */
    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        size = 0;
    }
    return 0; /* success */
}

int scull_release(struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    //读函数照常即可。
    ssize_t retval = 0;
     if (*f_pos >= size)
        goto out;
     if (*f_pos + count > size)
        count = size - *f_pos;
     if (copy_to_user(buf, store + *f_pos, count)) {
        retval = -EFAULT;
        goto out;
     }

     *f_pos += count;
     retval = count;
 out:
        return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos){
    return count;//返回输入的字符的size，假装已经写入了该写入的字符。
}

loff_t scull_llseek(struct file *filp, loff_t off, int whence){
    loff_t newpos;
    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;
        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;
        case 2: /* SEEK_END */
            newpos = size + off;
            break;
        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = scull_llseek,
    .read = scull_read,
    .write = scull_write,
    .release = scull_release,
};

int scull_init_module(void){
    int result; dev_t dev = 0;
/*
 * Get a range of minor numbers to work with, asking for a dynamic major
 */
    result = alloc_chrdev_region(&dev, scull_minor, 1, "scull");
    scull_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    }
    /* register our device */
    cdev_init(&cdev, &scull_fops);
    cdev.owner = THIS_MODULE;
    cdev.ops = &scull_fops;
    result = cdev_add (&cdev, dev, 1);
    if (result) {
        printk(KERN_WARNING "Error %d adding scull", result);
        unregister_chrdev_region(dev, 1);
        return result;
    }
    return 0; /* succeed */
}

void scull_cleanup_module(void){
    /* cleanup_module is never called if registering failed */
    dev_t dev;
    cdev_del(&cdev);
    dev = MKDEV(scull_major, scull_minor);
    unregister_chrdev_region(dev, 1);
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
