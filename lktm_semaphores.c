#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/semaphore.h>

MODULE_LICENSE("GPL");

#define NUM_OF_INVADERS 10

typedef struct
{
  int number;
  unsigned long delay;
  struct task_struct* th;
}invader_t;

static invader_t inv[NUM_OF_INVADERS];
static const char* vaider_said = "Together we will rule the galaxy";
static struct semaphore sem;

static int invaider_entery(void* data)
{
  int n = ((invader_t*)data) -> number;
  unsigned long delay = ((invader_t*)data) -> delay;

  printk(KERN_DEBUG "sema: invaider %d: delay %d\n", n, (int)delay);

  down(&sem);
  printk(KERN_DEBUG "sema: invaider %d: paradise land conquered successfully!\n", n);
  msleep(delay);
  up(&sem);
  printk(KERN_DEBUG "sema: invaider %d said: \'%s\'  and then, suddenly, died\n", n, vaider_said);

  return 0;
}

static int semaphores_init(void)
{
  int i;

  printk(KERN_DEBUG "sema: init\n");

  sema_init(&sem, 3);

  for( i = 0 ; i < NUM_OF_INVADERS ; i++ )
    {
      inv[i].number = i;
      inv[i].delay = (i + 1) * 100;
      inv[i].th = kthread_run(invaider_entery, &inv[i], "invaider");
    }

  return 0;
}
module_init(semaphores_init);

static void semaphores_exit(void)
{
  printk(KERN_DEBUG "sema: exit\n");
}
module_exit(semaphores_exit);
