#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>	/* printk(), min() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include "scull.h"		/* local definitions */

struct scull_pipe {
    wait_queue_head_t inq, outq;
    char* buffer, *end;
    int buffersize;
    char* rp, *wp;
    int nreaders, nwriters;
    struct fasync_struct* async_queue;
    struct semaphore sem;
    struct cdev cdev;
};

static int scull_p_nr_devs = SCULL_P_NR_DEVS;
int scull_p_buffer = SCULL_P_BUFFER;
dev_t scull_p_devno;			/* Our first device number */

//module_param(scull_p_nr_devs, int, 0);	/* FIXME check perms */
//module_param(scull_p_buffer, int, 0);

static struct scull_pipe *scull_p_devices;

static int spacefree(struct scull_pipe *dev)
{
    if (dev->wp == dev->rp)
    {
        return dev->buffersize - 1;
    }
    return (dev->rp + dev->buffersize - dev->wp) % dev->buffersize - 1;
}

static ssize_t scull_p_read(struct file* filp, char __user *buff, size_t count, loff_t* f_pos)
{
    struct scull_pipe* dev = (struct scull_pipe*)filp->private_data;

    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }
    PDEBUG("read data... count %ld", count);
    // NON_BLOCK
    while (dev->rp == dev->wp)
    {
        up(&dev->sem);
        if (filp->f_flags == O_NONBLOCK)
        {
            return -EAGAIN;
        }
        PDEBUG("\"%s\" reading: is going to sleep...", current->comm);
        if (wait_event_interruptible(dev->inq, dev->rp != dev->wp))
        {
            return -ERESTARTSYS;
        }
        if (down_interruptible(&dev->sem))
        {
            return -ERESTARTSYS;
        }
    }
    if (dev->rp < dev->wp)
    {
        count = (count > dev->wp - dev->rp) ? (dev->wp - dev->rp) : count;
    }
    else
    {
        count = (count > dev->end - dev->rp) ? (dev->end - dev->rp) : count;
    }
    if (copy_to_user(buff, dev->rp, count))
    {
        up(&dev->sem);
        return -EFAULT;
    }
    if (dev->rp + count == dev->end)
    {
        dev->rp = dev->buffer;
    }
    else
    {
        dev->rp += count;
    }

    up(&dev->sem);
    printk(KERN_INFO "%s did read %ld bytes...", current->comm, (long)count);
    wake_up_interruptible(&dev->outq);
    return count;
}

static int scull_getwritespace(struct scull_pipe *dev, struct file* filp)
{
    DEFINE_WAIT(wait);
    while (!spacefree(dev))
    {
        up(&dev->sem);
        if (filp->f_flags & O_NONBLOCK)
        {
            return -EAGAIN;
        }
        PDEBUG("\"%s\" writing: is going to sleep...", current->comm);
        prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
        if (!spacefree(dev))
        {
            schedule();
        }
        finish_wait(&dev->outq, &wait);
        // If the process is waken up by signal(i.e. a external kill command), this handler helps correctly
        // terminate the process. Otherwise, the loop executed infinitely.
        if (signal_pending(current))
        {
            return -ERESTARTSYS;
        }
        if (down_interruptible(&dev->sem))
        {
            return -ERESTARTSYS;
        }
    }
    return 0;
}

static ssize_t scull_p_write(struct file* filp, const char __user *buff, size_t count, loff_t* f_pos)
{
    int ret = 0, free_space_size = 0;
    struct scull_pipe* dev = (struct scull_pipe*)filp->private_data;

    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }
    PDEBUG("write data... count %ld", count);
    ret = scull_getwritespace(dev, filp);
    if (ret)
    {
        return ret;
    }
    free_space_size = spacefree(dev);
    if (dev->rp > dev->wp)
    {
        count = (count > dev->rp - dev->wp) ? (dev->rp - dev->wp) : count;
    }
    else
    {
        count = (count > dev->end - dev->wp) ? (dev->end - dev->wp) : count;
    }
    count = (count > free_space_size) ? free_space_size : count;
    if (copy_from_user(dev->wp, buff, count))
    {
        up(&dev->sem);
        return -EFAULT;
    }
    dev->wp += count;
    if (dev->wp == dev->end)
    {
        dev->wp = dev->buffer;
    }

    up(&dev->sem);
    printk(KERN_INFO "%s did write %ld bytes...", current->comm, (long)count);
    wake_up_interruptible(&dev->inq);
    return count;
}

static unsigned int scull_p_poll(struct file* filp, poll_table* wait)
{
    struct scull_pipe* dev = (struct scull_pipe*)filp->private_data;
    unsigned int mask = 0;

    if (!down_interruptible(&dev->sem))
    {
        return mask;
    }
    poll_wait(filp, &dev->inq, wait);
    poll_wait(filp, &dev->outq, wait);

    if (spacefree(dev) != 0)
    {
        mask |= (POLLOUT | POLLWRNORM);
    }
    if (dev->rp != dev->wp)
    {
        mask |= (POLLIN | POLLRDNORM);
    }
    up(&dev->sem);
    return mask;
}

static int scull_p_open(struct inode* inode, struct file* filp)
{
    struct scull_pipe* dev = container_of(inode->i_cdev, struct scull_pipe, cdev);
    filp->private_data = dev;
    PDEBUG("open device...");
    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }
    PDEBUG("open device...");
    if (!dev->buffer)
    {
        dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
        if (!dev->buffer)
        {
            printk(KERN_WARNING "alloc buffer failed");
            up(&dev->sem);
            return -ENOMEM;
        }
        dev->end = dev->buffer + scull_p_buffer;
        dev->rp = dev->wp = dev->buffer;
    }
    if (filp->f_mode & FMODE_READ)
    {
        dev->nreaders++;
    }
    if (filp->f_mode & FMODE_WRITE)
    {
        dev->nwriters++;
    }
    up(&dev->sem);
    return nonseekable_open(inode, filp);
}

static int scull_p_release(struct inode* inode, struct file* filp)
{
    struct scull_pipe* dev = (struct scull_pipe*)filp->private_data;
    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }
    PDEBUG("close device...");
    if (filp->f_mode & FMODE_READ)
    {
        dev->nreaders--;
    }
    if (filp->f_mode & FMODE_WRITE)
    {
        dev->nwriters--;
    }
    if (dev->nreaders + dev->nwriters == 0)
    {
        kfree(dev->buffer);
        dev->buffer = NULL;
    }
    up(&dev->sem);

    return 0;
}

static const struct file_operations scull_p_fops = {
    .owner = THIS_MODULE,
    .llseek = NULL,
    .read = scull_p_read,
    .write = scull_p_write,
    .unlocked_ioctl = scull_ioctl,
    .poll = scull_p_poll,
    .open = scull_p_open,
    .release = scull_p_release,
};


static bool scull_p_setup_cdev(struct scull_pipe* dev, int index)
{
    int devno = scull_p_devno + index, res;
    char buffer[10];
    PDEBUG("devno %s adding for pipe...", format_dev_t(buffer, devno));
    cdev_init(&dev->cdev, &scull_p_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_p_fops;
    res = cdev_add(&(dev->cdev), devno, 1);
    if (res < 0)
    {
        printk(KERN_WARNING "scull: cannot add cdev for this device, index %d", index);
        return false;
    }
    return true;
}

int scull_p_init(void)
{
    int res = 0, i = 0;
    dev_t dev;

    res = alloc_chrdev_region(&dev, 0, scull_p_nr_devs, "scull_p");
    if (res < 0)
    {
        printk(KERN_WARNING "scull_p: cannot get major dev no for this device");
        goto fail_region;
    }
    scull_p_devno = dev;
    /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
    scull_p_devices = kmalloc(sizeof(struct scull_pipe) * scull_p_nr_devs, GFP_KERNEL);
    if (!scull_p_devices)
    {
        goto fail_malloc;
    }
    memset(scull_p_devices, 0, sizeof(struct scull_pipe) * scull_p_nr_devs);
    for (i = 0; i < scull_p_nr_devs; i++)
    {
        scull_p_devices[i].buffersize = scull_p_buffer;
        scull_p_devices[i].buffer = NULL; // TODO
        scull_p_devices[i].end = NULL; // TODO
        init_waitqueue_head(&(scull_p_devices[i].inq));
        init_waitqueue_head(&(scull_p_devices[i].outq));
        sema_init(&scull_p_devices[i].sem, 1);
        if (!scull_p_setup_cdev(&scull_p_devices[i], i))
        {
            goto fail_cdev;
        }
    }
    PDEBUG("scull_p_devices %px", scull_p_devices);
    return 0;
fail_cdev:
    kfree(scull_p_devices);
fail_malloc:
    unregister_chrdev_region(dev, scull_p_nr_devs);
fail_region:
    return res;
}

void scull_p_cleanup(void)
{
    int i;

    for (i = 0; i < scull_p_nr_devs; i++)
    {
        struct scull_pipe* dev = &scull_p_devices[i];
        cdev_del(&(dev->cdev));
        if (dev->buffer)
        {
            kfree(dev->buffer);
        }
    }
    unregister_chrdev_region(scull_p_devno, scull_p_nr_devs);
    kfree(scull_p_devices);
    scull_p_devices = NULL;
}
