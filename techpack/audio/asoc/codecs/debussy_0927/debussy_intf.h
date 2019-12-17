#ifndef DEBUSSY_INTF_H
#define DEBUSSY_INTF_H

#include <linux/i2c.h>

typedef enum {
    IGO_CH_ACTION_NOP = 0,
    IGO_CH_ACTION_READ = 1,
    IGO_CH_ACTION_WRITE = 2,
    IGO_CH_ACTION_BATCH = 3,
} IGO_CH_ACTION_E;

typedef enum {
    IGO_CH_STATUS_NOP = 0,
    IGO_CH_STATUS_CMD_RDY = 1,
    IGO_CH_STATUS_DONE = 2,
    IGO_CH_STATUS_BUSY = 3,
    IGO_CH_STATUS_ACTION_ERROR = 4,
    IGO_CH_STATUS_ADDR_ERROR = 5,
    IGO_CH_STATUS_CFG_ERROR = 6,
    IGO_CH_STATUS_CFG_NOT_SUPPORT = 7,
    IGO_CH_STATUS_TIMEOUT = 8,

    // keeping in the last position
    IGO_CH_STATUS_MAX
} IGO_CH_STATUS_E;

extern int igo_i2c_read(struct i2c_client* client, unsigned int addr, unsigned int* value);
extern int igo_i2c_write(struct i2c_client* client, unsigned int addr, unsigned int* buff, unsigned int word_len);
extern int igo_i2c_read_buffer(struct i2c_client* client, unsigned int addr, unsigned int* buff, unsigned int word_len);

extern int igo_spi_read_buffer(unsigned int addr, unsigned int* buff, unsigned int word_len);
extern int igo_spi_read(unsigned int addr, unsigned int *value);
extern int igo_spi_write(unsigned int reg, unsigned int* data);

extern int igo_ch_write(struct device* dev, unsigned int reg, unsigned int data);
extern int igo_ch_read(struct device* dev, unsigned int reg, unsigned int* data);
extern int igo_ch_batch_write(struct device* dev, unsigned int cmd_index, unsigned int *data, unsigned int data_length);
extern int igo_ch_batch_finish_write(struct device* dev, unsigned int num_of_cmd);
extern int igo_ch_write_wait(struct device* dev, unsigned int reg, unsigned int data, unsigned int wait_time);
extern int igo_ch_buf_write(struct device* dev, unsigned int addr, unsigned int *data, unsigned int word_len);
extern int igo_ch_buf_read(struct device* dev, unsigned int addr, unsigned int *data, unsigned int word_len);
extern int igo_ch_buf_read_spi(struct device* dev, unsigned int addr, unsigned int *data, unsigned int word_len);

#define igo_ch_slow_write(dev, cmd, value)              igo_ch_write(dev, cmd, value)

#define IG_I2C_BUF_RW_LEN                               (512)       // bytes
#define IG_BUF_RW_LEN                                   (256)       // bytes

#endif
