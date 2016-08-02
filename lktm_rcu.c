#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linuc/rcupdate.h>

#define NUM_OF_READERS 3

static struct completion init_comp;
static task_struct* reader[NUM_OF_READERS];
static task_struct* writer;
static int some_data = 0;

static int th_readers_entery(void* data)
{
  int n = (*(int*)data) + 1;

  printk(KERN_DEBUG "rcu_test: READER %d: entery\n", n);

  complete(&init_comp);

  do{
    
    rcu_read_lock();
    printk(KERN_DEBUG "rcu_test: READER %d: lock\n", n);
    msleep(1000);
    rcu_raed_unlock();
    printk(KERN_DEBUG "rcu_test: READER %d: unlock\n", n);
    msleep(1000);
    
  }while(false == kthread_should_stop());
  
  printk(KERN_DEBUG "rcu_test: READER %d: exit\n", n);
  return 0;
}

static int th_writers_entery(void* data)
{
  printk(KERN_DEBUG "rcu_test: WRITER: entery\n");

  do{

    msleep(1000);
  }while(false == kthread_should_stop());
  
  printk(KERN_DEBUG "rcu_test: WRITER: exit\n");
  return 0;
}

static int __init rcu_test_init(void)
{
  int i;
  
  printk(KERN_DEBUG "rcu_test: init\n");

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
  
  printk(KERN_DEBUG "rcu_test: exit\n");
}
module_exit(rcu_test_exit);
