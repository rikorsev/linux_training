#include <linux/module.h>      /*this header is obvios for module development*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>          /* contains a lot of useful things such as file_operations structure prototypes of function to register/allocate device number etc.*/
#include <linux/uaccess.h>       /* to access to user space*/
#include <linux/moduleparam.h> /* header for manage params of module*/
#include <linux/cdev.h>        /* contains cdev struct and functions to work with it*/
#include <linux/slab.h>        /* for kalloc function */

MODULE_AUTHOR("Oleksandr Kolosov");
MODULE_LICENSE("GPL");

/* default device number */
static int dev_minor = 0;
static int dev_major = 0;
module_param(dev_major, int, 0);

/* default num of devices */
static int dev_num = 1;
module_param(dev_num, int, 0);

/* file operaion function prototypes */
static int     dev_open   (struct inode* node, struct file* f);
static int     dev_release(struct inode* node, struct file* f);
static ssize_t dev_read   (struct file* f, char __user* user_buff, size_t size, loff_t* loff);
static ssize_t dev_write  (struct file* f, const char __user* user_buff, size_t size, loff_t* loff);

typedef struct
{
  //  struct kobject* kobj;
  char* buff;
  size_t sz;
  struct cdev cdev;
}device_t;

/* pointer to cdev structure */
static device_t* device = NULL;

static struct file_operations dev_fops = {
  .owner   = THIS_MODULE,
  .open    = dev_open,
  .release = dev_release,
  .read    = dev_read,
  .write   = dev_write,
};

static int dev_register_num(int major, int num)
{
  dev_t dev = 0;
  int result = -1;
  
  /* There is two ways of character device registring */
  if(major != 0)
  {
    /* Registre defined region of major and minor numbers */
    dev = MKDEV(major, 0);
    result = register_chrdev_region(dev, num, "lktm");
  }
  else
  {
    /* Allocate free region of minor and major numbers */
    result = alloc_chrdev_region(&dev, dev_minor, dev_num, "lktm");
    dev_major = MAJOR(dev);
  }

  return result;
}

static int dev_register_cdev(int major, int num)
{
  int i = 0;
  int result = -1;
  dev_t devno = 0;

  /* Allocate memory for our character device */
  device = kmalloc(sizeof(device_t) * num, GFP_KERNEL);
  if(NULL == device)
  {
    printk(KERN_WARNING "dev: can't allocate memory for cdev\n");
    return -ENOMEM;
  }

  for(i = 0; i < num; i++)
  {
    devno = MKDEV(major, i);
    device[i].cdev.owner = THIS_MODULE;

    /* init characted device with file operations */
    cdev_init(&device[i].cdev, &dev_fops);
      
    /* add character device to the system */
    result = cdev_add(&device[i].cdev, devno, 1);
    if(result < 0)
    {
      printk(KERN_WARNING "dev: device %d-%d - adding fail\n", major, i);
      break;
    }

    printk(KERN_DEBUG "dev: device %d-%d - added successfully\n", major, i);
  }

  return result;
}

static int __init dev_init(void)
{
  int result = -1;

  printk(KERN_DEBUG "dev: init\n");

  /* Register majer and minor numbers for our device */
  result = dev_register_num(dev_major, dev_num);
  if(result < 0)
  {
    printk(KERN_WARNING "dev: can't get major %d\n", dev_major);
    return result;
  }

  printk(KERN_DEBUG "dev: registred %d devices,  major number %d\n", dev_num, dev_major);

  /* Register character device*/
  return dev_register_cdev(dev_major, dev_num);
}
module_init(dev_init);

static void __exit dev_exit(void)
{
  int i;

  printk(KERN_DEBUG "dev: clenup\n");

  for(i = 0; i < dev_num; i++)
  {
    /* Delete character device form system */
    cdev_del(&device[i].cdev);

    /* Free memory in buffers */
    if(device[i].buff != NULL) kfree(device[i].buff);
  }

  /* Free allocated memory */
  kfree(device);

  /* Unregister character device major, minor numbers */
  unregister_chrdev(dev_major, "lktm");
  
  printk(KERN_DEBUG "dev: exit\n");
}
module_exit(dev_exit);

static int dev_open(struct inode* node, struct file* f)
{
  printk(KERN_DEBUG "dev: open\n");

  f->private_data = container_of(node->i_cdev, device_t, cdev);

  return 0; /* success */
}

static int dev_release(struct inode* node, struct file* f)
{
  printk(KERN_DEBUG "dev: release\n");

  return 0;
}

static ssize_t dev_read(struct file* f, char __user* user_buff, size_t size, loff_t* loff)
{
  device_t* dev = f->private_data;

  printk(KERN_DEBUG "dev: read\n");
  printk(KERN_DEBUG "dev: dev->sz: %d, size: %d", dev->sz, size);
  
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