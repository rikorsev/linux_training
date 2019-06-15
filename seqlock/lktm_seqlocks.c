#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/seqlock.h>
#include <linux/delay.h>
#include <linux/completion.h>

#define NUM_OF_WRITERS 2
#define NUM_OF_READERS 3

MODULE_LICENSE("GPL");

static struct completion init_comp;
static seqlock_t lock;
static struct task_struct* writer[NUM_OF_WRITERS];
static struct task_struct* reader[NUM_OF_READERS];

static int th_readers_entery(void* data)
{
  int n = (*(int*)data) + 1;
  int count = 0;

  printk(KERN_DEBUG "seqlock: READER %d: entery\n", n);

  complete(&init_comp);
  
  do{
    
    do{
      
      count = read_seqbegin(&lock);
      printk(KERN_DEBUG "seqlock: READER %d: attempt to read - START\n", n);
      mdelay(200);
      if(0 == read_seqretry(&lock, count)) break;
      printk(KERN_DEBUG "seqlock: READER %d: attempt to read - FAIL\n", n);
      
      if(true == kthread_should_stop()) goto reader_exit;

      msleep(1000);

    }while(1);

    printk(KERN_DEBUG "seqlock: READER %d: attempt to read - SUCCESS\n", n);
    msleep(1000);

  }while(false == kthread_should_stop());

 reader_exit:

  printk(KERN_DEBUG "seqlock: READER %d: exit\n", n);
  return 0;
}

static int th_writers_entery(void* data)
{
  int n = (*(int*)data) + 1;

  printk(KERN_DEBUG "seqlock: WRITER %d: entery\n", n);

  complete(&init_comp);

  do{

    write_seqlock(&lock);
    printk(KERN_DEBUG "seqlock: WRITER %d: lock\n", n);
    mdelay(300);
    write_sequnlock(&lock);
    printk(KERN_DEBUG "seqlock: WRITER %d: unlock\n", n);
    msleep(300);

  }while(false == kthread_should_stop());

  printk(KERN_DEBUG "seqlock: WRITER %d: exit\n", n);
  return 0;
}

static int __init rw_sem_init(void)
{
  int i;
  
  printk(KERN_DEBUG "seqlock: init\n");

  init_completion(&init_comp);
  seqlock_init(&lock);
  
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
  
  printk(KERN_DEBUG "seqlock: clenup\n");

  for( i = 0 ; i < NUM_OF_WRITERS ; i++)
    {
      if(NULL != writer[i]) kthread_stop(writer[i]);
    }

  for( i = 0 ; i < NUM_OF_READERS ; i++)
    {
      if(NULL != reader[i]) kthread_stop(reader[i]);
    }
  
  printk(KERN_DEBUG "seqlock: exit\n");
}
module_exit(rw_sem_exit);
