#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ioctl.h>

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
extern int scull_quantum;
extern int scull_qset_n;

void scull_create_proc(void);
void scull_remove_proc(void);

#define USE_DEFAULT_CONF

#ifdef USE_DEFAULT_CONF
#define QSET_SIZE 1000
#define QUANTUM_SIZE 1024
#endif

#ifndef SCULL_P_NR_DEVS
#define SCULL_P_NR_DEVS 4  /* scullpipe0 through scullpipe3 */
#endif
/*
 * The pipe device is a simple circular buffer. Here its default size
 */
#ifndef SCULL_P_BUFFER
#define SCULL_P_BUFFER 1000
#endif

/*
 * Prototypes for shared functions
 */

int scull_p_init(void);
void scull_p_cleanup(void);

/*
 * Ioctl definitions 
 */

/* Use 'k' as magic number */
#define SCULL_IOC_MAGIC  'k'

#define SCULL_IOCRESET    _IO(SCULL_IOC_MAGIC, 0)

#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC,  1, int)
#define SCULL_IOCSQSET    _IOW(SCULL_IOC_MAGIC,  2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIC,   3)
#define SCULL_IOCTQSET    _IO(SCULL_IOC_MAGIC,   4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC,  5, int)
#define SCULL_IOCGQSET    _IOR(SCULL_IOC_MAGIC,  6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIC,   7)
#define SCULL_IOCQQSET    _IO(SCULL_IOC_MAGIC,   8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET    _IOWR(SCULL_IOC_MAGIC,10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIC,  11)
#define SCULL_IOCHQSET    _IO(SCULL_IOC_MAGIC,  12)
#define SCULL_P_IOCTSIZE _IO(SCULL_IOC_MAGIC,   13)
#define SCULL_P_IOCQSIZE _IO(SCULL_IOC_MAGIC,   14)
/* ... more to come */

#define SCULL_IOC_MAXNR 14

#endif
