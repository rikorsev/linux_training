#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> /* to access to user space*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksandr Kolosov");

/* define structure for our root object */
struct lktm_root
{
  struct kobject        obj;           // kernel object structure
  u32                   children_num;  // state variable
};

/* this function shows current state of lktm_root object */
static ssize_t lktm_state_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
  struct lktm_root *root = container_of(kobj, struct lktm_root, obj);

  printk(KERN_DEBUG "lktm: obj: show children number\n");

  /* what about copy to user ??? */

  return sprintf(buf, "%u\n", root->children_num);
}

/* this function stores new state of lktm_root object */
static ssize_t lktm_state_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
  //u8 i;
  //struct kobject *child = NULL;
  struct lktm_root *root = container_of(kobj, struct lktm_root, obj);

  printk(KERN_DEBUG "lktm: obj: store children number\n");

  /* what about copy from user ??? */

  sscanf(buf, "%u", &root->children_num);

  /* kill old children */
  // list_for_each(current, to_kset(kobj)->list)
  // {
  //   child = container_of(current, struct kset, list)->;
  // }

  // /* create new cildren */
  // for(i = 0; i < root->children_num; i++)
  // {
  //   child = kobject_create_and_add("child", kobj)
  //   if(child == NULL)
  //   {
  //     printk(KERN_WARNING "lktm: obj: child birth - fail\n");
  //     break;
  //   }

  //   if(kobject_set_name(child, "child%d", i) < 0)
  //   {
  //     printk(KERN_WARNING "lktm: obj: child rename - fail\n");
  //     break;
  //   }
  // }

  return count;
}

/* define state attribute */
static struct attribute lktm_root_def_attr = {
  .name = "state",
  .mode = S_IRUSR | S_IWUSR
};

/* it is the array for default attributes */
static struct attribute *lktm_root_def_attrs[] = {
  &lktm_root_def_attr,
  NULL
};

/* define sysfs operations for ktype */
static struct sysfs_ops lktm_root_def_ops = {
  .show = lktm_state_show,
  .store = lktm_state_store
};

/* define ktype for lktm_obj */
static struct kobj_type lktm_root_ktype = {
  .sysfs_ops = &lktm_root_def_ops,
  .default_attrs = lktm_root_def_attrs
};

/* define lktm root object */
struct lktm_root my_lktm_root = {0};

/* init module */
static int __init lktm_obj_init(void)
{
  int result = 0;

  printk(KERN_DEBUG "lktm: obj: init\n");

  printk(KERN_DEBUG "kset: 0x%0p\n", my_lktm_root.obj.kset);

  /* let's init object structure and add it to sysfs*/
  result = kobject_init_and_add(&my_lktm_root.obj, &lktm_root_ktype, kernel_kobj, "lktm");
  if(result < 0)
  {
    printk(KERN_WARNING "lktm: obj: kobject creation - fail\n");
    return result;
  }

  printk(KERN_DEBUG "kset: 0x%0p\n", my_lktm_root.obj.kset);

  return 0;
}
module_init(lktm_obj_init);

static void __exit lktm_obj_exit(void)
{
  printk(KERN_DEBUG "lktm: obj: clenup\n");

  /* remove root object */
  kobject_put(&my_lktm_root.obj);

  printk(KERN_DEBUG "lktm: obj: exit\n");
}
module_exit(lktm_obj_exit);