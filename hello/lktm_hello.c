#include <linux/init.h> /* This include for macroses __init and __exit */
#include <linux/module.h> /* This include for macroses module_init and module_exit */
#include <linux/kernel.h> /* Contains log level defines such as KERN_ALERT and so on */

MODULE_LICENSE("GPL");

static int __init hello_init(void)
{
  printk(KERN_ALERT "Hello, world!\n");
  return 0;
}

static void __exit hello_exit(void)
{
  printk(KERN_ALERT "Goodby, crue world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
