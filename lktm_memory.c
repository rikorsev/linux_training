#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define KMALLOC_BUF_SIZE 512
#define KMEM_CACHE_SIZE 1024
#define KMEM_NUM_OF_OBJ 10

static void kmalloc_test(void);
static void lookaside_cache_test(void);

static int __init mem_init(void)
{
  printk(KERN_DEBUG "mem: init\n");

  kmalloc_test();
  lookaside_cache_test();
  
  return 0;
}
module_init(mem_init);


static void __exit mem_exit(void)
{
  printk(KERN_DEBUG "mem: cleneup\n");

  printk(KERN_DEBUG "mem: exit\n");
}
module_exit(mem_exit);

static void kmalloc_test(void)
{
  int i = 0;
  int* kmalloc_buf_ptr = kmalloc(sizeof(int) * KMALLOC_BUF_SIZE, GFP_KERNEL);

  if(NULL == kmalloc_buf_ptr)
    {
      printk(KERN_WARNING "mem: kmalloc allocation - fail\n");
      return;
    }

  for(i = 0; i < KMALLOC_BUF_SIZE; i++)
    {
      kmalloc_buf_ptr[i] = i;
    }
  
  for(i = 0; i < KMALLOC_BUF_SIZE; i++)
    {
      printk(KERN_DEBUG "mem: kmalloc_buffer[%d] = %d\n", i, kmalloc_buf_ptr[i]);
    }

  kfree(kmalloc_buf_ptr);
}

static void kmem_cache_construct(void* data /* kmem_cache_t* cache, unsifned lonf flags */)
{
  int i = 0;
  static int j = 0;
  
  //if( flags & SLAB_CTOR_CONSTRUCTOR )
  //{
      printk(KERN_DEBUG "mem: kmem_cache: construct\n");
      for(i = 0; i < KMEM_CACHE_SIZE; i++, j++)
	{
	  ((int*)data)[i] = j;
	}
      // }
      //else
      // {
      // printk(KERN_DEBUG "mem: kmem_cache: destructor\n");
      //}
}

static void lookaside_cache_test(void)
{
  int i = 0, j = 0, allocated = 0;
  int* kmem_cache_obj[KMEM_NUM_OF_OBJ];
  struct kmem_cache* cache = kmem_cache_create("cache", sizeof(int) * KMEM_CACHE_SIZE, \
					0, 0, kmem_cache_construct);
    
  if(NULL == cache)
    {
      printk(KERN_WARNING "mem: kmem_chache allocation - fail\n");
      return;
    }

  for(i = 0; i < KMEM_NUM_OF_OBJ; i++)
    {
      kmem_cache_obj[i] = kmem_cache_alloc(cache, GFP_KERNEL);
      if(NULL == kmem_cache_obj[i])
	{
	  printk(KERN_WARNING "mem: kmem_cache_alloc for obj %d - fail\n", i);
	  break;
	}
    }

  allocated = i;
  
  for(i = 0; i < allocated; i++)
    {
      printk(KERN_DEBUG "\nmem: kmem_cache_obj %d \n\n", i);

      for(j = 0; j < KMEM_CACHE_SIZE; j++)
	{
	  printk(KERN_DEBUG "mem: kmem_cache_obj[%d][%d] = %d\n", i, j, kmem_cache_obj[i][j]);
	}
    }

  for(i = 0; i < allocated; i++)
    {
      kmem_cache_free(cache, kmem_cache_obj[i]);
    }

  kmem_cache_destroy(cache);

}

