//#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <asm/hardirq.h>

#undef PDEBUG
#define PDEBUG(fmt, args...) printk(KERN_DEBUG "jiq: "fmt"\n", ##args);

MODULE_AUTHOR("Shuqing Zhang");
MODULE_LICENSE("Dual BSD/GPL");

enum jiq_files {
    JIT_WQ = 0,
	JIT_WQ_DELAY = 1
};

struct jiq_proc_data {
    enum jiq_files jiq_file;
    ssize_t (*jiq_fn)(unsigned long data, char __user *buffer, size_t count, loff_t *ppos);
};

struct client_data {
    int left_loops;
    unsigned long prevjiffies;
    int delay;
    wait_queue_head_t wq;
    struct work_struct jiq_work;
    struct delayed_work jiq_delayed;
    char* buf;
};
static const unsigned interval = 1;
static struct client_data cdata;

static int jiq_print(struct client_data* data)
{
    unsigned long j = jiffies;
    data->left_loops--;
    data->buf += sprintf(data->buf, "%li  %li   %i   %i   %i   %s\n",
        j, j-data->prevjiffies, in_interrupt() ? 1 : 0, current->pid,
        smp_processor_id(), current->comm);
    data->prevjiffies = j;
    return data->left_loops;
}

static void jiq_print_wq(struct work_struct *work)
{
    struct client_data* data = container_of(work, struct client_data, jiq_work);

    if (! jiq_print(data))
    {
        wake_up_interruptible(&data->wq);
        return;
    }
    schedule_work(work);
}

static void jiq_print_delayed_wq(struct work_struct *work)
{
    struct client_data* data = container_of(work, struct client_data, jiq_delayed.work);

    if (! jiq_print(data))
    {
        wake_up_interruptible(&data->wq);
        return;
    }
    schedule_delayed_work(&data->jiq_delayed, data->delay);
}

static void schedule_wq(struct client_data* data, unsigned long param)
{
    if (param == JIT_WQ)
    {
        schedule_work(&data->jiq_work);
        return;
    }
    schedule_delayed_work(&data->jiq_delayed, data->delay);
}

static ssize_t jiq_wq(unsigned long data, char __user *buffer, size_t count, loff_t *ppos)
{
    ssize_t len;
    char* sbuf = NULL;
    DEFINE_WAIT(wait);

    sbuf = (char *)kmalloc(2400, GFP_KERNEL);
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

    INIT_WORK(&cdata.jiq_work, jiq_print_wq);
    INIT_DELAYED_WORK(&cdata.jiq_delayed, jiq_print_delayed_wq);

    cdata.left_loops = 5;
    cdata.prevjiffies = jiffies;
    cdata.delay = (data == JIT_WQ) ? 0 : 1;
    cdata.buf = sbuf;
    init_waitqueue_head(&cdata.wq);

    cdata.buf += sprintf(sbuf, "time   delta   preempt    pid    cpu    command\n");
    prepare_to_wait(&cdata.wq, &wait, TASK_INTERRUPTIBLE);
    schedule_wq(&cdata, data);
    schedule();
    finish_wait(&cdata.wq, &wait);
    len = cdata.buf - sbuf;
    if (copy_to_user(buffer, sbuf, len))
    {
        kfree(sbuf);
        return -EFAULT;
    }
    *ppos += len;
    kfree(sbuf);
    return len;
}

struct jiq_proc_data jd[] = {
    {JIT_WQ, jiq_wq},
    {JIT_WQ_DELAY, jiq_wq},
};

static int proc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long data = (unsigned long)PDE_DATA(file_inode(file));
    int i = 0;
    for (; i < sizeof(jd) / sizeof(struct jiq_proc_data); i++)
    {
        if (jd[i].jiq_file == data)
        {
            return jd[i].jiq_fn(data, buffer, count, ppos);
        }
    }
    PDEBUG("Not support jiq file: %lu", data);
    return jiq_wq(data, buffer, count, ppos);
}

static const struct proc_ops jiq_ops = {
	.proc_open  = proc_open,
	.proc_read  = proc_read,
	.proc_lseek = noop_llseek,
};

int create_various_proc_files(const char* file_name, void* proc_parameter)
{
    int success_or_fail = 0;
    struct proc_dir_entry *ent;
    ent = proc_create_data(file_name, S_IRUGO|S_IWUSR, NULL, &jiq_ops, proc_parameter);
    // PDEBUG("debug 4 proc_parameter = 0x%ld", (long)proc_parameter);
    if (!ent)
    {
        PDEBUG("Failed to create proc entry for jiq.");
        success_or_fail = -EFAULT;
    }
    return success_or_fail;
}

int __init jiq_init(void)
{
    int ret = 0;
    create_various_proc_files("jiqwq", (void*)JIT_WQ);
    create_various_proc_files("jiqwqdelay", (void*)JIT_WQ_DELAY);

    return ret;
}

void __exit jiq_cleanup(void)
{
    remove_proc_entry("jiqwq", NULL);
    remove_proc_entry("jiqwqdelay", NULL);
}

module_init(jiq_init);
module_exit(jiq_cleanup);
