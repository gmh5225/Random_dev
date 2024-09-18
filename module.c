#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <asm/uaccess.h> 

#include "ff.h"         

#define DEVICE_NAME "Random_Numbers_Module"

#define SUCCESS 0
#define MAX_LENGTH 32

/* To forbidde multi-using */
enum {
        CDEV_NOT_USED = 0,
        CDEV_EXCLUSIVE_OPEN = 1,
};
static atomic_t cdev_status = ATOMIC_INIT(CDEV_NOT_USED);

static int major;
static struct class *cls;

static size_t k_length = 0;
static uint8_t crs_coefficients[MAX_LENGTH];
static uint8_t crs_elements[MAX_LENGTH];
static uint8_t crs_c = 0;

static ff_elem_t *ff_crs_c = NULL;
static ff_elem_t *ff_crs_coefficients[MAX_LENGTH];
static ff_elem_t *ff_crs_elements[MAX_LENGTH];

static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset);
static ssize_t my_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset);

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = my_open,
        .release = my_release,
        .write = my_write,
        .read = my_read,
};

module_param(k_length, ulong, 0);
MODULE_PARM_DESC(k_length, "Length of CRS");

module_param_array(crs_coefficients, byte, NULL, 0);
MODULE_PARM_DESC(crs_coefficients, "CRS coefficients: a0, a1, ..., a_{k-1}");

module_param_array(crs_elements, byte, NULL, 0);
MODULE_PARM_DESC(crs_elements, "Initial CRS elements: x0, x1, ..., x_{k-1}");

module_param(crs_c, byte, 0);
MODULE_PARM_DESC(crs_c, "CRS constant");

/* Init dev in Kernel */
static int __init my_init(void)
{
        /* From the guide  */
        major = register_chrdev(0, DEVICE_NAME, &fops);
        if (major < 0) 
        {
                pr_alert("FAIL MAJOR %d\n", major);
                return major;
        }
        pr_info("OK MAJOR%d\n", major);

        /* From the guide */
        cls = class_create(DEVICE_NAME);
        device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
        pr_info("OK DEVICE PATH /dev/%s\n", DEVICE_NAME);

        /* Convert recieving number into f_{2^8}*/
        for (size_t i = 0; i < k_length; i++) 
        {
                ff_crs_coefficients[i] = uint8_to_ff_elem(crs_coefficients[i]);
                ff_crs_elements[i] = uint8_to_ff_elem(crs_elements[i]);
        }
        ff_crs_c = uint8_to_ff_elem(crs_c);
        return SUCCESS;
}

static void __exit my_clean(void)
{       
        ff_elem_free(ff_crs_c);
        for (size_t i = 0; i < k_length; ++i) {
                ff_elem_free(ff_crs_coefficients[i]);
                ff_elem_free(ff_crs_elements[i]);
        }
        device_destroy(cls, MKDEV(major, 0));
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        pr_info("OK clear\n");
        return;
}

static ssize_t my_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset)
{
        /* We don't write, we run it through the Kernel parameters */
        pr_alert("ERROR CAN'T WRITE.\n");
        return -EINVAL; 
}

static int my_open(struct inode *inode, struct file *file)
{
        /*compare-and-exchange*/
        if (atomic_cmpxchg(&cdev_status, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
                return -EBUSY;
        }
        pr_info("OK OPEN\n");
        try_module_get(THIS_MODULE);
        return SUCCESS;
}

static int my_release(struct inode *inode, struct file *file)
{
        atomic_set(&cdev_status, CDEV_NOT_USED);
        module_put(THIS_MODULE);
        pr_info("OK CLOSE\n");
        return SUCCESS;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vadim Ploskarev");
MODULE_DESCRIPTION("Generator of random numbers.");