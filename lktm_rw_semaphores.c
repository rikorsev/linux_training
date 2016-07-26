#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/rwsem.h>
#include <linux/delay.h>
#include <linux/completion.h>

#define NUM_OF_WRITERS 2
#define NUM_OF_READERS 3

static struct completion init_comp;
static struct rw_semaphore sem;
static struct task_struct* writer[NUM_OF_WRITERS];
static struct task_struct* reader[NUM_OF_READERS];

static int th_readers_entery(void* data)
{
  int n = (*(int*)data) + 1;

  printk(KERN_DEBUG "rw_sem: READER %d: entery\n", n);

  complete(&init_comp);
  
  do{

    down_read(&sem);
    printk(KERN_DEBUG "rw_sem: READER %d: down\n", n);
    msleep(1000);
    up_read(&sem);
    printk(KERN_DEBUG "rw_sem: READER %d: up\n", n);
    msleep(1000);
    
  }while(false == kthread_should_stop());

  printk(KERN_DEBUG "rw_sem: READER %d: exit\n", n);
  return 0;
}

static int th_writers_entery(void* data)
{
  int n = (*(int*)data) + 1;

  printk(KERN_DEBUG "rw_sem: WRITER %d: entery\n", n);

  complete(&init_comp);

  do{
    
    down_write(&sem);
    printk(KERN_DEBUG "rw_sem: WRITER %d: down\n", n);
    msleep(1000);
    up_write(&sem);
    printk(KERN_DEBUG "rw_sem: WRITER %d: up", n);
    msleep(1000);
    
  }while(false == kthread_should_stop());

  printk(KERN_DEBUG "re_sem: WRITER %d: exit\n", n);
  return 0;
}

static int __init rw_sem_init(void)
{
  int i;
  
  printk(KERN_DEBUG "rw_sem: init\n");

  init_completion(&init_comp);
  init_rwsem(&sem);
  
  for( i = 0 ; i < NUM_OF_READERS ; i++ )
    {
      reader[i] = kthread_run( th_readers_entery, &i, "reader %d", i);
      wait_for_completion(&init_comp);
    }

  for( i = 0 ; i < NUM_OF_WRITERS ; i++)
    {
      writer[i] = kthread_run( th_writers_entery, &i, "writer %d", i);
      wait_for_completion(&init_comp);
    }
  
  return 0;
}
module_init(rw_sem_init);

static void __exit rw_sem_exit(void)
{
  int i;
  
  printk(KERN_DEBUG "rw_sem: clenup\n");
  for( i = 0 ; i < NUM_OF_READERS ; i++)
    {
      if(NULL != reader[i]) kthread_stop(reader[i]);
    }

  for( i = 0 ; i < NUM_OF_WRITERS ; i++)
    {
      if(NULL != writer[i]) kthread_stop(writer[i]);
    }
  
  printk(KERN_DEBUG "rw_sem: exit\n");
}
module_exit(rw_sem_exit);
