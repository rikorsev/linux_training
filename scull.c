#include <linux/module.h> /*this header is obvios for module development*/
#include <linux/init.h>
#include <linux/moduleparam.h> /*header for manage params of module*/
#include <linux/kdev_t.h> /*contains define for dev_t*/
#include <linux/fs.h> /*contains a lot of useful things such as file_operations structure 
prototypes of function to register/allocate device number etc.*/
#include <linux/cdev.h> /*contains cdev struct and functions to work with it*/
#include <asm/uaccess.h> /* to access to user space*/
#include <linux/slab.h> /* for kalloc function */
#include <linux/semaphore.h> /* for down_interruptable ant up functions */


MODULE_LICENSE("BSD/GPL");

loff_t scull_llseek(struct file* f, loff_t offset, int val);
ssize_t scull_read(struct file* f, char* buff, size_t sz, loff_t* loff);
ssize_t scull_write(struct file* f, const char* buff, size_t sz, loff_t* loff);
long scull_ioctl(/*struct inode* node, */struct file* f, unsigned int i_num, unsigned long l_num);
int scull_open(struct inode* node, struct file* f);
int scull_release(struct inode* node, struct file* f);

static void /*__exit*/ scull_cleanup(void);

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = scull_llseek,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_open,
    .release = scull_release
};

struct scull_qset{
    void **data;
    struct scull_qset *next;
};

struct scull_dev{
  struct scull_qset* data;  /* Pointer to first quantum set */
  int quantum;              /* the current quantum size */
  int qset;                 /* the current arr size */
  unsigned long size;       /* amount of data stored here */
  unsigned long access_key; /* used by sculluid and scullpriv */
  struct semaphore sem;     /* mutal execution semaphore */
  struct cdev cdev;         /* Char device structure */
};

static int scull_trim(struct scull_dev *dev);

static int scull_major = 0;
module_param(scull_major, int, 0);

static int scull_minor = 0;
static int scull_nr_devs = 4;

static int scull_quantum = 4096;
module_param(scull_quantum, int, 0);

static int scull_qset = 100;
module_param(scull_qset, int, 0);

static struct scull_dev** scull_dev_set_ptr = NULL;

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
  int err, devno = MKDEV(scull_major, scull_minor + index);

  cdev_init(&dev->cdev, &scull_fops);
  dev->cdev.owner = THIS_MODULE;
  dev->cdev.ops = &scull_fops;
  err = cdev_add(&dev->cdev, devno, 1);

  if(err)
    {
      printk(KERN_NOTICE "scull: Error %d adding scull%d", err, index);
    }
}

static int /*__init*/ scull_init(void)
{
  int index;
  int result;
  dev_t dev;

  int* err_ptr = NULL;
  int zero_devision_result;
  
  printk(KERN_DEBUG "scull: init\n");
  
  if(scull_major != 0)
    {
      dev = MKDEV(scull_major, scull_minor);
      result = register_chrdev_region(dev, scull_nr_devs, "scull");
    }
  else
    {
      result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
      scull_major = MAJOR(dev);
    }

  if(result < 0)
    {
      printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
    }

  /* creating and registring new devices */
  scull_dev_set_ptr = kmalloc(sizeof(struct scull_dev*) * scull_nr_devs, GFP_KERNEL);
  if(scull_dev_set_ptr == NULL)
    {
      printk(KERN_WARNING "scull: scullset memory allocation fail\n");
      return -ENOMEM;
    }
  
  for(index = 0; index < scull_nr_devs; index++)
    {
      scull_dev_set_ptr[index] = kmalloc(sizeof(struct scull_dev), GFP_KERNEL);
      if(scull_dev_set_ptr[index] == NULL)
	{
	  printk(KERN_WARNING "scull: device %d memory allocation fail\n", index);
	  scull_cleanup();
	  return -ENOMEM;
	}
      scull_setup_cdev((struct scull_dev*)&scull_dev_set_ptr[index], index);
    }

  *err_ptr = 1234;
  printk(KERN_DEBUG "scull: dereference of NULL ptr. dereferenced value = %d/n", *err_ptr);

  zero_devision_result = 1234/0;
  printk(KERN_DEBUG "scull: zero devision result = %d/n", zero_devision_result);
  //scull_setup_cdev(struct scull_dev *dev, int index);

  return result;
}

static void /*__exit*/ scull_cleanup(void)
{
  int index;
  
  printk(KERN_DEBUG "scull: cleneup\n");
  
  if(scull_dev_set_ptr != NULL)
    {
      for(index = 0; index < scull_nr_devs; index++)
	{
	  if(scull_dev_set_ptr[index] != NULL) kfree(scull_dev_set_ptr[index]);
	}
      kfree(scull_dev_set_ptr);
    }
}

module_init(scull_init);
module_exit(scull_cleanup);

loff_t scull_llseek(struct file* f, loff_t offset, int val)
{
    return 0;
}

struct scull_qset* scull_follow(struct scull_dev *dev, int item)
{
  return NULL;
}

ssize_t scull_read(struct file* f, char* buff, size_t sz, loff_t* loff)
{
    struct scull_dev *dev = f->private_data;
    struct scull_qset *dptr; /*the first listitem*/
    int quantum = dev->quantum, qset = dev->qset;
    int itemsize = quantum * qset; /* how manu bytes in the listitem */
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    printk(KERN_DEBUG "scull: read\n");

    //if(down_interruptable(&dev->sem))
    //  return -ERESTARTSYS;

    down(&dev->sem);
    
    if(*loff >= dev->size)
      goto out;
    if(*loff + sz > dev->size)
      sz = dev->size - *loff;

    /* find listitem, qset index and offset in the quantum */
    item = (long)*loff/itemsize;
    rest = (long)*loff % itemsize;
    s_pos = rest / quantum; q_pos = rest % quantum;

    /* follow the list up to the right position (defined elsewhere) */
    dptr = scull_follow(dev, item);

    if(dptr == NULL || !dptr->data || !dptr->data[s_pos])
      goto out; /* don't fill holes */

    /* read only up to the end of this quantum */
    if(sz > quantum - q_pos)
      sz = quantum - q_pos;

    if(copy_to_user(buff, (dptr->data[s_pos] + q_pos), sz))
      {
	retval = -EFAULT;
	goto out;
      }

    *loff += sz;
    retval = sz;

 out:
    up(&dev->sem);
    return retval;
}

ssize_t scull_write(struct file* f, const char* buff, size_t sz, loff_t* loff)
{
  struct scull_dev *dev = f->private_data;
  struct scull_qset *dptr;
  int quantum = dev->quantum, qset = dev->qset;
  int itemsize = quantum * qset;
  int item, s_pos, q_pos, rest;
  ssize_t retval = -ENOMEM; /* value used in "goto out" statement */

  printk(KERN_DEBUG "scull: write\n");
  
  //if (down_interrupt(&dev->sem))
  //  return -ERESTARTSYS;

  down(&dev->sem);
  
  /* find listitem, qset index and offset in the quantum */
  item = (long)*loff / itemsize;
  rest = (long)*loff % itemsize;
  s_pos = rest / quantum; q_pos = rest % quantum;

  /* follow the list up to the right position */
  dptr = scull_follow(dev, item);
  if (dptr == NULL)
    goto out;
  if (!dptr->data)
    {
      dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
      if(!dptr->data)
	goto out;
      memset(dptr->data, 0, qset * sizeof(char*));
    }
  if (!dptr->data[s_pos])
    {
      dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
      if(!dptr->data[s_pos])
	goto out;
    }
  /* write only up to the end of this quantum */
  if(sz > quantum - q_pos)
    sz = quantum - q_pos;

  if(copy_from_user(dptr->data[s_pos] + q_pos, buff, sz))
    {
      retval = -EFAULT;
      goto out;
    }
  *loff += sz;
  retval = sz;

  /* update the size */
  if (dev->size < *loff)
    dev->size = *loff;

 out:
  up(&dev->sem);
  return retval;
}

long scull_ioctl(/*struct inode* node, */struct file* f, unsigned int i_num, unsigned long l_num)
{
    return 0;
}

static int scull_trim(struct scull_dev *dev)
{
    struct scull_qset *next, *dptr;
    int qset = dev->qset; /* "dev" is no null */
    int i;

    /* all the list items*/
    for (dptr = dev->data; dptr; dptr = next)
    {
        if (dptr->data)
        {
            for (i = 0; i < qset; i++)
                kfree(dptr->data[i]);
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;
    return 0;
}

int scull_open(struct inode* node, struct file* f)
{
  struct scull_dev *dev; /* device information */

  printk(KERN_DEBUG "scull: open");

  dev = container_of(node->i_cdev, struct scull_dev, cdev);
  f->private_data = dev; /* for othre methods */

  /* now trim to 0 the lrngth of the device ifopen was write-only */
  if((f->f_flags & O_ACCMODE) == O_WRONLY)
    {
      scull_trim(dev); /* ignore errors */
    }

  return 0; /* success */
}

int scull_release(struct inode* node, struct file* f)
{
    printk(KERN_DEBUG "scull: release\n");

    return 0;
}

