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

#include "debussy.h"
#include "debussy_intf.h"

#define DEBUSSY_FLASH_PAGE_SZ       (4096)
#define DEBUSSY_FLASH_SECTOR_SZ     (65536)
#define DEBUSSY_FLASH_POLLING_CNT   (300 / 2)     // about 300ms
#define DEBUSSY_WRITE_PAGE_SIZE     (256)

static void _debussy_flash_control(struct device* dev, unsigned int cmd, unsigned int faddr,
    unsigned int baddr, unsigned int len, unsigned int f2b)
{
    struct i2c_client* client;
    uint32_t buf[5];

    client = to_i2c_client(dev);

    if (client != NULL) {
        buf[0] = cmd;
        buf[1] = faddr;
        buf[2] = baddr;
        buf[3] = len;
        buf[4] = f2b;
        igo_i2c_write(client, 0x2A013024, buf, 5);
    }
}

static unsigned int _debussy_flash_readStatus(struct device* dev, unsigned int cmd)
{
    unsigned int ret = 1;
    struct i2c_client* client;

    client = to_i2c_client(dev);

    if (client != NULL) {
        _debussy_flash_control(dev, 0x410A0000 + cmd, 0, 0x2A0C0000, 4, 1);
        igo_i2c_read(client, 0x2A0C0000, &ret);
    }
    else {
        ret = 0;
    }

    return ret;
}

static unsigned int _debussy_flash_pollingDone(struct device* dev)
{
    int count;
    unsigned int ret;

    count = 0;
    // About 300ms
    while (count < DEBUSSY_FLASH_POLLING_CNT) {
        ret = _debussy_flash_readStatus(dev, 0x05);
        if ((ret & 0x1) == 0x0)
            break;
        usleep_range(1999, 2000);       // 2ms
        count++;
    }

    if (count == DEBUSSY_FLASH_POLLING_CNT) {
        printk("debussy: wait flash complete timeout\n");
        return 1;
    }

    return 0;
}

static void _debussy_flash_wren(struct device* dev)
{
    _debussy_flash_control(dev, 0x400E0006, 0, 0, 0, 0);
}

static unsigned int debussy_flash_page_erase(struct device* dev, unsigned int faddr)
{
    _debussy_flash_wren(dev);
    _debussy_flash_control(dev, 0x500E0020, faddr, 0, 0, 0);
    return _debussy_flash_pollingDone(dev);
}

static unsigned int debussy_flash_sector_erase(struct device* dev, unsigned int faddr)
{
    _debussy_flash_wren(dev);
    _debussy_flash_control(dev, 0x500E00D8, faddr, 0, 0, 0);
    return _debussy_flash_pollingDone(dev);
}

static unsigned int debussy_flash_32k_erase(struct device* dev, unsigned int faddr)
{
    _debussy_flash_wren(dev);
    _debussy_flash_control(dev, 0x500E0052, faddr, 0, 0, 0);
    return _debussy_flash_pollingDone(dev);
}


static unsigned int _debussy_flash_writepage(struct device* dev, unsigned int faddr, unsigned int* data)
{
    unsigned int cmd;
    int is_ignore, i;
    struct i2c_client* client;

    client = to_i2c_client(dev);

    cmd = 0x530C0032;
    is_ignore = 1;

    for (i = 0; i < (DEBUSSY_WRITE_PAGE_SIZE >> 2); ++i) {
        if (data[i] != 0xffffffff)
            is_ignore = 0;
    }

    if (!is_ignore) {
        _debussy_flash_wren(dev);
        igo_i2c_write(client, 0x2A0C0004, data, DEBUSSY_WRITE_PAGE_SIZE >> 2);
        _debussy_flash_control(dev, cmd, faddr, 0x2A0C0004, DEBUSSY_WRITE_PAGE_SIZE, 0);
        return _debussy_flash_pollingDone(dev);
    }

    return 0;
}

static unsigned int _debussy_flash_erase(struct device* dev, unsigned int faddr, size_t write_size)
{
    int i;
    unsigned int erase_64k_num;
    unsigned int erase_32k_num;
    unsigned int erase_4k_num;

    erase_64k_num = write_size / DEBUSSY_FLASH_SECTOR_SZ;
    erase_32k_num = (write_size % DEBUSSY_FLASH_SECTOR_SZ) / 32768;
    erase_4k_num = (write_size % 32768) / DEBUSSY_FLASH_PAGE_SZ;
    
    if ((write_size % 32768) % DEBUSSY_FLASH_PAGE_SZ > 0) {
        ++erase_4k_num;
    }

    for (i = 0; i < erase_64k_num; ++i) {
        if (0 != debussy_flash_sector_erase(dev, faddr)) {
            return 1;
        }
        faddr += DEBUSSY_FLASH_SECTOR_SZ;
    }

    for (i = 0; i < erase_32k_num; ++i) {
        if (0 != debussy_flash_32k_erase(dev, faddr)) {
            return 1;
        }
        faddr += 32768;
    }

    for (i = 0; i < erase_4k_num; ++i) {
        if (0 != debussy_flash_page_erase(dev, faddr)) {
            return 1;
        }
        faddr += DEBUSSY_FLASH_PAGE_SZ;
    }

    return 0;
}

static unsigned int _flash_write_bin(struct device* dev, unsigned int faddr, const u8* data, size_t size)
{
    int page_num, left_cnt;
    int idx, i, j;
    int write_cnt;
    unsigned int wdata[DEBUSSY_WRITE_PAGE_SIZE >> 2];
    int percent = -1;

    write_cnt = 0;
    page_num = (size / sizeof(char)) / DEBUSSY_WRITE_PAGE_SIZE;
    left_cnt = (size / sizeof(char)) % DEBUSSY_WRITE_PAGE_SIZE;

    if (0 != _debussy_flash_erase(dev, faddr, size)) {
        return 1;
    }

    for (i = 0; i < page_num; ++i) {
        for (j = 0; j < (DEBUSSY_WRITE_PAGE_SIZE >> 2); ++j) {
            idx = i * DEBUSSY_WRITE_PAGE_SIZE + j * 4;
            wdata[j] = (data[idx + 3] << 24) | (data[idx + 2] << 16) | (data[idx + 1] << 8) | (data[idx]);
        }

        //printk("debussy to flash writepage\n");
        if (0 != _debussy_flash_writepage(dev, faddr + write_cnt, wdata)) {
            return 1;
        }

        write_cnt += DEBUSSY_WRITE_PAGE_SIZE;

        if ((write_cnt * 10 / size) != percent) {
            percent = write_cnt * 10 / size;
            dev_info(dev, "%s: %d%%\n", __func__, percent * 10);
        }
    }

    if (left_cnt != 0) {
        memset(wdata, 0xff, sizeof(wdata));
        for (j = 0; j < (left_cnt / 4); ++j) {
            idx = i * DEBUSSY_WRITE_PAGE_SIZE + j * 4;
            wdata[j] = (data[idx + 3] << 24) | (data[idx + 2] << 16) | (data[idx + 1] << 8) | (data[idx]);
        }

        //printk("debussy flash write page\n");
        if (0 != _debussy_flash_writepage(dev, faddr + write_cnt, wdata)) {
            return 1;
        }

        if ((write_cnt * 10 / size) != percent) {
            percent = write_cnt * 10 / size;
            dev_info(dev, "%s: %d%%\n", __func__, percent * 10);
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void debussy_flash_update_firmware(struct device* dev, unsigned int faddr, const u8* data, size_t size)
{
    int ret;
    unsigned int addr = 0x00000;
    unsigned int tmp_data;
    struct i2c_client* client;

    client = to_i2c_client(dev);

    /* Update firmware init sequence */
    tmp_data = 0;           // LV3
    igo_i2c_write(client, 0x2A00003C, &tmp_data, 1);
    tmp_data = 1;           // HOLD
    igo_i2c_write(client, 0x2A000018, &tmp_data, 1);
    tmp_data = 1;           // LV1
    igo_i2c_write(client, 0x2A000030, &tmp_data, 1);
    msleep(500);
    tmp_data = 0x830001;    // LV3
    igo_i2c_write(client, 0x2A00003C, &tmp_data, 1);
    tmp_data = 0x4;         // AGN_BRUST
    igo_i2c_write(client, 0x2A013020, &tmp_data, 1);
    tmp_data = 0x5500;      // PHY_CTL
    igo_i2c_write(client, 0x2A0130C8, &tmp_data, 1);
    tmp_data = 0x0;         // AGN_PWD
    igo_i2c_write(client, 0x2A013048, &tmp_data, 1);

    dev_info(dev, "Download to address  0x%08x\n", addr);
    ret = _flash_write_bin(dev, addr, data, size);

    if (ret == 0) {
        struct debussy_priv *debussy = i2c_get_clientdata(client);

        dev_info(dev, "Debussy FW update done\n");
        debussy->reset_chip(dev);
    }
    else {
        dev_info(dev, "Debussy FW update fail\n");
    }
}
