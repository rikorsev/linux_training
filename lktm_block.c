#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h> /* to register_blkdev, unregister_blkdev functions */
#include <linux/moduleparam.h>
#include <linux/blkdev.h> /* for block device functions such as blk_init_queue */

MODULE_LICENSE("GPL");

static int blk_major = 0;
module_param(blk_major, int, 0);

static int blk_num = 1;
module_param(blk_num, int, 0);

static int blk_sectors_num = 128;
module_param(blk_sectors_num, int, 0);

static short blk_sector_size = 1024;
module_param(blk_sector_size, short, 0);

typedef struct
{
  struct gendisk* disk;
}blkdev_t;

static blkdev_t my_blk;

static void blkdev_request(struct request_queue* q);

/* device operations prototype */
static int blkdev_open(struct block_device* dev, fmode_t mode);
static void blkdev_close(struct gendisk* disk, fmode_t mode);
static int blkdev_ioctl_handler(struct block_device* dev, fmode_t mode, unsigned int cmd, unsigned long data);

static const struct block_device_operations bdops = {
  .open = blkdev_open,
  .release = blkdev_close,
  .ioctl = blkdev_ioctl_handler
};

static int __init blkdev_init(void)
{
  printk(KERN_DEBUG "blkdev: init\n");

  /* if major number is 0, major number will be allocated automatically */
  blk_major = register_blkdev(blk_major, "lktmblk");
  if(0 >= blk_major)
    {
      printk(KERN_WARNING "blkdev: block device allocation - fail\n");
      return -1;
    }
  printk(KERN_DEBUG "blkdev: registred, major number %d\n", blk_major);

  my_blk.disk = alloc_disk(blk_num);
  if(NULL == my_blk.disk)
    {
      printk(KERN_WARNING "blkdev: disk allocation - fail\n");
      blkdev_clenup();
      return -1;
    }
  
  /* init disk */
  my_blk.disk -> major = blk_major;
  my_blk.disk -> first_minor = 0;
  my_blk.disk -> fops = &bdops;
  my_blk.disk -> private_data = &my_blk;
  my_blk.disk -> strcpy(my_blk.disk -> disk_name, "lktmblk");
  my_blk.disk -> queue = blk_init_queue(blkdev_request, NULL);
  if(NULL == my_blk.disk -> queue)
    {
      printk(KERN_DEBUG "blkdev: request queue allocation - fail\n");
      blkdev_cleanup();
      return -1;
    }
  blk_queue_hardsect_size(my_blk.disk -> queue, blk_sector_size);
  set_capacity(my_blk.disk, blk_sectors_num);
  add_disk(my_blk.disk); /* It's alive! */
  
  return 0;
}
module_init(blkdev_init);

static void __exit blkdev_cleanup(void)
{
  printk(KERN_DEBUG "blkdev: cleanup\n");

  if(0 < blk_major)
    {
      unregister_blkdev(blk_major, "lktmblk");
    }

  if(NULL != my_blk.disk -> queue)
    {
      blk_cleanup_queue(my_blk.disk -> queue);
    }
  
  if(NULL != my_blk.disk)
    {
      /* free disk */
      put_disk(my_blk.disk);
    }
  
  printk(KERN_DEBUG "blkdev: exit\n");
}
module_exit(blkdev_cleanup);

static void blkdev_request(struct request_queue* q)
{
  printk(KERN_DEBUG "blkdev: request handler entry\n");

  printk(KERN_DEBUG "blkdev: request handler exit\n");
}

static int blkdev_ioctl_handler(struct block_device* dev, fmode_t mode, unsigned int cmd, unsigned long data)
{
  printk(KERN_DEBUG "blkdev: ioctl %d data %ld", cmd, data);

  return 0;
}

static int blkdev_open(struct block_device* dev, fmode_t mode)
{
  printk(KERN_DEBUG "blkdev: open\n");

  return 0;
}

static void blkdev_close(struct gendisk* disk, fmode_t mode)
{
  printk(KERN_DEBUG "blkdev: close\n");
}
