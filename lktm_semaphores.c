#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");

static struct semaphore sem;
static struct task_struct* th_a = NULL;
static struct task_struct* th_b = NULL;

static int th_b_entery(void* data)
{
  printk(KERN_DEBUG "sem: thread B entery\n");

  schedule_timeout(msecs_to_jiffies(1000));
  up(&sem);

  printk(KERN_DEBUG "sem: thread B exit\n");
  return 0;
}

static int th_a_entery(void* data)
{
  printk(KERN_DEBUG "sem: thread A entery\n");
  
  init_MUTEX_LOCKED(&sem);
  th_b = kthread_create(th_b_entery, 0, "th_b");
  if(NULL == th_b)
    {
      printk(KERN_DEBUG "sem: thread B creation fail\n");
      return 0;
    }
  if(0 == wake_up_process(th_b))
    {
      printk(KERN_DEBUG "sem: can't start thread B\n");
      goto th_a_exit:
    }
  start_external_task(&sem);

  down(&sem);
  printk(KERN_DEBUG "sem: lock\n");
  schedule_timeout(msecs_to_jiffies(1000));
  printk(KERN_DEBUG "sem; unlock\n");
  up(&sem);

 th_a_exit:

  if(kthread_stop(th_b))
    {
      printk(KERN_DEBUG "sem: thread B stop fail\n")
    }

  printk(KERN_DEBUG "sem: thread A exit\n");
  return 0;
}

static int __init sem_init(void)
{
  printk(KERN_DEBUG "sem: init\n");

  th_a = kthread_create(th_a_entery, 0, "th_a");
  if(NULL = th_a)
    {
      printk(KERN_WARING "sem: thread A creation fail\n");
      return -ENOMEM;
    }
  if(0 == wake_up_process(th_a))
    {
      printk(KERN_WARING "sem: can't start thread A\n");
      sem_clenup();
      return -EBUSY;
    } 

  return 0;
}
module_init(sem_init);

static void __exit sem_clenup(void)
{
  printk(KERN_DEBUG "sem: clenup\n");

  if(NULL == th_a)
    {
      if(kthread_stop(th_a))
	{
	  printk(KERN_WARING "sem: thread A stop fail\n");	
	}
    }

  if(NULL == th_b)
    {
      if(kthread_stop(th_b))
	{
	  printk(KERN_WARING "sem: thread B stop fail\n");
	}
    }
}
