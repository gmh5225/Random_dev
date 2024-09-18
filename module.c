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