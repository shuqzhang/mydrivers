#include "scull.h"
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <asm/uaccess.h>

struct scull_dev* scull_devices;
int scull_nr_devs = 4;
int scull_major = 0;
int scull_minor = 0;

static int scull_open(struct inode* inode, struct file* filp)
{
    int ret = 0;
    struct scull_dev* dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    PDEBUG("hello, open it");
    filp->private_data = dev;
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        PDEBUG("hello, open it with write mode");
        scull_trim(dev);
    }

    return ret;
}

static int scull_release(struct inode* inode, struct file* filp)
{
    int ret = 0;

    return ret;
}

static const struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_open,
    .release = scull_release,
};


static bool scull_setup_cdev(struct scull_dev* dev, int index)
{
    int devno = MKDEV(scull_major, scull_minor + index), res;
    char buffer[10];
    PDEBUG("devno %s adding...", format_dev_t(buffer, devno));
    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_fops;
    res = cdev_add(&(dev->cdev), devno, 1);
    if (res < 0)
    {
        printk(KERN_WARNING "scull: cannot add cdev for this device, index %d", index);
        return false;
    }
    return true;
}

static int __init scull_init(void)
{
    int res = 0, i = 0;
    dev_t dev = MKDEV(scull_major, 0);
    PDEBUG("hello, pdebug");

    if (scull_major)
    {
        res = register_chrdev_region(dev, scull_nr_devs, "scull");
    }
    else
    {
        res = alloc_chrdev_region(&dev, 0, scull_nr_devs, "scull");
        scull_major = MAJOR(dev);
        scull_minor = MINOR(dev);
    }
    if (res < 0)
    {
        printk(KERN_WARNING "scull: cannot get major dev no for this device");
        goto fail_region;
    }
    /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
    scull_devices = kmalloc(sizeof(struct scull_dev) * scull_nr_devs, GFP_KERNEL);
    if (!scull_devices)
    {
        goto fail_malloc;
    }
    memset(scull_devices, 0, sizeof(struct scull_dev) * scull_nr_devs);
    for (i = 0; i < scull_nr_devs; i++)
    {
        scull_devices[i].qset = scull_qset_n;
        scull_devices[i].quantum = scull_quantum;
        scull_devices[i].data = NULL;
        sema_init(&scull_devices[i].sem, 1);
        if (!scull_setup_cdev(&scull_devices[i], i))
        {
            goto fail_cdev;
        }
    }
    PDEBUG("scull_devices %px", scull_devices);

#ifdef SCULL_DEBUG /* only when debugging */
    PDEBUG("SCULL_DEBUG used");
	scull_create_proc();
#endif
    scull_p_init();
    scull_access_init();

    return 0;
fail_cdev:
    kfree(scull_devices);
fail_malloc:
    unregister_chrdev_region(dev, scull_nr_devs);
fail_region:
    return res;
}

module_init(scull_init);

static void __exit scull_exit(void)
{
    int i;
    PDEBUG("good bye, pdebug");
#ifdef SCULL_DEBUG /* use proc only if debugging */
	scull_remove_proc();
#endif
    for (i = 0; i < scull_nr_devs; i++)
    {
        struct scull_dev* dev = &scull_devices[i];
        scull_trim(dev);
        cdev_del(&(dev->cdev));
    }
    unregister_chrdev_region(MKDEV(scull_major, scull_minor), scull_nr_devs);
    //scull_devices = NULL;// create a oops
    //scull_devices[0].size = 0;
    kfree(scull_devices);
    scull_p_cleanup();
    scull_access_cleanup();
}

module_exit(scull_exit);

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");
