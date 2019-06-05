#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> /* to access to user space*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksandr Kolosov");

/* define structure for our root object */
struct lktm_root
{
  struct kobject        obj;   // kernel object structure
  struct kobj_attribute attr;  // attribute structure
  u32                   state; // state variable
};

static ssize_t lktm_control_set(struct kobject *kobj, struct kobj_attribute *attr,  char *buf)
{

}

static ssize_t lktm_control_get(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

}

/* devine control attribute */
static struct kobj_attribute control = __ATTR(control, 0660, lktm_control_get,
                                                             lktm_control_set);


/* define sysfs operations for ktype */
// const struct sysfs_ops lktm_root_sysfs_ops = 
// {

// };

/* define ktype for lktm_obj */
// static struct kobj_type lktm_root_ktype = {
//   .sysfs_ops = &cache_sysfs_ops,
// };

/* define lktm root object */
struct lktm_root my_lktm_root = {0};

/* init module */
static int __init lktm_obj_init(void)
{
  int result = 0;

  printk(KERN_DEBUG "lktm: obj: init\n");

  /* let's init object structure and add it to sysfs*/
  result = kobject_init_and_add(&my_lktm_root.obj, struct kobj_type *ktype, kernel_kobj, "lktm")
  if(result < 0)
  {
    printk(KERN_WARNING "lktm: obj: kobject creation - fail\n");
    return result;
  }




  lktm_obj = kobject_create_and_add("lktm", kernel_kobj);
  if(NULL == lktm_obj)
  {
    printk(KERN_WARNING "lktm: obj: kobject creation - fail\n");
    return -1;
  }

  if(0 != sysfs_create_file(lktm_obj, &control.attr))
  {
    printk(KERN_WARNING "sysfs: file creation - fail\n");
  }

  return 0;
}
module_init(lktm_obj_init);

static void __exit lktm_obj_exit(void)
{
  printk(KERN_DEBUG "lktm: obj: clenup\n");

  /* remove control attribute */
  sysfs_remove_file(lktm_obj, &control.attr);

  /* remove root object */
  kobject_put(lktm_obj);

  printk(KERN_DEBUG "lktm: obj: exit\n");
}
module_exit(lktm_obj_exit);