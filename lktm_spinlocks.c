#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>

DEFINE_SPINLOCK(lock);
struct task_struct* th_a = NULL;
struct task_struct* th_b = NULL;


static int th_a_entery(void* data)
{
  printk(KERN_DEBUG "slock: thread A entery\n");

  do
    {
      
    }while(false == kthread_should_stop);

  printk(KERN_DEBUG "slock: thread A exit\n");
  return 0;
}

static int th_b_entery(void* data)
{
  printk(KERN_DEBUG "slock: thread B entery\n");
  
  do
    {
	
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
  printk(KERN_DEBUG "slock: exit\n");
}
module_exit(slock_exit);
