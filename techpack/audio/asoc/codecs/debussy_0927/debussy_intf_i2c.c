#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include "debussy.h"
#include "debussy_intf.h"

int igo_i2c_read_buffer(struct i2c_client* client, unsigned int addr, unsigned int* buff, unsigned int word_len)
{
    unsigned char buf[4];
    struct i2c_msg xfer[2];
    int ret;

#ifdef ENABLE_DEBUSSY_I2C_REGMAP
    struct debussy_priv* debussy = i2c_get_clientdata(client);

    if (debussy->i2c_regmap) {
        return regmap_raw_read(debussy->i2c_regmap, addr, buff, word_len << 2);
    }
#endif

    buf[0] = (unsigned char)(addr >> 24);
    buf[1] = (unsigned char)(addr >> 16);
    buf[2] = (unsigned char)(addr >> 8);
    buf[3] = (unsigned char)(addr & 0xff);

    memset(xfer, 0, sizeof(xfer));

    xfer[0].addr = client->addr;
    xfer[0].flags = 0;
    xfer[0].len = 4;
    xfer[0].buf = buf;

    xfer[1].addr = client->addr;
    xfer[1].flags = I2C_M_RD;
    xfer[1].len = word_len << 2;
    xfer[1].buf = (u8*) buff;

    ret = i2c_transfer(client->adapter, xfer, 2);
    if (ret < 0) {
        pr_err("debussy: %s - i2c_transfer error => %d, Reg = 0x%X\n", __func__, ret, addr);
        return ret;
    }
    else if (ret != 2) {
        pr_err("debussy: %s - i2c_transfer error => -EIO, Reg = 0x%X\n", __func__, addr);
        return -EIO;
    }

    return 0;
}

int igo_i2c_read(struct i2c_client* client, unsigned int addr, unsigned int *value)
{
    unsigned char buf[4];
    struct i2c_msg xfer[2];
    unsigned int regVal;
    int ret;

#ifdef ENABLE_DEBUSSY_I2C_REGMAP
    struct debussy_priv* debussy = i2c_get_clientdata(client);

    if (debussy->i2c_regmap) {
        return regmap_read(debussy->i2c_regmap, addr, value);
    }
#endif

    memset(xfer, 0, sizeof(xfer));

    buf[0] = (unsigned char)(addr >> 24);
    buf[1] = (unsigned char)(addr >> 16);
    buf[2] = (unsigned char)(addr >> 8);
    buf[3] = (unsigned char)(addr & 0xff);

    xfer[0].addr = client->addr;
    xfer[0].flags = 0;
    xfer[0].len = 4;
    xfer[0].buf = buf;

    xfer[1].addr = client->addr;
    xfer[1].flags = I2C_M_RD;
    xfer[1].len = 4;
    xfer[1].buf = (u8 *) &regVal;

    ret = i2c_transfer(client->adapter, xfer, 2);
    if (ret < 0) {
        pr_err("debussy: %s - i2c_transfer error => %d, Reg = 0x%X\n", __func__, ret, addr);
        return ret;
    }
    else if (ret != 2) {
        pr_err("debussy: %s - i2c_transfer error => -EIO, Reg = 0x%X\n", __func__, addr);
        return -EIO;
    }

    *value = regVal;

    return 0;
}

int igo_i2c_write(struct i2c_client* client, unsigned int addr, unsigned int* buff, unsigned int word_len)
{
    unsigned char *buf;
    struct i2c_msg xfer[1];
    int i, ret;

#ifdef ENABLE_DEBUSSY_I2C_REGMAP
    struct debussy_priv* debussy = i2c_get_clientdata(client);

    if (debussy->i2c_regmap) {
        return regmap_raw_write(debussy->i2c_regmap, addr, buff, word_len << 2);
    }
#endif

    buf = (unsigned char *) devm_kzalloc(&client->dev, IG_I2C_BUF_RW_LEN + 4, GFP_KERNEL);

    if (NULL == buf) {
        pr_err("debussy: %s: alloc fail\n", __func__);
        return -EFAULT;
    }

    if (word_len <= (IG_I2C_BUF_RW_LEN >> 2)) {
        buf[0] = (unsigned char) (addr >> 24);
        buf[1] = (unsigned char) (addr >> 16);
        buf[2] = (unsigned char) (addr >> 8);
        buf[3] = (unsigned char) (addr & 0xff);

        for (i = 0; i < word_len; i++) {
            buf[(i + 1) * 4 + 0] = (unsigned char)(buff[i] & 0xff);
            buf[(i + 1) * 4 + 1] = (unsigned char)(buff[i] >> 8);
            buf[(i + 1) * 4 + 2] = (unsigned char)(buff[i] >> 16);
            buf[(i + 1) * 4 + 3] = (unsigned char)(buff[i] >> 24);
        }

        memset(xfer, 0, sizeof(xfer));

        xfer[0].addr = client->addr;
        xfer[0].flags = 0;
        xfer[0].len = 4 + word_len * 4;
        xfer[0].buf = (u8*)buf;

        ret = i2c_transfer(client->adapter, xfer, 1);
        devm_kfree(&client->dev, buf);

        if (ret < 0) {
            pr_err("debussy: %s - i2c_transfer error => %d, Reg = 0x%X\n", __func__, ret, addr);
            return ret;
        }
        else if (ret != 1) {
            pr_err("debussy: %s - i2c_transfer error => -EIO, Reg = 0x%X\n", __func__, addr);
            return -EIO;
        }

        return 0;
    }

    devm_kfree(&client->dev, buf);
    pr_err("debussy: %s - write length-%d is exceed to %d bytes\n", __func__, word_len, IG_I2C_BUF_RW_LEN);

    return -EIO;
}
