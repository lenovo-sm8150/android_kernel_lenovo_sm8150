#ifndef DEBUSSY_H
#define DEBUSSY_H

#include <sound/soc.h>
#include <linux/ioctl.h>

#define IGO_RST_GPIO                    (-2)        // Need modify, depend-on PCBA board

//#define ENABLE_GENETLINK
//#define ENABLE_CDEV
//#define DEBUSSY_TYPE_PSRAM              // else Flash type

#ifdef CONFIG_REGMAP_I2C
#define ENABLE_DEBUSSY_I2C_REGMAP
#endif

#ifdef CONFIG_REGMAP_SPI
//#define ENABLE_DEBUSSY_SPI_REGMAP         // TBD
#endif

//#define IG_CH_CMD_USING_SPI             // TBD

#define IGO_RST_HOLD_TIME_MIN           (5)
#define IGO_RST_RELEASE_TIME_MIN        (50)

#ifndef GPIO_LOW
#define GPIO_LOW        0
#endif

#ifndef GPIO_HIGH
#define GPIO_HIGH       1
#endif

#define IGO_CH_ACTION_OFFSET (28)

#define IGO_CH_RSV_ADDR     (0x2A0C7EF0)
#define IGO_CH_OPT_ADDR     (IGO_CH_RSV_ADDR + 0x04)
#define IGO_CH_STATUS_ADDR  (IGO_CH_RSV_ADDR + 0x08)
#define IGO_CH_CMD_ADDR     (IGO_CH_RSV_ADDR + 0x0C)
#define IGO_CH_BUF_ADDR     (IGO_CH_RSV_ADDR + 0x10)

struct debussy_priv {
    struct regmap *i2c_regmap;
	struct	i2c_client	*client;
    struct device* dev;
    struct regmap *spi_regmap;
    struct device* spi_dev;
    struct snd_soc_codec* codec;
    struct workqueue_struct* debussy_wq;
    struct work_struct fw_work;
    struct work_struct irq_work;
    struct dentry* dir;
    struct mutex igo_ch_lock;
    atomic_t maskConfigCmd;
    atomic_t referenceCount;
    atomic_t kws_triggered;
    atomic_t kws_count;
    void (*reset_chip)(struct device *);
    void (*mcu_hold)(struct device *, uint32_t);
    uint32_t reg_address;
    int32_t reset_gpio;
	int32_t power_en_gpio;
    uint32_t reset_hold_time;           // Reset Active Time
    uint32_t reset_release_time;        // Waiting time after reset released
    int32_t mcu_hold_gpio;
	struct regulator *avdd;
	unsigned int power_on_delay_us;
	struct	clk		*s_clk;
};

enum {
    DEBUSSY_AIF,
};

struct ig_ioctl_rg_arg {
    uint32_t address;
    uint32_t data;
};

struct ig_ioctl_kws_arg {
    uint32_t count;
    uint32_t status;
};

struct ig_ioctl_enroll_model_arg {
    uint32_t byte_len;
    uint32_t buf_addr;
};

#define IOCTL_ID                    '\x66'
#define IOCTL_REG_SET               _IOW(IOCTL_ID, 0, struct ig_ioctl_rg_arg)
#define IOCTL_REG_GET               _IOR(IOCTL_ID, 1, struct ig_ioctl_rg_arg)
#define IOCTL_KWS_STATUS_CLEAR      _IO(IOCTL_ID, 2)
#define IOCTL_KWS_STATUS_GET        _IOR(IOCTL_ID, 3, struct ig_ioctl_kws_arg)
#define IOCTL_ENROLL_MOD_GET        _IOR(IOCTL_ID, 4, struct ig_ioctl_enroll_model_arg)
#define IOCTL_ENROLL_MOD_SET        _IOW(IOCTL_ID, 5, struct ig_ioctl_enroll_model_arg)

extern struct debussy_priv* p_debussy_priv;

extern int debussy_cdev_init(void *priv_data);
extern void debussy_cdev_exit(void);

extern void debussy_flash_update_firmware(struct device* dev, unsigned int faddr, const u8* data, size_t size);
extern void debussy_psram_update_firmware(struct device* dev, unsigned int faddr, const u8* data, size_t size);

#endif
