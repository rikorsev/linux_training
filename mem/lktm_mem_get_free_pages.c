#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gfp.h>

MODULE_LICENSE("GPL");

#define NUM_OF_PAGES 4
#define PAGES_ORDER 2

static inline void show_array(void* arr ,int size);
static void get_free_page_test(void);
static void get_zeroed_page_test(void);
static void get_free_pages(void);

static int mem_gfp_init(void)
{
  printk(KERN_DEBUG "\n---=== GET FREE PAGE TEST STARTED ===---\n\n");

  get_free_page_test();
  get_zeroed_page_test();
  get_free_pages();
  
  return -1;
}
module_init(mem_gfp_init);


static void get_free_page_test(void)
{
  int i = 0;
  unsigned char* page = NULL;

  printk(KERN_DEBUG "\nmem: __get_free_page(GFP_KERNEL)\n\n");
  page = (unsigned char*) __get_free_page(GFP_KERNEL);
  if (NULL == page)
    {
      printk(KERN_WARNING "mem: get_zeroed_page allocation - fail\n");
      return;
    }

  printk(KERN_DEBUG "mem: after __get_free_page allocation\n");
  show_array(page, PAGE_SIZE);
  
  for(i = 0; i < PAGE_SIZE; i++)
    {
      page[i] = i % 0x100;
    }

  printk(KERN_DEBUG "mem: after writing to page\n");
  show_array(page, PAGE_SIZE);
  
  free_page((unsigned long)page);
  
}

static void get_zeroed_page_test(void)
{
  int i = 0;
  unsigned char* page = NULL;

  printk(KERN_DEBUG "\nmem: get_zeroed_page(GFP_KERNEL)\n\n");

  page = (unsigned char*) get_zeroed_page(GFP_KERNEL);
  if (NULL == page)
    {
      printk(KERN_WARNING "mem: get_zeroed_page allocation - fail\n");
      return;
    }

  printk(KERN_DEBUG "mem: after get_zeroed_page allocation\n");
  show_array(page, PAGE_SIZE);
  
  for(i = 0; i < PAGE_SIZE; i++)
    {
      page[i] = i % 0x100;
    }

  printk(KERN_DEBUG "mem: after writing to page\n");
  show_array(page, PAGE_SIZE);
  
  free_page((unsigned long)page);
}

static void get_free_pages(void)
{
  int i = 0;
  unsigned char* page = NULL;
  
  printk(KERN_DEBUG "\nmem: __get_free_pages(GFP_KERNEL)\n\n");

  page = (unsigned char*) __get_free_pages(GFP_KERNEL, PAGES_ORDER);

  if(NULL == page)
    {
      printk(KERN_DEBUG "mem: __get_free_pages allocation - fail\n");
      return;
    }
  
  printk(KERN_DEBUG "mem: after allocation with __get_free_pages\n");
  show_array(page, PAGE_SIZE * NUM_OF_PAGES);
    
   for(i = 0; i < PAGE_SIZE * NUM_OF_PAGES; i++)
    {
      page[i] = i % 0x100;
    }

  printk(KERN_DEBUG "mem: after writing to page\n");
  show_array(page, PAGE_SIZE * NUM_OF_PAGES);
    
  free_pages((unsigned long)page, PAGES_ORDER);  
}

static inline void show_array(void* arr ,int size)
{
  int i = 0;
  unsigned char* a =  (unsigned char*) arr;
   for(i = 0; i < size; i+=16)
    {
      printk(KERN_DEBUG "mem: offs %d: \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x \t%x\n", \
	     i, a[i], a[i + 1], a[i + 2], a[i + 3],		\
	     a[i + 4], a[i + 5], a[i + 6], a[i + 7], \
	     a[i + 8], a[i + 9], a[i + 10], a[i + 11], \
	     a[i + 12], a[i + 13], a[i + 14], a[i + 15]);
    }

}

