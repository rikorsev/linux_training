#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/rcupdate.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#define NUM_OF_READERS 3

MODULE_LICENSE("GPL");

static struct completion init_comp;
static struct task_struct* reader[NUM_OF_READERS];
static struct task_struct* writer;
static int* some_data = NULL;

static int th_readers_entery(void* data)
{
  int n = (*(int*)data) + 1;

  printk(KERN_DEBUG "rcu_test: READER %d: entery\n", n);

  complete(&init_comp);

  do{
    
    rcu_read_lock();
    printk(KERN_DEBUG "rcu_test: READER %d: lock\n", n);
    msleep(1000);
    printk(KERN_DEBUG "rcu_test: READER %d: value %d\n", n, *rcu_dereference(some_data));
    rcu_read_unlock();
    printk(KERN_DEBUG "rcu_test: READER %d: unlock\n", n);
    msleep(1000);
    
  }while(false == kthread_should_stop());
  
  printk(KERN_DEBUG "rcu_test: READER %d: exit\n", n);
  return 0;
}

static int th_writers_entery(void* data)
{
  int* new_data = NULL;
  int* old_data = NULL; 

  printk(KERN_DEBUG "rcu_test: WRITER: entery\n");
 
  do{
    new_data = kmalloc(sizeof(int), GFP_KERNEL);

    if(NULL == new_data)
      {
	printk(KERN_WARNING "rcu_test: WRITER: can't allocate memory\n");
	continue;
      }

    old_data = rcu_dereference(some_data);
    printk(KERN_DEBUG "rcu_test: WRITER: old_data %d\n", *old_data);
    *new_data = *old_data++;
    printk(KERN_DEBUG "rcu_test: WRITER: new_data %d\n", *new_data);
    rcu_assign_pointer(some_data, new_data);

    synchronize_rcu();
    printk(KERN_DEBUG "rcu_test: WRITER: synchronize\n");

    kfree(old_data);

    msleep(1000);
  }while(false == kthread_should_stop());
  
  printk(KERN_DEBUG "rcu_test: WRITER: exit\n");
  return 0;
}

static int __init rcu_test_init(void)
{
  int i;
  
  printk(KERN_DEBUG "rcu_test: init\n");

  some_data = kmalloc(sizeof(int), GFP_KERNEL);
  if(NULL == some_data) 
    {
      printk(KERN_WARNING "rcu_test: cant allocate memory\n");
      return -1;
    }
  *some_data = 0;

  init_completion(&init_comp);
  
  for( i = 0 ; i < NUM_OF_READERS ; i++ )
    {
      reader[i] = kthread_run( th_readers_entery, &i, "reader %d", i);
      wait_for_completion(&init_comp);
    }

  writer = kthread_run( th_writers_entery, 0, "writer");
    
  return 0;
}
module_init(rcu_test_init);

static void __exit rcu_test_exit(void)
{
  int i;
  
  printk(KERN_DEBUG "rcu_test: cleneup\n");

  for( i = 0 ; i < NUM_OF_READERS ; i++ )
    {
      if(NULL != reader[i]) kthread_stop(reader[i]);
    }

  if(NULL != writer) kthread_stop(writer);

  if(NULL != some_data) kfree(some_data);
  
  printk(KERN_DEBUG "rcu_test: exit\n");
}
module_exit(rcu_test_exit);
