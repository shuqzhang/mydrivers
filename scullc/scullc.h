#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct scullc_qset
{
    void** data;
    struct scullc_qset* next;
};

struct scullc_dev
{
    struct scullc_qset* data;
    int quantum;
    int qset;
    unsigned int access_key;
    struct semaphore sem;
    struct cdev cdev;
    ssize_t size;
};

struct scullc_dev* scullc_devices;

#define USE_DEFAULT_CONF

#ifdef USE_DEFAULT_CONF
#define QSET_SIZE 1000
#define QUANTUM_SIZE 1024
#endif
