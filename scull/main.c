#include "scull.h"
#include <linux/kdev_t.h>
#include <asm/uaccess.h>

struct scull_dev* scull_devices;
int scull_nr_devs = 4;
int scull_major = 0;
int scull_minor = 0;

static void scull_trim(struct scull_dev* dev)
{
    struct scull_qset *dptr, *next;
    int i;
    void** data;
    dptr = dev->data;
    while (dptr)
    {
        next = dptr->next;
        data = dptr->data;
        if (data)
        {
            for (i = 0; i < dev->qset; i++)
            {
                if (data[i])
                {
                    kfree(data[i]);
                }
            }
            kfree(data);
        }
        kfree(dptr);
        dptr = next;
    }
    dev->size = 0;
    dev->data = NULL;
    dev->qset = QSET_SIZE;
    dev->quantum = QUANTUM_SIZE;
}

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

struct scull_qset* scull_follow(struct scull_qset** head, int item)
{
    struct scull_qset *qs = *head, *prev = NULL;
    if (*head == NULL)
    {
        qs = (struct scull_qset*)kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (qs == NULL)
        {
            printk(KERN_ALERT "alloc qset failure.");
            return NULL;
        }
        memset(qs, 0, sizeof(struct scull_qset));
        *head = qs;
    }
    while (item >= 0)
    {
        if (qs == NULL)
        {
            qs = (struct scull_qset*)kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (qs == NULL)
            {
                printk(KERN_ALERT "alloc qset failure.");
                return NULL;
            }
            memset(qs, 0, sizeof(struct scull_qset));
            prev->next = qs;
        }
        prev = qs;
        qs = qs->next;
        item--;
    }
    return prev;
}

static ssize_t scull_read(struct file* filp, char __user *buff, size_t count, loff_t* f_pos)
{
    struct scull_dev* dev = filp->private_data;
    struct scull_qset* dptr = NULL;
    int quantum = dev->quantum;
    int qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    PDEBUG("hello, read it");
    PDEBUG("f_pos=%lld count %ld dev->size %ld", *f_pos, count, dev->size);

    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }
    if ((long)(*f_pos) >= dev->size)
    {
        goto out;
    }
    item = (long)(*f_pos) / itemsize;
    rest = (long)(*f_pos) % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;
    PDEBUG("item %d, rest %d, s_pos %d, q_pos %d", item, rest, s_pos, q_pos);
    dptr = scull_follow(&dev->data, item);
    if (dptr == NULL || dptr->data == NULL || dptr->data[s_pos] == NULL)
    {
        goto out;
    }
    if (count > quantum - q_pos)
    {
        count = quantum - q_pos;
    }
    if (copy_to_user(buff, dptr->data[s_pos] + q_pos, count))
    {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

out:
    up(&dev->sem);
    PDEBUG("retval=%ld", retval);
    return retval;
}

static ssize_t scull_write(struct file* filp, const char __user *buff, size_t count, loff_t* f_pos)
{
    ssize_t retval = -ENOMEM;
    struct scull_dev* dev = (struct scull_dev*)filp->private_data;
    struct scull_qset* dptr = NULL;
    int qset = dev->qset, quantum = dev->quantum;
    int qset_size = qset * quantum;
    int item, rest, s_pos, q_pos;
    PDEBUG("test write ");
    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }

    item = (*f_pos) / qset_size;
    rest = (*f_pos) % qset_size;
    s_pos = rest / quantum;
    q_pos = rest % quantum;
    dptr = scull_follow(&dev->data, item);

    if (dptr == NULL)
    {
        goto out;
    }
    if (dptr->data == NULL)
    {
        dptr->data = kmalloc(sizeof(char*) * qset, GFP_KERNEL);
        if (dptr->data == NULL)
        {
            printk(KERN_ALERT "alloc qset pointers failure");
            goto out;
        }
        memset(dptr->data, 0, sizeof(char*) * qset);
    }
    if (dptr->data[s_pos] == NULL)
    {
        dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (dptr->data[s_pos] == NULL)
        {
            printk(KERN_ALERT "alloc quantum data failure");
            goto out;
        }
        memset(dptr->data[s_pos], 0, quantum);
    }
    if (count > dev->quantum - q_pos)
    {
        count = dev->quantum - q_pos;
    }

    if (copy_from_user(dptr->data[s_pos] + q_pos, buff, count))
    {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    dev->size += count;
    retval = count;

out:
    up(&dev->sem);
    return retval;
}

static long scull_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    int err = 0, tmp;
    int retval = 0;

    if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

    // verify address
    // if (_IOC_DIR(cmd) & _IOC_READ)
    // {
    //     err = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
    // }
    // if (_IOC_DIR(cmd) & _IOC_WRITE)
    // {
    //     err = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
    // }
    err = !access_ok((void __user*)arg, _IOC_SIZE(cmd));
    if (err)
    {
        return -EFAULT;
    }

    return retval;
}

static const struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = NULL, // globalmem_llseek,
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
        scull_devices[i].qset = QSET_SIZE;
        scull_devices[i].quantum = QUANTUM_SIZE;
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
        cdev_del(&(dev->cdev));
    }
    unregister_chrdev_region(MKDEV(scull_major, scull_minor), scull_nr_devs);
    //scull_devices = NULL;// create a oops
    //scull_devices[0].size = 0;
    kfree(scull_devices);
}

module_exit(scull_exit);

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");
