#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h> /* timer API */
#include <linux/interrupt.h> /* tasklet API */
#include <linux/jiffies.h> /* jiffies API */
#include <linux/delay.h> /* for delay functions */
static void tasklet_run_timer_handler(unsigned long val);

static int some_value = 0;
static struct timer_list inc_timer;
static struct timer_list tasklet_run_timer = TIMER_INITIALIZER(tasklet_run_timer_handler, 0, 0);
static struct tasklet_struct some_tasklet;

static void inc_timer_handler(unsigned long val_ptr)
{
  (*(int*)val_ptr)++;
  
  printk(KERN_DEBUG "sync: inc %d\n", (*(int*)val_ptr));

  mod_timer(&inc_timer, jiffies + msecs_to_jiffies(1000));
}

static void tasklet_run_timer_handler(unsigned long val)
{
  printk(KERN_DEBUG "sync: tasklet start\n");

  tasklet_schedule(&some_tasklet);
  
  mod_timer(&tasklet_run_timer, jiffies + msecs_to_jiffies(2000));
}

static void tasklet_handler(unsigned long val)
{
  printk(KERN_DEBUG "sync: tasklet starts\n");
}

static int sync_init(void)
{
  printk(KERN_DEBUG "sync: init\n");

  ndelay(1000);
  printk(KERN_DEBUG "sync: ndelay(1000) expires\n");

  udelay(1000);
  printk(KERN_DEBUG "sync: udelay(1000) expires\n");

  mdelay(1000);
  printk(KERN_DEBUG "sync: mdelay(1000) expires\n");
  
  tasklet_init(&some_tasklet, tasklet_handler, 0);
  
  inc_timer.expires = jiffies + msecs_to_jiffies(1000);
  inc_timer.function = inc_timer_handler;
  inc_timer.data = (unsigned long)(&some_value);

  init_timer(&inc_timer);
  tasklet_run_timer.expires = jiffies + msecs_to_jiffies(2000);
  
  add_timer(&inc_timer);
  add_timer(&tasklet_run_timer);
    
  return 0;
}
module_init(sync_init);

static void sync_cleanup(void)
{
  printk(KERN_DEBUG "sync: exit\n");

  del_timer_sync(&inc_timer);
  del_timer_sync(&tasklet_run_timer);
  tasklet_kill(&some_tasklet);  
}
module_exit(sync_cleanup);
