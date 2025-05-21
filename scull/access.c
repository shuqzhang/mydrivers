#include "scull.h"
#include <asm/uaccess.h>
#include <linux/fs.h>

static int scull_access_nr_devs = SCULL_ACCESS_NR_DEVS;
dev_t scull_access_devno;			/* Our first device number */

/*First device*/
static struct scull_dev scull_s_dev;
static int scull_s_open(struct inode* inode, struct file* filp)
{
    int ret = 0;
    struct scull_dev* dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    PDEBUG("hello, open single access");
    filp->private_data = dev;
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        PDEBUG("hello, open it with write mode");
        scull_trim(dev);
    }

    return ret;
}

static int scull_s_release(struct inode* inode, struct file* filp)
{
    int ret = 0;

    return ret;
}


static struct file_operations scull_s_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_s_open,
    .release = scull_s_release,
};

static struct scull_adev_info
{
    const char* name;
    struct scull_dev* adev;
    struct file_operations* scull_a_fops;
} scull_adev_infos[] = {
    {"scull_single", &scull_s_dev, &scull_s_fops}
};

static bool scull_access_setup_cdev(struct scull_adev_info* dev_info, dev_t devno)
{
    int res;
    struct scull_dev* dev = dev_info->adev;
    char buffer[10];
    PDEBUG("devno %s adding for %s ...", format_dev_t(buffer, devno), dev_info->name);
    cdev_init(&dev->cdev, dev_info->scull_a_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = dev_info->scull_a_fops;
    res = cdev_add(&(dev->cdev), devno, 1);
    if (res < 0)
    {
        printk(KERN_WARNING "scull access: cannot add cdev for this device %s", dev_info->name);
        return false;
    }
    return true;
}

int __init scull_access_init(void)
{
    int res = 0, i = 0;
    PDEBUG("hello, pdebug access");

    res = alloc_chrdev_region(&scull_access_devno, 0, scull_access_nr_devs, "scull_single");

    if (res < 0)
    {
        printk(KERN_WARNING "scull: cannot get major dev no for this device");
        goto fail_region;
    }

    for (i = 0; i < scull_access_nr_devs; i++)
    {
        scull_adev_infos[i].adev->qset = scull_qset_n;
        scull_adev_infos[i].adev->quantum = scull_quantum;
        scull_adev_infos[i].adev->data = NULL;
        sema_init(&scull_adev_infos[i].adev->sem, 1);
        scull_access_setup_cdev(&scull_adev_infos[i], scull_access_devno + i);
    }

    return 0;
fail_region:
    return res;
}

void __exit scull_access_cleanup(void)
{
    int i;
    PDEBUG("good bye, access control");

    for (i = 0; i < scull_access_nr_devs; i++)
    {
        struct scull_dev* dev = scull_adev_infos[i].adev;
        cdev_del(&(dev->cdev));
    }
    unregister_chrdev_region(scull_access_devno, scull_access_nr_devs);
}

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");