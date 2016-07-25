#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h> /* timer API */
#include <linux/interrupt.h> /* tasklet API */
#include <linux/jiffies.h> /* jiffies API */
#include <linux/delay.h> /* for delay functions */
#include <linux/fs.h> /* for file_operations */
#include <linux/proc_fs.h> /* for adding files to /proc */
#include <linux/slab.h> /* for kmalloc */

MODULE_LICENSE("GPL");

#define PROC_DIR_NAME "sync"
#define PROC_FILE_NAME "stat"

static ssize_t proc_read(struct file* f, char* __user buff, size_t sz, loff_t* offp);

static const struct file_operations proc_fops = 
  {
    .owner = THIS_MODULE,
    //    .open = proc_open,
    .read = proc_read
  };

static void tasklet_run_timer_handler(unsigned long val);

static int timer_var = 0;
static int tasklet_var = 0;
static struct timer_list inc_timer;
static struct timer_list tasklet_run_timer = TIMER_INITIALIZER(tasklet_run_timer_handler, 0, 0);
static struct tasklet_struct some_tasklet;

static struct proc_dir_entry* proc_dir_entry;
static struct proc_dir_entry* proc_file_entry;
/*
static int proc_open(struct inode* node, struct file* f)
{
  return 0;
}
*/
static ssize_t proc_read(struct file* f, char* __user buff, size_t sz, loff_t* offp)
{
  size_t len = 0;
  char* tmp_buff = NULL;

  printk(KERN_DEBUG "sync: proc read\n");
  printk(KERN_DEBUG "sync: size = %d\n", sz);
  if(4096 == sz)
    {
      tmp_buff = kmalloc(sz, GFP_KERNEL);
      if(NULL == tmp_buff)
	{
	  printk(KERN_WARNING "sync: tmp buff allocation fail\n");
	  return 0;
	}

      len = sprintf(tmp_buff, "timer_var = %d, tasklet_var = %d\n", timer_var, tasklet_var);
      if ( 0 != copy_to_user(buff, tmp_buff, len))
	{
	  printk(KERN_WARNING "sync: could not copy data to user buffer \n");
	  len = 0;
	}
      kfree(tmp_buff);

      return len;
    }
  else
    {
      return 0;
    }
}

static void inc_timer_handler(unsigned long val_ptr)
{
  (*(int*)val_ptr)++;
  
  printk(KERN_DEBUG "sync: timer inc %d\n", (*(int*)val_ptr));

  mod_timer(&inc_timer, jiffies + msecs_to_jiffies(1000));
}

static void tasklet_run_timer_handler(unsigned long val)
{
  printk(KERN_DEBUG "sync: tasklet schaduled\n");

  tasklet_schedule(&some_tasklet);
  
  mod_timer(&tasklet_run_timer, jiffies + msecs_to_jiffies(2000));
}

static void tasklet_handler(unsigned long val)
{
  (*(int*)val)++;

  printk(KERN_DEBUG "sync: tasklet inc %d\n", (*(int*)val));
}

static int sync_init(void)
{
  printk(KERN_DEBUG "sync: init\n");

  ndelay(1000);
  printk(KERN_DEBUG "sync: ndelay(1000) expires\n");

  udelay(1000);
  printk(KERN_DEBUG "sync: udelay(1000) expires\n");

  mdelay(100);
  printk(KERN_DEBUG "sync: mdelay(100) expires\n");
  
  proc_dir_entry = proc_mkdir(PROC_DIR_NAME, NULL);
  proc_file_entry = proc_create(PROC_FILE_NAME, 0, proc_dir_entry, &proc_fops );
 
  tasklet_init(&some_tasklet, tasklet_handler, (unsigned long)(&tasklet_var));
  
  inc_timer.expires = jiffies + msecs_to_jiffies(1000);
  inc_timer.function = inc_timer_handler;
  inc_timer.data = (unsigned long)(&timer_var);

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
  
  proc_remove(proc_file_entry);
  proc_remove(proc_dir_entry);

  del_timer_sync(&inc_timer);
  del_timer_sync(&tasklet_run_timer);
  tasklet_kill(&some_tasklet);  
}
module_exit(sync_cleanup);
