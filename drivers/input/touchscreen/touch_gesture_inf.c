#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/tp_gesture.h>

uint8_t DouTap_enable = 0;               // double tap
uint8_t UpVee_enable  = 0;               // ^
uint8_t DownVee_enable  = 0;             // v
uint8_t LeftVee_enable = 0;              // <
uint8_t RightVee_enable = 0;             // >
uint8_t Circle_enable = 0;               // O
uint8_t DouSwip_enable = 0;              // ||
uint8_t Up2DownSwip_enable = 0;          // |v
uint8_t Down2UpSwip_enable = 0;          // |^
uint8_t Mgestrue_enable = 0;             // M

struct tp_sys_info{
        struct class *tp_class;
        int index;
        struct device *dev;
};

static void dummy_func(int en)
{
	pr_info("%s, en = %d\n", __func__, en);
}

struct tp_dev new_tp = {
	.update_status = dummy_func,
};
static atomic_t tp_device_count;
static int double_gesture_switch;
#ifdef CONFIG_PRODUCT_HEART
extern void gsx_gesture_en(int enable);
#endif

void tp_gesture_register(struct tp_dev *opt)
{
	new_tp.update_status = opt->update_status;
	pr_info("%s \n", __func__);
	return;
}
EXPORT_SYMBOL(tp_gesture_register);

#ifdef CONFIG_PRODUCT_ZIPPO
static ssize_t ft_gesture_wakeup_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n",double_gesture_switch);
}

static ssize_t ft_gesture_wakeup_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
       int val;

       sscanf(buf, "%d", &val);
	if(val)
		double_gesture_switch = 1;
	else
		double_gesture_switch = 0;

	new_tp.update_status(double_gesture_switch);

       return count;

}

static struct device_attribute attrs[] = {
        __ATTR(gesture_on, 0664,
                        ft_gesture_wakeup_show,
                        ft_gesture_wakeup_store),
};
#endif

#define GESTURE_ATTR(name, out) \
static ssize_t name##_enable_show_func(struct device *dev, struct device_attribute *attr, char *buf) \
{ \
    return snprintf(buf, PAGE_SIZE, "%d\n", out); \
} \
static ssize_t name##_enable_store_func(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) \
{ \
    int enabled = 0; \
    sscanf(buf, "%d", &enabled); \
    out = enabled > 0 ? 1 : 0; \
    return count; \
} \
DEVICE_ATTR(name##_enable, 0664, name##_enable_show_func, name##_enable_store_func)

GESTURE_ATTR(double_tap, DouTap_enable);
GESTURE_ATTR(up_arrow, UpVee_enable);
GESTURE_ATTR(down_arrow, DownVee_enable);
GESTURE_ATTR(left_arrow, LeftVee_enable);
GESTURE_ATTR(right_arrow, RightVee_enable);
GESTURE_ATTR(double_swipe, DouSwip_enable);
GESTURE_ATTR(up_swipe, Up2DownSwip_enable);
GESTURE_ATTR(down_swipe, Down2UpSwip_enable);
GESTURE_ATTR(letter_o, Circle_enable);
GESTURE_ATTR(letter_m, Mgestrue_enable);

#define CREATE_SYS_NODE(PARENT, NAME) \
    err = sysfs_create_file(PARENT, &dev_attr_##NAME); \
    if (err < 0) { \
        pr_err("%s: Couldn't create proc entry, %d\n", __func__, __LINE__); \
}

#define CREATE_GESTURE_NODE(NAME) \
    CREATE_SYS_NODE(&ts->dev->kobj, NAME##_enable)

static int tp_gesture_ctl_class(void)
{
    int attr_count = 0;
    int err;
    struct tp_sys_info *ts;

    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    memset(ts, 0, sizeof(*ts));
    ts->tp_class = class_create(THIS_MODULE, "touch");
    if (IS_ERR(ts->tp_class)) {
       printk("create tp class err!");
    } else atomic_set(&tp_device_count, 0);

    ts->index = atomic_inc_return(&tp_device_count);
    ts->dev = device_create(ts->tp_class, NULL,
                            MKDEV(0, ts->index), NULL, "tp_dev");
    if (IS_ERR(ts->dev)) {
        printk("create device err!");
    }
#ifdef CONFIG_PRODUCT_ZIPPO
    for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
          err = sysfs_create_file(&ts->dev->kobj,
                          &attrs[attr_count].attr);
          if (err < 0) {
               pr_err("%s: Failed to create sysfs attributes\n", __func__);
               return err;
         }
    }
#else
    gsx_gesture_en(1);
    CREATE_GESTURE_NODE(double_tap);
    CREATE_GESTURE_NODE(up_arrow);
    CREATE_GESTURE_NODE(down_arrow);
    CREATE_GESTURE_NODE(left_arrow);
    CREATE_GESTURE_NODE(right_arrow);
    CREATE_GESTURE_NODE(double_swipe);
    CREATE_GESTURE_NODE(up_swipe);
    CREATE_GESTURE_NODE(down_swipe);
    CREATE_GESTURE_NODE(letter_o);
    CREATE_GESTURE_NODE(letter_m);
#endif
    dev_set_drvdata(ts->dev,ts);
    //end tp class to show tp info

    return 0;
}


static int __init tp_gesture_ctl_init(void)
{
	return tp_gesture_ctl_class();
}

static void __exit tp_gesture_ctl_exit(void)
{
	//if (ts & ts->tp_class)
	//	class_destroy(ts->tp_class);
}

module_init(tp_gesture_ctl_init);
module_exit(tp_gesture_ctl_exit);

MODULE_AUTHOR("xxx <xxx@android.com>");
MODULE_DESCRIPTION("tp gesture control class driver");
MODULE_LICENSE("GPL");
