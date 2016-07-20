#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct mutex mtx;
static struct task_struct* th_a;
static struct task_struct* th_b;

static int th_a_entery(void* data)
{
  printk(KERN_DEBUG "mtx: thread A started\n");

  while(false == kthread_should_stop())
    {
      mutex_lock(&mtx);
      printk(KERN_DEBUG "mtx: thread A: lock\n");
      msleep(1000);
      mutex_unlock(&mtx);
      printk(KERN_DEBUG "mtx: thread A: unlock\n");
      msleep(1000);
    }

  printk(KERN_DEBUG "mtx: thread A exit\n");
  return 0;
}

static int th_b_entery(void* data)
{
  printk(KERN_DEBUG "mtx: thread B started\n");

  while(false == kthread_should_stop())
    {
      if(true == mutex_trylock(&mtx))
	{
	  printk(KERN_DEBUG "mtx: thread B: lock\n");
	  msleep(1000);
	  mutex_unlock(&mtx);
	  printk(KERN_DEBUG "mtx: thread B: unlock\n");
	}
      else
	{
	  printk(KERN_DEBUG "mtx: thread B: try lock - fail\n");
	}
      msleep(1000);
    }	

  printk(KERN_DEBUG "mtx: thread B exit\n");
  return 0;
}

static int __init mtx_init(void)
{
  printk(KERN_DEBUG "mtx: init\n");

  mutex_init(&mtx);

  th_a = kthread_run(th_a_entery, 0, "th_a");
  th_b = kthread_run(th_b_entery, 0, "th_b");

  return 0;
}
module_init(mtx_init);

static void __exit mtx_exit(void)
{
  kthread_stop(th_a);
  kthread_stop(th_b);

  printk(KERN_DEBUG "mtx: exit\n");
}
module_exit(mtx_exit);
