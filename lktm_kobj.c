#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> /* to access to user space*/
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksandr Kolosov");

/* define structure for our root object */
struct lktm_kobj
{
  struct kobject        kobj;  // kernel object structure
  u32                   data;  // state variable
};

/* this function shows current state of lktm_root object */
static ssize_t lktm_value_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
  struct lktm_kobj *child = container_of(kobj, struct lktm_kobj, kobj);

  printk(KERN_DEBUG "lktm: obj: show value\n");

  return sprintf(buf, "%u\n", child->data);
}

/* this function stores new state of lktm_root object */
static ssize_t lktm_value_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
  struct lktm_kobj *child = container_of(kobj, struct lktm_kobj, kobj);

  printk(KERN_DEBUG "lktm: obj: store value\n");

  sscanf(buf, "%u", &child->data);
  return count;
}

/* release function */
static void lktm_release(struct kobject *kobj)
{
  struct lktm_kobj *child = container_of(kobj, struct lktm_kobj, kobj);

  printk(KERN_DEBUG "lktm: obj: release %s\n", kobject_name(kobj));

  kfree(child);
}

/* define state attribute */
static struct attribute lktm_def_attr = {
  .name = "value",
  .mode = S_IRUSR | S_IWUSR
};

/* it is the array for default attributes */
static struct attribute *lktm_def_attrs[] = {
  &lktm_def_attr,
  NULL
};

/* define sysfs operations for ktype */
static struct sysfs_ops lktm_def_ops = {
  .show = lktm_value_show,
  .store = lktm_value_store
};

/* define ktype for lktm_obj */
static struct kobj_type lktm_ktype = {
  .release = lktm_release,
  .sysfs_ops = &lktm_def_ops,
  .default_attrs = lktm_def_attrs
};

/* define lktm root object */
static struct kset *parent_kset = NULL;

/* create new object */
static void lktm_create(u8 index)
{
  struct lktm_kobj *child = NULL;
  int result = -1;

  child = kzalloc(sizeof(*child), GFP_KERNEL);
  if(child == NULL)
  {
    printk(KERN_DEBUG "lktm: obj: fail to create an object\n");
    return;
  }

  child->kobj.kset = parent_kset;

  result = kobject_init_and_add(&child->kobj, &lktm_ktype, NULL, "child%d", index);
  if(result < 0)
  {
    printk(KERN_WARNING "lktm: obj: kobject init and add - fail. Result %d\n", result);
    kfree(child);
  }
}

static void lktm_del_all(void)
{
  struct kobject *k = NULL;
  struct kobject *tmp = NULL;

  list_for_each_entry_safe(k, tmp, &parent_kset->list, entry) {
    kobject_put(k);
  }
}

static u8 lktm_get_number(void)
{
  u8 count = 0;
  struct list_head *pos = NULL;

  list_for_each(pos, &parent_kset->list) {
    count++;
  }
  return count;
}

static ssize_t lktm_kobj_create_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
  printk(KERN_DEBUG "lktm: obj: show number of created children\n");

  return sprintf(buf, "%u\n", lktm_get_number());
}

static ssize_t lktm_kobj_create_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
  u32 number = 0;
  u8 i;

  printk(KERN_DEBUG "lktm: obj: store new children\n");

  sscanf(buf, "%u", &number);

  lktm_del_all();

  for(i = 0; i < number; i++)
  {
    lktm_create(i);
  }

  return count;
}

static struct kobj_attribute create = __ATTR(create, S_IRUSR | S_IWUSR, lktm_kobj_create_show, lktm_kobj_create_store);

/* init module */
static int __init lktm_obj_init(void)
{
  printk(KERN_DEBUG "lktm: obj: init\n");

  parent_kset = kset_create_and_add("lktm", NULL, kernel_kobj);
  if(parent_kset == NULL)
  {
    printk(KERN_WARNING "lktm: obj: kset creation - fail\n");
    return -1;
  }

  if(0 != sysfs_create_file(&parent_kset->kobj, &create.attr))
  {
    printk(KERN_WARNING "lktm: obj: file creation - fail\n");
    goto clean_kset;
  }

  return 0;

clean_kset:
  kset_unregister(parent_kset);

  return result;
}
module_init(lktm_obj_init);

static void __exit lktm_obj_exit(void)
{
  printk(KERN_DEBUG "lktm: obj: clenup\n");

  lktm_del_all();

  sysfs_remove_file(&parent_kset->kobj, &create.attr);

  kset_unregister(parent_kset);

  printk(KERN_DEBUG "lktm: obj: exit\n");
}
module_exit(lktm_obj_exit);