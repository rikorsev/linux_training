#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h> /* for workqueue API */ 
#include <linux/kthread.h> /* for kthread API */
#include <linux/sched.h> /* for schedule_timeout */
#include <linux/jiffies.h> 

MODULE_LICENSE("GPL");

static void incrementer(struct work_struct* work);
static void decrementer(struct work_struct* work);

static int some_val_dec = 65535;
static int some_val_inc = 0;
static struct workqueue_struct* wq;

DECLARE_DELAYED_WORK(work_dec, decrementer);
struct work_struct work_inc;

static struct task_struct* th;

static void decrementer(struct work_struct* work)
{
  some_val_dec--;
  printk(KERN_DEBUG "th: dec %d\n", some_val_dec);
}

static void incrementer(struct work_struct* work)
{
  some_val_inc++;
  printk(KERN_DEBUG "th: inc %d\n", some_val_inc);
  
  /* workqueues can runs non atomic, so let's check it here */
  set_current_state(TASK_INTERRUPTIBLE);
  schedule_timeout(msecs_to_jiffies(500));

  printk(KERN_DEBUG "th: inc exit\n");
}

static int th_entery(void* data)
{

  printk(KERN_DEBUG "th: entered\n");

  while(1)
    {
      set_current_state(TASK_INTERRUPTIBLE);
      if(0 != schedule_timeout(msecs_to_jiffies(5000)))
	{
	  printk(KERN_WARNING "th: schadule timeout - fail\n");
	  break;
	}
      if(false == queue_work(wq, &work_inc))
	{
	  printk(KERN_WARNING "th: can't add work_inc to wq\n");
	}
      if(false == queue_delayed_work(wq, &work_dec, msecs_to_jiffies(500)))
	{
	  printk(KERN_WARNING "th: can't add work_dec to wq\n");
	}
      printk( KERN_DEBUG "+\n");
    }

  return 0;
}

static int th_init(void)
{
  wq = create_workqueue("my_wq");

  INIT_WORK(&work_inc, incrementer);

  th = kthread_create(th_entery, 0, "th");
  if(0 == wake_up_process(th))
    {
      printk(KERN_WARNING "th: can't starts th thread\n");
    }

  return 0;
}
module_init(th_init);

static void th_exit(void)
{
  int status;

  printk(KERN_DEBUG "th:exit\n");

  flush_workqueue(wq);
  destroy_workqueue(wq);

  status = kthread_stop(th);
  if(0 != status)
    {
      printk(KERN_WARNING "kthread_stop returns %d\n", status);
    }
}
module_exit(th_exit);
