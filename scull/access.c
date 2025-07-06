#include "scull.h"
#include <asm/uaccess.h>
#include <asm/alternative.h>
#include <asm/atomic.h>
#include <linux/fs.h>
#include <linux/spinlock.h>

dev_t scull_access_devno[SCULL_ACCESS_NR_DEVS];			/* All device numbers */

/*First device*/
static struct scull_dev scull_s_dev;
static atomic_t scull_s_available = ATOMIC_INIT(1);
static int scull_s_open(struct inode* inode, struct file* filp)
{
    int ret = 0;
    struct scull_dev* dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    // if (!atomic_dec_and_test(&scull_s_available))
    // {
    //     atomic_inc(&scull_s_available);
    //     return -EBUSY;
    // }
    ///// comment the above lines because compile issue below, 
    //     In file included from ././include/linux/compiler_types.h:85,
    //                  from <command-line>:
    // In function ‘arch_static_branch_jump’,
    //     inlined from ‘system_uses_lse_atomics’ at ./arch/arm64/include/asm/lse.h:24:10:
    // ./include/linux/compiler-gcc.h:88:38: warning: ‘asm’ operand 0 probably does not match constraints
    //    88 | #define asm_volatile_goto(x...) do { asm goto(x); asm (""); } while (0)
    //       |                                      ^~~
    // ./arch/arm64/include/asm/jump_label.h:38:9: note: in expansion of macro ‘asm_volatile_goto’
    //    38 |         asm_volatile_goto(

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
    // atomic_inc(&scull_s_available);
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

// Second device, can only be accessed by one user.
static struct scull_dev scull_u_dev;
// facilities used by this device
static int scull_u_count = 0;
static kuid_t scull_u_owner = KUIDT_INIT(0);

static DEFINE_SPINLOCK(scull_u_lock);

static int scull_u_open(struct inode* inode, struct file* filp)
{
    int ret = 0;
    struct scull_dev* dev = NULL;
    spin_lock(&scull_u_lock);
    if (scull_u_count > 0 &&
        !uid_eq(scull_u_owner, current_uid()) &&
        !uid_eq(scull_u_owner, current_euid()) &&
        !capable(CAP_DAC_OVERRIDE))
    {
        spin_unlock(&scull_u_lock);
        return -EBUSY;
    }
    if (scull_u_count == 0)
    {
        scull_u_owner = current_uid();
    }
    scull_u_count++;

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);

    PDEBUG("hello, open uid [%d] access count %d", scull_u_owner.val, scull_u_count);
    filp->private_data = dev;
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        PDEBUG("hello, open it with write mode");
        scull_trim(dev);
    }
    spin_unlock(&scull_u_lock);

    return ret;
}

static int scull_u_release(struct inode* inode, struct file* filp)
{
    int ret = 0;
    spin_lock(&scull_u_lock);
    scull_u_count--;
    if (scull_u_count == 0)
    {
        scull_u_owner = KUIDT_INIT(0);
    }
    spin_unlock(&scull_u_lock);
    return ret;
}

static struct file_operations scull_u_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_u_open,
    .release = scull_u_release,
};

/// third device, instead of ebusy, open device with blocking mode
static struct scull_dev scull_w_dev;
// facilities used by this device
static int scull_w_count = 0;
static kuid_t scull_w_owner = KUIDT_INIT(0);

static DEFINE_SPINLOCK(scull_w_lock);
static DECLARE_WAIT_QUEUE_HEAD(scull_w_wait);

static inline int scull_w_available(void)
{
    return scull_w_count == 0 ||
        uid_eq(scull_w_owner, current_uid()) ||
        uid_eq(scull_w_owner, current_euid()) ||
        capable(CAP_DAC_OVERRIDE);
}

static int scull_w_open(struct inode* inode, struct file* filp)
{
    int ret = 0;
    struct scull_dev* dev = NULL;
    spin_lock(&scull_w_lock);
    while (!scull_w_available())
    {
        spin_unlock(&scull_w_lock);
        if (filp->f_flags & O_NONBLOCK)
        {
            return -EAGAIN;
        }
        if (wait_event_interruptible(scull_w_wait, scull_w_available()))
        {
            return -ERESTARTSYS;
        }
        spin_lock(&scull_w_lock);
    }
    
    if (scull_w_count == 0)
    {
        scull_w_owner = current_uid();
    }
    scull_w_count++;

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);

    PDEBUG("hello, open blocking uid [%d] access count %d", scull_w_owner.val, scull_w_count);
    filp->private_data = dev;
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        PDEBUG("hello, open it with write mode");
        scull_trim(dev);
    }
    spin_unlock(&scull_w_lock);

    return ret;
}

static int scull_w_release(struct inode* inode, struct file* filp)
{
    int ret = 0, temp;
    spin_lock(&scull_w_lock);
    scull_w_count--;
    if (scull_w_count == 0)
    {
        scull_w_owner = KUIDT_INIT(0);
    }
    temp = scull_w_count;
    spin_unlock(&scull_w_lock);
    if (temp == 0)
    {
        wake_up_interruptible_sync(&scull_w_wait);
    }
    return ret;
}

static struct file_operations scull_w_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_w_open,
    .release = scull_w_release,
};

static struct scull_adev_info
{
    const char* name;
    struct scull_dev* adev;
    struct file_operations* scull_a_fops;
} scull_adev_infos[] = {
    {"scull_single", &scull_s_dev, &scull_s_fops},
    {"scull_uid", &scull_u_dev, &scull_u_fops},
    {"scull_wuid", &scull_w_dev, &scull_w_fops}
};

//static int scull_access_nr_devs = SCULL_ACCESS_NR_DEVS;
static int scull_access_nr_devs = sizeof(scull_adev_infos) / sizeof(struct scull_adev_info);

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
    PDEBUG("hello, pdebug access, number of devices %d", scull_access_nr_devs);

    for (i = 0; i < scull_access_nr_devs; i++)
    {
        res = alloc_chrdev_region(&scull_access_devno[i], 0, 1, scull_adev_infos[i].name);

        if (res < 0)
        {
            printk(
                KERN_WARNING "scull: cannot get major dev no for this device : %s",
                scull_adev_infos[i].name);
            goto fail_region;
        }
        scull_adev_infos[i].adev->qset = scull_qset_n;
        scull_adev_infos[i].adev->quantum = scull_quantum;
        scull_adev_infos[i].adev->data = NULL;
        sema_init(&scull_adev_infos[i].adev->sem, 1);
        scull_access_setup_cdev(&scull_adev_infos[i], scull_access_devno[i]);
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
        unregister_chrdev_region(scull_access_devno[i], 1);
    }
}

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");