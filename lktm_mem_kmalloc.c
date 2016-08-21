#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define KMALLOC_BUF_SIZE 128

static int __init mem_init(void)
{
  int i = 0;
  int* kmalloc_buf_ptr = kmalloc(sizeof(int) * KMALLOC_BUF_SIZE, GFP_KERNEL);

  printk(KERN_DEBUG "\n---=== KMALLOC TEST STARTED ===---\n\n");
  
  if(NULL == kmalloc_buf_ptr)
    {
      printk(KERN_WARNING "mem: kmalloc allocation - fail\n");
      return -1;
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
  return -1;
}
module_init(mem_init);
