#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/completion.h>

struct completion_dev
{
	struct cdev cdev;
};

int completion_major = 0;
struct completion_dev* completion_devp;

DECLARE_COMPLETION(my_comp);

static int completion_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int completion_release(struct inode* inode, struct file* filp)
{
    return 0;
}


static ssize_t completion_read(struct file* filp, char __user *buf, size_t size, loff_t* ppos)
{
	printk(KERN_DEBUG "current process id: %d (%s) is going to sleep", current->pid, current->comm);
	wait_for_completion(&my_comp);
	printk(KERN_DEBUG "current process id: %d (%s) is waken up", current->pid, current->comm);
    
    return 0;
}

static ssize_t completion_write(struct file* filp, const char __user *buf, size_t size, loff_t* ppos)
{
	printk(KERN_DEBUG "current process id: %d (%s) awakening the readers", current->pid, current->comm);
	complete(&my_comp);
    return size;
}

static const struct file_operations completion_fops = {
    .owner = THIS_MODULE,
    .read = completion_read,
    .write = completion_write,
    .open = completion_open,
    .release = completion_release,
};

static void completion_setup_cdev(struct completion_dev* dev, int index)
{
    int err, devno = MKDEV(completion_major, index);
    cdev_init(&dev->cdev, &completion_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE "Error %d adding completion%d", err, index);
    }
}

static int __init completion_init(void)
{
    int ret = 0;
    dev_t devno = MKDEV(completion_major, 0);

    if (completion_major)
    {
        ret = register_chrdev_region(devno, 1, "completion");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, 0, 1, "completion");
        completion_major = MAJOR(devno);
    }
    if (ret < 0)
    {
        return ret;
    }
    completion_devp = kzalloc(sizeof(struct completion_dev), GFP_KERNEL);
    if (!completion_devp)
    {
        ret = -ENOMEM;
        goto fail_malloc;
    }
    completion_setup_cdev(completion_devp, 0);
    return 0;
fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}
module_init(completion_init);

static void __exit completion_exit(void)
{
    cdev_del(&completion_devp->cdev);
    kfree(completion_devp);
    unregister_chrdev_region(MKDEV(completion_major, 0), 1);
}

module_exit(completion_exit);

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");
