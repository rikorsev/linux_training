#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* to access to user space*/
#include <linux/moduleparam.h> /*header for manage params of module*/
#include <linux/cdev.h> /*contains cdev struct and functions to work with it*/
#include <linux/slab.h>

MODULE_LICENSE("GPL");

/* default device number */
static int dev_minor = 0;
static int dev_major = 0;
module_param(dev_major, int, 0);

/* default num of devices */
static int dev_num = 1;
module_param(dev_num, int, 0);

static void dev_register_num(void);
static void dev_register_cdev(void);

static ssize_t dev_read(struct file* f, char __user* user_buff, size_t size, loff_t* loff);
static ssize_t dev_write(struct file* f, const char __user* user_buff, size_t size, loff_t* loff);
static long dev_ioctl(struct file * f, unsigned int ioctl, unsigned long data);
static int dev_open(struct inode* node, struct file* f);
static int dev_release(struct inode* node, struct file* f);

typedef struct
{
  char* buff;
  size_t sz;
  struct cdev cdev;
}device_t;

/* pointer to cdev structure */
static device_t* device = NULL;

static struct file_operations dev_fops = {
  .owner = THIS_MODULE,
  .open = dev_open,
  .release = dev_release,
  .read = dev_read,
  .write = dev_write,
  .unlocked_ioctl = dev_ioctl
};

static int __init dev_init(void)
{
  printk(KERN_DEBUG "dev: init\n");

  dev_register_num();
  dev_register_cdev();
  
  return 0;
}
module_init(dev_init);

static void __exit dev_exit(void)
{
  int i = 0;
  
  printk(KERN_DEBUG "dev: clenup\n");

  unregister_chrdev(dev_major, "lktm");
  
  for(i = 0; i < dev_num; i++)
    {
      cdev_del(&device[i].cdev);
      kfree(&device[i]);
    }
  
  printk(KERN_DEBUG "dev: exit\n");
}
module_exit(dev_exit);

static void dev_register_num(void)
{
  dev_t dev = 0;
  int result = -1;
  
  if(dev_major != 0)
    {
      dev = MKDEV(dev_major, dev_minor);
      result = register_chrdev_region(dev, dev_num, "lktm");
    }
  else
    {
      result = alloc_chrdev_region(&dev, dev_minor, dev_num, "lktm");
      dev_major = MAJOR(dev);
    }

  if(result < 0)
    {
      printk(KERN_WARNING "dev: can't get major %d\n", dev_major);
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

  device = kmalloc(sizeof(device_t) * dev_num, GFP_KERNEL);

  if(NULL == device)
    {
      printk(KERN_WARNING "dev: can't allocate memory for cdev\n");
      return;
    }

  for(i = 0; i < dev_num; i++)
    {
      dev = MKDEV(dev_major, dev_minor + i);
      device[i].cdev.owner = THIS_MODULE;
      cdev_init(&device[i].cdev, &dev_fops);
      
      if (0 == cdev_add(&device[i].cdev, dev, 1))
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
  device_t* dev = f->private_data;

  printk(KERN_DEBUG "dev: read\n");
  
  if(NULL == dev->buff || 0 == dev->sz) return 0;

  if(size > dev->sz) size = dev->sz;
  
  size -= copy_to_user(user_buff, dev->buff, size);
  dev->sz -= size;

  return size;
}

static ssize_t dev_write(struct file* f, const char __user* user_buff, size_t size, loff_t* loff)
{
  device_t* dev = f->private_data;

  printk("dev: write\n");

  if(0 == size) return 0;
  
  if(NULL != dev->buff) kfree(dev->buff);

  dev->buff = kmalloc(size, GFP_KERNEL);
  if(NULL == dev->buff)
    {
      printk(KERN_WARNING "dev: memory allocation fail\n");
      return 0;
    }

  size -= copy_from_user(dev->buff, user_buff, size);
  dev->sz = size;

  return size;
  
}

static long dev_ioctl(struct file * f, unsigned int ioctl, unsigned long data)
{
  return 0;
}

static int dev_open(struct inode* node, struct file* f)
{
  device_t *dev; /* device information */

  printk(KERN_DEBUG "dev: open");

  dev = container_of(node->i_cdev, device_t, cdev);
  f->private_data = dev; /* for othre methods */

  return 0; /* success */
}

static int dev_release(struct inode* node, struct file* f)
{
    printk(KERN_DEBUG "dev: release\n");

    return 0;
}

