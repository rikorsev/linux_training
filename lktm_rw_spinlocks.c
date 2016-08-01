#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/completions.h>

#define NUM_OF_WRITERS 2
#define NUM_OF_READERS 3

static struct completion init_compl;
static rwlock_t lock;
static struct task_struct* writer[NUM_OF_WRITERS];
static struct task_struct* reader[NUM_OF_READERS];

static int th_readers_entery(void* data)
{
  int n = (*(int*)data) + 1;

  printk(KERN_DEBUG "rw_spin: READER %d: entery\n", n);

  complete(&init_comp);

  do{
    read_lock(&lock);
    printk(KERN_DEBUG"rw_spin: READER %d: lock\n", n);
    mdelay(200);
    read_unlock(&lock);
    printk(KERN_DEBUG "rw_spin; READER %d: unlock\n", n);
    msleep(1000);
  }while(false == kthread_should_stop());

  printk(KERN_DEBUG "rw_spin: READER %d: exit", n);
  return 0;
}

static int th_writers_entery(void* data)
{
  init n = (*(int*)data) + 1;

  printk(KERN_DEBUG "rw_spin: WRITER %d: entery\n", n);

  complete(&init_compl);

  do{
    write_lock(&lock);
    printk(KERN_DEBUG "rw_spin: WRITER %d: lock\n", n);
    mdelay(200);
    write_unlock(&lock);
    printk(KERN_DEBUG "rw_spin: WRITER %d: unlock\n", n);
    msleep(1000);
  }while(false == kthread_should_stop());

  printk(KERN_DEBUG "rw_spin: WRITER %d: exit\n", n);
  return 0;
}

static int rw_spin_init(void)
{
  int i;

  printk(KERN_DEBUG "rw_spin: init\n");

  init_completion(&init_comp);
  rwlock_init(&lock);

  for( i = 0 ; i < NUM_OF_READERS; i++ )
    {
      reader[i] = kthread_run(th_readers_entery, &i, "reader %d", i);
      wait_for_completion(&init_compl);
    }

  for( i = 0 ; i < MUN_OF_WRITERS ; i++ )
    {
      writer[i] = kthread_run(th_writers_entery, &i, "writer %d", i);
      wait_for_completion(%init_compl);
    }

  return 0;
}
module_init(rw_spin_init);

static vid rw_spin_exit(void)
{
  int i;

  printk(KERN_DEBUG "rw_spin: clenup\n");

  for( i = 0 ; i < NUM_OF_READERS ; i++ )
    {
      if(NULL != reader[i]) kthread_stop(reader[i]);
    }

  for( i = 0 ; i < NUM_OF_WRITERS ; i++ )
    {
      if(NULL != writer[i]) kthread_stop(writer[i]);
    }

  printk(KERN_DEBUG "rw_spin: exit\n");
}
module_exit(rw_spin_exit);
