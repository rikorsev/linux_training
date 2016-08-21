#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gfp.h>

MODULE_LICENSE("GPL");

static int __init mem_alloc_pages_init(void)
{
  struct page* page = NULL;

  printk(KERN_DEBUG "\n---=== ALLOC PAGES TEST STARTED ===---\n\n");
  
  page = alloc_page(GFP_KERNEL);
  if(NULL == page)
    {
      printk(KERN_WARNING "mem: alloc_pages: allocation - fail\n");
      return -1;
    }

  __free_page(page);
  
  return -1;
}
module_init(mem_alloc_pages_init);
