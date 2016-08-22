#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* to access to user space*/
#include <linux/moduleparam.h> /*header for manage params of module*/
#include <linux/cdev.h> /*contains cdev struct and functions to work with it*/

MODULE_LICENSE("GPL");

/* default device number */
static int dev_minor = 0;
static int dev_major = 0;
module_param(dev_major, int, 0);

/* default num of devices */
static int dev_num = 1;
module_param(dev_num, int, 0);

/* pointer to cdev structure */
static struct cdev* dev_cdev_ptr = NULL;

static struct file_operations dev_fops = {
  .owner = THIS_MODULE,
  .read = dev_read,
  .write = dev_write,
  .unlocked_ioctl = dev_ioctl
}

static int __init dev_init(void)
{
  printk(KERN_DEBUG "dev: init\n");

  dev_register_num();
  dev_register_cdev();
  
  return 0;
}

static void __exit dev_exit(void)
{
  printk(KERN_DEBUG "dev: clenup\n");
  
  printk(KERN_DEBUG "dev: exit\n");
}

static void dev_register_num(void)
{
  dev_t dev = 0;
  
  if(dev_major != 0)
    {
      dev = MKDEV(dev_major, dev_minor);
      result = register_chrdev_region(dev, dev_num, "lktm");
    }
  else
    {
      result = alloc_chrdev_region(&dev, dev_minor, dev_num, "lktm");
      scull_major = MAJOR(dev);
    }

  if(result < 0)
    {
      printk(KERN_WARNING "dev: can't get major %d\n", scull_major);
    }
  else
    {
      printk(KERN_DEBUG "dev: registred %d devices,  major number %d\n", dev_num, dev_major);
    }
}

static void dev_register_cdev(void)
{
  int i = 0;
  dev_t dev = 0;

  dev_cdev_ptr = kmalloc(sizeof(struct cdev) * dev_num, GFP_KERNEL);

  if(NULL == dev_cedv_ptr)
    {
      printk(KERN_WARNING "dev: can't allocate memory for cdev\n");
      return;
    }

  for(i = 0; i < dev_num; i++)
    {
      dev = MKDEV(dev_major, dev_minor + i);
      cedv_init(dev_cdev_ptr[i], &dev_fops);
      if (0 == cdev_add(dev_cedv_ptr[i], dev, 1))
	{
	  printk(KERN_DEBUG "dev: device %d-%d - added successfully\n", dev_major, dev_minor + i);
	}
      else
	{
	  printk(KERN_WARNING "dev: device %d-%d - adding fail\n", dev_major, dev_minor + i);
	}
    }
}

static ssize_t dev_read(struct file* f, char __user* user_buff, size_t size, loff_t* loff)
{
  
}

static ssize_t dev_write(struct file* f, const char __user* user_buff, size_t size, loff_t* loff)
{
  
}

static long dev_ioctl(struct file * f, unsigned int ioctl, unsigned long data)
{
  
}
