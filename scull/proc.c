#include "scull.h"
#include <linux/seq_file.h>

static int proc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    struct scull_dev *devices = PDE_DATA(file_inode(file));
    int i, j, len = 0;
    // int limit = count - 80;
    char *sbuf = NULL;
    const int max_pages = 20;
    const int page_size = 4096;
    PDEBUG("scull_devices %px ppos %lld", devices, *ppos);

    sbuf = (char *)kmalloc(max_pages * page_size, GFP_KERNEL);
    if (!sbuf)
    {
        PDEBUG("failed to malloc for sbuf.");
        return 0;
    }
    for (i = 0; i < scull_nr_devs; i++)
    {
        struct scull_dev* d = &devices[i];
        struct scull_qset *qs = d->data;
        if (down_interruptible(&d->sem))
        {
            kfree(sbuf);
            return -ERESTARTSYS;
        }
        len += sprintf(sbuf + len, "\n Device %d: qset %d, quantum %d, sz %ld\n",
            i, d->qset, d->quantum, d->size);
        for (; qs; qs = qs->next)
        {
            len += sprintf(sbuf + len, "    item at %px, qset at %px\n", qs, qs->data);
            if (qs->data && !qs->next)
            {
                for (j = 0; j < d->qset; j++)
                {
                    len += sprintf(sbuf + len, "    %4d: %px\n", j, qs->data[j]);
                }
            }
        }

        up(&d->sem);
    }
    if (*ppos >= len)
    {
        kfree(sbuf);
        return 0;
    }
    PDEBUG("ppos %lld len %d count %ld", *ppos, len, count);
    if (*ppos + count > len)
    {
        count = len - *ppos;
    }
    if (copy_to_user(buffer, sbuf + *ppos, count))
    {
        kfree(sbuf);
        return -EFAULT;
    }

    *ppos += count; // not needed ? 
    PDEBUG("ppos %lld len %d count %ld", *ppos, len, count);
    kfree(sbuf);
    return count;
}

static const struct proc_ops scullmem_ops = {
	.proc_open  = proc_open,
	.proc_read  = proc_read,
	.proc_lseek = noop_llseek,
};

void * scull_seq_start(struct seq_file *m, loff_t *pos)
{
    if (*pos >= scull_nr_devs)
    {
        return NULL;
    }
    return scull_devices + *pos;
}

void scull_seq_stop(struct seq_file *m, void *v)
{

}

void * scull_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    PDEBUG("113");
    (*pos)++;
    if (*pos >= scull_nr_devs)
    {
        return NULL;
    }
    return scull_devices + *pos;
}



int scull_seq_show(struct seq_file *m, void *v)
{
    struct scull_dev* dev = (struct scull_dev *)v;
    struct scull_qset *d = (struct scull_qset *)dev->data;
    int i;
    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }
    seq_printf(m, "\nDevice %i : qset %i, q %i, sz %li\n",
        (int)(dev - scull_devices), dev->qset, dev->quantum, dev->size);
    for (; d; d = d->next)
    {
        seq_printf(m, "     item at %px qset at %px\n", d, d->data);
        if (d->data && !d->next)
        {
            for (i = 0; i < dev->qset; i++)
            {
                if (d->data[i])
                {
                    seq_printf(m, "             %4d: %px", i, d->data[i]);
                }
            }
        }
    }
    up(&dev->sem);
    return 0;
}

static const struct seq_operations scull_seq_ops = {
    .start = scull_seq_start,
    .next = scull_seq_next,
    .stop = scull_seq_stop,
    .show = scull_seq_show
};

static int scull_proc_open(struct inode* inode, struct file* file)
{
    PDEBUG("111");
    return seq_open(file, &scull_seq_ops);
}

static const struct proc_ops scull_proc_ops = {
    .proc_open = scull_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

void scull_create_proc(void)
{
    struct proc_dir_entry *ent;
    ent = proc_create_data("scullmem", S_IRUGO|S_IWUSR, NULL,
		&scullmem_ops, (void*)scull_devices); /* scull proc entry */
	if (!ent)
    {
        PDEBUG("Failed to create proc entry for scull.");
    }
    ent = proc_create("scullseq", 0, NULL, &scull_proc_ops);
    if (!ent)
    {
        PDEBUG("Failed to create proc entry(scullseq) for scull.");
    }
}

void scull_remove_proc(void)
{
    remove_proc_entry("scullmem", NULL);
    remove_proc_entry("scullseq", NULL);
}