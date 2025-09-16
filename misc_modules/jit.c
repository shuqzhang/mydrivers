//#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include <asm/hardirq.h>

#undef PDEBUG
#define PDEBUG(fmt, args...) printk(KERN_DEBUG "jit: "fmt"\n", ##args);

int delay = HZ;  // default delay 1 seconds

module_param(delay, int, 0);

MODULE_AUTHOR("Shuqing Zhang");
MODULE_LICENSE("Dual BSD/GPL");

enum jit_files {
    JIT_CURRENT,
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO,
    JIT_TIMER
};

struct jit_data {
    enum jit_files jit_file;
    ssize_t (*jit_fn)(unsigned long data, char __user *buffer, size_t count, loff_t *ppos);
};

struct jit_timer_data {
    unsigned left_loops;
    unsigned long prevjiffies;
    struct timer_list timer;
    wait_queue_head_t wq;
    char* buf;
};
static const unsigned interval = 100;
static struct jit_timer_data jitimer_data;

static ssize_t jit_current(unsigned long data, char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long j1;
    u64 j2;
    struct timespec64 tv;
    ssize_t len;
    char* sbuf = NULL;

    sbuf = (char *)kmalloc(240, GFP_KERNEL);
    if (!sbuf)
    {
        PDEBUG("failed to malloc for sbuf.");
        return 0;
    }
    if (*ppos > 0) // just shown once
    {
        kfree(sbuf);
        return 0;
    }
    j1 = jiffies;
    j2 = get_jiffies_64();
    ktime_get_real_ts64(&tv);

    len = sprintf(sbuf, "j1 = 0x%08lx, j2 = 0x%016Lx\n"
        "%40i %09i\n",
        j1, j2,
        (int)tv.tv_sec, (int)tv.tv_nsec);
    if (copy_to_user(buffer, sbuf, len))
    {
        kfree(sbuf);
        return -EFAULT;
    }
    *ppos += len;
    kfree(sbuf);
    return len;
}

// function print the jiffies before and after execute it.
// different ways to get one seconds delay.
static ssize_t jit_delay(unsigned long data, char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long j0, j1;
    unsigned long delay;
    ssize_t len;
    char* sbuf = NULL;
    wait_queue_head_t wq;
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

static void jit_timer_fn(struct timer_list* arg)
{
    struct jit_timer_data* data = from_timer(data, arg, timer);
    unsigned long j = jiffies;
    data->buf += sprintf(data->buf, "%li  %4li  %i   %6i    %i    %s\n",
        j, j-data->prevjiffies, in_interrupt() ? 1 : 0, current->pid,
        smp_processor_id(), current->comm);

    if (--data->left_loops > 0)
    {
        data->prevjiffies = j;
        data->timer.expires = j + interval;
        add_timer(&data->timer);
    }
    else
    {
        wake_up_interruptible(&data->wq);
    }
}

static ssize_t jit_timer(unsigned long data, char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long j = 0;
    char* sbuf = NULL;
    ssize_t len = 0;

    sbuf = (char *)kmalloc(2400, GFP_KERNEL);
    if (!sbuf)
    {
        PDEBUG("failed to malloc for sbuf.");
        return 0;
    }
    j = jiffies;
    jitimer_data.buf = sbuf;
    jitimer_data.buf += sprintf(sbuf, "time        duration     inirq    pid       cpu      proc\n");
    jitimer_data.left_loops = 10;
    jitimer_data.prevjiffies = j;
    init_waitqueue_head(&jitimer_data.wq);
    timer_setup(&jitimer_data.timer, jit_timer_fn, 0);
    jitimer_data.timer.expires = j + interval;
    add_timer(&jitimer_data.timer);

    wait_event_interruptible(jitimer_data.wq, !jitimer_data.left_loops);
    len = strlen(sbuf);
    if (copy_to_user(buffer, sbuf, len))
    {
        kfree(sbuf);
        return -EFAULT;
    }
    return len;
}

struct jit_data jd[] = {
    {JIT_CURRENT, jit_current},
    {JIT_BUSY, jit_delay},
    {JIT_QUEUE, jit_delay},
    {JIT_SCHED, jit_delay},
    {JIT_SCHEDTO, jit_delay},
    {JIT_TIMER, jit_timer},
};

static int proc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long data = (unsigned long)PDE_DATA(file_inode(file));
    int i = 0;
    for (; i < sizeof(jd) / sizeof(struct jit_data); i++)
    {
        if (jd[i].jit_file == data)
        {
            return jd[i].jit_fn(data, buffer, count, ppos);
        }
    }
    PDEBUG("Not support jit file: %lu", data);
    return jit_current(data, buffer, count, ppos);
}

static const struct proc_ops jit_ops = {
	.proc_open  = proc_open,
	.proc_read  = proc_read,
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
    create_various_proc_files("jitcurrent", (void*)JIT_CURRENT);
    create_various_proc_files("jitbusy", (void*)JIT_BUSY);
    create_various_proc_files("jitsched", (void*)JIT_SCHED);
    create_various_proc_files("jitqueue", (void*)JIT_QUEUE);
    create_various_proc_files("jitschedto", (void*)JIT_SCHEDTO);
    create_various_proc_files("jitimer", (void*)JIT_TIMER);

    return ret;
}

void __exit jit_cleanup(void)
{
    remove_proc_entry("jitcurrent", NULL);
    remove_proc_entry("jitbusy", NULL);
    remove_proc_entry("jitsched", NULL);
    remove_proc_entry("jitqueue", NULL);
    remove_proc_entry("jitschedto", NULL);
    remove_proc_entry("jitimer", NULL);
}

module_init(jit_init);
module_exit(jit_cleanup);
