#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#undef PDEBUG
#ifdef SCULL_DEBUG
    //#ifdef __KERNEL__
    #define PDEBUG(fmt, args...) printk(KERN_DEBUG "scull: "fmt"\n", ##args);
    //#else
    //#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args);
    //#endif
#else
    #define PDEBUG(fmt, args...)
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...)

struct scull_qset
{
    void** data;
    struct scull_qset* next;
};

struct scull_dev
{
    struct scull_qset* data;
    int quantum;
    int qset;
    unsigned int access_key;
    struct semaphore sem;
    struct cdev cdev;
    ssize_t size;
};

extern struct scull_dev* scull_devices;
extern int scull_nr_devs;
extern int scull_major;
extern int scull_minor;

void scull_create_proc(void);
void scull_remove_proc(void);

#define USE_DEFAULT_CONF

#ifdef USE_DEFAULT_CONF
#define QSET_SIZE 1000
#define QUANTUM_SIZE 1024
#endif
