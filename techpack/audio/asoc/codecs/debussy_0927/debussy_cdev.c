#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "debussy.h"
#include "debussy_intf.h"
#include "debussy_snd_ctrl.h"

#define  DEVICE_NAME    "debussy_cdev"
#define  CLASS_NAME     "debussy"

static int    majorNumber;
static struct class*  debussy_class  = NULL;
static struct device* debussy_device = NULL;
static struct debussy_priv* p_debussy;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long    dev_io_ctrl(struct file *, unsigned int, unsigned long);

static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .open       = dev_open,
    .read       = dev_read,
    .write      = dev_write,
    .release    = dev_release,
    .unlocked_ioctl = dev_io_ctrl,
};

int debussy_cdev_init(void *priv_data) {
    printk(KERN_INFO "debussy: %s\n", __func__);

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "debusy: %s failed to register a major number\n", __func__);
        return majorNumber;
    }

    printk(KERN_INFO "debusy: registered correctly with major number %d\n", majorNumber);

    // Register the device class
    debussy_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(debussy_class)){                // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "debusy: Failed to register device class\n");
        return PTR_ERR(debussy_class);          // Correct way to return an error on a pointer
    }

    printk(KERN_INFO "debusy: device class registered correctly\n");

    // Register the device driver
    debussy_device = device_create(debussy_class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(debussy_device)) {
        class_destroy(debussy_class);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "debussy: Failed to create the device\n");
        return PTR_ERR(debussy_device);
    }

    p_debussy = priv_data;
    printk(KERN_INFO "debussy: device class created correctly\n"); // Made it! device was initialized

    return 0;
}

void debussy_cdev_exit(void) {
    device_destroy(debussy_class, MKDEV(majorNumber, 0));   // remove the device
    class_unregister(debussy_class);                        // unregister the device class
    class_destroy(debussy_class);                           // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);            // unregister the major number
    printk(KERN_INFO "debussy: %s\n", __func__);
}

static int dev_open(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "debussy: Device has been opened\n");
    filep->private_data = p_debussy;

    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
    struct debussy_priv* debussy = filep->private_data;
    struct i2c_client* client;
    u32 *input_data;
    u32 i;

    pr_info("debussy: %s -\n", __func__);

    len = (len + 3) & 0xFFFFFFFC;

    client = to_i2c_client(debussy->dev);

    input_data = (u32 *) devm_kzalloc(debussy->dev, len, GFP_KERNEL);
    *offset = 0;

    if (NULL == input_data) {
        pr_err("debussy: %s: devm_kzalloc fail\n", __func__);
        return -ENOMEM;
    }

    mutex_lock(&debussy->igo_ch_lock);
    igo_ch_buf_read(debussy->dev, IGO_CH_VAD_ENROLL_MD_ADDR, input_data, len >> 2);
    if (0 != copy_to_user(buffer, input_data, len)) {
        // devm_kfree(debussy->dev, input_data);
        // mutex_unlock(&debussy->igo_ch_lock);
        // return -ENOMEM;
    }

    for (i = 0; i < (len >> 2); i++) {
        printk(KERN_INFO "debussy: [igo-mdl 0x%08X] 0x%08X\n", i << 2, input_data[i]);
    }

    igo_ch_read(debussy->dev, IGO_CH_VAD_ENROLL_CNT_ADDR, &i);
    pr_info("debussy: %s: IGO_CH_VAD_ENROLL_CNT_ADDR - %d\n", __func__, i);
    mutex_unlock(&debussy->igo_ch_lock);

    devm_kfree(debussy->dev, input_data);

    return len;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    u32 *output_data;
    struct debussy_priv* debussy = filep->private_data;
    struct i2c_client *client;
    u32 i;

    pr_info("debussy: %s -\n", __func__);
    client = to_i2c_client(debussy->dev);

    output_data = memdup_user(buffer, len);
    if (IS_ERR(output_data))
        return PTR_ERR(output_data);

    mutex_lock(&debussy->igo_ch_lock);
    igo_ch_write(debussy->dev, IGO_CH_VAD_ENROLL_APPLY_ADDR, VAD_ENROLL_APPLY_DISABLE);
    igo_ch_buf_write(debussy->dev, IGO_CH_VAD_ENROLL_MD_ADDR, output_data, len >> 2);

    for (i = 0; i < (len >> 2); i++) {
        printk(KERN_INFO "debussy: [igo-mdl 0x%08X] 0x%08X\n", i << 4, output_data[i]);
    }

    igo_ch_write(debussy->dev, IGO_CH_VAD_ENROLL_APPLY_ADDR, VAD_ENROLL_APPLY_APLLY);
    igo_ch_read(debussy->dev, IGO_CH_VAD_ENROLL_CNT_ADDR, &i);
    pr_info("debussy: %s: IGO_CH_VAD_ENROLL_CNT_ADDR - %d\n", __func__, i);
    mutex_unlock(&debussy->igo_ch_lock);

    devm_kfree(debussy->dev, output_data);

    return len;
}

static int dev_release(struct inode *inodep, struct file *filep) {
  printk(KERN_INFO "debussy: Device successfully closed\n");
  return 0;
}

static long dev_io_ctrl(struct file *filep, unsigned int cmd, unsigned long arg) {
    struct debussy_priv* debussy = filep->private_data;
    struct i2c_client *client;
    int retval = 0;
    struct ig_ioctl_rg_arg param;
    struct ig_ioctl_kws_arg kws_status;

    pr_info("debussy: %s -\n", __func__);
    client = to_i2c_client(debussy->dev);
    memset(&param, 0, sizeof(struct ig_ioctl_rg_arg));

    switch (cmd) {
    case IOCTL_REG_SET:
        if (copy_from_user(&param, (int __user *)arg, sizeof(struct ig_ioctl_rg_arg))) {
            retval = -EFAULT;
            goto done;
        }

        pr_info("debussy: IOCTL_REG_SET");
        pr_info("Address: 0x%08X\n", param.address);
        pr_info("Address: 0x%08X\n", param.data);

        mutex_lock(&debussy->igo_ch_lock);
        igo_i2c_write(to_i2c_client(debussy->dev), param.address, &param.data, 1);
        mutex_unlock(&debussy->igo_ch_lock);
        break;

    case IOCTL_REG_GET:
        if (copy_from_user(&param, (int __user *)arg, sizeof(struct ig_ioctl_rg_arg))) {
            retval = -EFAULT;
            goto done;
        }

        pr_info("debussy: IOCTL_REG_SET");
        pr_info("Address: 0x%08X\n", param.address);

        mutex_lock(&debussy->igo_ch_lock);
        igo_i2c_read(to_i2c_client(debussy->dev), param.address, &param.data);
        mutex_unlock(&debussy->igo_ch_lock);
        pr_info("Data: 0x%08X\n", param.data);

        if (copy_to_user((int __user *)arg, &param, sizeof(struct ig_ioctl_rg_arg)) ) {
            retval = -EFAULT;
            goto done;
        }
        break;

    case IOCTL_KWS_STATUS_CLEAR:
        atomic_set(&p_debussy->kws_triggered, 0);
        break;

    case IOCTL_KWS_STATUS_GET:
        kws_status.status = atomic_read(&p_debussy->kws_triggered);
        if (kws_status.status)
            kws_status.count = atomic_read(&p_debussy->kws_count);
        else
            kws_status.count = 0;

        if (copy_to_user((int __user *)arg, &kws_status, sizeof(struct ig_ioctl_kws_arg)) ) {
            retval = -EFAULT;
            goto done;
        }
        break;

#if 0
    case IOCTL_ENROLL_MOD_GET:
        {
            struct debussy_priv* debussy = filep->private_data;
            struct ig_ioctl_enroll_model_arg enroll_mod;
            u32 *input_data, i;
            u8 *enroll_ptr = (u8 *) &enroll_mod;

            //_IOR(IOCTL_ID, 4, struct ig_ioctl_enroll_model_arg)
            memset(enroll_ptr, 0, sizeof(struct ig_ioctl_enroll_model_arg));

            if (copy_from_user(enroll_ptr, (int __user *)arg, sizeof(struct ig_ioctl_enroll_model_arg))) {
                retval = -EFAULT;
                goto done;
            }

            input_data = (u32 *) devm_kzalloc(debussy->dev, enroll_mod.byte_len, GFP_KERNEL);

            if (NULL == input_data) {
                pr_err("debussy: %s: IOCTL_ENROLL_MOD_GET devm_kzalloc fail\n", __func__);
                retval = -ENOMEM;
                goto done;
            }

            mutex_lock(&debussy->igo_ch_lock);
            igo_ch_buf_read(debussy->dev, IGO_CH_VAD_ENROLL_MD_ADDR, input_data, enroll_mod.byte_len >> 2);
            if (0 != copy_to_user((int __user *) (char *) enroll_mod.buf_addr, input_data, enroll_mod.byte_len)) {
                // devm_kfree(debussy->dev, input_data);
                // mutex_unlock(&debussy->igo_ch_lock);
                // return -ENOMEM;
            }

            for (i = 0; i < (enroll_mod.byte_len >> 2); i++) {
                printk(KERN_INFO "debussy: [igo-mdl 0x%08X] 0x%08X\n", i << 2, input_data[i]);
            }

            pr_info("debussy: %s: IGO_CH_VAD_ENROLL_CNT_ADDR - %d\n", __func__, i);
            mutex_unlock(&debussy->igo_ch_lock);

            igo_ch_read(debussy->dev, IGO_CH_VAD_ENROLL_CNT_ADDR, &i);
            devm_kfree(debussy->dev, input_data);
        }
        break;

    case IOCTL_ENROLL_MOD_SET:
        {
            struct debussy_priv* debussy = filep->private_data;
            struct ig_ioctl_enroll_model_arg enroll_mod;
            u32 *output_data, i;

            memset(&enroll_mod, 0, sizeof(struct ig_ioctl_enroll_model_arg));

            if (copy_from_user(&enroll_mod, (int __user *)arg, sizeof(struct ig_ioctl_enroll_model_arg))) {
                retval = -EFAULT;
                goto done;
            }

            output_data = memdup_user((const void __user *) enroll_mod.buf_addr, enroll_mod.byte_len);
            if (IS_ERR(output_data))
                return PTR_ERR(output_data);

            mutex_lock(&debussy->igo_ch_lock);
            igo_ch_write(debussy->dev, IGO_CH_VAD_ENROLL_APPLY_ADDR, VAD_ENROLL_APPLY_DISABLE);
            igo_ch_buf_write(debussy->dev, IGO_CH_VAD_ENROLL_MD_ADDR, output_data, enroll_mod.byte_len >> 2);

            for (i = 0; i < (enroll_mod.byte_len >> 2); i++) {
                printk(KERN_INFO "debussy: [igo-mdl 0x%08X] 0x%08X\n", i << 4, output_data[i]);
            }

            igo_ch_write(debussy->dev, IGO_CH_VAD_ENROLL_APPLY_ADDR, VAD_ENROLL_APPLY_APLLY);
            igo_ch_read(debussy->dev, IGO_CH_VAD_ENROLL_CNT_ADDR, &i);
            pr_info("debussy: %s: IGO_CH_VAD_ENROLL_CNT_ADDR - %d\n", __func__, i);
            mutex_unlock(&debussy->igo_ch_lock);

            devm_kfree(debussy->dev, output_data);
        }
        break;
#endif

    default:
        retval = -ENOTTY;
        break;
    }

done:
    return retval;
}
