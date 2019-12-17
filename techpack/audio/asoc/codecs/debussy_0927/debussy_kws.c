#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include "debussy.h"
#include "debussy_intf.h"
#include "debussy_snd_ctrl.h"
#ifdef ENABLE_GENETLINK
#include "debussy_genetlink.h"
#endif

static u32 gpio_pin, gpio_kws_deb;
static u32 kws_irq;

static const struct of_device_id kws_of_match[] = {
    { .compatible = "intelligo,debussy", },
    {},
};

static struct {
    struct debussy_priv* debussy;
} debussy_kws_priv;

static void debussy_kws_irq_work(struct work_struct* work)
{
    struct debussy_priv* debussy;

    debussy = container_of(work, struct debussy_priv, irq_work);
    dev_info(debussy->dev, "%s\n", __func__);

    // Add Customer Code to wakeup AP
    atomic_set(&debussy->kws_triggered, 1);
    atomic_inc(&debussy->kws_count);
    dev_info(debussy->dev, "####### debussy->kws_count = %d #######\n", atomic_read(&debussy->kws_count));

    #ifdef ENABLE_GENETLINK
    dev_info(debussy->dev, "Send: debussy_genetlink_multicast\n");
    debussy_genetlink_multicast("GENL:VAD_TIRGERED", atomic_read(&debussy->kws_count));
    #endif
}

#if 0
void debussy_kws_disable(void){
    int status = IGO_CH_STATUS_DONE;
    struct debussy_priv* debussy = debussy_kws_priv.debussy;

    mutex_lock(&debussy->igo_ch_lock);

    status = igo_ch_write(debussy->dev, IGO_CH_VAD_CLEAR_ADDR, VAD_CLEAR_ENABLE);
    igo_ch_write(debussy->dev, IGO_CH_POWER_MODE_ADDR, POWER_MODE_STANDBY);
    dev_info(debussy->dev, "%s: ret %d\n", __func__, status);

    mutex_unlock(&debussy->igo_ch_lock);
}

void debussy_kws_enable(void){
    int status = IGO_CH_STATUS_DONE;
    struct debussy_priv* debussy = debussy_kws_priv.debussy;

    mutex_lock(&debussy->igo_ch_lock);

    igo_ch_write(debussy->dev, IGO_CH_POWER_MODE_ADDR, POWER_MODE_WORKING);
    igo_ch_write(debussy->dev, IGO_CH_DMIC_M_CLK_SRC_ADDR, DMIC_M_CLK_SRC_INTERNAL);
    igo_ch_write(debussy->dev, IGO_CH_DMIC_M_BCLK_ADDR, DMIC_M_BCLK_ADAPTIVE);
    igo_ch_write(debussy->dev, IGO_CH_DMIC_M0_P_MODE_ADDR, DMIC_M0_P_MODE_ENABLE);
    igo_ch_write(debussy->dev, IGO_CH_CH1_RX_ADDR, CH1_RX_DMIC_M0_P);
    igo_ch_write(debussy->dev, IGO_CH_VAD_INT_PIN_ADDR, VAD_INT_PIN_DAI2_RXDAT);
    status = igo_ch_write(debussy->dev, IGO_CH_OP_MODE_ADDR, OP_MODE_VAD);
    dev_info(debussy->dev, "%s: ret %d\n", __func__, status);

    mutex_unlock(&debussy->igo_ch_lock);
}
#endif

static irqreturn_t debussy_eint_handler(int irq, void *data)
{
    struct debussy_priv* debussy = debussy_kws_priv.debussy;

    dev_info(debussy->dev, "%s -\n", __func__);
    queue_work(debussy->debussy_wq, &debussy->irq_work);

    return IRQ_HANDLED;
}

int debussy_kws_init(struct debussy_priv* debussy)
{
    int ret;
    unsigned long kws_irqflags = IRQF_TRIGGER_RISING;
    struct device_node   *node = NULL;

    debussy_kws_priv.debussy = debussy;

    INIT_WORK(&debussy->irq_work, debussy_kws_irq_work);

    node = of_find_matching_node(node, kws_of_match);
    if (!node) {
        dev_err(debussy->dev, "%s: there is no this node\n", __func__);
        return -1;
    }

    gpio_pin = of_get_named_gpio(node, "ig,deb-gpios", 0);
    dev_info(debussy->dev, "%s: kws gpio pin found:%d\n", __func__, gpio_pin);
    ret = of_property_read_u32(node, "ig,debounce", &gpio_kws_deb);
    if (ret) {
        dev_err(debussy->dev, "%s: gpio debounce not found,ret:%d\n", __func__, ret);
        return ret;
    }

    dev_info(debussy->dev, "%s: gpio debounce found:%d\n", __func__, gpio_kws_deb);
    gpio_set_debounce(gpio_pin, gpio_kws_deb);

#if 0
    kws_irq = irq_of_parse_and_map(node, 0);
    ret = of_property_read_u32_array(node, "interrupts", ints, ARRAY_SIZE(ints));
    if (ret) {
        dev_err(debussy->dev, "%s: interrupts not found,ret:%d\n", __func__, ret);
        return ret;
    }
    dev_info(debussy->dev, "%s: get interrupts: %d, %d, %d, %d\n", __func__, ints[0], ints[1], ints[2], ints[3]);
    kws_eint_type = ints[1];
#else
    kws_irq = gpio_to_irq(gpio_pin);
#endif

    ret = request_irq(kws_irq, debussy_eint_handler, kws_irqflags, "debussy-kws-eint", NULL);
    if (ret) {
        dev_err(debussy->dev, "%s: request_irq fail, ret:%d.\n", __func__, ret);
        return ret;
    }

    /*ret = fsfuncirq_set_irq_wake(kws_irq, 1);*/
    ret = enable_irq_wake(kws_irq);
    if (ret) {
        dev_err(debussy->dev, "%s: set_irq_wake fail, ret:%d.\n", __func__, ret);
        return ret;
    }

    dev_info(debussy->dev, "%s: set gpio EINT finished, irq=%d, gpio_headset_deb=%d\n", __func__, kws_irq, gpio_kws_deb);

    return 0;
}
