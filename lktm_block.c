#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h> /* to register_blkdev, unregister_blkdev functions */
#include <linux/moduleparam.h>
#include <linux/blkdev.h> /* for block device functions such as blk_init_queue */
#include <linux/hdreg.h> /* for HDIO_GETGEO */ 

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
  int sect_size;
  int sect_num;
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

static int blk_read(blk_t* blk, unsigned long sector, unsigned long nsect, char* buf);
static int blk_write(blk_t* blk, unsigned long sector, unsigned long nsect, char* buf);

/* device operations prototype */
static int blk_open(struct block_device* dev, fmode_t mode);
static void blk_close(struct gendisk* disk, fmode_t mode);
static int blk_ioctl_handler(struct block_device* dev, fmode_t mode, unsigned int cmd, unsigned long data);

static const struct block_device_operations bdops = {
  .owner = THIS_MODULE,
  .open = blk_open,
  .release = blk_close,
  .ioctl = blk_ioctl_handler
};

static int __init blk_init(void)
{
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

  my_blk.sect_size = blk_sector_size;
  my_blk.sect_num = blk_sectors_num;
  
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
  my_blk.disk -> queue = blk_init_queue(blk_request, NULL); /* second arg is a pointer to spinlock */
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
  //struct req_iterator iter;
  
  printk(KERN_DEBUG "blk: request handler entry\n");

  //while((req = elv_next_request(q)) != NULL)
  //while((req == blk_fetch_request(q)) != NULL)
  do
    {
      req = blk_fetch_request(q);
      
      if(NULL == req)
	{
	  printk(KERN_DEBUG "blk: no more request's\n");
	  break;
	}

      //if(false == blk_fs_request(req))
      if(REQ_TYPE_FS != req->cmd_type)
	{
	  printk(KERN_DEBUG "blk: not fs request\n");
	  blk_end_request(req, 1, 0); /* second arg 0 if success > 0 if error */
	  continue;
	}
            
      /* Transfer data TBD */
      blk = req -> rq_disk -> private_data;
      /* 
       iter.bio = req->bio;
       
       rq_for_each_segment(,,)
	{
	  
	}
      */
      //      trp = blk_xfer_bio(blk, req->bio);

      trp = blk_xfer_request(blk, req);
      
      blk_end_request(req, 0, trp); /* second arg 0 if success > 0 if error */
      
    }while(true /* req != NULL */);
  
  printk(KERN_DEBUG "blk: request handler exit\n");
}
/*
static int blk_xfer_bio(blk_t* blk, struct bio* bio)
{
  int i;
  struct bio_vec* bvec;
  sector_t sector = bio->bi_sector;
  char* buffer = NULL;
  int trp = 0;
  
  printk(KERN_DEBUG "blk: bio handling\n");
  
  bio_for_each_segment(bvec, bio, i)
    {
      //printk(KERN_DEBUG "blk: bio: bvec_iter: sector %d, size %d, index %d, done %d\n");
      
      buffer = __bio_kmap_atomic(bio, i, KM_USER0);

      if(WRITE == bio_data_dir(bio))
	{
	  trp = blk_write(blk, sector, bio_cur_sectors(bio), buffer);
	  printk(KERN_DEBUG "blk: written %d bytes\n", trp);
	}
      else
	{
	  trp = blk_read(blk, sector, bio_cur_sectors(bio), buffer);
	  printk(KERN_DEBUG "blk: readen %d bytes\n", trp);	  
	}
      
      sector += bio_cur_sectors(bio);
      __bio_kunmap_atomic(bio, KM_USER0);
    }
  
  return trp;
}
*/

static int blk_xfer_request(blk_t* blk, struct request *req)
{

	struct req_iterator iter;
	//int nsect = 0;
	struct bio_vec bvec;
	char* buffer = NULL;
	sector_t sector = 0;//iter.bio->bi_iter.sector;
	int trp = 0;
	
	/* Macro rq_for_each_bio is gone.
	 * In most cases one should use rq_for_each_segment.
	 */
	rq_for_each_segment(bvec, req, iter)
	  {
	    sector = iter.bio -> bi_iter.bi_sector;
	    buffer = __bio_kmap_atomic(iter.bio, iter.iter);

	    /*sbull_transfer(dev, sector, bio_cur_sectors(iter.bio),
	      buffer, bio_data_dir(iter.bio) == WRITE);
	    */

	    printk(KERN_DEBUG "blk: bio: bvec_iter: sector %d, size %d, index %d, done %d\n", \
		   (int)iter.bio -> bi_iter.bi_sector,			\
		   iter.bio -> bi_iter.bi_size, \
		   iter.bio -> bi_iter.bi_idx, \
		   iter.bio -> bi_iter.bi_bvec_done);
		
	    if(WRITE == bio_data_dir(iter.bio))
	      {
		trp = blk_write(blk, sector, bio_sectors(iter.bio), buffer);
		printk(KERN_DEBUG "blk: written %d bytes\n", trp);
	      }
	    else
	      {
		trp = blk_read(blk, sector, bio_sectors(iter.bio), buffer);
		printk(KERN_DEBUG "blk: readen %d bytes\n", trp);
	      }
		
	    //sector += bio_sectors(iter.bio);
	    __bio_kunmap_atomic(iter.bio);
	    //nsect += iter.bio->bi_size/KERNEL_SECTOR_SIZE;
	  }
	return trp;//nsect;
}


static int blk_ioctl_handler(struct block_device* dev, fmode_t mode, unsigned int cmd, unsigned long data)
{
  //long size;
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

static int blk_read(blk_t* blk, unsigned long sector, unsigned long nsect, char* buf)
{
  int i;

  if (NULL == buf)
    {
      printk(KERN_WARNING "blk: attempt to read to NULL pointer\n");
      return 0;

    }
  
  for(i = 0; i < nsect && sector < blk->sect_num; i++, sector++)
    {
      printk(KERN_DEBUG "blk: read: sector %d\n", (int)sector);

      if(NULL == blk->sector[sector])
	{
	  printk(KERN_DEBUG "blk: sector %d is empty\n", (int)sector);
	  //i--;
	  //nsect--;
	  continue;
	}
      memcpy(buf + blk->sect_size * i, blk->sector[sector], blk->sect_size);
    }
  
  return blk->sect_size * i;
}

static int blk_write(blk_t* blk, unsigned long sector, unsigned long nsect, char* buf)
{
  int i;
  
  if (NULL == buf)
    {
      printk(KERN_DEBUG "blk: attrmpt to write from NULL pointer\n");
      return 0;
    }

  for(i = 0; i < nsect && sector < blk->sect_num; i++, sector++)
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
	      nsect--;
	      continue;
	    }
	}
      memcpy(blk->sector[sector], buf + i * blk->sect_size, blk->sect_size);
    }
  
  return blk->sect_num * i;
}

static void blk_sector_construct(void* data)
{
  printk(KERN_DEBUG "blk: new sector construct");
}
