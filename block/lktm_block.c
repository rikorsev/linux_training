#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h> /* to register_blkdev, unregister_blkdev functions */
#include <linux/moduleparam.h>
#include <linux/blkdev.h> /* for block device functions such as blk_init_queue */
#include <linux/hdreg.h> /* for HDIO_GETGEO */ 
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

static int blk_major = 0;
module_param(blk_major, int, 0);

static int blk_num = 1;
module_param(blk_num, int, 0);

static int blk_sectors_num = 1024;
module_param(blk_sectors_num, int, 0);

static short blk_sector_size = 512;
module_param(blk_sector_size, short, 0);

typedef struct
{
  int sect_size;
  int sect_num;
  spinlock_t lock;
  struct kmem_cache* cache;
  unsigned char** sector;
  struct gendisk* disk;
}blk_t;

static blk_t my_blk;

static void __exit blk_cleanup(void);
static void blk_request(struct request_queue* q);

static void blk_sector_construct(void* data);

//static int blk_xfer_bio(blk_t *blk, struct bio *bio);
static int blk_xfer_request(blk_t* blk, struct request *req);

/* device operations prototype */
static int blk_open(struct block_device* dev, fmode_t mode);
static void blk_close(struct gendisk* disk, fmode_t mode);
static int blk_ioctl_handler(struct block_device* dev, fmode_t mode, unsigned int cmd, unsigned long data);
static int blk_getgeo(struct block_device* dev, struct hd_geometry* geo);
static int blk_read(blk_t* blk, unsigned long sector, char* buf, unsigned long size);
static int blk_write(blk_t* blk, unsigned long sector, char* buf, unsigned long size);


static const struct block_device_operations bdops = {
  .owner = THIS_MODULE,
  .open = blk_open,
  .release = blk_close,
  .getgeo = blk_getgeo,
  .ioctl = blk_ioctl_handler
};

static int __init blk_init(void)
{
  int i;
  
  printk(KERN_DEBUG "blk: init\n");
  
  my_blk.cache = kmem_cache_create("blk_dev_cache",  blk_sector_size, 0, 0, blk_sector_construct);

  if (NULL == my_blk.cache)
    {
      printk(KERN_DEBUG "blk: cache allocation - fail\n");
      blk_cleanup();
      return -1;
    }

  my_blk.sector = kmalloc(sizeof(unsigned char*) * blk_sector_size, GFP_KERNEL);
  if(NULL == my_blk.sector)
    {
      printk(KERN_DEBUG "blk: sectors pointers allocation - fail\n");
      blk_cleanup();
      return 1;
    }
  memset(my_blk.sector, 0, sizeof(unsigned char*) * blk_sector_size);

 /* allocate sectors */
  for(i = 0; i < blk_sectors_num; i++)
    {
      printk(KERN_DEBUG "blk: allocate new sector %d\n", i);
      my_blk.sector[i] = kmem_cache_alloc(my_blk.cache, GFP_KERNEL);
      if(NULL == my_blk.sector[i])
	{
	  printk(KERN_WARNING "blk: new sector allocation - fail\n");
	  break;
	}
    }
  
  my_blk.sect_size = blk_sector_size;
  my_blk.sect_num = blk_sectors_num;
  spin_lock_init(&my_blk.lock);
  if(true == spin_is_locked(&my_blk.lock))
    {
      printk(KERN_DEBUG "blk: spin is locked... unlocking\n");
      spin_unlock(&my_blk.lock);
    }
  
  /* if major number is 0, major number will be allocated automatically */
  blk_major = register_blkdev(blk_major, "lktmblk");
  if(0 >= blk_major)
    {
      printk(KERN_WARNING "blk: block device allocation - fail\n");
      return -1;
    }
  printk(KERN_DEBUG "blk: registred, major number %d\n", blk_major);

  my_blk.disk = alloc_disk(blk_num);
  if(NULL == my_blk.disk)
    {
      printk(KERN_WARNING "blk: disk allocation - fail\n");
      blk_cleanup();
      return -1;
    }
  
  /* init disk */
  my_blk.disk -> major = blk_major;
  my_blk.disk -> first_minor = 0;
  my_blk.disk -> fops = &bdops;
  my_blk.disk -> private_data = &my_blk;
  strcpy(my_blk.disk -> disk_name, "lktmblk");
  my_blk.disk -> queue = blk_init_queue(blk_request, &my_blk.lock /*NULL*/); /* second arg is a pointer to spinlock */
  if(NULL == my_blk.disk -> queue)
    {
      printk(KERN_DEBUG "blk: request queue allocation - fail\n");
      blk_cleanup();
      return -1;
    }
  blk_queue_logical_block_size(my_blk.disk -> queue, blk_sector_size);
  set_capacity(my_blk.disk, blk_sectors_num);
  add_disk(my_blk.disk); /* It's alive! */
  
  return 0;
}
module_init(blk_init);

static void __exit blk_cleanup(void)
{
  int i = 0;
  
  printk(KERN_DEBUG "blk: cleanup\n");

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
      //put_disk(my_blk.disk);
      del_gendisk(my_blk.disk);
    }

  if(NULL != my_blk.sector)
    {
      for(i = 0; i < my_blk.sect_num; i++)
	{
	  if(NULL != my_blk.sector[i])
	    {
	      kmem_cache_free(my_blk.cache, my_blk.sector[i]);
	    }
	}
    }
  
  if(NULL != my_blk.cache)
    {
      kmem_cache_destroy(my_blk.cache);
    }
  
  printk(KERN_DEBUG "blk: exit\n");
}
module_exit(blk_cleanup);

static void blk_request(struct request_queue* q)
{
  struct request* req = NULL;
  blk_t* blk = NULL;
  int trp = 0; /* amount of transported data */
 
  printk(KERN_DEBUG "blk: request handler entry\n");

  //while((req == blk_fetch_request(q)) != NULL)
  do
    {
      req = blk_fetch_request(q);
      
      if(NULL == req)
	{
	  printk(KERN_DEBUG "blk: no more request's\n");
	  break;
	}

      if(REQ_TYPE_FS != req->cmd_type)
	{
	  printk(KERN_DEBUG "blk: not fs request\n");
	  __blk_end_request(req, 1, 0); /* second arg 0 if success > 0 if error */
	  continue;
	}

      printk(KERN_DEBUG "blk: request is valid. handling...\n");
      
      /* Transfer data TBD */
      blk = req -> rq_disk -> private_data;

      trp = blk_xfer_request(blk, req);

      printk(KERN_DEBUG "blk: total transported = %d\n", trp);
      
      __blk_end_request(req, 0, trp); /* second arg 0 if success > 0 if error */
      
    }while(true /* req != NULL */);
  
  printk(KERN_DEBUG "blk: request handler exit\n");
}

static int blk_xfer_request(blk_t* blk, struct request *req)
{

	struct req_iterator iter;
	struct bio_vec bvec;
	char* buffer = NULL;
	sector_t sector = 0;
	int trp = 0;
	int trp_total = 0;
	
	/* Macro rq_for_each_bio is gone.
	 * In most cases one should use rq_for_each_segment.
	 */
	
	rq_for_each_segment(bvec, req, iter)
	  {
	    sector = iter.iter.bi_sector;
	    buffer = __bio_kmap_atomic(iter.bio, iter.iter);

	    printk(KERN_DEBUG "blk: bio 0x%p, next bio 0x%p\n", iter.bio, iter.bio -> bi_next);
	    
	    printk(KERN_DEBUG "blk: buffer 0x%p\n", buffer);

	    printk(KERN_DEBUG "blk: bio: bvec_iter: sector %d, size %d, index %d, done %d\n", \
		   (int)iter.iter.bi_sector,				\
		   iter.iter.bi_size,		\
		   iter.iter.bi_idx,		\
		   iter.iter.bi_bvec_done);
		   
	    printk(KERN_DEBUG "blk: bio: page 0x%p, len %d, offset %d\n", \
		   bvec.bv_page,					\
		   bvec.bv_len,		\
		   bvec.bv_offset);
	    
	    if(WRITE == bio_data_dir(iter.bio))
	      {
	    	trp = blk_write(blk, sector, buffer, bvec.bv_len/*4096*/);
	    	printk(KERN_DEBUG "blk: written %d bytes\n", trp);
	      }
	    else
	      {
	    	trp = blk_read(blk, sector, buffer, bvec.bv_len/*4096*/);
	    	printk(KERN_DEBUG "blk: readen %d bytes\n", trp);
	      }

	    __bio_kunmap_atomic(iter.bio);
	    
	    trp_total += trp;

	  }
	return trp_total;
}


static int blk_ioctl_handler(struct block_device* dev, fmode_t mode, unsigned int cmd, unsigned long data)
{
  struct hd_geometry geo;
  blk_t* blk = dev->bd_disk->private_data;
  int status = 0;
  
  printk(KERN_DEBUG "blk: ioctl %d data %ld", cmd, data);

  switch(cmd)
    {
    case HDIO_GETGEO:

      printk(KERN_DEBUG "blk: ioctl: get geometry\n");
      
      geo.cylinders = 1;
      geo.heads = 1;
      geo.sectors = blk->sect_num;
      geo.start = 0;

      if(copy_to_user((void __user *) data, &geo, sizeof(geo)))
	{
	  printk(KERN_WARNING "blk: copy to user - fail\n");
	  status = -EFAULT;
	}
      break;

    default:
      status = -ENOTTY;
    }
  
  return status;
}

static int blk_open(struct block_device* dev, fmode_t mode)
{
  printk(KERN_DEBUG "blk: open\n");

  return 0;
}

static void blk_close(struct gendisk* disk, fmode_t mode)
{
  printk(KERN_DEBUG "blk: close\n");
}

static int blk_read(blk_t* blk, unsigned long sector, char* buf, unsigned long size)
{
  int i;
  int num_of_sect = 0;
  
  if (NULL == buf || 0 == size)
    {
      printk(KERN_DEBUG "blk: attrmpt to read to NULL pointer of buffer lenght is zero\n");
      return 0;
    }

  num_of_sect = size < blk->sect_size ? 1 : size / blk->sect_size;
  
  for(i = 0; i < num_of_sect && sector < blk->sect_num; i++, sector++)
    {
      printk(KERN_DEBUG "blk: read: sector %d\n", (int)sector);

      if(NULL == blk->sector[sector])
	{
	  printk(KERN_DEBUG "blk: sector %d is empty\n", (int)sector);
	  continue;
	}
      
      memcpy(buf + blk->sect_size * i, blk->sector[sector], blk->sect_size);
    }
  
  return blk->sect_size * i;
}

static int blk_write(blk_t* blk, unsigned long sector, char* buf, unsigned long size)
{
  int i;
  int num_of_sect = 0;
  
  if (NULL == buf || 0 == size)
    {
      printk(KERN_DEBUG "blk: attrmpt to write from NULL pointer of buffer lenght is zero\n");
      return 0;
    }

  num_of_sect = size < blk->sect_size ? 1 : size / blk->sect_size;
  
  for(i = 0; i < num_of_sect && sector < blk->sect_num; i++, sector++)
    {
      printk(KERN_DEBUG "blk: write: sector %d\n", (int)sector);
      
      if(NULL == blk->sector[sector])
	{
	  printk(KERN_DEBUG "blk: allocate new sector %d\n", (int)sector);
	  blk->sector[sector] = kmem_cache_alloc(blk->cache, GFP_KERNEL);
	  if(NULL == blk->sector[sector])
	    {
	      printk(KERN_DEBUG "blk: new sector allocation - fail\n");
	      i--;
	      num_of_sect--;
	      continue;
	    }
	}
      
      memcpy(blk->sector[sector], buf + i * blk->sect_size, blk->sect_size);
    }
  
  return blk->sect_size * i;
}

static int blk_getgeo(struct block_device* dev, struct hd_geometry* geo)
{
  blk_t* blk = dev->bd_disk->private_data;
  
  printk(KERN_DEBUG "blk: get geo\n");

  if(NULL == dev || NULL == geo)
    {
      printk(KERN_DEBUG"blk: get geo - fail. null pointer\n");
      return -1;
    }

  geo->cylinders = 1;
  geo->heads = 1;
  geo->sectors = blk->sect_num;
  geo->start = 0;
  
  return 0;
}


static void blk_sector_construct(void* data)
{
  printk(KERN_DEBUG "blk: new sector construct\n");
}
