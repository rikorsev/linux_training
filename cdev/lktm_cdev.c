#include <linux/module.h>      /*this header is obvios for module development*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>          /* contains a lot of useful things such as file_operations structure prototypes of function to register/allocate device number etc.*/
#include <linux/uaccess.h>       /* to access to user space*/
#include <linux/moduleparam.h> /* header for manage params of module*/
#include <linux/cdev.h>        /* contains cdev struct and functions to work with it*/
#include <linux/slab.h>        /* for kalloc function */
#include <linux/kfifo.h>       /* for kfifo functions */

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

/* fifo len */
static int fbuff_sz = 1024;
module_param(fbuff_sz, int , 0);

/* file operaion function prototypes */
static int     dev_open     (struct inode* node, struct file* f);
static int     dev_release  (struct inode* node, struct file* f);
static ssize_t dev_read     (struct file* f, char __user* user_buff, size_t size, loff_t* loff);
static ssize_t dev_write    (struct file* f, const char __user* user_buff, size_t size, loff_t* loff);
static ssize_t dev_write_bnb(struct file* f, const char __user* user_buff, size_t size, loff_t* loff);
static ssize_t dev_read_bnb (struct file* f, char __user* user_buff, size_t size, loff_t* loff);
static long    dev_ioctl    (struct file* f, unsigned int cmd, unsigned long arg);

static struct file_operations dev_fops = {
  .owner          = THIS_MODULE,
  .open           = dev_open,
  .release        = dev_release,
  .read           = dev_read,
  .write          = dev_write,
  .unlocked_ioctl = dev_ioctl
};

static struct file_operations dev_fops_bnb = {
  .owner          = THIS_MODULE,
  .open           = dev_open,
  .release        = dev_release,
  .read           = dev_read_bnb,
  .write          = dev_write_bnb,
  .unlocked_ioctl = dev_ioctl
};

struct lkt_cdev
{
  struct            cdev cdev;
  char*             buff;
  size_t            sz;
  u32               value;
  struct            kfifo fifo;
  char              *fifo_buf;
  wait_queue_head_t waitq_w;
  wait_queue_head_t waitq_r;
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

  /* Init wait queue */
  init_waitqueue_head(&lkt_cdev->waitq_w);
  init_waitqueue_head(&lkt_cdev->waitq_r);

  /* Allocate and init fifo */
  result = kfifo_alloc(&lkt_cdev->fifo, fbuff_sz, GFP_KERNEL);
  if(result < 0)
  {
    printk(KERN_WARNING "dev%d: fail to allocate fifo. Result: %d", index, result);
    return result;
  }

  /* if nodes should be created from kernel space */
  if(is_creat_node == true)
  {
    dev = device_create(lktm_class, NULL, devno, NULL, "lktm%d", index);
    if(IS_ERR(dev))
    {
      printk(KERN_WARNING "dev%d: can't create device. Result %d\n", index, result);
      result = PTR_ERR(dev);
      goto free_fifo;
    }
  }

  /* init characted device with file operations */
  cdev_init(&lkt_cdev->cdev, &dev_fops);
      
  /* add character device to the system */
  result = cdev_add(&lkt_cdev->cdev, devno, 1);
  if(result < 0)
  {
    printk(KERN_WARNING "dev%d: adding fail. Result: %d\n", index, result);
    goto destroy_device;
  }

  printk(KERN_DEBUG "dev%d: added successfully\n", index);

  return result;

destroy_device:
  /* Destroy device if it was created */
  if(is_creat_node == true)
  {
    device_destroy(lktm_class, devno);
  }

free_fifo:
  kfifo_free(&lkt_cdev->fifo);

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
    /* Free fifo */
    kfifo_free(&lkt_cdev[i].fifo);
    
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
  dev_t devno = node->i_cdev->dev;
  printk(KERN_DEBUG "dev%d: open\n", MINOR(devno));

  /* put pointer to our lkt_cdev structure in privat data of file */
  f->private_data = container_of(node->i_cdev, struct lkt_cdev, cdev);

  return 0; /* success */
}

static int dev_release(struct inode* node, struct file* f)
{
  dev_t devno = node->i_cdev->dev;
  printk(KERN_DEBUG "dev%d: release\n", MINOR(devno));

  return 0;
}

static ssize_t dev_read(struct file* f, char __user* user_buff, size_t size, loff_t* loff)
{
  struct lkt_cdev *dev = f->private_data;
  int minor = MINOR(dev->cdev.dev);

  printk(KERN_DEBUG "dev%d: read\n", minor);
  printk(KERN_DEBUG "dev%d: dev->sz: %d, size: %d", minor, dev->sz, size);
  
  if(NULL == dev->buff || 0 == dev->sz) return 0;

  if(size > dev->sz) size = dev->sz;
  
  size -= copy_to_user(user_buff, dev->buff, size);
  dev->sz -= size;

  return size;
}

static ssize_t dev_write(struct file* f, const char __user* user_buff, size_t size, loff_t* loff)
{
  struct lkt_cdev *dev = f->private_data;
  int minor = MINOR(dev->cdev.dev);

  printk(KERN_DEBUG "dev%d: write\n", minor);

  if(0 == size) return 0;
  
  if(NULL != dev->buff) kfree(dev->buff);

  dev->buff = kmalloc(size, GFP_KERNEL);
  if(NULL == dev->buff)
    {
      printk(KERN_WARNING "dev%d: memory allocation fail\n", minor);
      return 0;
    }

  size -= copy_from_user(dev->buff, user_buff, size);
  dev->sz = size;

  return size;
}

static ssize_t dev_write_bnb(struct file* f, const char __user* user_buff, size_t size, loff_t* loff)
{
  struct lkt_cdev *dev = f->private_data;
  int minor            = MINOR(dev->cdev.dev);
  int space            = 0;
  size_t copied        = 0;
  int result           = -EFAULT;

  printk(KERN_DEBUG "dev%d: write\n", minor);

  /* if operation is non blocked */
  if (f->f_flags & O_NONBLOCK)
  {
    if(kfifo_is_full(&dev->fifo) == true)
    {
      return -EAGAIN;
    }
  }
  /* if it is blocked */
  else
  {
    /* check if fifo is full */
    /* ATTENTION: pass queue by value, not by address */
    result = wait_event_interruptible(dev->waitq_w, kfifo_is_full(&dev->fifo) != true);
    if(result < 0)
    {
      printk(KERN_DEBUG "dev%d: write queue interrupted by signal\n", minor);
      return result;
    }
  }

  printk(KERN_DEBUG "dev%d: has free space in fifo\n", minor);

  space = kfifo_avail(&dev->fifo);

  size = size < space ? size : space;

  result = kfifo_from_user(&dev->fifo, user_buff, size, &copied);
  if(result < 0)
  {
    printk(KERN_WARNING "dev%d: fail to copy data from user space. Result: %d\n", minor, result);
    return result;
  }

  /* wake up read queue */
  if(copied > 0)
  {
      wake_up_interruptible(&dev->waitq_r);
  }

  printk(KERN_DEBUG "dev%d: copied: %d\n", minor, copied);

  return copied;
}

static ssize_t dev_read_bnb(struct file* f, char __user* user_buff, size_t size, loff_t* loff)
{
  struct lkt_cdev *dev = f->private_data;
  int minor            = MINOR(dev->cdev.dev);
  int space            = 0;
  size_t copied        = 0;
  int result           = -EFAULT;

  /* if operation is non blocked */
  if (f->f_flags & O_NONBLOCK)
  {
    if(kfifo_is_empty(&dev->fifo) == true)
    {
      return -EAGAIN;
    }
  }
  /* if it is blocked */
  else
  {
    /* check if fifo is full */
    /* ATTENTION: pass queue by value, not by address */
    result = wait_event_interruptible(dev->waitq_r, kfifo_is_empty(&dev->fifo) != true);
    if(result < 0)
    {
      printk(KERN_DEBUG "dev%d: write queue interrupted by signal\n", minor);
      return result;
    }
  }

  printk(KERN_DEBUG "dev%d: has data in fifo\n", minor);

  space = kfifo_len(&dev->fifo);

  size = size < space ? size : space;

  result = kfifo_to_user(&dev->fifo, user_buff, size, &copied);
  if(result < 0)
  {
    printk(KERN_WARNING "dev%d: fail to copy data to user space. Result: %d\n", minor, result);
    return result;
  }

  /* wake up write queue */
  if(copied > 0)
  {
      wake_up_interruptible(&dev->waitq_r);
  }

  printk(KERN_DEBUG "dev%d: copied: %d\n", minor, copied);

  return copied;
}

struct lktm_cdev_xfer
{
  u8 *buffer;
  u32 size;
};

#define LKTM_CDEV_IOCTL_MAGIC 0xF7

#define LKTM_CDEV_IOCTL_DO_NOTHING     _IO(LKTM_CDEV_IOCTL_MAGIC,   0)
#define LKTM_CDEV_IOCTL_READ_MAGIC     _IOR(LKTM_CDEV_IOCTL_MAGIC,  1, u8 *)
#define LKTM_CDEV_IOCTL_WRITE_STRING   _IOW(LKTM_CDEV_IOCTL_MAGIC,  2, struct lktm_cdev_xfer *)
#define LKTM_CDEV_IOCTL_RW_VALUE       _IOWR(LKTM_CDEV_IOCTL_MAGIC, 3, u32 *)
#define LKTM_CDEV_IOCTL_SWITCH_RW_FUNC _IOW(LKTM_CDEV_IOCTL_MAGIC,  4, u8)

#define SIMPLE_RW_FUNC 0
#define BNB_RW_FUNC    1

static long dev_ioctl(struct file * f, unsigned int cmd, unsigned long arg)
{
  struct lkt_cdev *dev = f->private_data;
  int minor = MINOR(dev->cdev.dev);
  u32 new_value = 0;
  struct lktm_cdev_xfer xfer = {0};
  long result = 0;

  switch(cmd)
  {
    case LKTM_CDEV_IOCTL_DO_NOTHING:
      printk(KERN_DEBUG "dev%d: LKTM_CDEV_IOCTL_DO_NOTHING\n", minor);
    break;

    case LKTM_CDEV_IOCTL_READ_MAGIC:
      printk(KERN_DEBUG "dev%d: LKTM_CDEV_IOCTL_READ_MAGIC", minor);
      result = put_user(LKTM_CDEV_IOCTL_MAGIC, (u8 __user *)arg);
      if(result < 0)
      {
        printk(KERN_WARNING "dev%d: unable to put data from user\n", minor);
        return -1;
      }

    break;

    case LKTM_CDEV_IOCTL_WRITE_STRING:
      printk(KERN_DEBUG "dev%d: LKTM_CDEV_IOCTL_WRITE_STRING\n", minor);
      
      /* copy xfer structure from user */
      result = copy_from_user(&xfer, (struct lktm_cdev_xfer __user *)arg, sizeof(struct lktm_cdev_xfer));
      if(result != 0)
      {
        printk(KERN_WARNING "dev%d: unable to copy xfer structure\n", minor);
        return -1;
      }
    
      /* copy data to buffer */
      result = dev_write(f, xfer.buffer, xfer.size, 0);

    break;

    case LKTM_CDEV_IOCTL_RW_VALUE:
      printk(KERN_DEBUG "dev%d: LKTM_CDEV_IOCTL_RW_VALUE\n", minor);
      
      /* get new value, send old value back */
      result = get_user(new_value, (u32 __user *)arg);
      if(result < 0)
      {
        printk(KERN_WARNING "dev%d: unable to get data from user\n", minor);
        return result;
      }

      result = put_user(dev->value, (u32 __user *)arg);
      if(result < 0)
      {
        printk(KERN_WARNING "dev%d: unable to put data from user\n", minor);
        return result;
      }

      dev->value = new_value;

    break;

    case LKTM_CDEV_IOCTL_SWITCH_RW_FUNC:
      printk(KERN_DEBUG "dev%d: LKTM_CDEV_IOCTL_SWITCH_RW_FUNC\n", minor);
      
      switch(arg)
      {
        case SIMPLE_RW_FUNC:
          printk(KERN_DEBUG "dev%d: swich to simple rw functions\n", minor);

          // dev->cdev.ops->read = dev_read;
          // dev->cdev.ops->write = dev_write;

          cdev_init(&dev->cdev, &dev_fops);
        break;

        case BNB_RW_FUNC:
          printk(KERN_DEBUG "dev%d: switch to bloking/nonbloking rw functions\n", minor);

          // dev->cdev.ops->read = dev_read_bnb;
          // dev->cdev.ops->write = dev_write_bnb;

          cdev_init(&dev->cdev, &dev_fops_bnb);

        default:
          printk(KERN_WARNING "dev%d: wrong argument\n", minor);
          return -1;
      }

    break;

    default:
      printk(KERN_WARNING "dev%d: wrong ioctl: 0x%x\n", minor, cmd);

  }

  return result;
}