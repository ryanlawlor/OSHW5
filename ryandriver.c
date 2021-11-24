/* ryandriver.c */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/console_struct.h>

// names for registering class and device
#define DEVICE_NAME "led"
#define CLASS_NAME "ryan"
static struct class* ryandriverClass = NULL;
static struct device* ryandriverDevice = NULL;

// variables to set up for driver LED setting
extern int fg_console;
struct tty_driver *my_driver;

bool lighton = false; // boolean variable to determine if light is on or not

static ssize_t device_read(struct file *filp, char *buff, size_t length, loff_t *offset)
{
    //long int lightnum = 0;
    const char *s_ptr;
    int error_code = 0;
    ssize_t len = 0;

    // ((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDGETLED, lightnum);
    // printk(KERN_INFO "KDGETLED is: %ld\n", lightnum);

    // displaying ASCII lightbulb if CAPS-lock LED is on or not
    if (lighton)
    {                               //  _
        s_ptr = " _\n(*)\n =\n";    // (*)
    }                               //  =
    else
    {                               //  _
        s_ptr = " _\n(.)\n =\n";    // (.)
    }                               //  =
    
    // filling length varibale to return from the read function
    len = min( (unsigned long)(strlen(s_ptr) - *offset) , (unsigned long)(length) );
    if(len <= 0)
    {
        return 0;
    }

    //  copy the actual file contents from the string pointer (s_ptr) to display to the user
    error_code = copy_to_user(buff, s_ptr + *offset, len);
    printk("The file was read! len: %d \n", len);
    printk("Error code: %d\n", error_code);

    *offset += len;
    return len; // return the length with the read function
}

static ssize_t device_write(struct file *filp, const char __user *buff, size_t length, loff_t *offset)
{
    int error_code = 0;
    static char msg[10]; // sets up string to store the message that the user is writing
    printk(KERN_ALERT "Writing to LED file!\n");

    // copies the msg from the user into the msg variable
    error_code = copy_from_user(msg, buff, length);

    printk(KERN_INFO "Message is: %s\n", msg);

    // turns on CAPS-lock LED if the user writes "on" to the file
    if (strncmp(msg, "on", 2) == 0)
    {
        ((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0x4);
        lighton = true;
    }
    if (strncmp(msg, "off", 3) == 0) // turns off CAPS-lock LED if the user writes "off" to the file
    {
        ((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0x0);
        lighton = false;
    }
    if (strncmp(msg, "reset", 5) == 0) // resets the CAPS-lock LED if the user writes "reset" to the file
    {
        ((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0xFF);
        lighton = false;
    }

    return length;
}

static struct file_operations fops = {
    .read = device_read,
    .write = device_write
};

static int major; // declare major variable to use when registering file later
static int __init start(void)
{
    printk(KERN_ALERT "LED char-dev module loaded!\n");

    // create device files
    major = register_chrdev(0, "led", &fops);
    if(major < 0)
    {
        printk(KERN_ALERT "Failed to register char-dev file!\n");
        return major;
    }
    printk("New char-dev 'led' created with major %d and minor %d\n", major, 0);

    // register device class
    ryandriverClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ryandriverClass))
    {
        class_destroy(ryandriverClass);
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(ryandriverClass);
    }
    printk(KERN_INFO "Device class registered correctly\n");

    // register device driver
    ryandriverDevice = device_create(ryandriverClass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ryandriverDevice))
    {
        class_destroy(ryandriverClass);
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(ryandriverDevice);
    }
    printk(KERN_INFO "Device class created correctly\n");

    // initializing my driver
    my_driver = vc_cons[fg_console].d->port.tty->driver;

    printk(KERN_INFO "My_driver created correctly\n");

    return 0;
}

static void __exit end(void)
{
    // resets the CAPS-lock LED when exiting the module in case the user did not reset manually
    ((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0xFF);
    unregister_chrdev(major, "led");
    printk("LED char-dev module Unloaded!\n");
}

module_init(start);
module_exit(end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ryan Lawlor");
MODULE_DESCRIPTION("A Kernel module to change CAPS-lock LED");