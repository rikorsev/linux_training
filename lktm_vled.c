#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");

static unsigned short vled_pwm = 0;
static bool vled_state = false;

static int __init vled_init(void)
{
  printk(KERN_DEBUG "vled: init\n");
  return 0;
}
module_init(vled_init);

static void __exit vled_exit(void)
{
  printk(KERN_DEBUG "vled: exit\n");
}
module_exit(vled_exit);

static void vled_set_state(bool state)
{
  printk(KERN_DEBUG"vled: set state: %s\n", true == state ? "on": "off");
  vled_state = state;
}
EXPORT_SYMBOL(vled_set_state);

static bool vled_get_state(void)
{
  printk(KERN_DEBUG"vled: get state: %s\n", true == vled_state ? "on": "off");
  return vled_state;
}
EXPORT_SYMBOL(vled_get_state);

static void vled_set_pwm(unsigned short pwm)
{
  printk(KERN_DEBUG "vled: set pwm: %d\n", pwm);
  vled_pwm = pwm;
}
EXPORT_SYMBOL(vled_set_pwm);

static unsigned short vled_get_pwm(void)
{
  printk(KERN_DEBUG "vled: get pwm: %d\n", vled_pwm);
  return vled_pwm;
}
EXPORT_SYMBOL(vled_get_pwm);
