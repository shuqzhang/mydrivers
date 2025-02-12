#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/sched.h>  /* current and everything */
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/types.h>  /* size_t */
#include <linux/wait.h>

MODULE_LICENSE("Dual BSD/GPL");

static int sleepy_major = 0;
static struct cdev cdev;

static DECLARE_WAIT_QUEUE_HEAD(wq);
static int flag = 0;

static ssize_t sleepy_read(struct file* filp, char __user *buf, size_t size, loff_t* ppos)
{
	printk(KERN_DEBUG "current process id: %d (%s) is going to sleep", current->pid, current->comm);
	wait_event_interruptible(wq, flag != 0);
    flag = 0;
	printk(KERN_DEBUG "current process id: %d (%s) is waken up", current->pid, current->comm);
    
    return 0;
}

static ssize_t sleepy_write(struct file* filp, const char __user *buf, size_t size, loff_t* ppos)
{
	printk(KERN_DEBUG "current process id: %d (%s) awakening the readers", current->pid, current->comm);
	flag = 1;
    wake_up_interruptible(&wq);
    return size;
}

static const struct file_operations sleepy_fops = {
    .owner = THIS_MODULE,
    .read = sleepy_read,
    .write = sleepy_write,
};

static int __init sleepy_init(void)
{
    int ret = 0;
    int err;
    dev_t devno = MKDEV(sleepy_major, 0);
    ret = alloc_chrdev_region(&devno, 0, 1, "sleepy");
    if (ret < 0 )
    {
        return ret;
    }
    sleepy_major = MAJOR(devno);
    cdev_init(&cdev, &sleepy_fops);
    cdev.owner = THIS_MODULE;
    cdev.ops = &sleepy_fops;
    err = cdev_add(&cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE "Failed to add cdev sleepy");
    }

    return ret;
}

module_init(sleepy_init);

static void __exit sleepy_exit(void)
{
    cdev_del(&cdev);
    unregister_chrdev_region(MKDEV(sleepy_major, 0), 1);
}
module_exit(sleepy_exit);

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");
