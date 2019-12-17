#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "debussy_intf.h"
#include "debussy.h"
#include "debussy_snd_ctrl.h"

#define IGO_TIMEOUT_TICKS (1 * HZ)      // sec
//#define IGO_TIMEOUT_RESET_CHIP          // Reset iGo chip while iGo CMD timeout occured
#define ENABLE_MASK_CONFIG_CMD
//#define ENABLE_CMD_BATCH_MODE_BUFFERED

#define HW_RESET_INSTEAD_OF_STANDBY

#ifdef ENABLE_CMD_BATCH_MODE_BUFFERED
#define MAX_BATCH_MODE_CMD_NUM          (32)
static atomic_t batchMode = ATOMIC_INIT(0);
static atomic_t batchCmdNum = ATOMIC_INIT(0);
static unsigned int batch_cmd_buffer[64];
#endif

int igo_ch_chk_done(struct device* dev)
{
    struct i2c_client* client;
    unsigned int status = IGO_CH_STATUS_CMD_RDY;
    unsigned long start_time = jiffies;
    client = to_i2c_client(dev);

    while (status == IGO_CH_STATUS_NOP || status == IGO_CH_STATUS_BUSY || status == IGO_CH_STATUS_CMD_RDY || status > IGO_CH_STATUS_MAX) {
        usleep_range(50, 50);
        igo_i2c_read(client, IGO_CH_STATUS_ADDR, &status);
        if (jiffies - start_time >= IGO_TIMEOUT_TICKS) {
            dev_err(dev, "igo cmd timeout\n");
            return IGO_CH_STATUS_TIMEOUT;
        }
    }

    return status;
}

int igo_ch_write_wait(struct device* dev, unsigned int reg, unsigned int data, unsigned int wait_time)
{
    struct i2c_client* client;
    unsigned int cmd[2];
    unsigned int status = IGO_CH_STATUS_CMD_RDY;

    client = to_i2c_client(dev);
    igo_i2c_write(client, IGO_CH_BUF_ADDR, &data, 1);
    cmd[0] = IGO_CH_STATUS_CMD_RDY;
    cmd[1] = (IGO_CH_ACTION_WRITE << IGO_CH_ACTION_OFFSET) + reg;
    igo_i2c_write(client, IGO_CH_CMD_ADDR, &cmd[1], 1);
    igo_i2c_write(client, IGO_CH_STATUS_ADDR, &cmd[0], 1);

    if (wait_time)
        msleep(wait_time);

    status = igo_ch_chk_done(dev);

    if (status != IGO_CH_STATUS_DONE) {
        dev_err(dev, "igo cmd write 0x%08x : 0x%08x fail, error no : %d\n", reg, data, status);

#ifdef IGO_TIMEOUT_RESET_CHIP
        #if 0
        // All CMD
        if (IGO_CH_STATUS_TIMEOUT == status) {
            struct debussy_priv* debussy = i2c_get_clientdata(client);

            // Reset IGO chip while IGO CMD timeout occured
            debussy->reset_chip(dev);
        }
        #else
        // Only power mode CMD
        if ((IGO_CH_STATUS_TIMEOUT == status) && (IGO_CH_POWER_MODE_ADDR == reg)) {
            // Force HW Reset about 40ms
            struct debussy_priv* debussy = i2c_get_clientdata(client);

            debussy->reset_chip(dev);
        }
        #endif
#endif
    }

    return status;
}

int igo_ch_write(struct device* dev, unsigned int reg, unsigned int data)
{
    struct debussy_priv *debussy = i2c_get_clientdata(to_i2c_client(dev));
    unsigned int noDelayAfterWrite = 0;

    switch (reg) {
    case IGO_CH_POWER_MODE_ADDR:
#ifdef ENABLE_MASK_CONFIG_CMD
        if (POWER_MODE_WORKING == data) {
            atomic_add(1, &debussy->referenceCount);
            dev_info(dev, "%s: POWER_MODE_WORKING: ref = %d\n", __func__, atomic_read(&debussy->referenceCount));
            atomic_set(&debussy->maskConfigCmd, 0);
            dev_info(dev, "%s: POWER_MODE_WORKING: Disable mask configure setting\n", __func__);
        }
        else {
            // POWER_MODE_STANDBY by restore setting
            if (atomic_read(&debussy->maskConfigCmd)) {
                atomic_set(&debussy->maskConfigCmd, 0);
                dev_info(dev, "%s: POWER_MODE_STANDBY: POWER_MODE_STANDBY by restore setting\n", __func__);
                return IGO_CH_STATUS_DONE;
            }

            // from debussy_shutdown or OP_MODE_CONFIG
            atomic_set(&debussy->maskConfigCmd, 1);
            dev_info(dev, "%s: POWER_MODE_STANDBY: Enable mask configure setting\n", __func__);

            if (atomic_read(&debussy->referenceCount)) {
                atomic_sub(1, &debussy->referenceCount);
            }

            dev_info(dev, "%s: POWER_MODE_STANDBY: ref = %d\n", __func__, atomic_read(&debussy->referenceCount));

            if (atomic_read(&debussy->referenceCount)) {
                return IGO_CH_STATUS_DONE;
            }
        }
#endif

#ifdef ENABLE_CMD_BATCH_MODE_BUFFERED
        if (IGO_CH_POWER_MODE_ADDR == reg) {
            atomic_set(&batchCmdNum, 0);

            if (POWER_MODE_WORKING == data) {
                atomic_set(&batchMode, 1);
                dev_info(dev, "%s: Enable batch CMD mode\n", __func__);
            }
            else {
                // POWER_MODE_STANDBY
                atomic_set(&batchMode, 0);
                dev_info(dev, "%s: Disable batch CMD mode\n", __func__);
            }
        }
#endif
        break;

    //case IGO_CH_OP_MODE_ADDR:
    default:
#ifdef ENABLE_MASK_CONFIG_CMD
        if (atomic_read(&debussy->maskConfigCmd)) {
            return IGO_CH_STATUS_DONE;
        }

        if (IGO_CH_OP_MODE_ADDR == reg) {
            if (OP_MODE_CONFIG == data) {
                // debussy_shutdown is ignored, change to Standby CMD
                reg = IGO_CH_POWER_MODE_ADDR;
                data = POWER_MODE_STANDBY;
                break;
            }
        }
#endif

#ifdef ENABLE_CMD_BATCH_MODE_BUFFERED
        if (atomic_read(&batchMode)) {
            int status = IGO_CH_STATUS_DONE;
            unsigned int index = atomic_read(&batchCmdNum) << 1;

            batch_cmd_buffer[index] = reg;
            batch_cmd_buffer[index + 1] = data;
            atomic_add(1, &batchCmdNum);

            if ((atomic_read(&batchCmdNum) >= MAX_BATCH_MODE_CMD_NUM) || (IGO_CH_OP_MODE_ADDR == reg)) {
                igo_ch_batch_write(dev, 0, batch_cmd_buffer, atomic_read(&batchCmdNum) << 1);
                status = igo_ch_batch_finish_write(dev, atomic_read(&batchCmdNum));
                atomic_set(&batchCmdNum, 0);
                dev_info(dev, "%s: igo_ch_batch_finish_write ret = %d\n", __func__,status);

                if (IGO_CH_OP_MODE_ADDR == reg) {
                    atomic_set(&batchMode, 0);
                    dev_info(dev, "%s: IGO_CH_OP_MODE_ADDR: Disable batch mode\n", __func__);
                }
            }
            else {
                dev_info(dev, "%s: BatchMode without write \n", __func__);
            }

            return status;
        }
#endif
        break;
    }

    if (IGO_CH_POWER_MODE_ADDR == reg) {
        if (POWER_MODE_STANDBY == data) {
            #ifdef HW_RESET_INSTEAD_OF_STANDBY
            dev_info(dev, "%s: POWER_MODE_STANDBY: HW_RESET_INSTEAD_OF_STANDBY\n", __func__);
            // about 40ms
            debussy->reset_chip(debussy->dev);
            return IGO_CH_STATUS_DONE;
            #endif
        }

        noDelayAfterWrite = 20;
        dev_info(dev, "%s: POWER_MODE_WORKING: set noDelayAfterWrite = %d \n", __func__,noDelayAfterWrite);
    }

    return igo_ch_write_wait(dev, reg, data, noDelayAfterWrite);
}

int igo_ch_read(struct device* dev, unsigned int reg, unsigned int* data)
{
    struct i2c_client* client;
    unsigned int cmd[2];
    unsigned int status = IGO_CH_STATUS_CMD_RDY;

    client = to_i2c_client(dev);

    cmd[0] = status;
    cmd[1] = (IGO_CH_ACTION_READ << IGO_CH_ACTION_OFFSET) + reg;
    igo_i2c_write(client, IGO_CH_CMD_ADDR, &cmd[1], 1);
    igo_i2c_write(client, IGO_CH_STATUS_ADDR, &cmd[0], 1);

    status = igo_ch_chk_done(dev);

    igo_i2c_read(client, IGO_CH_BUF_ADDR, data);

    if (status != IGO_CH_STATUS_DONE) {
        dev_err(dev, "igo cmd read 0x%08x fail, error no : %d\n", reg, status);
    }

    return status;
}

#ifdef ENABLE_CMD_BATCH_MODE_BUFFERED
int igo_ch_batch_write(struct device* dev, unsigned int cmd_index, unsigned int *data, unsigned int data_length)
{
    struct i2c_client* client;

    client = to_i2c_client(dev);
    return igo_i2c_write(client, IGO_CH_BUF_ADDR + (cmd_index << 3), data, data_length);
}

int igo_ch_batch_finish_write(struct device* dev, unsigned int num_of_cmd)
{
    struct i2c_client* client;
    unsigned int cmd[2];
    unsigned int status = IGO_CH_STATUS_CMD_RDY;

    client = to_i2c_client(dev);
    cmd[0] = IGO_CH_STATUS_CMD_RDY;
    cmd[1] = (IGO_CH_ACTION_BATCH << IGO_CH_ACTION_OFFSET) + num_of_cmd;
    igo_i2c_write(client, IGO_CH_CMD_ADDR, &cmd[1], 1);
    igo_i2c_write(client, IGO_CH_STATUS_ADDR, &cmd[0], 1);

    status = igo_ch_chk_done(dev);

    if (status != IGO_CH_STATUS_DONE) {
        dev_err(dev, "igo batch cmd write fail, error no : %d\n", status);
        igo_i2c_read(client, IGO_CH_RSV_ADDR, &cmd[0]);
        dev_err(dev, "igo cmd batch write fail: error bit 0x%08X\n", cmd[0]);

#ifdef IGO_TIMEOUT_RESET_CHIP
        #if 0
        // All CMD
        if (IGO_CH_STATUS_TIMEOUT == status) {
            struct debussy_priv *debussy = i2c_get_clientdata(client);

            // Reset IGO chip while IGO CMD timeout occured
            debussy->reset_chip(dev);
        }
        #else
        // Only power mode CMD
        if ((IGO_CH_STATUS_TIMEOUT == status) && (IGO_CH_POWER_MODE_ADDR == reg)) {
            // Force HW Reset about 40ms
            struct debussy_priv *debussy = i2c_get_clientdata(client);

            debussy->reset_chip(dev);
        }
        #endif
#endif
    }

    return status;
}
#endif

int igo_ch_buf_write(struct device* dev, unsigned int addr, unsigned int *data, unsigned int word_len) {
    struct i2c_client* client;
    unsigned int cmd[2];
    unsigned int status = IGO_CH_STATUS_CMD_RDY;
    unsigned int writeSize, writeSize_bytes, offset = 0;

    client = to_i2c_client(dev);

    while (word_len) {
        writeSize = (word_len >= (IG_BUF_RW_LEN >> 2)) ? (IG_BUF_RW_LEN >> 2) : word_len;
        writeSize_bytes = writeSize << 2;
        igo_i2c_write(client, IGO_CH_BUF_ADDR, &data[offset], writeSize);

        cmd[0] = IGO_CH_STATUS_CMD_RDY;
        cmd[1] = (IGO_CH_ACTION_WRITE << IGO_CH_ACTION_OFFSET) + addr;
        igo_i2c_write(client, IGO_CH_CMD_ADDR, &cmd[1], 1);
        igo_i2c_write(client, IGO_CH_OPT_ADDR, &writeSize_bytes, 1);
        igo_i2c_write(client, IGO_CH_STATUS_ADDR, &cmd[0], 1);

        status = igo_ch_chk_done(dev);

        if (status != IGO_CH_STATUS_DONE) {
            dev_err(dev, "igo buffer write fail, error no : %d\n", status);

#ifdef IGO_TIMEOUT_RESET_CHIP
            #if 0
            // All CMD
            if (IGO_CH_STATUS_TIMEOUT == status) {
                struct debussy_priv *debussy = i2c_get_clientdata(client);

                // Reset IGO chip while IGO CMD timeout occured
                debussy->reset_chip(dev);
            }
            #endif
#endif

            return status;

        }

        addr += writeSize << 2;
        word_len -= writeSize;
        offset += writeSize;
    }

    return status;
}

int igo_ch_buf_read(struct device* dev, unsigned int addr, unsigned int *data, unsigned int word_len) {
    struct i2c_client* client;
    unsigned int cmd[2];
    unsigned int status = IGO_CH_STATUS_CMD_RDY;
    unsigned int read_size, read_size_byte, offset = 0;

    while (word_len) {
        read_size = (word_len >= (IG_BUF_RW_LEN >> 2)) ? (IG_BUF_RW_LEN >> 2) : word_len;
        read_size_byte = read_size << 2;
        client = to_i2c_client(dev);

        cmd[0] = IGO_CH_STATUS_CMD_RDY;
        cmd[1] = (IGO_CH_ACTION_READ << IGO_CH_ACTION_OFFSET) + addr;
        igo_i2c_write(client, IGO_CH_CMD_ADDR, &cmd[1], 1);
        igo_i2c_write(client, IGO_CH_OPT_ADDR, &read_size_byte, 1);
        igo_i2c_write(client, IGO_CH_STATUS_ADDR, &cmd[0], 1);

        status = igo_ch_chk_done(dev);

        if (status != IGO_CH_STATUS_DONE) {
            dev_err(dev, "igo buffer read fail, error no : %d\n", status);

#ifdef IGO_TIMEOUT_RESET_CHIP
            #if 0
            // All CMD
            if (IGO_CH_STATUS_TIMEOUT == status) {
                struct debussy_priv *debussy = i2c_get_clientdata(client);

                // Reset IGO chip while IGO CMD timeout occured
                debussy->reset_chip(dev);
            }
            #endif
#endif

            return status;
        }

        igo_i2c_read_buffer(client, IGO_CH_BUF_ADDR, &data[offset], read_size);
        word_len -= read_size;
        offset += read_size;
        addr += read_size << 2;
    }

    return status;
}
