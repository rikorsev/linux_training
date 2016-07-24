#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>
//#include <linux/getcpu.h>
#include <linux/smp.h>

DEFINE_SPINLOCK(lock);
static struct task_struct* th_a = NULL;
static struct task_struct* th_b = NULL;
static struct timer_list tim;

static void tim_handler(unsigned long data)
{
  printk(KERN_DEBUG "slock: timer handler entery\n");
}

static int th_a_entery(void* data)
{
  unsigned long flag;
  
  printk(KERN_DEBUG "slock: thread A entery\n");

  do
    {
      printk(KERN_DEBUG "slock: thread A: CPU %d\n", smp_processor_id());
      
      tim.expires = jiffies + msecs_to_jiffies(100);
      tim.function = tim_handler;
      tim.data = 0;
      add_timer(&tim);
      printk(KERN_DEBUG "slock: tim go!\n");
      
      spin_lock_irqsave(&lock, flag);
      printk(KERN_DEBUG "slock: locked by thread A\n");
      mdelay(200);
      spin_unlock_irqrestore(&lock, flag);
      printk(KERN_DEBUG "slock: unlocked by thread A\n");
      msleep(1000);
      
    }while(false == kthread_should_stop());

  printk(KERN_DEBUG "slock: thread A exit\n");
  return 0;
}

static int th_b_entery(void* data)
{
  printk(KERN_DEBUG "slock: thread B entery\n");
  
  do
    {
      printk(KERN_DEBUG "slock: thread B: CPU %d\n", smp_processor_id());
      
      if(true == spin_trylock(&lock))
	{
	  printk(KERN_DEBUG "slock: locked by thread B\n");
	  mdelay(200);
	  spin_unlock(&lock);
	  printk(KERN_DEBUG "slock: unlocked by thread B\n");
	}
      else
	{
	  printk(KERN_DEBUG "slock: attempt to lock by thread B - fail \n");
	}
      msleep(300);
      
    }while(false == kthread_should_stop());

  printk(KERN_DEBUG "slock: thread B exit \n");
  return 0;
}

static int slock_init(void)
{
  printk(KERN_DEBUG "slock: init \n");

  th_a = kthread_run(th_a_entery, 0, "th a");
  th_b = kthread_run(th_b_entery, 0, "th b");
  
  return 0;
}
module_init(slock_init);

static void slock_exit(void)
{
  kthread_stop(th_a);
  kthread_stop(th_b);
  
  printk(KERN_DEBUG "slock: exit\n");
}
module_exit(slock_exit);
