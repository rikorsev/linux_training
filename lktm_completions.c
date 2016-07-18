#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/completion.h>

MODULE_LICENSE("GPL");

#define NUM_OF_WAITERS 3

//static struct completion compl_a;
static struct completion compl_b;
static struct completion compl_waiter_a;
static struct completion compl_waiter_b;
static struct completion compl_waiter_c;
static struct task_struct* th_waiter_a = NULL;
static struct task_struct* th_waiter_b = NULL;
static struct task_struct* th_waiter_c = NULL;
static struct task_struct* th_initializer = NULL;

static void __exit compl_clenup(void);

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

static int compl_th_initializer_entery(void* data)
{
  int i;

  printk(KERN_DEBUG "compl: thread initializer entery\n");

  for(i = 0 ; i < NUM_OF_WAITERS ; i++)
    {
      set_current_state(TASK_INTERRUPTIBLE);
      schedule_timeout(msecs_to_jiffies(1000));
      printk(KERN_DEBUG "compl: complete INIT #%d\n", i+1);
      complete(&compl_b);
    }

  printk(KERN_DEBUG "compl: thread initializer exit\n");
    
  return 0;
}

static int compl_th_waiter_a_entery(void* data)
{
  printk(KERN_DEBUG "compl: waiter A entery\n");
  
  //init_completion(&compl_a);

  while(false == kthread_should_stop())
    {
      th_initializer = kthread_create_and_run(compl_th_initializer_entery, 0, "th_initializer");
      if(NULL == th_initializer)
	{
	  printk(KERN_DEBUG"compl: initializer thread creation fail\n");
	  	  goto th_waiter_a_exit;
	}
      
      complete_all(&compl_waiter_a);
      //reinit_completion(&compl_a);
 
      printk(KERN_DEBUG "compl: waiter A: compl INIT: start waiting...\n");
      wait_for_completion(&compl_b);
      printk(KERN_DEBUG "compl: waiter A: compl INIT: completion recived\n");

      //set_current_state(TASK_INTERRUPTIBLE);
      //schedule_timeout(msecs_to_jiffies(5000));
      
      wait_for_completion(&compl_waiter_b);
      printk(KERN_DEBUG "compl: waiter A: recive completion from waiter B\n");

      wait_for_completion(&compl_waiter_c);
      printk(KERN_DEBUG "compl: waiter A: recive completion from waiter C\n");

      printk(KERN_DEBUG "==============COMPL_CYCLE_IS_OVER=============\n\n");
    }

  th_waiter_a_exit:
  
  printk(KERN_DEBUG "compl: waiter A exit\n");
  //complete_and_exit(&compl_waiter_a, 0);
  complete_all(&compl_waiter_a);
  complete_all(&compl_b);
  kthread_stop(th_waiter_b);
  kthread_stop(th_waiter_c);
  return 0;
}

static int compl_th_waiter_b_entery(void* data)
{
  printk(KERN_DEBUG "compl: waiter B entery\n");

  while(false == kthread_should_stop())
    {
      printk(KERN_DEBUG "compl: waiter B: reinit waiter A completion\n");
      reinit_completion(&compl_waiter_a);

      printk(KERN_DEBUG "compl: waiter B: waiting for A start...\n");
      if(0 == wait_for_completion_timeout(&compl_waiter_a, msecs_to_jiffies(5000)))
	{
	  printk(KERN_DEBUG "compl: waiter B: waiting for A - timeout\n");
	  //continue;
	}
      else
	{
	  printk(KERN_DEBUG "compl: waiter B: recived completion from waiter A\n");
	}
      
      printk(KERN_DEBUG "compl: waiter B: compl INIT: start waiting...\n");
      if(0 == wait_for_completion_timeout(&compl_b, msecs_to_jiffies(5000)))
	{
	  printk(KERN_DEBUG "compl: waiter B: waiting for INIT timeout\n");
	  //continue;
	}
      else
	{
	  printk(KERN_DEBUG "compl: waiter B: compl INIT: completion recived\n");
	}
      
      complete(&compl_waiter_b);
    }

  //waiter_b_exit:
  printk(KERN_DEBUG "compl: waiter B exit\n");
  complete_and_exit(&compl_waiter_b, 0);

  return 0;
}

static int compl_th_waiter_c_entery(void* data)
{
  printk(KERN_DEBUG "compl: waiter C entery\n");

  while(false == kthread_should_stop())
    {

      printk(KERN_DEBUG "compl: waiter C: reinit waiter A completion\n");
      reinit_completion(&compl_waiter_a);

      printk(KERN_DEBUG "compl: waiter C: waiting for A start...\n");
      if(0 == wait_for_completion_timeout(&compl_waiter_a, msecs_to_jiffies(5000)))
	{
	  printk(KERN_DEBUG "compl: waiter C: waiting for A timeout!!!\n");
	  //continue;
	  //	  goto waiter_c_exit;
	}
      else
	{
	  printk(KERN_DEBUG "compl: waiter C: recived completion from waiter A\n");
	}
      
      printk(KERN_DEBUG "compl: waiter C: compl INIT: start waiting...\n");
	if(0 == wait_for_completion_timeout(&compl_b, msecs_to_jiffies(5000)))
	{
	  printk(KERN_DEBUG "compl: waiter C: waiting for INIT timeout!!!\n");
	  //	  continue;
	  //goto waiter_c_exit;
	}
      else
	{
	  printk(KERN_DEBUG "compl: waiter C: compl INIT: completion recived\n");
	}
      
      complete(&compl_waiter_c);
    }

  //waiter_c_exit:
  printk(KERN_DEBUG "compl: waiter C exit\n");
  complete_and_exit(&compl_waiter_c, 0);

  return 0;
}

static int __init compl_init(void)
{
  printk(KERN_DEBUG "compl: init\n");

  init_completion(&compl_b);
  init_completion(&compl_waiter_a);
  init_completion(&compl_waiter_b);
  init_completion(&compl_waiter_c);

  th_waiter_a = kthread_create_and_run(compl_th_waiter_a_entery, 0, "waiter_a");
  th_waiter_b = kthread_create_and_run(compl_th_waiter_b_entery, 0, "waiter_b");
  th_waiter_c = kthread_create_and_run(compl_th_waiter_c_entery, 0, "waiter_c");
 
  if(NULL == th_waiter_a ||
     NULL == th_waiter_b ||
     NULL == th_waiter_c)
    {
      compl_clenup();
      return -1;
    }
 
  return 0;
}
module_init(compl_init);

static void __exit compl_clenup(void)
{
  printk(KERN_DEBUG "compl: clenup\n");

if( NULL != th_waiter_a) 
    {
      printk(KERN_DEBUG "compl: waiter A: cleanup started...\n");
      kthread_stop(th_waiter_a);
      //wait_for_completion(&compl_waiter_a);
      printk(KERN_DEBUG "compl: waiter A: completed\n");
    }
/*
  if( NULL != th_waiter_b) 
    {
      printk(KERN_DEBUG "compl: waiter B: cleanup started...\n");
      //complete_all(&compl_waiter_a);
      //complete_all(&compl_b);
      kthread_stop(th_waiter_b);
      //wait_for_completion(&compl_waiter_b);

      printk(KERN_DEBUG "compl: waiter B: completed\n");
    }

  if( NULL != th_waiter_c)
    {
      printk(KERN_DEBUG "compl: waiter C: cleanup started...\n");
      //complete(&compl_waiter_a);
      //complete(&compl_b);
      kthread_stop(th_waiter_c);
      //wait_for_completion(&compl_waiter_c);
      printk(KERN_DEBUG "compl: waiter C: completed\n");  
    }
*/
}
module_exit(compl_clenup);
