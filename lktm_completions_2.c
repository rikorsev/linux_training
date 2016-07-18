#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/completion.h>

MODULE_LICENSE("GPL");

typedef struct
{
  int number;
  //struct completion compl;
  struct task_struct* th;
} waiter_t;

#define NUM_OF_WAITERS 3

static waiter_t waiter[NUM_OF_WAITERS];

static struct completion compl_INIT_a;
static struct task_struct* th_INIT_a;

static struct completion compl_INIT_b;
static struct task_struct* th_INIT_b;


inline static struct task_struct* kthread_create_and_run(int (*entery)(void* data), void* data, char* name)
{
  struct task_struct* th;
  th = kthread_create(entery, data, name);
  if(NULL == th)
    {
      printk(KERN_WARNING "compl: thread %s creation fail\n", name);
      return th;
    }
  if(0 == wake_up_process(th))
    {
      printk(KERN_WARNING "compl: can't start thread %s\n", name);
    } 

  return th;
}

static int th_INIT_a_entery(void* data)
{
  int i;

  printk(KERN_DEBUG "compl: INIT A entery\n");

  /* one extra completion for clenup function */
  for(i = 0 ; i <= NUM_OF_WAITERS ; i++)
    {
      set_current_state(TASK_INTERRUPTIBLE);
      schedule_timeout(msecs_to_jiffies(1000));
      printk(KERN_DEBUG "compl: complete INIT A #%d\n", i+1);
      complete(&compl_INIT_a);
    }

  wake_up_process(th_INIT_b);

  printk(KERN_DEBUG "compl: INIT A exit\n");
    
  return 0;
}

static int th_INIT_b_entery(void* data)
{
  printk(KERN_DEBUG "compl: INIT B entery\n");

  set_current_state(TASK_INTERRUPTIBLE);
  schedule_timeout(msecs_to_jiffies(1000));

  complete_all(&compl_INIT_b);

  printk(KERN_DEBUG "compl: INIT B exit\n");
  return 0;
}

static int waiter_thread_entery(void* data)
{
  int n = *(int*)data;

  printk(KERN_DEBUG "compl: waiter %d: enter\n", n);

  printk(KERN_DEBUG "compl: waiter %d: wait for INIT A started...\n", n);
  wait_for_completion(&compl_INIT_a);
  printk(KERN_DEBUG "compl: waiter %d: INIT A completed\n", n);

  printk(KERN_DEBUG "compl: waiter %d: wait for INIT B statred...\n", n);
  wait_for_completion(&compl_INIT_b);
  printk(KERN_DEBUG "compl: waiter %d: INIT B completed\n", n);

  printk(KERN_DEBUG "compl: waiter %d: exit\n", n);

  return 0;
}


static int __init compl_init(void)
{
  int i;
  
  printk(KERN_DEBUG "compl: enter\n");

  init_completion(&compl_INIT_a);
  init_completion(&compl_INIT_b);

  for( i = 0 ; i < NUM_OF_WAITERS ; i++ )
    {
      waiter[i].number = i;
      waiter[i].th = NULL;
      waiter[i].th = kthread_create_and_run(waiter_thread_entery, &waiter[i].number, "waiter");
    }

  th_INIT_b = kthread_create(th_INIT_b_entery, 0, "INIT b");
  th_INIT_a = kthread_create_and_run(th_INIT_a_entery, 0, "INIT a");

  return 0;
}
module_init(compl_init);

static void __exit compl_exit(void)
{
  printk(KERN_DEBUG "compl: clenup\n");
  wait_for_completion(&compl_INIT_a);
  wait_for_completion(&compl_INIT_b);
  printk(KERN_DEBUG "compl: exit\n");
}
module_exit(compl_exit)
