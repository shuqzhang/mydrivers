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

static int spacefree(struct scull_pipe *dev);

static const struct file_operations scull_p_fops = {
    .owner = THIS_MODULE,
    .llseek = NULL,
    .read = NULL, // TODO
    .write = NULL, // TODO
    .unlocked_ioctl = NULL,
    .open = NULL, // TODO
    .release = NULL, //TODO
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

static int scull_p_init(void)
{
    int res = 0, i = 0;
    dev_t dev;

    res = alloc_chrdev_region(&dev, 0, scull_p_nr_devs, "scull_p");
    if (res < 0)
    {
        printk(KERN_WARNING "scull_p: cannot get major dev no for this device");
        goto fail_region;
    }

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

static void scull_p_cleanup(void)
{
    int i;

    for (i = 0; i < scull_p_nr_devs; i++)
    {
        struct scull_pipe* dev = &scull_p_devices[i];
        cdev_del(&(dev->cdev));
    }
    unregister_chrdev_region(, scull_p_nr_devs);
    kfree(scull_p_devices);
}
