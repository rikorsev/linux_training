#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
//#include <linux/fs.h>
#include <asm/uaccess.h> /* to access to user space*/

MODULE_LICENSE("GPL");

struct vled
{
  char* name;
  int num;
  unsigned int pwm;
  bool state;
};

static struct vled my_vled =
  {
    .name = "LED",
    .num = 0,
    .pwm = 0,
    .state = false
  };

static int value = 0;
static struct kobject* kobj = NULL;

static ssize_t lktm_sysfs_get_value(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t lktm_sysfs_set_value(struct kobject *kobj, struct kobj_attribute *attr, \
				    const char *buf, size_t count);

static ssize_t lktm_vled_get_params(struct kobject *kobj, struct kobj_attribute *attr,	char *buf);
static ssize_t lktm_vled_set_params(struct kobject *kobj, struct kobj_attribute *attr,
				    const char *buf, size_t count);

static struct kobj_attribute attr = __ATTR(value, 0660, lktm_sysfs_get_value, \
					   lktm_sysfs_set_value);

static struct kobj_attribute vled_attr = __ATTR(vled, 0660, lktm_vled_get_params, \
						lktm_vled_set_params);

static int __init lktm_sysfs_init(void)
{
  printk(KERN_DEBUG "sysfs: init\n");

  kobj = kobject_create_and_add("lktm", kernel_kobj);
  if(NULL == kobj)
    {
      printk(KERN_WARNING "sysfs: kobject creation - fail\n");
      return -1;
    }

  if(0 != sysfs_create_file(kobj, &attr.attr))
    {
      printk(KERN_WARNING "sysfs: file creation - fail\n");
    }

  if(0 != sysfs_create_file(kobj, &vled_attr.attr))
    {
      printk(KERN_WARNING "sysfs: vled: file creation - fail\n");    
    }
  
  return 0;
}
module_init(lktm_sysfs_init);

static void __exit lktm_sysfs_exit(void)
{
  printk(KERN_DEBUG "sysfs: clenup\n");

  sysfs_remove_file(kobj, &attr.attr);
  sysfs_remove_file(kobj, &vled_attr.attr);
  kobject_put(kobj);
  
  printk(KERN_DEBUG "sysfs: exit\n");
}
module_exit(lktm_sysfs_exit);

static ssize_t lktm_sysfs_get_value(struct kobject *kobj, struct kobj_attribute *attr,	char *buf)
{
  printk(KERN_DEBUG "sysfs: set value\n");
  return sprintf(buf, "%d\n", value);
}

static ssize_t lktm_sysfs_set_value(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
  printk(KERN_DEBUG "sysfs: get value\n");
  sscanf(buf, "%du", &value);
  return count;
}

static ssize_t lktm_vled_get_params(struct kobject *kobj, struct kobj_attribute *attr,	char *buf)
{
  printk(KERN_DEBUG "sysfs: vled: get params\n");

  if(0 != copy_to_user(buf, &my_vled, sizeof(struct vled)))
    {
      printk(KERN_DEBUG "sysfs: vled: params copy - fail\n");
      return -1;
    }

  return sizeof(struct vled);
}

static ssize_t lktm_vled_set_params(struct kobject *kobj, struct kobj_attribute *attr,
				    const char *buf, size_t count)
{
  printk(KERN_DEBUG "sysfs: vled: set params\n");

  if(sizeof(struct vled) != count)
    {
      printk(KERN_WARNING "sysfs: vled: wrong params size\n");
      return -1;
    }

  if(0 != copy_from_user(&my_vled, buf, count))
    {
      printk(KERN_DEBUG "sysfs: vled: params copy - fail\n");
      return -1;
    }
  return count;
    
}
