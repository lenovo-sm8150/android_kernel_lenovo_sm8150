#include <linux/version.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/string.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>

#include "debussy.h"
#include "debussy_intf.h"
#include "debussy_snd_ctrl.h"
#ifdef ENABLE_GENETLINK
#include "debussy_genetlink.h"
#endif
#include "debussy_kws.h"

#define DEBUSSY_FW_PATH "./"
#define DEBUSSY_FW_ADDR (0x0)
#define DEBUSSY_FW_NAME_SIZE (256)
#define DEBUSSY_FW_VER_OFFSET (0x00002014)

#define DEBUSSY_RATES SNDRV_PCM_RATE_48000
#define DEBUSSY_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

struct debussy_priv* p_debussy_priv = NULL;

extern int debussy_cdev_init(void *priv_data);
extern void debussy_cdev_exit(void);

#ifdef ENABLE_DEBUSSY_I2C_REGMAP
static bool debussy_readable(struct device *dev, unsigned int reg)
{
    return true;
}

static bool debussy_writeable(struct device *dev, unsigned int reg)
{
    return true;
}

static bool debussy_volatile(struct device *dev, unsigned int reg)
{
    return true;
}

static bool debussy_precious(struct device *dev, unsigned int reg)
{
    return false;
}

const struct regmap_config debussy_i2c_regmap = {
    .reg_bits = 32,
    .val_bits = 32,
    .reg_stride = 4,

	//.cache_type = REGCACHE_NONE,
    .cache_type = REGCACHE_RBTREE,
    .reg_format_endian = REGMAP_ENDIAN_BIG,
    .val_format_endian = REGMAP_ENDIAN_LITTLE,

    .max_register = (unsigned int) -1,
    .readable_reg = debussy_readable,
    .writeable_reg = debussy_writeable,
    .volatile_reg = debussy_volatile,
    .precious_reg = debussy_precious,
};
#endif

static ssize_t _debussy_download_firmware(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_download_firmware_fops = {
    .open = simple_open,
    .write = _debussy_download_firmware,
};

static ssize_t _debussy_reset_chip(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_reset_chip_fops = {
    .open = simple_open,
    .write = _debussy_reset_chip,
};

static ssize_t _debussy_reset_gpio_pull_down(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_reset_gpio_pull_down_fops = {
    .open = simple_open,
    .write = _debussy_reset_gpio_pull_down,
};

static ssize_t _debussy_reset_gpio_pull_up(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_reset_gpio_pull_up_fops = {
    .open = simple_open,
    .write = _debussy_reset_gpio_pull_up,
};

static ssize_t _debussy_mcu_hold(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_mcu_hold_fops = {
    .open = simple_open,
    .write = _debussy_mcu_hold,
};

static ssize_t _debussy_get_fw_version(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_get_fw_version_fops = {
    .open = simple_open,
    .write = _debussy_get_fw_version,
};

static ssize_t _debussy_reg_get(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_reg_get_fops = {
    .open = simple_open,
    .write = _debussy_reg_get,
};

static ssize_t _debussy_reg_put(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_reg_put_fops = {
    .open = simple_open,
    .write = _debussy_reg_put,
};

static loff_t _debussy_reg_seek(struct file *file,
    loff_t p, int orig);
static ssize_t _debussy_reg_read(struct file* file,
    char __user* user_buf,
    size_t count, loff_t* ppos);
static ssize_t _debussy_reg_write(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos);
static const struct file_operations debussy_reg_fops = {
    .open = simple_open,
    .llseek  = _debussy_reg_seek,
    .read = _debussy_reg_read,
    .write = _debussy_reg_write,
};

static const struct i2c_device_id igo_i2c_id[] = {
    { "debussy", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, igo_i2c_id);

static const struct of_device_id debussy_of_match[] = {
    { .compatible = "intelligo,debussy" },
    {},
};
MODULE_DEVICE_TABLE(of, debussy_of_match);

static void _debussy_reset_chip_ctrl(struct device* dev)
{
    struct debussy_priv* debussy = i2c_get_clientdata(to_i2c_client(dev));

    if (gpio_is_valid(debussy->reset_gpio)) {
        gpio_set_value(debussy->reset_gpio, GPIO_LOW);
        msleep(debussy->reset_hold_time);
        gpio_set_value(debussy->reset_gpio, GPIO_HIGH);
        msleep(debussy->reset_release_time);
        dev_info(dev, "%s: Reset debussy chip done\n", __func__);
    }
    else {
        dev_err(dev, "%s: Reset GPIO-%d is invalid\n", __func__, debussy->reset_gpio);
    }
}

static ssize_t _debussy_reset_chip(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    unsigned int data = 0;

    debussy->reset_chip(debussy->dev);
    igo_i2c_write(to_i2c_client(debussy->dev), 0x2a000018, &data, 1);

    return count;
}

static ssize_t _debussy_reset_gpio_pull_down(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;

    if (gpio_is_valid(debussy->reset_gpio))
        gpio_direction_output(debussy->reset_gpio, 0);
    else {
        dev_err(debussy->dev, "%s: Reset GPIO-%d is invalid\n", __func__, debussy->reset_gpio);
    }

    return count;
}

static ssize_t _debussy_reset_gpio_pull_up(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;

    if (gpio_is_valid(debussy->reset_gpio)) {
        gpio_direction_output(debussy->reset_gpio, 1);
    }
    else {
        dev_err(debussy->dev, "%s: Reset GPIO-%d is invalid\n", __func__, debussy->reset_gpio);
    }

    return count;
}

static void _debussy_mcu_hold_ctrl(struct device* dev, uint32_t hold)
{
    struct debussy_priv* debussy = i2c_get_clientdata(to_i2c_client(dev));

    if (gpio_is_valid(debussy->mcu_hold_gpio)) {
        gpio_set_value(debussy->mcu_hold_gpio, hold ? GPIO_HIGH : GPIO_LOW);
        dev_info(dev, "%s: MCU Hold - %dms\n", __func__, hold);
    }
    else {
        dev_err(debussy->dev, "%s: MCU Hold GPIO-%d is invalid\n", __func__, debussy->mcu_hold_gpio);
    }
}

static ssize_t _debussy_mcu_hold(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    unsigned int data, position;
    char *input_data;

    if ((input_data = devm_kzalloc(debussy->dev, count + 1, GFP_KERNEL)) == NULL) {
        dev_err(debussy->dev, "%s: alloc fail\n", __func__);
        return EFAULT;
    }

    memset(input_data, 0, count + 1);
    if (copy_from_user(input_data, user_buf, count)){
        devm_kfree(debussy->dev, input_data);
        return -EFAULT;
    }

    position = strcspn(input_data, "1234567890abcdefABCDEF");
    data = simple_strtoul(&input_data[position], NULL, 10);
    debussy->mcu_hold(debussy->dev, data ? 1 : 0);

    return count;
}

static ssize_t _debussy_get_fw_version(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    unsigned int fw_ver = 0;

    mutex_lock(&debussy->igo_ch_lock);
    igo_ch_read(debussy->dev, IGO_CH_FW_VER_ADDR, &fw_ver);
    dev_info(debussy->dev, "CHIP FW VER: 0x%08X\n", fw_ver);
    mutex_unlock(&debussy->igo_ch_lock);

    return count;
}

// Usage: echo address > /d/debussy/reg_get
static ssize_t _debussy_reg_get(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    struct i2c_client* client;
    unsigned int reg, data, position;
    char *input_data;

    client = to_i2c_client(debussy->dev);

    dev_info(debussy->dev, "%s -\n", __func__);

    if ((input_data = devm_kzalloc(debussy->dev, count + 1, GFP_KERNEL)) == NULL) {
        dev_err(debussy->dev, "%s: alloc fail\n", __func__);
        return EFAULT;
    }

    memset(input_data, 0, count + 1);
    if (copy_from_user(input_data, user_buf, count)) {
        devm_kfree(debussy->dev, input_data);
        return -EFAULT;
    }

    dev_info(debussy->dev, "%s: input_data = %s\n", __func__, input_data);

    position = strcspn(input_data, "1234567890abcdefABCDEF");               // find first number
    reg = simple_strtoul(&input_data[position], NULL, 16);
    mutex_lock(&debussy->igo_ch_lock);
    igo_i2c_read(client, reg, &data);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(debussy->dev, "%s: Reg0x%X = 0x%X\n", __func__, reg, data);
    devm_kfree(debussy->dev, input_data);

    return count;
}

// Usage: echo address, data > /d/debussy/reg_put
static ssize_t _debussy_reg_put(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    struct i2c_client* client;
    unsigned int reg, data;
    size_t position;
    char *input_data, *next_data;

    client = to_i2c_client(debussy->dev);

    if ((input_data = devm_kzalloc(debussy->dev, count + 1, GFP_KERNEL)) == NULL) {
        dev_err(debussy->dev, "%s: alloc fail\n", __func__);
        return EFAULT;
    }

    memset(input_data, 0, count + 1);
    if (copy_from_user(input_data, user_buf, count)){
        devm_kfree(debussy->dev, input_data);
        return -EFAULT;
    }

    dev_info(debussy->dev, "%s: input_data = %s\n", __func__, input_data);

    position = strcspn(input_data, "1234567890abcdefABCDEF");               // find first number
    reg = simple_strtoul(&input_data[position], &next_data, 16);
    position = strcspn(next_data, "1234567890abcdefABCDEF");                // find next number
    data = simple_strtoul(&next_data[position], NULL, 16);
    dev_info(debussy->dev, "%s: reg = 0x%X, data = 0x%X\n", __func__, reg, data);

    mutex_lock(&debussy->igo_ch_lock);
    igo_i2c_write(client, reg, &data, 1);
    devm_kfree(debussy->dev, input_data);
    mutex_unlock(&debussy->igo_ch_lock);

    return count;
}

static loff_t _debussy_reg_seek(struct file *file,
    loff_t p, int orig)
{
    struct debussy_priv* debussy = file->private_data;

    debussy->reg_address = p & 0xFFFFFFFC;
    dev_info(debussy->dev, "%s - 0x%X\n", __func__, debussy->reg_address);

    return p;
}

static ssize_t _debussy_reg_read(struct file* file,
    char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    struct i2c_client* client;
    char *input_data;

    client = to_i2c_client(debussy->dev);
    dev_info(debussy->dev, "%s -\n", __func__);
    count &= 0xFFFFFFFC;

    if (count) {
        if ((input_data = devm_kzalloc(debussy->dev, count, GFP_KERNEL)) == NULL) {
            dev_err(debussy->dev, "%s: alloc fail\n", __func__);
            return EFAULT;
        }

        memset(input_data, 0, count);

        mutex_lock(&debussy->igo_ch_lock);
        igo_i2c_read_buffer(client, debussy->reg_address, (unsigned int *) input_data, count >> 4);
        mutex_unlock(&debussy->igo_ch_lock);

        if (copy_to_user(user_buf, input_data, count)) {
            count = -EFAULT;
        }

        devm_kfree(debussy->dev, input_data);
    }

    return count;
}

static ssize_t _debussy_reg_write(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy = file->private_data;
    struct i2c_client* client;
    unsigned int *input_data;

    client = to_i2c_client(debussy->dev);
    dev_info(debussy->dev, "%s -\n", __func__);
    count &= 0xFFFFFFFC;

    if (count) {
        if ((input_data = devm_kzalloc(debussy->dev, count, GFP_KERNEL)) == NULL) {
            dev_err(debussy->dev, "%s: alloc fail\n", __func__);
            return EFAULT;
        }

        if (copy_from_user((char *) input_data, user_buf, count)) {
            count = -EFAULT;
        }
        else {
            mutex_lock(&debussy->igo_ch_lock);
            igo_i2c_write(client, debussy->reg_address, input_data, count >> 2);
            debussy->reg_address += count;
            mutex_unlock(&debussy->igo_ch_lock);
        }

        devm_kfree(debussy->dev, input_data);
    }

    return count;
}

static ssize_t _debussy_download_firmware(struct file* file,
    const char __user* user_buf,
    size_t count, loff_t* ppos)
{
    struct debussy_priv* debussy;

    debussy = file->private_data;
    dev_info(debussy->dev, "%s\n", __func__);

    queue_work(debussy->debussy_wq, &debussy->fw_work);

    return count;
}

static int debussy_hw_params(struct snd_pcm_substream* substream,
    struct snd_pcm_hw_params* params, struct snd_soc_dai* dai)
{
    return 0;
}

static int debussy_set_dai_fmt(struct snd_soc_dai* dai, unsigned int fmt)
{
    return 0;
}

static int debussy_set_dai_sysclk(struct snd_soc_dai* dai,
    int clk_id, unsigned int freq, int dir)
{
    return 0;
}

static int debussy_set_dai_pll(struct snd_soc_dai* dai, int pll_id, int source,
    unsigned int freq_in, unsigned int freq_out)
{
    return 0;
}

void debussy_shutdown(struct snd_pcm_substream* stream,
    struct snd_soc_dai* dai)
{
    struct debussy_priv* debussy = i2c_get_clientdata(to_i2c_client(dai->codec->dev));

    pr_info("%s\n", __func__);

    mutex_lock(&debussy->igo_ch_lock);
    igo_ch_write(debussy->dev, IGO_CH_POWER_MODE_ADDR, POWER_MODE_STANDBY);
    dev_info(debussy->dev, "%s: Start to mask iGo CMD\n", __func__);
    mutex_unlock(&debussy->igo_ch_lock);
}

static const struct snd_soc_dai_ops debussy_aif_dai_ops = {
    .hw_params = debussy_hw_params,
    .set_fmt = debussy_set_dai_fmt,
    .set_sysclk = debussy_set_dai_sysclk,
    .set_pll = debussy_set_dai_pll,
    .shutdown = debussy_shutdown,
};

static struct snd_soc_dai_driver debussy_dai[] = {
    {
        .name = "debussy-aif",
        .id = DEBUSSY_AIF,
        .playback = {
            .stream_name = "AIF Playback",
            .channels_min = 1,
            .channels_max = 2,
            .rates = DEBUSSY_RATES,
            .formats = DEBUSSY_FORMATS,
        },
        .capture = {
            .stream_name = "AIF Capture",
            .channels_min = 1,
            .channels_max = 2,
            .rates = DEBUSSY_RATES,
            .formats = DEBUSSY_FORMATS,
        },
        .ops = &debussy_aif_dai_ops,
    },
};
EXPORT_SYMBOL_GPL(debussy_dai);

static int _debussy_fw_update(struct debussy_priv* debussy, int force_update)
{
    const struct firmware* fw;
    char* fw_path;
    char* fw_name = "debussy.bin";
    int ret, checkAgain = 0;
    unsigned int fw_ver, data;

    fw_path = devm_kzalloc(debussy->dev, DEBUSSY_FW_NAME_SIZE, GFP_KERNEL);
    if (!fw_path) {
        dev_err(debussy->dev, "%s: Failed to allocate fw_path\n", __func__);
        return 0;
    }

    scnprintf(fw_path, DEBUSSY_FW_NAME_SIZE,
        "%s%s", DEBUSSY_FW_PATH, fw_name);
    dev_info(debussy->dev, "debussy fw name =%s", fw_path);

    ret = request_firmware(&fw, fw_path, debussy->dev);
    if (ret) {
        dev_err(debussy->dev,
            "%s: Failed to locate firmware %s errno = %d\n", __func__, fw_path, ret);
        devm_kfree(debussy->dev, fw_path);

        return 0;
    }

    mutex_lock(&debussy->igo_ch_lock);

    dev_info(debussy->dev, "%s size = %d\n", fw_path, (int)(fw->size));
    fw_ver = *(unsigned int*)&fw->data[DEBUSSY_FW_VER_OFFSET];
    dev_info(debussy->dev, "BIN VER: 0x%08X\n", fw_ver);

    ret = igo_ch_read(debussy->dev, IGO_CH_FW_VER_ADDR, &data);
    if (ret != IGO_CH_STATUS_DONE) {
        data = 0;
    }
    dev_info(debussy->dev, "CHIP FW VER: 0x%08X\n", data);

    if ((fw_ver > data) || (force_update == 1)) {
        debussy->mcu_hold(debussy->dev, 1);
        debussy->reset_chip(debussy->dev);
        debussy->mcu_hold(debussy->dev, 0);
        dev_info(debussy->dev, "Update FW to 0x%08x\n", fw_ver);
        #ifdef DEBUSSY_TYPE_PSRAM
        debussy_psram_update_firmware(debussy->dev, DEBUSSY_FW_ADDR, fw->data, fw->size);
        #else
        debussy_flash_update_firmware(debussy->dev, DEBUSSY_FW_ADDR, fw->data, fw->size);
        #endif
        checkAgain = 1;
    } else {
        dev_info(debussy->dev, "Use chip built-in FW\n");
    }

    mutex_unlock(&debussy->igo_ch_lock);

    release_firmware(fw);
    devm_kfree(debussy->dev, fw_path);

    return checkAgain;
}

static void _debussy_manual_load_firmware(struct work_struct* work)
{
    struct debussy_priv* debussy;

    debussy = container_of(work, struct debussy_priv, fw_work);
    _debussy_fw_update(debussy, 1);
}

///////////////////////////////////////////////////////////////////////
extern int debussy_kws_init(struct debussy_priv* debussy);

static int _debussy_codec_probe(struct snd_soc_codec* codec)
{
    struct debussy_priv* debussy = i2c_get_clientdata(to_i2c_client(codec->dev));
    uint32_t enable_kws;

    dev_info(codec->dev, "%s\n", __func__);
    debussy_add_codec_controls(codec);
    debussy->codec = codec;

    if (_debussy_fw_update(debussy, 0)) {
        _debussy_fw_update(debussy, 0);
    }

    /////////////////////////////////////////////////////////////////////////
    if (of_property_read_u32(to_i2c_client(debussy->dev)->dev.of_node, "ig,enable-kws", &enable_kws)) {
        dev_err(debussy->dev, "Unable to get \"ig,enable-kws\"\n");
        enable_kws = 0;
    }

    dev_info(debussy->dev, "ig,enable-kws = %d\n", enable_kws);

    if (enable_kws) {
        debussy_kws_init(debussy);
    }

    pr_info("%s codec probe suscess\n", __func__);
    return 0;
}

static int _debussy_codec_remove(struct snd_soc_codec* codec)
{
    dev_info(codec->dev, "%s\n", __func__);

    return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_debussy = {
    .probe = _debussy_codec_probe,
    .remove = _debussy_codec_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_debussy);

static int _debussy_debufs_init(struct debussy_priv* debussy)
{
    int ret;

    struct dentry* dir;

    debussy->reg_address = 0x2A000000;

    dir = debugfs_create_dir("debussy", NULL);
    if (IS_ERR_OR_NULL(dir)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node - %s\n",
            __func__, "debussy");
        dir = NULL;
        ret = -ENODEV;
        goto err_create_dir;
    }

    debussy->dir = dir;

    if (!debugfs_create_file("download_firmware", S_IWUSR,
            dir, debussy, &debussy_download_firmware_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node - %s\n",
            __func__, "download_firmware");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("reset_chip", S_IWUSR,
            dir, debussy, &debussy_reset_chip_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node - %s\n",
            __func__, "reset_chip");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("reset_gpio_pull_down", S_IWUSR,
            dir, debussy, &debussy_reset_gpio_pull_down_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node - %s\n",
            __func__, "reset_gpio_pull_down");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("reset_gpio_pull_up", S_IWUSR,
            dir, debussy, &debussy_reset_gpio_pull_up_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node - %s\n",
            __func__, "reset_gpio_pull_up");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("mcu_hold", S_IWUSR,
            dir, debussy, &debussy_mcu_hold_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node %s\n",
            __func__, "mcu_hold");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("get_fw_version", S_IWUSR,
            dir, debussy, &debussy_get_fw_version_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node %s\n",
            __func__, "get_fw_version");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("debussy_reg", S_IWUSR,
            dir, debussy, &debussy_reg_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node %s\n",
            __func__, "debussy_reg");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("reg_get", S_IWUSR,
            dir, debussy, &debussy_reg_get_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node %s\n",
            __func__, "reg_get");
        ret = -ENODEV;
        goto err_create_entry;
    }

    if (!debugfs_create_file("reg_put", S_IWUSR,
            dir, debussy, &debussy_reg_put_fops)) {
        dev_err(debussy->dev, "%s: Failed to create debugfs node %s\n",
            __func__, "reg_put");
        ret = -ENODEV;
        goto err_create_entry;
    }

err_create_dir:
    debugfs_remove(dir);

err_create_entry:
    return ret;
}
static int igo_req_mclk_enable(struct debussy_priv* debussy,
				     bool enable)
{
	int ret = 0;

	if (enable) {
		ret = clk_prepare_enable(debussy->s_clk);
		if (ret) {
			dev_err(debussy->dev, "%s: ext clk enable failed\n",
				__func__);
		}
	} else {
		clk_disable_unprepare(debussy->s_clk);
	}
	
	return ret;
}

int debussy_power_on(struct debussy_priv *core_debussy)
{
	int r;
	struct device *dev = NULL;
	struct device_node *node = core_debussy->dev->of_node;
	dev = core_debussy->dev;
	
	dev_info(dev,"debussy init\n");
	
	/* dev:i2c client device or spi slave device*/


		core_debussy->avdd = devm_regulator_get(dev,
				 "vdebussy");
		if (IS_ERR_OR_NULL(core_debussy->avdd)) {
			r = PTR_ERR(core_debussy->avdd);
			dev_err(dev,"Failed to get regulator avdd:%d\n", r);
			core_debussy->avdd = NULL;
			return r;
		}

	r = of_property_read_u32(node, "debussy,power-on-delay-us",
				&core_debussy->power_on_delay_us);
	if (!r) {
		/* 1000ms is too large, maybe you have pass
		 * a wrong value */
		if (core_debussy->power_on_delay_us > 1000 * 1000) {
			dev_err(dev,"Power on delay time exceed 1s, please check\n");
			core_debussy->power_on_delay_us = 0;
		}
	}

	dev_info(dev,"debussy power on start\n");


	if (core_debussy->avdd) {
		r = regulator_enable(core_debussy->avdd);
		if (!r) {
			dev_info(dev,"regulator enable SUCCESS\n");
			if (core_debussy->power_on_delay_us)
				usleep_range(core_debussy->power_on_delay_us,
						core_debussy->power_on_delay_us);
		} else {
			dev_err(dev,"Failed to enable analog power:%d\n", r);
			return r;
		}
	}
	dev_info(dev,"debussy power on done\n");

	return 0;
}

static int igo_i2c_probe(struct i2c_client* i2c,
    const struct i2c_device_id* id)
{
    struct debussy_priv* debussy;
    int ret;
	struct clk *ext_clk;
    struct device_node *node = i2c->dev.of_node;

    dev_info(&i2c->dev, "%s -\n", __func__);
    dev_info(&i2c->dev, "Linux Version = %d.%d.%d\n",
        (LINUX_VERSION_CODE >> 16) & 0xFF,
        (LINUX_VERSION_CODE >> 8) & 0xFF,
        (LINUX_VERSION_CODE) & 0xFF);

    debussy = devm_kzalloc(&i2c->dev, sizeof(struct debussy_priv), GFP_KERNEL);
    if (!debussy) {
        dev_err(&i2c->dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

    #ifdef ENABLE_DEBUSSY_I2C_REGMAP
    dev_info(&i2c->dev, "Enable I2C regmap\n");
    debussy->i2c_regmap = devm_regmap_init_i2c(i2c, &debussy_i2c_regmap);
    if (IS_ERR(debussy->i2c_regmap)) {
        dev_err(&i2c->dev, "Failed to allocate I2C regmap!\n");
        debussy->i2c_regmap = NULL;
    }
    else {
        regcache_cache_bypass(debussy->i2c_regmap, 1);
    }
    #endif

    p_debussy_priv = debussy;
    debussy->dev = &i2c->dev;
	debussy->client = i2c;
    atomic_set(&debussy->maskConfigCmd, 1);
    atomic_set(&debussy->referenceCount, 0);
    atomic_set(&debussy->kws_triggered, 0);
    atomic_set(&debussy->kws_count, 0);
    debussy->reset_chip = _debussy_reset_chip_ctrl;
    debussy->mcu_hold = _debussy_mcu_hold_ctrl;
    i2c_set_clientdata(i2c, debussy);

    /////////////////////////////////////////////////////////////////////////
    #if LINUX_VERSION_CODE <= KERNEL_VERSION(3,13,11)
    // For Android 5.1
    // NOP
    #else
    {
        char *dev_name = NULL;

        if (0 == of_property_read_string(node, "ig,device_name", (const char **) &dev_name)) {
            dev_info(debussy->dev, "ig,device_name = %s\n", dev_name);
            dev_set_name(debussy->dev, "%s", dev_name);
        }
    }
    #endif

	debussy_power_on(debussy);
	dev_info(&i2c->dev, "%s 1.8 power success\n", __func__);
    /////////////////////////////////////////////////////////////////////////
#if 0
    debussy->power_en_gpio= of_get_named_gpio(node, "power_en_gpio-gpios", 0);
    if (debussy->power_en_gpio < 0) {
        dev_err(debussy->dev, "Unable to get \"power_en_gpio\"\n");
        debussy->power_en_gpio = -2;
    }
    else {
        dev_info(debussy->dev, "debussy->power_en_gpio = %d\n", debussy->power_en_gpio);
    }

    if (gpio_is_valid(debussy->power_en_gpio)) {
        if (0 == gpio_request(debussy->power_en_gpio, "IGO_POWER")) {
            gpio_direction_output(debussy->power_en_gpio, GPIO_HIGH);
        }
        else {
            dev_err(debussy->dev, "IGO_POWER: gpio_request fail\n");
        }
    }
    else {
        dev_err(debussy->dev, "debussy->power_en_gpio is invalid: %d\n", debussy->power_en_gpio);
    }
#endif
    /////////////////////////////////////////////////////////////////////////
    debussy->mcu_hold_gpio = of_get_named_gpio(node, "ig,mcu-hold-gpios", 0);
    if (debussy->mcu_hold_gpio < 0) {
        dev_err(debussy->dev, "Unable to get \"ig,mcu-hold-gpio\"\n");
    }
    else {
        dev_info(debussy->dev, "debussy->mcu_hold_gpio = %d\n", debussy->mcu_hold_gpio);

        if (gpio_is_valid(debussy->mcu_hold_gpio)) {
            if (0 == gpio_request(debussy->mcu_hold_gpio, "IGO_MCU_HOLD")) {
                gpio_direction_output(debussy->mcu_hold_gpio, GPIO_LOW);
                debussy->mcu_hold(&i2c->dev, false);
            }
            else {
                dev_err(debussy->dev, "IGO_MCU_HOLD: gpio_request fail\n");
            }
        }
        else {
            dev_err(debussy->dev, "debussy->mcu_hold_gpio is invalid\n");
        }
    }

    /////////////////////////////////////////////////////////////////////////
    if (of_property_read_u32(node, "ig,reset-hold-time", &debussy->reset_hold_time)) {
        dev_err(debussy->dev, "Unable to get \"ig,reset-hold-time\"\n");
        debussy->reset_hold_time = IGO_RST_HOLD_TIME_MIN;
    }
    else if (debussy->reset_hold_time < IGO_RST_HOLD_TIME_MIN) {
        debussy->reset_hold_time = IGO_RST_HOLD_TIME_MIN;
    }

    dev_info(debussy->dev, "debussy->reset_hold_time = %d\n", debussy->reset_hold_time);

    if (of_property_read_u32(node, "ig,reset-release-time", &debussy->reset_release_time)) {
        dev_err(debussy->dev, "Unable to get \"ig,reset-release-time\"\n");
        debussy->reset_release_time = IGO_RST_RELEASE_TIME_MIN;
    }
    else if (debussy->reset_release_time < IGO_RST_RELEASE_TIME_MIN) {
        debussy->reset_release_time = IGO_RST_RELEASE_TIME_MIN;
    }

    dev_info(debussy->dev, "debussy->reset_release_time = %d\n", debussy->reset_release_time);

    /////////////////////////////////////////////////////////////////////////
    debussy->reset_gpio = of_get_named_gpio(node, "ig,reset-gpios", 0);
    if (debussy->reset_gpio < 0) {
        dev_err(debussy->dev, "Unable to get \"ig,reset-gpio\"\n");
        debussy->reset_gpio = IGO_RST_GPIO;
    }
    else {
        dev_info(debussy->dev, "debussy->reset_gpio = %d\n", debussy->reset_gpio);
    }

    if (gpio_is_valid(debussy->reset_gpio)) {
        if (0 == gpio_request(debussy->reset_gpio, "IGO_RESET")) {
            unsigned int data = 0;

            gpio_direction_output(debussy->reset_gpio, GPIO_HIGH);
            debussy->reset_chip(debussy->dev);
            igo_i2c_write(to_i2c_client(debussy->dev), 0x2a000018, &data, 1);
        }
        else {
            dev_err(debussy->dev, "IGO_RESET: gpio_request fail\n");
        }
    }
    else {
        dev_err(debussy->dev, "debussy->reset_gpio is invalid: %d\n", debussy->reset_gpio);
    }

	/* Register for Clock */
	ext_clk = clk_get(&debussy->client->dev, "igo_mclk");
	if (IS_ERR(ext_clk)) {
		dev_err(debussy->dev, "%s: clk get %s failed\n",
			__func__, "igo_clk");
	}

	debussy->s_clk = ext_clk;
	igo_req_mclk_enable(debussy, true);

    msleep(5);

    /////////////////////////////////////////////////////////////////////////
    debussy->debussy_wq = create_singlethread_workqueue("debussy");
    if (debussy->debussy_wq == NULL) {
        dev_err(debussy->dev, "create singlethread workqueue fail\n");
        devm_kfree(debussy->dev, debussy);
        ret = -ENOMEM;
        goto out;
    }

    INIT_WORK(&debussy->fw_work, _debussy_manual_load_firmware);

    _debussy_debufs_init(debussy);

    mutex_init(&debussy->igo_ch_lock);

    {
        // Test only
        uint32_t data;

        dev_info(debussy->dev, "I2C Test ...\n");
        igo_i2c_read(to_i2c_client(debussy->dev), 0x2A000000, &data);
        dev_info(debussy->dev, "CHIP ID: 0x%08X\n", data);
    }

    dev_info(debussy->dev, "Register Codec\n");
    ret = snd_soc_register_codec(debussy->dev, &soc_codec_dev_debussy,
        debussy_dai, ARRAY_SIZE(debussy_dai));



    dev_info(debussy->dev, "End of igo_i2c_probe\n");

    #ifdef ENABLE_GENETLINK
    debussy_genetlink_init((void *) debussy);
    #endif
    #ifdef ENABLE_CDEV
    debussy_cdev_init(debussy);
    #endif

out:
    return ret;
}

static int igo_i2c_remove(struct i2c_client* i2c)
{
    #ifdef ENABLE_DEBUSSY_I2C_REGMAP
    if (p_debussy_priv->i2c_regmap) {
        regmap_exit(p_debussy_priv->i2c_regmap);
    }
    #endif

    if (gpio_is_valid(p_debussy_priv->reset_gpio)) {
        gpio_free(p_debussy_priv->reset_gpio);
    }

    if (gpio_is_valid(p_debussy_priv->mcu_hold_gpio)) {
        gpio_free(p_debussy_priv->mcu_hold_gpio);
    }

    devm_kfree(&i2c->dev, p_debussy_priv);
    p_debussy_priv = NULL;

    #ifdef ENABLE_GENETLINK
    debussy_genetlink_exit();
    #endif
    #ifdef ENABLE_CDEV
    debussy_cdev_exit();
    #endif
    snd_soc_unregister_codec(&i2c->dev);

    return 0;
}

static struct i2c_driver igo_i2c_driver = {
    .driver = {
        .name = "debussy",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(debussy_of_match),
    },
    .probe = igo_i2c_probe,
    .remove = igo_i2c_remove,
    .id_table = igo_i2c_id,
};

#ifdef module_i2c_driver
module_i2c_driver(igo_i2c_driver);
#else
static int __init igo_driver_init(void)
{
    return i2c_register_driver(&igo_i2c_driver);
}
module_init(igo_driver_init);

static void __exit igo_driver_exit(void)
{
    return i2c_del_driver(&igo_i2c_driver);
}
module_exit(igo_driver_exit);
#endif

MODULE_DESCRIPTION("Debussy driver");
MODULE_LICENSE("GPL v2");
