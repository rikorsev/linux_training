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

static bool is_creat_node = false;
module_param(is_creat_node, bool, 0);

/* file operaion function prototypes */
static int     dev_open   (struct inode* node, struct file* f);
static int     dev_release(struct inode* node, struct file* f);
static ssize_t dev_read   (struct file* f, char __user* user_buff, size_t size, loff_t* loff);
static ssize_t dev_write  (struct file* f, const char __user* user_buff, size_t size, loff_t* loff);

static struct file_operations dev_fops = {
  .owner   = THIS_MODULE,
  .open    = dev_open,
  .release = dev_release,
  .read    = dev_read,
  .write   = dev_write,
};

struct lkt_cdev
{
  char* buff;
  size_t sz;
  struct cdev cdev;
};

/* pointer to cdev structure */
static struct lkt_cdev *lkt_cdev = NULL;
static struct class *lktm_class = NULL;
 
static int __init dev_register_num(int major, int num)
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

  /* if it is defined to use class for node creation */
  if(is_creat_node == true)
  {
    /* Create class to have opportunity to create device in devfs */
    lktm_class = class_create(THIS_MODULE, "lktm_class");
    if(IS_ERR(lktm_class))
    {
      result = PTR_ERR(lktm_class);

      /* Unregister character device major, minor numbers */
      unregister_chrdev_region(MKDEV(major, 0), num);
    }
  }

  return result;
}

static int __init lktm_cdev_init_single(struct lkt_cdev *lkt_cdev, int major, int index)
{
  int result = 0;
  dev_t devno = MKDEV(major, index);
  struct device *dev = NULL;

  lkt_cdev->cdev.owner = THIS_MODULE;

  /* if nodes should be created from kernel space */
  if(is_creat_node == true)
  {
    dev = device_create(lktm_class, NULL, devno, NULL, "lktm%d", index);
    if(IS_ERR(dev))
    {
      printk(KERN_WARNING "dev: can't create device. Result %d\n", result);
      return PTR_ERR(dev);
    }
  }

  /* init characted device with file operations */
  cdev_init(&lkt_cdev->cdev, &dev_fops);
      
  /* add character device to the system */
  result = cdev_add(&lkt_cdev->cdev, devno, 1);
  if(result < 0)
  {
    printk(KERN_WARNING "dev: device %d-%d - adding fail. Result: %d\n", major, index, result);
    
    /* Destroy device if it was created */
    if(is_creat_node == true)
    {
      device_destroy(lktm_class, devno);
    }
    
    return result;
  }

  printk(KERN_DEBUG "dev: device %d-%d - added successfully\n", major, index);

  return result;
}

static int __init lktm_cdev_init(int major, int num)
{
  int i = 0;
  int result = -1;

  /* Allocate memory for our character device */
  lkt_cdev = kzalloc(sizeof(struct lkt_cdev) * num, GFP_KERNEL);
  if(NULL == lkt_cdev)
  {
    printk(KERN_WARNING "dev: can't allocate memory for cdev\n");
    return -ENOMEM;
  }

  for(i = 0; i < num; i++)
  {
    /* init single lkt device */
    result = lktm_cdev_init_single(&lkt_cdev[i], major, i + 1);
    if(result < 0)
    {
      /* Free allocated memory */
      kfree(lkt_cdev);
      return result;
    }
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
    printk(KERN_WARNING "dev: can't register major %d\n. Result: %d", dev_major, result);
    return result;
  }

  printk(KERN_DEBUG "dev: registred %d devices,  major number %d\n", dev_num, dev_major);

  /* Init lktm character devices */
  return lktm_cdev_init(dev_major, dev_num);
}
module_init(dev_init);

static void __exit dev_exit(void)
{
  int i;

  printk(KERN_DEBUG "dev: clenup\n");

  for(i = 0; i < dev_num; i++)
  {
    /* Destroy created devfs device */
    if(is_creat_node == true)
    {
      device_destroy(lktm_class, MKDEV(dev_major, i));
    }

    /* Delete character device form system */
    cdev_del(&lkt_cdev[i].cdev);

    /* Free memory in buffers */
    if(lkt_cdev[i].buff != NULL) kfree(lkt_cdev[i].buff);
  }

  /* Destroy created class if necessary */
  if(is_creat_node == true)
  {
    class_destroy(lktm_class);
  }

  /* Free allocated memory */
  kfree(lkt_cdev);
    
  /* Unregister character device major, minor numbers */
  unregister_chrdev_region(MKDEV(dev_major, 0), dev_num);
  
  printk(KERN_DEBUG "dev: exit\n");
}
module_exit(dev_exit);

static int dev_open(struct inode* node, struct file* f)
{
  printk(KERN_DEBUG "dev: open\n");

  f->private_data = container_of(node->i_cdev, struct lkt_cdev, cdev);

  return 0; /* success */
}

static int dev_release(struct inode* node, struct file* f)
{
  printk(KERN_DEBUG "dev: release\n");

  return 0;
}

static ssize_t dev_read(struct file* f, char __user* user_buff, size_t size, loff_t* loff)
{
  struct lkt_cdev *dev = f->private_data;

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
  struct lkt_cdev *dev = f->private_data;

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