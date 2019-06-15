#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mempool.h>

MODULE_LICENSE("GPL");

/* defines for mem_cache */
#define KMEM_CACHE_SIZE 64
#define KMEM_NUM_OF_OBJ 10

/* defines for mempool */
#define MEMPOOL_OBJECTS_MIN 5
#define MEMPOOL_OBJECTS_MAX 15

static void lookaside_cache_test(void);
static void mempool_test_standart(void);

static int __init mem_cache_init(void)
{
  lookaside_cache_test();
  mempool_test_standart();
  
  return -1;
}
module_init(mem_cache_init);

static void kmem_cache_construct(void* data)
{
  int i = 0;
  static int j = 0;
  
  printk(KERN_DEBUG "mem: kmem_cache: construct\n");
  printk(KERN_DEBUG "mem: data address %p\n", data);
  for(i = 0; i < KMEM_CACHE_SIZE; i++, j++)
    {
      ((int*)data)[i] = j;
    }
}

static void lookaside_cache_test(void)
{
  int i = 0, j = 0, allocated = 0;
  int* kmem_cache_obj[KMEM_NUM_OF_OBJ];
  struct kmem_cache* cache = kmem_cache_create("cache", sizeof(int) * KMEM_CACHE_SIZE, \
					0, 0, kmem_cache_construct);

  printk(KERN_DEBUG "\n---=== LOOKASIDE CACHE TEST STARTED ===---\n\n");
  
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

static void mempool_test_standart(void)
{
  int i = 0, j = 0;
  struct kmem_cache* cache = NULL;
  mempool_t* pool = NULL;
  int* mempool_obj[MEMPOOL_OBJECTS_MAX];

  printk(KERN_DEBUG "\n---=== MEMPOOL TEST STANDART STARTED ===---\n\n");
  
  /* Standart realization */
  cache = kmem_cache_create("cache", sizeof(int) * KMEM_CACHE_SIZE, 0, 0, kmem_cache_construct); 
  pool = mempool_create(MEMPOOL_OBJECTS_MIN, mempool_alloc_slab, mempool_free_slab, cache);

  printk(KERN_DEBUG "mem: mempool created\n");
  printk(KERN_DEBUG "mem: mempool: curr_nr = %d\n", pool->curr_nr);
  
  for (i = 0; i < MEMPOOL_OBJECTS_MIN; i++)
    {
      mempool_obj[i] = (int*)pool->elements[i];
      printk(KERN_DEBUG "mem: pool->element[%d] = %p\n", i, pool->elements[i]);
    }
  
  for (i = MEMPOOL_OBJECTS_MIN; i < MEMPOOL_OBJECTS_MAX; i++)
    {
      mempool_obj[i] = mempool_alloc(pool, GFP_KERNEL);
      printk(KERN_DEBUG "mem: allocated %p\n", mempool_obj[i]);
    }

  printk(KERN_DEBUG "mem: mempool: curr_nr = %d\n", pool->curr_nr);
  
  for (i = 0; i < MEMPOOL_OBJECTS_MAX; i++)
    {
      printk(KERN_DEBUG "\nmem: mempool_obj %d \n\n", i);

      for (j = 0; j < KMEM_CACHE_SIZE; j++)
	{
	  printk(KERN_DEBUG "mem: mempool_obj[%d][%d] = %d\n", i, j, mempool_obj[i][j]);
	}
    }

  for (i = 0; i < MEMPOOL_OBJECTS_MAX; i++)
    {
      mempool_free(mempool_obj[i], pool);
    }

  printk(KERN_DEBUG "mem: mempool: curr_nr = %d\n", pool->curr_nr);

 for (i = 0; i < MEMPOOL_OBJECTS_MIN; i++)
    {
      printk(KERN_DEBUG "\nmem: mempool after free %d\n\n", i);
      printk(KERN_DEBUG "mem: pool->element[%d] = %p\n", i, pool->elements[i]);
      for (j = 0; j < KMEM_CACHE_SIZE; j++)
	{
	  printk(KERN_DEBUG "mem: after free mem[%d][%d] = %d\n", i, j, \
		 ((int*)pool->elements[i])[j]);
	}
    }
  
  mempool_destroy(pool);
}
