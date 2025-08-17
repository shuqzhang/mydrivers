//#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <asm/hardirq.h>

#undef PDEBUG
#define PDEBUG(fmt, args...) printk(KERN_DEBUG "jit: "fmt"\n", ##args);

int delay = HZ;  // default delay 1 seconds

module_param(delay, int, 0);

MODULE_AUTHOR("Shuqing Zhang");
MODULE_LICENSE("Dual BSD/GPL");

enum jit_files {
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO
};

// function print the jiffies before and after execute it.
// different ways to get one seconds delay.
static ssize_t jit_fn(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long j0, j1;
    unsigned long delay;
    ssize_t len;
    char* sbuf = NULL;
    wait_queue_head_t wq;
    unsigned long data = (unsigned long)PDE_DATA(file_inode(file));
    init_waitqueue_head(&wq);

    sbuf = (char *)kmalloc(24, GFP_KERNEL);
    if (!sbuf)
    {
        PDEBUG("failed to malloc for sbuf.");
        return 0;
    }

    if (*ppos >= count)
    {
        kfree(sbuf);
        return 0;
    }

    j0 = jiffies;
    j1 = j0 + HZ;
    switch (data)
    {
        case JIT_BUSY:
            while(time_before(jiffies, j1))
            {
                cpu_relax();
            }
            break;
        case JIT_SCHED:
            while(time_before(jiffies, j1))
            {
                schedule();
            }
            break;
        case JIT_QUEUE:
            wait_event_interruptible_timeout(wq, 0, HZ);
            break;
        case JIT_SCHEDTO:
            set_current_state(TASK_INTERRUPTIBLE);
            delay = HZ;
            while (0 < delay)
            {
                delay = schedule_timeout(delay);
            }
            break;
        default:
            break;
    }
    j1 = jiffies;
    len = sprintf(sbuf, "%li %li\n", j0, j1);
    if (copy_to_user(buffer, sbuf, len))
    {
        kfree(sbuf);
        return -EFAULT;
    }
    *ppos += len;
    kfree(sbuf);
    return len;
}

static int proc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct proc_ops jit_ops = {
	.proc_open  = proc_open,
	.proc_read  = jit_fn,
	.proc_lseek = noop_llseek,
};

int create_various_proc_files(const char* file_name, void* proc_parameter)
{
    int success_or_fail = 0;
    struct proc_dir_entry *ent;
    ent = proc_create_data(file_name, S_IRUGO|S_IWUSR, NULL, &jit_ops, proc_parameter);
    if (!ent)
    {
        PDEBUG("Failed to create proc entry for jit.");
        success_or_fail = -EFAULT;
    }
    return success_or_fail;
}

int __init jit_init(void)
{
    int ret = 0;
    create_various_proc_files("jitbusy", (void*)JIT_BUSY);
    create_various_proc_files("jitsched", (void*)JIT_SCHED);
    create_various_proc_files("jitqueue", (void*)JIT_QUEUE);
    create_various_proc_files("jitschedto", (void*)JIT_SCHEDTO);

    return ret;
}

void __exit jit_cleanup(void)
{
    remove_proc_entry("jitbusy", NULL);
}

module_init(jit_init);
module_exit(jit_cleanup);
