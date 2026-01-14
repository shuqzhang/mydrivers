#include "scullc.h"
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <asm/uaccess.h>

int scull_quantum = QUANTUM_SIZE;
int scull_qset_n = QSET_SIZE;
struct kmem_cache* scull_cache = NULL;

void scull_trim(struct scull_dev* dev)
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
                    if (scull_cache)
                    {
                        kmem_cache_free(scull_cache, data[i]);
                        continue;
                    }
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
    dev->qset = scull_qset_n;
    dev->quantum = scull_quantum;
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

ssize_t scull_read(struct file* filp, char __user *buff, size_t count, loff_t* f_pos)
{
    struct scull_dev* dev = filp->private_data;
    struct scull_qset* dptr = NULL;
    int quantum = dev->quantum;
    int qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;
    loff_t left_quatity = dev->size - *f_pos;

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
    
    count = (count > left_quatity) ? left_quatity : count;
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

ssize_t scull_write(struct file* filp, const char __user *buff, size_t count, loff_t* f_pos)
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
        dptr->data[s_pos] =
            scull_cache ? kmem_cache_alloc(scull_cache, GFP_KERNEL)
                        : kmalloc(quantum, GFP_KERNEL);
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

long scull_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
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
    switch (cmd)
    {
        case SCULL_IOCRESET:
            scull_quantum = QUANTUM_SIZE;
            scull_qset_n = QSET_SIZE;
            break;
        ///////////////////////// below for quantum ////////////////////////////
        case SCULL_IOCSQUANTUM: // set
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __get_user(scull_quantum, (int __user *)arg);
            break;
        case SCULL_IOCTQUANTUM: // tell
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            scull_quantum = arg;
            break;
        case SCULL_IOCGQUANTUM: // get
            retval = __put_user(scull_quantum, (int __user *)arg);
            break;
        case SCULL_IOCQQUANTUM: // query
            return scull_quantum;
        case SCULL_IOCXQUANTUM: // eXchange
            tmp = scull_quantum;
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __get_user(scull_quantum, (int __user *)arg);
            if (retval == 0)
                retval = __put_user(tmp, (int __user *)arg);
            break;
        case SCULL_IOCHQUANTUM: // sHift
            tmp = scull_quantum;
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            scull_quantum = arg;
            return tmp;
        ///////////////////////// below for qset ////////////////////////////
        case SCULL_IOCSQSET: // set
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __get_user(scull_qset_n, (int __user *)arg);
            break;
        case SCULL_IOCTQSET: // tell
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            scull_qset_n = arg;
            break;
        case SCULL_IOCGQSET: // get
            retval = __put_user(scull_qset_n, (int __user *)arg);
            break;
        case SCULL_IOCQQSET: // query
            return scull_qset_n;
        case SCULL_IOCXQSET: // eXchange
            tmp = scull_qset_n;
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            retval = __get_user(scull_qset_n, (int __user *)arg);
            if (retval == 0)
                retval = __put_user(scull_qset_n, (int __user *)arg);
            break;
        case SCULL_IOCHQSET: // sHift
            tmp = scull_qset_n;
            if (!capable(CAP_SYS_ADMIN))
                return -EPERM;
            scull_qset_n = arg;
            return tmp;        
        default:
            return -ENOTTY;

    }

    return retval;
}

MODULE_AUTHOR("shuqzhan <linuxshuqzhan@outlook.com>");
MODULE_LICENSE("GPL v2");
