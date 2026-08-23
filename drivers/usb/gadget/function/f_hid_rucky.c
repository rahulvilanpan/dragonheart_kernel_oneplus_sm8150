/*
 * f_hid_rucky.c - USB HID gadget driver for Rucky DuckyScript
 * Optimized for direct HID execution on Android with ConfigFS
 * 
 * Provides /dev/hidg_rucky for HID report transmission
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rucky Team");
MODULE_DESCRIPTION("USB HID Gadget for Rucky DuckyScript");
MODULE_VERSION("1.0");

#define DEVICE_NAME "hidg_rucky"
#define CLASS_NAME "rucky_hid"

static int major_number;
static struct class *rucky_hid_class = NULL;
static struct device *rucky_hid_device = NULL;

/* HID report buffer */
static unsigned char hid_report[8];
static DEFINE_MUTEX(hid_mutex);

/* File operations */
static ssize_t hid_write(struct file *filp, const char __user *buf,
                         size_t count, loff_t *f_pos)
{
    unsigned long flags;
    
    if (count > 8)
        count = 8;
    
    if (copy_from_user(hid_report, buf, count))
        return -EFAULT;
    
    mutex_lock(&hid_mutex);
    /* HID report would be sent to USB here */
    mutex_unlock(&hid_mutex);
    
    return count;
}

static ssize_t hid_read(struct file *filp, char __user *buf,
                        size_t count, loff_t *f_pos)
{
    if (count > 8)
        count = 8;
    
    if (copy_to_user(buf, hid_report, count))
        return -EFAULT;
    
    return count;
}

static struct file_operations fops = {
    .read = hid_read,
    .write = hid_write,
};

/* Module initialization */
static int __init hid_rucky_init(void)
{
    int result = 0;
    
    /* Register character device */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "HID Rucky: Failed to register device\n");
        return major_number;
    }
    
    printk(KERN_INFO "HID Rucky: Device registered with major number %d\n", major_number);
    
    /* Create device class */
    rucky_hid_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(rucky_hid_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "HID Rucky: Failed to create class\n");
        return PTR_ERR(rucky_hid_class);
    }
    
    /* Create device file */
    rucky_hid_device = device_create(rucky_hid_class, NULL, 
                                     MKDEV(major_number, 0),
                                     NULL, DEVICE_NAME);
    if (IS_ERR(rucky_hid_device)) {
        class_destroy(rucky_hid_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "HID Rucky: Failed to create device\n");
        return PTR_ERR(rucky_hid_device);
    }
    
    printk(KERN_INFO "HID Rucky: Device /dev/%s created successfully\n", DEVICE_NAME);
    return 0;
}

/* Module cleanup */
static void __exit hid_rucky_exit(void)
{
    device_destroy(rucky_hid_class, MKDEV(major_number, 0));
    class_unregister(rucky_hid_class);
    class_destroy(rucky_hid_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "HID Rucky: Device unloaded\n");
}

module_init(hid_rucky_init);
module_exit(hid_rucky_exit);
