#include <linux/version.h>
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
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include "debussy.h"
#include "debussy_intf.h"
#include "debussy_snd_ctrl.h"
#define DEBUSSY_SCENARIOUS_C
#include "debussy_scenarios.h"

#define IGO_CH_NO_INVERT (0)
#define IGO_CH_MAX (0xFFFFFFFF)
#define IGO_CH_SHIFT (0)

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,13,11)
// For Android 5.1
#define snd_soc_kcontrol_codec          snd_kcontrol_chip
#endif

static const char* const enum_power_mode[] = {
    "POWER_MODE_STANDBY",
    "POWER_MODE_WORKING",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_power_mode, enum_power_mode);


static const char* const enum_op_mode[] = {
    "OP_MODE_CONFIG",
    "OP_MODE_NR",
    "OP_MODE_VAD",
    "OP_MODE_BYPASS",
    "OP_MODE_NR_AEC",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_op_mode, enum_op_mode);


static const char* const enum_mclk[] = {
    "MCLK_NONE",
    "MCLK_26M",
    "MCLK_24_576M",
    "MCLK_20M",
    "MCLK_19_2M",
    "MCLK_12_288M",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_mclk, enum_mclk);


static const char* const enum_ck_output[] = {
    "CK_OUTPUT_DISABLE",
    "CK_OUTPUT_12M",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ck_output, enum_ck_output);


static const char* const enum_ch0_rx[] = {
    "CH0_RX_DISABLE",
    "CH0_RX_DAI0_RX_L",
    "CH0_RX_DAI0_RX_R",
    "CH0_RX_DAI1_RX_L",
    "CH0_RX_DAI1_RX_R",
    "CH0_RX_DAI2_RX_L",
    "CH0_RX_DAI2_RX_R",
    "CH0_RX_DAI3_RX_L",
    "CH0_RX_DAI3_RX_R",
    "CH0_RX_DMIC_M0_P",
    "CH0_RX_DMIC_M0_N",
    "CH0_RX_DMIC_M1_P",
    "CH0_RX_DMIC_M1_N",
    "CH0_RX_DMIC_COMBO_M0_P",
    "CH0_RX_DMIC_COMBO_M0_N",
    "CH0_RX_DBG_MODE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch0_rx, enum_ch0_rx);


static const char* const enum_ch0_tx[] = {
    "CH0_TX_DISABLE",
    "CH0_TX_DAI0_TX_L",
    "CH0_TX_DAI0_TX_R",
    "CH0_TX_DAI1_TX_L",
    "CH0_TX_DAI1_TX_R",
    "CH0_TX_DAI2_TX_L",
    "CH0_TX_DAI2_TX_R",
    "CH0_TX_DAI3_TX_L",
    "CH0_TX_DAI3_TX_R",
    "CH0_TX_DMIC_S0_P",
    "CH0_TX_DMIC_S0_N",
    "CH0_TX_DMIC_S1_P",
    "CH0_TX_DMIC_S1_N",
    "CH0_TX_DMIC_COMBO_S0_P",
    "CH0_TX_DMIC_COMBO_S0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch0_tx, enum_ch0_tx);


static const char* const enum_ch0_tx_gain[] = {
    "CH0_TX_GAIN_0_DB",
    "CH0_TX_GAIN_P_1_DB",
    "CH0_TX_GAIN_P_2_DB",
    "CH0_TX_GAIN_P_3_DB",
    "CH0_TX_GAIN_P_4_DB",
    "CH0_TX_GAIN_P_5_DB",
    "CH0_TX_GAIN_P_6_DB",
    "CH0_TX_GAIN_P_7_DB",
    "CH0_TX_GAIN_P_8_DB",
    "CH0_TX_GAIN_P_9_DB",
    "CH0_TX_GAIN_P_10_DB",
    "CH0_TX_GAIN_P_11_DB",
    "CH0_TX_GAIN_P_12_DB",
    "CH0_TX_GAIN_P_13_DB",
    "CH0_TX_GAIN_P_14_DB",
    "CH0_TX_GAIN_P_15_DB",
    "CH0_TX_GAIN_P_16_DB",
    "CH0_TX_GAIN_N_1_DB",
    "CH0_TX_GAIN_N_2_DB",
    "CH0_TX_GAIN_N_3_DB",
    "CH0_TX_GAIN_N_4_DB",
    "CH0_TX_GAIN_N_5_DB",
    "CH0_TX_GAIN_N_6_DB",
    "CH0_TX_GAIN_N_7_DB",
    "CH0_TX_GAIN_N_8_DB",
    "CH0_TX_GAIN_N_9_DB",
    "CH0_TX_GAIN_N_10_DB",
    "CH0_TX_GAIN_N_11_DB",
    "CH0_TX_GAIN_N_12_DB",
    "CH0_TX_GAIN_N_13_DB",
    "CH0_TX_GAIN_N_14_DB",
    "CH0_TX_GAIN_N_15_DB",
    "CH0_TX_GAIN_N_16_DB",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch0_tx_gain, enum_ch0_tx_gain);


static const char* const enum_ch1_rx[] = {
    "CH1_RX_DISABLE",
    "CH1_RX_DAI0_RX_L",
    "CH1_RX_DAI0_RX_R",
    "CH1_RX_DAI1_RX_L",
    "CH1_RX_DAI1_RX_R",
    "CH1_RX_DAI2_RX_L",
    "CH1_RX_DAI2_RX_R",
    "CH1_RX_DAI3_RX_L",
    "CH1_RX_DAI3_RX_R",
    "CH1_RX_DMIC_M0_P",
    "CH1_RX_DMIC_M0_N",
    "CH1_RX_DMIC_M1_P",
    "CH1_RX_DMIC_M1_N",
    "CH1_RX_DMIC_COMBO_M0_P",
    "CH1_RX_DMIC_COMBO_M0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_rx, enum_ch1_rx);


static const char* const enum_ch1_tx[] = {
    "CH1_TX_DISABLE",
    "CH1_TX_DAI0_TX_L",
    "CH1_TX_DAI0_TX_R",
    "CH1_TX_DAI1_TX_L",
    "CH1_TX_DAI1_TX_R",
    "CH1_TX_DAI2_TX_L",
    "CH1_TX_DAI2_TX_R",
    "CH1_TX_DAI3_TX_L",
    "CH1_TX_DAI3_TX_R",
    "CH1_TX_DMIC_S0_P",
    "CH1_TX_DMIC_S0_N",
    "CH1_TX_DMIC_S1_P",
    "CH1_TX_DMIC_S1_N",
    "CH1_TX_DMIC_COMBO_S0_P",
    "CH1_TX_DMIC_COMBO_S0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_tx, enum_ch1_tx);


static const char* const enum_ch1_tx_gain[] = {
    "CH1_TX_GAIN_0_DB",
    "CH1_TX_GAIN_P_1_DB",
    "CH1_TX_GAIN_P_2_DB",
    "CH1_TX_GAIN_P_3_DB",
    "CH1_TX_GAIN_P_4_DB",
    "CH1_TX_GAIN_P_5_DB",
    "CH1_TX_GAIN_P_6_DB",
    "CH1_TX_GAIN_P_7_DB",
    "CH1_TX_GAIN_P_8_DB",
    "CH1_TX_GAIN_P_9_DB",
    "CH1_TX_GAIN_P_10_DB",
    "CH1_TX_GAIN_P_11_DB",
    "CH1_TX_GAIN_P_12_DB",
    "CH1_TX_GAIN_P_13_DB",
    "CH1_TX_GAIN_P_14_DB",
    "CH1_TX_GAIN_P_15_DB",
    "CH1_TX_GAIN_P_16_DB",
    "CH1_TX_GAIN_N_1_DB",
    "CH1_TX_GAIN_N_2_DB",
    "CH1_TX_GAIN_N_3_DB",
    "CH1_TX_GAIN_N_4_DB",
    "CH1_TX_GAIN_N_5_DB",
    "CH1_TX_GAIN_N_6_DB",
    "CH1_TX_GAIN_N_7_DB",
    "CH1_TX_GAIN_N_8_DB",
    "CH1_TX_GAIN_N_9_DB",
    "CH1_TX_GAIN_N_10_DB",
    "CH1_TX_GAIN_N_11_DB",
    "CH1_TX_GAIN_N_12_DB",
    "CH1_TX_GAIN_N_13_DB",
    "CH1_TX_GAIN_N_14_DB",
    "CH1_TX_GAIN_N_15_DB",
    "CH1_TX_GAIN_N_16_DB",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_tx_gain, enum_ch1_tx_gain);


static const char* const enum_dai_0_mode[] = {
    "DAI_0_MODE_DISABLE",
    "DAI_0_MODE_SLAVE",
    "DAI_0_MODE_MASTER",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_0_mode, enum_dai_0_mode);


static const char* const enum_dai_0_clk_src[] = {
    "DAI_0_CLK_SRC_DISABLE",
    "DAI_0_CLK_SRC_MCLK",
    "DAI_0_CLK_SRC_INTERNAL",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_0_clk_src, enum_dai_0_clk_src);


static const char* const enum_dai_0_clk[] = {
    "DAI_0_CLK_16K",
    "DAI_0_CLK_32K",
    "DAI_0_CLK_48K",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_0_clk, enum_dai_0_clk);


static const char* const enum_dai_0_data_bit[] = {
    "DAI_0_DATA_BIT_32",
    "DAI_0_DATA_BIT_16",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_0_data_bit, enum_dai_0_data_bit);


static const char* const enum_dai_1_mode[] = {
    "DAI_1_MODE_DISABLE",
    "DAI_1_MODE_SLAVE",
    "DAI_1_MODE_MASTER",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_1_mode, enum_dai_1_mode);


static const char* const enum_dai_1_clk_src[] = {
    "DAI_1_CLK_SRC_DISABLE",
    "DAI_1_CLK_SRC_MCLK",
    "DAI_1_CLK_SRC_INTERNAL",
    "DAI_1_CLK_SRC_DAI_0",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_1_clk_src, enum_dai_1_clk_src);


static const char* const enum_dai_1_clk[] = {
    "DAI_1_CLK_16K",
    "DAI_1_CLK_32K",
    "DAI_1_CLK_48K",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_1_clk, enum_dai_1_clk);


static const char* const enum_dai_1_data_bit[] = {
    "DAI_1_DATA_BIT_32",
    "DAI_1_DATA_BIT_16",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_1_data_bit, enum_dai_1_data_bit);


static const char* const enum_dai_2_mode[] = {
    "DAI_2_MODE_DISABLE",
    "DAI_2_MODE_SLAVE",
    "DAI_2_MODE_MASTER",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_2_mode, enum_dai_2_mode);


static const char* const enum_dai_2_clk_src[] = {
    "DAI_2_CLK_SRC_DISABLE",
    "DAI_2_CLK_SRC_MCLK",
    "DAI_2_CLK_SRC_INTERNAL",
    "DAI_2_CLK_SRC_DAI_0",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_2_clk_src, enum_dai_2_clk_src);


static const char* const enum_dai_2_clk[] = {
    "DAI_2_CLK_16K",
    "DAI_2_CLK_32K",
    "DAI_2_CLK_48K",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_2_clk, enum_dai_2_clk);


static const char* const enum_dai_2_data_bit[] = {
    "DAI_2_DATA_BIT_32",
    "DAI_2_DATA_BIT_16",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_2_data_bit, enum_dai_2_data_bit);


static const char* const enum_dai_3_mode[] = {
    "DAI_3_MODE_DISABLE",
    "DAI_3_MODE_SLAVE",
    "DAI_3_MODE_MASTER",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_3_mode, enum_dai_3_mode);


static const char* const enum_dai_3_clk_src[] = {
    "DAI_3_CLK_SRC_DISABLE",
    "DAI_3_CLK_SRC_MCLK",
    "DAI_3_CLK_SRC_INTERNAL",
    "DAI_3_CLK_SRC_DAI_0",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_3_clk_src, enum_dai_3_clk_src);


static const char* const enum_dai_3_clk[] = {
    "DAI_3_CLK_16K",
    "DAI_3_CLK_32K",
    "DAI_3_CLK_48K",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_3_clk, enum_dai_3_clk);


static const char* const enum_dai_3_data_bit[] = {
    "DAI_3_DATA_BIT_32",
    "DAI_3_DATA_BIT_16",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dai_3_data_bit, enum_dai_3_data_bit);


static const char* const enum_dmic_m_clk_src[] = {
    "DMIC_M_CLK_SRC_DMIC_S",
    "DMIC_M_CLK_SRC_MCLK",
    "DMIC_M_CLK_SRC_INTERNAL",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m_clk_src, enum_dmic_m_clk_src);


static const char* const enum_dmic_m_bclk[] = {
    "DMIC_M_BCLK_3_072M",
    "DMIC_M_BCLK_2_4M",
    "DMIC_M_BCLK_2_0M",
    "DMIC_M_BCLK_1_625M",
    "DMIC_M_BCLK_1_536M",
    "DMIC_M_BCLK_1_0M",
    "DMIC_M_BCLK_ADAPTIVE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m_bclk, enum_dmic_m_bclk);


static const char* const enum_dmic_s_bclk[] = {
    "DMIC_S_BCLK_3_072M",
    "DMIC_S_BCLK_2_4M",
    "DMIC_S_BCLK_2_0M",
    "DMIC_S_BCLK_1_625M",
    "DMIC_S_BCLK_1_536M",
    "DMIC_S_BCLK_1_0M",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_s_bclk, enum_dmic_s_bclk);


static const char* const enum_dmic_m0_p_mode[] = {
    "DMIC_M0_P_MODE_DISABLE",
    "DMIC_M0_P_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m0_p_mode, enum_dmic_m0_p_mode);


static const char* const enum_dmic_m0_n_mode[] = {
    "DMIC_M0_N_MODE_DISABLE",
    "DMIC_M0_N_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m0_n_mode, enum_dmic_m0_n_mode);


static const char* const enum_dmic_m1_p_mode[] = {
    "DMIC_M1_P_MODE_DISABLE",
    "DMIC_M1_P_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m1_p_mode, enum_dmic_m1_p_mode);


static const char* const enum_dmic_m1_n_mode[] = {
    "DMIC_M1_N_MODE_DISABLE",
    "DMIC_M1_N_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m1_n_mode, enum_dmic_m1_n_mode);


static const char* const enum_dmic_m2_p_mode[] = {
    "DMIC_M2_P_MODE_DISABLE",
    "DMIC_M2_P_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m2_p_mode, enum_dmic_m2_p_mode);


static const char* const enum_dmic_m2_n_mode[] = {
    "DMIC_M2_N_MODE_DISABLE",
    "DMIC_M2_N_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m2_n_mode, enum_dmic_m2_n_mode);


static const char* const enum_dmic_m3_p_mode[] = {
    "DMIC_M3_P_MODE_DISABLE",
    "DMIC_M3_P_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m3_p_mode, enum_dmic_m3_p_mode);


static const char* const enum_dmic_m3_n_mode[] = {
    "DMIC_M3_N_MODE_DISABLE",
    "DMIC_M3_N_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_m3_n_mode, enum_dmic_m3_n_mode);


static const char* const enum_dmic_s0_p_mode[] = {
    "DMIC_S0_P_MODE_DISABLE",
    "DMIC_S0_P_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_s0_p_mode, enum_dmic_s0_p_mode);


static const char* const enum_dmic_s0_n_mode[] = {
    "DMIC_S0_N_MODE_DISABLE",
    "DMIC_S0_N_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_s0_n_mode, enum_dmic_s0_n_mode);


static const char* const enum_dmic_s1_p_mode[] = {
    "DMIC_S1_P_MODE_DISABLE",
    "DMIC_S1_P_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_s1_p_mode, enum_dmic_s1_p_mode);


static const char* const enum_dmic_s1_n_mode[] = {
    "DMIC_S1_N_MODE_DISABLE",
    "DMIC_S1_N_MODE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_dmic_s1_n_mode, enum_dmic_s1_n_mode);


static const char* const enum_nr_ch0[] = {
    "NR_CH0_DISABLE",
    "NR_CH0_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_nr_ch0, enum_nr_ch0);


static const char* const enum_nr_ch1[] = {
    "NR_CH1_DISABLE",
    "NR_CH1_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_nr_ch1, enum_nr_ch1);


static const char* const enum_ch1_ref_mode[] = {
    "CH1_REF_MODE_DISABLE",
    "CH1_REF_MODE_MODE0",
    "CH1_REF_MODE_MODE1",
    "CH1_REF_MODE_MODE2",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_ref_mode, enum_ch1_ref_mode);


static const char* const enum_ch1_ref_rx[] = {
    "CH1_REF_RX_DISABLE",
    "CH1_REF_RX_DAI0_RX_L",
    "CH1_REF_RX_DAI0_RX_R",
    "CH1_REF_RX_DAI1_RX_L",
    "CH1_REF_RX_DAI1_RX_R",
    "CH1_REF_RX_DAI2_RX_L",
    "CH1_REF_RX_DAI2_RX_R",
    "CH1_REF_RX_DAI3_RX_L",
    "CH1_REF_RX_DAI3_RX_R",
    "CH1_REF_RX_DMIC_M0_P",
    "CH1_REF_RX_DMIC_M0_N",
    "CH1_REF_RX_DMIC_M1_P",
    "CH1_REF_RX_DMIC_M1_N",
    "CH1_REF_RX_DMIC_COMBO_M0_P",
    "CH1_REF_RX_DMIC_COMBO_M0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_ref_rx, enum_ch1_ref_rx);


static const char* const enum_ch0_floor[] = {
    "CH0_FLOOR_LVL_DEFAULT",
    "CH0_FLOOR_LVL_0",
    "CH0_FLOOR_LVL_1",
    "CH0_FLOOR_LVL_2",
    "CH0_FLOOR_LVL_3",
    "CH0_FLOOR_LVL_4",
    "CH0_FLOOR_LVL_5",
    "CH0_FLOOR_LVL_6",
    "CH0_FLOOR_LVL_7",
    "CH0_FLOOR_LVL_8",
    "CH0_FLOOR_LVL_9",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch0_floor, enum_ch0_floor);


static const char* const enum_ch1_floor[] = {
    "CH1_FLOOR_LVL_DEFAULT",
    "CH1_FLOOR_LVL_0",
    "CH1_FLOOR_LVL_1",
    "CH1_FLOOR_LVL_2",
    "CH1_FLOOR_LVL_3",
    "CH1_FLOOR_LVL_4",
    "CH1_FLOOR_LVL_5",
    "CH1_FLOOR_LVL_6",
    "CH1_FLOOR_LVL_7",
    "CH1_FLOOR_LVL_8",
    "CH1_FLOOR_LVL_9",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_floor, enum_ch1_floor);


static const char* const enum_ch0_od[] = {
    "CH0_OD_LVL_DEFAULT",
    "CH0_OD_LVL_0",
    "CH0_OD_LVL_1",
    "CH0_OD_LVL_2",
    "CH0_OD_LVL_3",
    "CH0_OD_LVL_4",
    "CH0_OD_LVL_5",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch0_od, enum_ch0_od);


static const char* const enum_ch1_od[] = {
    "CH1_OD_LVL_DEFAULT",
    "CH1_OD_LVL_0",
    "CH1_OD_LVL_1",
    "CH1_OD_LVL_2",
    "CH1_OD_LVL_3",
    "CH1_OD_LVL_4",
    "CH1_OD_LVL_5",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_od, enum_ch1_od);


static const char* const enum_ch0_thr[] = {
    "CH0_THR_LVL_DEFAULT",
    "CH0_THR_LVL_0",
    "CH0_THR_LVL_1",
    "CH0_THR_LVL_2",
    "CH0_THR_LVL_3",
    "CH0_THR_LVL_4",
    "CH0_THR_LVL_5",
    "CH0_THR_LVL_6",
    "CH0_THR_LVL_7",
    "CH0_THR_LVL_8",
    "CH0_THR_LVL_9",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch0_thr, enum_ch0_thr);


static const char* const enum_ch1_thr[] = {
    "CH1_THR_LVL_DEFAULT",
    "CH1_THR_LVL_0",
    "CH1_THR_LVL_1",
    "CH1_THR_LVL_2",
    "CH1_THR_LVL_3",
    "CH1_THR_LVL_4",
    "CH1_THR_LVL_5",
    "CH1_THR_LVL_6",
    "CH1_THR_LVL_7",
    "CH1_THR_LVL_8",
    "CH1_THR_LVL_9",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_ch1_thr, enum_ch1_thr);


static const char* const enum_hw_bypass_dai_0[] = {
    "HW_BYPASS_DAI_0_DISABLE",
    "HW_BYPASS_DAI_0_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_hw_bypass_dai_0, enum_hw_bypass_dai_0);


static const char* const enum_hw_bypass_dmic_s0[] = {
    "HW_BYPASS_DMIC_S0_DISABLE",
    "HW_BYPASS_DMIC_S0_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_hw_bypass_dmic_s0, enum_hw_bypass_dmic_s0);


static const char* const enum_sw_bypass_0_rx[] = {
    "SW_BYPASS_0_RX_DISABLE",
    "SW_BYPASS_0_RX_DAI0_RX_L",
    "SW_BYPASS_0_RX_DAI0_RX_R",
    "SW_BYPASS_0_RX_DAI1_RX_L",
    "SW_BYPASS_0_RX_DAI1_RX_R",
    "SW_BYPASS_0_RX_DAI2_RX_L",
    "SW_BYPASS_0_RX_DAI2_RX_R",
    "SW_BYPASS_0_RX_DAI3_RX_L",
    "SW_BYPASS_0_RX_DAI3_RX_R",
    "SW_BYPASS_0_RX_DMIC_M0_P",
    "SW_BYPASS_0_RX_DMIC_M0_N",
    "SW_BYPASS_0_RX_DMIC_M1_P",
    "SW_BYPASS_0_RX_DMIC_M1_N",
    "SW_BYPASS_0_RX_DMIC_COMBO_M0_P",
    "SW_BYPASS_0_RX_DMIC_COMBO_M0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_sw_bypass_0_rx, enum_sw_bypass_0_rx);


static const char* const enum_sw_bypass_0_tx[] = {
    "SW_BYPASS_0_TX_DISABLE",
    "SW_BYPASS_0_TX_DAI0_TX_L",
    "SW_BYPASS_0_TX_DAI0_TX_R",
    "SW_BYPASS_0_TX_DAI1_TX_L",
    "SW_BYPASS_0_TX_DAI1_TX_R",
    "SW_BYPASS_0_TX_DAI2_TX_L",
    "SW_BYPASS_0_TX_DAI2_TX_R",
    "SW_BYPASS_0_TX_DAI3_TX_L",
    "SW_BYPASS_0_TX_DAI3_TX_R",
    "SW_BYPASS_0_TX_DMIC_S0_P",
    "SW_BYPASS_0_TX_DMIC_S0_N",
    "SW_BYPASS_0_TX_DMIC_S1_P",
    "SW_BYPASS_0_TX_DMIC_S1_N",
    "SW_BYPASS_0_TX_DMIC_COMBO_S0_P",
    "SW_BYPASS_0_TX_DMIC_COMBO_S0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_sw_bypass_0_tx, enum_sw_bypass_0_tx);


static const char* const enum_sw_bypass_1_rx[] = {
    "SW_BYPASS_1_RX_DISABLE",
    "SW_BYPASS_1_RX_DAI0_RX_L",
    "SW_BYPASS_1_RX_DAI0_RX_R",
    "SW_BYPASS_1_RX_DAI1_RX_L",
    "SW_BYPASS_1_RX_DAI1_RX_R",
    "SW_BYPASS_1_RX_DAI2_RX_L",
    "SW_BYPASS_1_RX_DAI2_RX_R",
    "SW_BYPASS_1_RX_DAI3_RX_L",
    "SW_BYPASS_1_RX_DAI3_RX_R",
    "SW_BYPASS_1_RX_DMIC_M0_P",
    "SW_BYPASS_1_RX_DMIC_M0_N",
    "SW_BYPASS_1_RX_DMIC_M1_P",
    "SW_BYPASS_1_RX_DMIC_M1_N",
    "SW_BYPASS_1_RX_DMIC_COMBO_M0_P",
    "SW_BYPASS_1_RX_DMIC_COMBO_M0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_sw_bypass_1_rx, enum_sw_bypass_1_rx);


static const char* const enum_sw_bypass_1_tx[] = {
    "SW_BYPASS_1_TX_DISABLE",
    "SW_BYPASS_1_TX_DAI0_TX_L",
    "SW_BYPASS_1_TX_DAI0_TX_R",
    "SW_BYPASS_1_TX_DAI1_TX_L",
    "SW_BYPASS_1_TX_DAI1_TX_R",
    "SW_BYPASS_1_TX_DAI2_TX_L",
    "SW_BYPASS_1_TX_DAI2_TX_R",
    "SW_BYPASS_1_TX_DAI3_TX_L",
    "SW_BYPASS_1_TX_DAI3_TX_R",
    "SW_BYPASS_1_TX_DMIC_S0_P",
    "SW_BYPASS_1_TX_DMIC_S0_N",
    "SW_BYPASS_1_TX_DMIC_S1_P",
    "SW_BYPASS_1_TX_DMIC_S1_N",
    "SW_BYPASS_1_TX_DMIC_COMBO_S0_P",
    "SW_BYPASS_1_TX_DMIC_COMBO_S0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_sw_bypass_1_tx, enum_sw_bypass_1_tx);


static const char* const enum_aec[] = {
    "AEC_DISABLE",
    "AEC_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_aec, enum_aec);


static const char* const enum_aec_ref_rx[] = {
    "AEC_REF_RX_DISABLE",
    "AEC_REF_RX_CH0_TX",
    "AEC_REF_RX_DAI0_RX_L",
    "AEC_REF_RX_DAI0_RX_R",
    "AEC_REF_RX_DAI1_RX_L",
    "AEC_REF_RX_DAI1_RX_R",
    "AEC_REF_RX_DAI2_RX_L",
    "AEC_REF_RX_DAI2_RX_R",
    "AEC_REF_RX_DAI3_RX_L",
    "AEC_REF_RX_DAI3_RX_R",
    "AEC_REF_RX_DMIC_M0_P",
    "AEC_REF_RX_DMIC_M0_N",
    "AEC_REF_RX_DMIC_M1_P",
    "AEC_REF_RX_DMIC_M1_N",
    "AEC_REF_RX_DMIC_COMBO_M0_P",
    "AEC_REF_RX_DMIC_COMBO_M0_N",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_aec_ref_rx, enum_aec_ref_rx);


static const char* const enum_vad_status[] = {
    "VAD_STATUS_STANDBY",
    "VAD_STATUS_TRIGGERED",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_status, enum_vad_status);


static const char* const enum_vad_clear[] = {
    "VAD_CLEAR_NOP",
    "VAD_CLEAR_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_clear, enum_vad_clear);


static const char* const enum_vad_int_mod[] = {
    "VAD_INT_MOD_DISABLE",
    "VAD_INT_MOD_EDGE",
    "VAD_INT_MOD_LEVEL",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_int_mod, enum_vad_int_mod);


static const char* const enum_vad_int_pin[] = {
    "VAD_INT_PIN_DAI0_BCLK",
    "VAD_INT_PIN_DAI0_LRCLK",
    "VAD_INT_PIN_DAI0_RXDAT",
    "VAD_INT_PIN_DAI0_TXDAT",
    "VAD_INT_PIN_DAI1_BCLK",
    "VAD_INT_PIN_DAI1_LRCLK",
    "VAD_INT_PIN_DAI1_RXDAT",
    "VAD_INT_PIN_DAI1_TXDAT",
    "VAD_INT_PIN_DAI2_BCLK",
    "VAD_INT_PIN_DAI2_LRCLK",
    "VAD_INT_PIN_DAI2_RXDAT",
    "VAD_INT_PIN_DAI2_TXDAT",
    "VAD_INT_PIN_DAI3_BCLK",
    "VAD_INT_PIN_DAI3_LRCLK",
    "VAD_INT_PIN_DAI3_RXDAT",
    "VAD_INT_PIN_DAI3_TXDAT",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_int_pin, enum_vad_int_pin);


static const char* const enum_vad_keyword[] = {
    "VAD_KEYWORD_KEY_0",
    "VAD_KEYWORD_KEY_1",
    "VAD_KEYWORD_KEY_2",
    "VAD_KEYWORD_KEY_3",
    "VAD_KEYWORD_KEY_4",
    "VAD_KEYWORD_KEY_5",
    "VAD_KEYWORD_KEY_6",
    "VAD_KEYWORD_KEY_7",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_keyword, enum_vad_keyword);


static const char* const enum_vad_key_group_sel[] = {
    "VAD_KEY_GROUP_SEL_GROUP_0",
    "VAD_KEY_GROUP_SEL_GROUP_1",
    "VAD_KEY_GROUP_SEL_GROUP_2",
    "VAD_KEY_GROUP_SEL_GROUP_3",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_key_group_sel, enum_vad_key_group_sel);


static const char* const enum_vad_voice_enhance[] = {
    "VAD_VOICE_ENHANCE_DISABLE",
    "VAD_VOICE_ENHANCE_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_voice_enhance, enum_vad_voice_enhance);


static const char* const enum_vad_voice_enroll[] = {
    "VAD_VOICE_ENROLL_DISABLE",
    "VAD_VOICE_ENROLL_ENROLL",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_voice_enroll, enum_vad_voice_enroll);


static const char* const enum_vad_enroll_cnt[] = {
    "VAD_ENROLL_CNT_CNT_0",
    "VAD_ENROLL_CNT_CNT_1",
    "VAD_ENROLL_CNT_CNT_2",
    "VAD_ENROLL_CNT_CNT_3",
    "VAD_ENROLL_CNT_CNT_4",
    "VAD_ENROLL_CNT_ENROLL_DONE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_enroll_cnt, enum_vad_enroll_cnt);


static const char* const enum_vad_enroll_apply[] = {
    "VAD_ENROLL_APPLY_DISABLE",
    "VAD_ENROLL_APPLY_APLLY",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_enroll_apply, enum_vad_enroll_apply);


static const char* const enum_vad_enroll_rst[] = {
    "VAD_ENROLL_RST_DISABLE",
    "VAD_ENROLL_RST_ENROLL_RST",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_enroll_rst, enum_vad_enroll_rst);


static const char* const enum_vad_keyword_hit_clear[] = {
    "VAD_KEYWORD_HIT_CLEAR_NOP",
    "VAD_KEYWORD_HIT_CLEAR_ENABLE",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_vad_keyword_hit_clear, enum_vad_keyword_hit_clear);

//////////////////////////////////////////////////////////////////////////
static const char* const enum_scenarious_sel[] = {
    "SCENARIOUS_SEL_STANDBY",
    "SCENARIOUS_SEL_HANDSET_0",
    "SCENARIOUS_SEL_HANDSET_1",
    "SCENARIOUS_SEL_HANDSET_2",
    "SCENARIOUS_SEL_HANDFREE_0",
    "SCENARIOUS_SEL_HANDFREE_1",
    "SCENARIOUS_SEL_HANDFREE_2",
    "SCENARIOUS_SEL_HEADSET_0",
    "SCENARIOUS_SEL_HEADSET_1",
    "SCENARIOUS_SEL_HEADSET_2",
    "SCENARIOUS_SEL_VOICE_REC_0",
    "SCENARIOUS_SEL_VOICE_REC_1",
    "SCENARIOUS_SEL_VOICE_REC_2",
    "SCENARIOUS_SEL_VIDEO_REC_0",
    "SCENARIOUS_SEL_VIDEO_REC_1",
    "SCENARIOUS_SEL_VIDEO_REC_2",
    "SCENARIOUS_SEL_ASR_0",
    "SCENARIOUS_SEL_ASR_1",
    "SCENARIOUS_SEL_ASR_2",
    "SCENARIOUS_SEL_BYPASS_0",
    "SCENARIOUS_SEL_BYPASS_1",
    "SCENARIOUS_SEL_BYPASS_2",
    "SCENARIOUS_SEL_RESERVED_0",
    "SCENARIOUS_SEL_RESERVED_1",
    "SCENARIOUS_SEL_RESERVED_2",
};
static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_scenarious_sel, enum_scenarious_sel);

static int igo_ch_power_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_POWER_MODE_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "POWER_MODE", IGO_CH_POWER_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_power_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_slow_write(codec->dev, IGO_CH_POWER_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "POWER_MODE", IGO_CH_POWER_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_fw_ver_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_FW_VER_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "FW_VER", IGO_CH_FW_VER_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_fw_ver_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_fw_sub_ver_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_FW_SUB_VER_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "FW_SUB_VER", IGO_CH_FW_SUB_VER_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_fw_sub_ver_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_chip_id_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_CHIP_ID_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "CHIP_ID", IGO_CH_CHIP_ID_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_chip_id_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_op_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_OP_MODE_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "OP_MODE", IGO_CH_OP_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_op_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_OP_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "OP_MODE", IGO_CH_OP_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_mclk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_mclk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_MCLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "MCLK", IGO_CH_MCLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ck_output_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ck_output_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CK_OUTPUT_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CK_OUTPUT", IGO_CH_CK_OUTPUT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch0_rx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch0_rx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH0_RX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH0_RX", IGO_CH_CH0_RX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch0_tx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch0_tx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH0_TX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH0_TX", IGO_CH_CH0_TX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch0_tx_gain_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch0_tx_gain_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH0_TX_GAIN_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH0_TX_GAIN", IGO_CH_CH0_TX_GAIN_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_rx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_rx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_RX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_RX", IGO_CH_CH1_RX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_tx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_tx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_TX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_TX", IGO_CH_CH1_TX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_tx_gain_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_tx_gain_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_TX_GAIN_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_TX_GAIN", IGO_CH_CH1_TX_GAIN_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_0_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_0_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_0_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_0_MODE", IGO_CH_DAI_0_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_0_clk_src_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_0_clk_src_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_0_CLK_SRC_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_0_CLK_SRC", IGO_CH_DAI_0_CLK_SRC_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_0_clk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_0_clk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_0_CLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_0_CLK", IGO_CH_DAI_0_CLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_0_data_bit_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_0_data_bit_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_0_DATA_BIT_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_0_DATA_BIT", IGO_CH_DAI_0_DATA_BIT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_1_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_1_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_1_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_1_MODE", IGO_CH_DAI_1_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_1_clk_src_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_1_clk_src_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_1_CLK_SRC_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_1_CLK_SRC", IGO_CH_DAI_1_CLK_SRC_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_1_clk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_1_clk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_1_CLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_1_CLK", IGO_CH_DAI_1_CLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_1_data_bit_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_1_data_bit_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_1_DATA_BIT_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_1_DATA_BIT", IGO_CH_DAI_1_DATA_BIT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_2_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_2_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_2_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_2_MODE", IGO_CH_DAI_2_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_2_clk_src_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_2_clk_src_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_2_CLK_SRC_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_2_CLK_SRC", IGO_CH_DAI_2_CLK_SRC_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_2_clk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_2_clk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_2_CLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_2_CLK", IGO_CH_DAI_2_CLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_2_data_bit_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_2_data_bit_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_2_DATA_BIT_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_2_DATA_BIT", IGO_CH_DAI_2_DATA_BIT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_3_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_3_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_3_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_3_MODE", IGO_CH_DAI_3_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_3_clk_src_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_3_clk_src_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_3_CLK_SRC_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_3_CLK_SRC", IGO_CH_DAI_3_CLK_SRC_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_3_clk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_3_clk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_3_CLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_3_CLK", IGO_CH_DAI_3_CLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dai_3_data_bit_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dai_3_data_bit_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DAI_3_DATA_BIT_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DAI_3_DATA_BIT", IGO_CH_DAI_3_DATA_BIT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m_clk_src_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m_clk_src_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M_CLK_SRC_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M_CLK_SRC", IGO_CH_DMIC_M_CLK_SRC_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m_bclk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m_bclk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M_BCLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M_BCLK", IGO_CH_DMIC_M_BCLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_s_bclk_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_s_bclk_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_S_BCLK_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_S_BCLK", IGO_CH_DMIC_S_BCLK_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m0_p_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m0_p_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M0_P_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M0_P_MODE", IGO_CH_DMIC_M0_P_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m0_n_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m0_n_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M0_N_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M0_N_MODE", IGO_CH_DMIC_M0_N_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m1_p_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m1_p_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M1_P_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M1_P_MODE", IGO_CH_DMIC_M1_P_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m1_n_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m1_n_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M1_N_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M1_N_MODE", IGO_CH_DMIC_M1_N_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m2_p_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m2_p_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M2_P_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M2_P_MODE", IGO_CH_DMIC_M2_P_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m2_n_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m2_n_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M2_N_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M2_N_MODE", IGO_CH_DMIC_M2_N_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m3_p_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m3_p_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M3_P_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M3_P_MODE", IGO_CH_DMIC_M3_P_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_m3_n_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_m3_n_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_M3_N_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_M3_N_MODE", IGO_CH_DMIC_M3_N_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_s0_p_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_s0_p_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_S0_P_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_S0_P_MODE", IGO_CH_DMIC_S0_P_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_s0_n_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_s0_n_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_S0_N_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_S0_N_MODE", IGO_CH_DMIC_S0_N_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_s1_p_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_s1_p_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_S1_P_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_S1_P_MODE", IGO_CH_DMIC_S1_P_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_dmic_s1_n_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_dmic_s1_n_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_DMIC_S1_N_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "DMIC_S1_N_MODE", IGO_CH_DMIC_S1_N_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_nr_ch0_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_nr_ch0_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_NR_CH0_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "NR_CH0", IGO_CH_NR_CH0_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_nr_ch1_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_nr_ch1_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_NR_CH1_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "NR_CH1", IGO_CH_NR_CH1_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_ref_mode_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_ref_mode_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_REF_MODE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_REF_MODE", IGO_CH_CH1_REF_MODE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_ref_rx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_ref_rx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_REF_RX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_REF_RX", IGO_CH_CH1_REF_RX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch0_floor_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch0_floor_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH0_FLOOR_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH0_FLOOR", IGO_CH_CH0_FLOOR_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_floor_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_floor_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_FLOOR_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_FLOOR", IGO_CH_CH1_FLOOR_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch0_od_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch0_od_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH0_OD_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH0_OD", IGO_CH_CH0_OD_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_od_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_od_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_OD_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_OD", IGO_CH_CH1_OD_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch0_thr_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch0_thr_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH0_THR_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH0_THR", IGO_CH_CH0_THR_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_ch1_thr_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_ch1_thr_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_CH1_THR_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "CH1_THR", IGO_CH_CH1_THR_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_hw_bypass_dai_0_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_hw_bypass_dai_0_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_HW_BYPASS_DAI_0_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "HW_BYPASS_DAI_0", IGO_CH_HW_BYPASS_DAI_0_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_hw_bypass_dmic_s0_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_hw_bypass_dmic_s0_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_HW_BYPASS_DMIC_S0_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "HW_BYPASS_DMIC_S0", IGO_CH_HW_BYPASS_DMIC_S0_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_sw_bypass_0_rx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_sw_bypass_0_rx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_SW_BYPASS_0_RX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "SW_BYPASS_0_RX", IGO_CH_SW_BYPASS_0_RX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_sw_bypass_0_tx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_sw_bypass_0_tx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_SW_BYPASS_0_TX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "SW_BYPASS_0_TX", IGO_CH_SW_BYPASS_0_TX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_sw_bypass_1_rx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_sw_bypass_1_rx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_SW_BYPASS_1_RX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "SW_BYPASS_1_RX", IGO_CH_SW_BYPASS_1_RX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_sw_bypass_1_tx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_sw_bypass_1_tx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_SW_BYPASS_1_TX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "SW_BYPASS_1_TX", IGO_CH_SW_BYPASS_1_TX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_aec_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_aec_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_AEC_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "AEC", IGO_CH_AEC_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_aec_ref_rx_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_aec_ref_rx_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_AEC_REF_RX_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "AEC_REF_RX", IGO_CH_AEC_REF_RX_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_status_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_STATUS_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_STATUS", IGO_CH_VAD_STATUS_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_status_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_vad_clear_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_clear_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_CLEAR_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_CLEAR", IGO_CH_VAD_CLEAR_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_int_mod_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_int_mod_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_INT_MOD_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_INT_MOD", IGO_CH_VAD_INT_MOD_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_int_pin_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_int_pin_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_INT_PIN_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_INT_PIN", IGO_CH_VAD_INT_PIN_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_keyword_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_KEYWORD_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_KEYWORD", IGO_CH_VAD_KEYWORD_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_keyword_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_vad_key_group_sel_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_key_group_sel_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_KEY_GROUP_SEL_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_KEY_GROUP_SEL", IGO_CH_VAD_KEY_GROUP_SEL_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_voice_enhance_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_voice_enhance_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_VOICE_ENHANCE_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_VOICE_ENHANCE", IGO_CH_VAD_VOICE_ENHANCE_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_voice_enroll_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_voice_enroll_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_VOICE_ENROLL_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_VOICE_ENROLL", IGO_CH_VAD_VOICE_ENROLL_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_enroll_cnt_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_ENROLL_CNT_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_ENROLL_CNT", IGO_CH_VAD_ENROLL_CNT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_enroll_cnt_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_vad_enroll_apply_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_ENROLL_APPLY_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_ENROLL_APPLY", IGO_CH_VAD_ENROLL_APPLY_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_enroll_apply_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_ENROLL_APPLY_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_ENROLL_APPLY", IGO_CH_VAD_ENROLL_APPLY_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_enroll_md_sz_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_ENROLL_MD_SZ_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_ENROLL_MD_SZ", IGO_CH_VAD_ENROLL_MD_SZ_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_enroll_md_sz_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_vad_enroll_md_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
#if 0
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_ENROLL_MD_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_ENROLL_MD", IGO_CH_VAD_ENROLL_MD_ADDR, (int)ucontrol->value.integer.value[0], status);
#endif

    return 0;
}

static int igo_ch_vad_enroll_md_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
#if 0
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_ENROLL_MD_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_ENROLL_MD", IGO_CH_VAD_ENROLL_MD_ADDR, (int)ucontrol->value.integer.value[0], status);
#endif

    return 0;
}

static int igo_ch_vad_enroll_rst_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_enroll_rst_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_ENROLL_RST_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_ENROLL_RST", IGO_CH_VAD_ENROLL_RST_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_keyword_hit_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);
    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_read(codec->dev, IGO_CH_VAD_KEYWORD_HIT_ADDR, (unsigned int*)&ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: read %s (0x%08x) : %d ret %d\n", __func__, "VAD_KEYWORD_HIT", IGO_CH_VAD_KEYWORD_HIT_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static int igo_ch_vad_keyword_hit_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    return 0;
}

static int igo_ch_vad_keyword_hit_clear_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = 0;

    return 0;
}

static int igo_ch_vad_keyword_hit_clear_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;

    struct debussy_priv* debussy;
    debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);
    status = igo_ch_write(codec->dev, IGO_CH_VAD_KEYWORD_HIT_CLEAR_ADDR, ucontrol->value.integer.value[0]);
    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "VAD_KEYWORD_HIT_CLEAR", IGO_CH_VAD_KEYWORD_HIT_CLEAR_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

/////////////////////////////////////////////////////////////////////////
static int igo_ch_scenarious_sel_get(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    ucontrol->value.integer.value[0] = SCENARIOUS_SEL_STANDBY;

    return 0;
}

static int igo_ch_scenarious_sel_put(struct snd_kcontrol* kcontrol,
    struct snd_ctl_elem_value* ucontrol)
{
    struct snd_soc_codec* codec = snd_soc_kcontrol_codec(kcontrol);
    int status = IGO_CH_STATUS_DONE;
    int index;
    struct ST_IGO_DEBUSSY_CFG *pst_scenarious_tab;
    struct debussy_priv* debussy = dev_get_drvdata(codec->dev);

    mutex_lock(&debussy->igo_ch_lock);

    pst_scenarious_tab = (struct ST_IGO_DEBUSSY_CFG *) scenarious_mode_table[ucontrol->value.integer.value[0]];

    index = 0;
    while (1) {
        if ((IGO_CH_WAIT_ADDR == (pst_scenarious_tab + index)->cmd_addr) && (0xFFFFFFFF == (pst_scenarious_tab + index)->cmd_data)) {
            break;
        }

        if ((IGO_CH_WAIT_ADDR == (pst_scenarious_tab + index)->cmd_addr) && (0xFFFFFFFF > (pst_scenarious_tab + index)->cmd_data)) {
            msleep((pst_scenarious_tab + index)->cmd_data);
        }
        else {
            status = igo_ch_write(codec->dev, (unsigned int) (pst_scenarious_tab + index)->cmd_addr, (unsigned int) (pst_scenarious_tab + index)->cmd_data);
        }

        index++;
    }

    mutex_unlock(&debussy->igo_ch_lock);
    dev_info(codec->dev, "%s: write %s (0x%08x) :%d ret %d\n", __func__, "SCENARIOUS_SEL", IGO_CH_SCENARIOUS_SEL_ADDR, (int)ucontrol->value.integer.value[0], status);

    return 0;
}

static const struct snd_kcontrol_new debussy_snd_controls[] = {
    SOC_ENUM_EXT("IGO POWER_MODE", soc_enum_power_mode,
        igo_ch_power_mode_get, igo_ch_power_mode_put),
    SOC_SINGLE_EXT("IGO FW_VER", IGO_CH_FW_VER_ADDR,
        IGO_CH_SHIFT, IGO_CH_MAX, IGO_CH_NO_INVERT,
        igo_ch_fw_ver_get, igo_ch_fw_ver_put),
    SOC_SINGLE_EXT("IGO FW_SUB_VER", IGO_CH_FW_SUB_VER_ADDR,
        IGO_CH_SHIFT, IGO_CH_MAX, IGO_CH_NO_INVERT,
        igo_ch_fw_sub_ver_get, igo_ch_fw_sub_ver_put),
    SOC_SINGLE_EXT("IGO CHIP_ID", IGO_CH_CHIP_ID_ADDR,
        IGO_CH_SHIFT, IGO_CH_MAX, IGO_CH_NO_INVERT,
        igo_ch_chip_id_get, igo_ch_chip_id_put),
    SOC_ENUM_EXT("IGO OP_MODE", soc_enum_op_mode,
        igo_ch_op_mode_get, igo_ch_op_mode_put),
    SOC_ENUM_EXT("IGO MCLK", soc_enum_mclk,
        igo_ch_mclk_get, igo_ch_mclk_put),
    SOC_ENUM_EXT("IGO CK_OUTPUT", soc_enum_ck_output,
        igo_ch_ck_output_get, igo_ch_ck_output_put),
    SOC_ENUM_EXT("IGO CH0_RX", soc_enum_ch0_rx,
        igo_ch_ch0_rx_get, igo_ch_ch0_rx_put),
    SOC_ENUM_EXT("IGO CH0_TX", soc_enum_ch0_tx,
        igo_ch_ch0_tx_get, igo_ch_ch0_tx_put),
    SOC_ENUM_EXT("IGO CH0_TX_GAIN", soc_enum_ch0_tx_gain,
        igo_ch_ch0_tx_gain_get, igo_ch_ch0_tx_gain_put),
    SOC_ENUM_EXT("IGO CH1_RX", soc_enum_ch1_rx,
        igo_ch_ch1_rx_get, igo_ch_ch1_rx_put),
    SOC_ENUM_EXT("IGO CH1_TX", soc_enum_ch1_tx,
        igo_ch_ch1_tx_get, igo_ch_ch1_tx_put),
    SOC_ENUM_EXT("IGO CH1_TX_GAIN", soc_enum_ch1_tx_gain,
        igo_ch_ch1_tx_gain_get, igo_ch_ch1_tx_gain_put),
    SOC_ENUM_EXT("IGO DAI_0_MODE", soc_enum_dai_0_mode,
        igo_ch_dai_0_mode_get, igo_ch_dai_0_mode_put),
    SOC_ENUM_EXT("IGO DAI_0_CLK_SRC", soc_enum_dai_0_clk_src,
        igo_ch_dai_0_clk_src_get, igo_ch_dai_0_clk_src_put),
    SOC_ENUM_EXT("IGO DAI_0_CLK", soc_enum_dai_0_clk,
        igo_ch_dai_0_clk_get, igo_ch_dai_0_clk_put),
    SOC_ENUM_EXT("IGO DAI_0_DATA_BIT", soc_enum_dai_0_data_bit,
        igo_ch_dai_0_data_bit_get, igo_ch_dai_0_data_bit_put),
    SOC_ENUM_EXT("IGO DAI_1_MODE", soc_enum_dai_1_mode,
        igo_ch_dai_1_mode_get, igo_ch_dai_1_mode_put),
    SOC_ENUM_EXT("IGO DAI_1_CLK_SRC", soc_enum_dai_1_clk_src,
        igo_ch_dai_1_clk_src_get, igo_ch_dai_1_clk_src_put),
    SOC_ENUM_EXT("IGO DAI_1_CLK", soc_enum_dai_1_clk,
        igo_ch_dai_1_clk_get, igo_ch_dai_1_clk_put),
    SOC_ENUM_EXT("IGO DAI_1_DATA_BIT", soc_enum_dai_1_data_bit,
        igo_ch_dai_1_data_bit_get, igo_ch_dai_1_data_bit_put),
    SOC_ENUM_EXT("IGO DAI_2_MODE", soc_enum_dai_2_mode,
        igo_ch_dai_2_mode_get, igo_ch_dai_2_mode_put),
    SOC_ENUM_EXT("IGO DAI_2_CLK_SRC", soc_enum_dai_2_clk_src,
        igo_ch_dai_2_clk_src_get, igo_ch_dai_2_clk_src_put),
    SOC_ENUM_EXT("IGO DAI_2_CLK", soc_enum_dai_2_clk,
        igo_ch_dai_2_clk_get, igo_ch_dai_2_clk_put),
    SOC_ENUM_EXT("IGO DAI_2_DATA_BIT", soc_enum_dai_2_data_bit,
        igo_ch_dai_2_data_bit_get, igo_ch_dai_2_data_bit_put),
    SOC_ENUM_EXT("IGO DAI_3_MODE", soc_enum_dai_3_mode,
        igo_ch_dai_3_mode_get, igo_ch_dai_3_mode_put),
    SOC_ENUM_EXT("IGO DAI_3_CLK_SRC", soc_enum_dai_3_clk_src,
        igo_ch_dai_3_clk_src_get, igo_ch_dai_3_clk_src_put),
    SOC_ENUM_EXT("IGO DAI_3_CLK", soc_enum_dai_3_clk,
        igo_ch_dai_3_clk_get, igo_ch_dai_3_clk_put),
    SOC_ENUM_EXT("IGO DAI_3_DATA_BIT", soc_enum_dai_3_data_bit,
        igo_ch_dai_3_data_bit_get, igo_ch_dai_3_data_bit_put),
    SOC_ENUM_EXT("IGO DMIC_M_CLK_SRC", soc_enum_dmic_m_clk_src,
        igo_ch_dmic_m_clk_src_get, igo_ch_dmic_m_clk_src_put),
    SOC_ENUM_EXT("IGO DMIC_M_BCLK", soc_enum_dmic_m_bclk,
        igo_ch_dmic_m_bclk_get, igo_ch_dmic_m_bclk_put),
    SOC_ENUM_EXT("IGO DMIC_S_BCLK", soc_enum_dmic_s_bclk,
        igo_ch_dmic_s_bclk_get, igo_ch_dmic_s_bclk_put),
    SOC_ENUM_EXT("IGO DMIC_M0_P_MODE", soc_enum_dmic_m0_p_mode,
        igo_ch_dmic_m0_p_mode_get, igo_ch_dmic_m0_p_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M0_N_MODE", soc_enum_dmic_m0_n_mode,
        igo_ch_dmic_m0_n_mode_get, igo_ch_dmic_m0_n_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M1_P_MODE", soc_enum_dmic_m1_p_mode,
        igo_ch_dmic_m1_p_mode_get, igo_ch_dmic_m1_p_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M1_N_MODE", soc_enum_dmic_m1_n_mode,
        igo_ch_dmic_m1_n_mode_get, igo_ch_dmic_m1_n_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M2_P_MODE", soc_enum_dmic_m2_p_mode,
        igo_ch_dmic_m2_p_mode_get, igo_ch_dmic_m2_p_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M2_N_MODE", soc_enum_dmic_m2_n_mode,
        igo_ch_dmic_m2_n_mode_get, igo_ch_dmic_m2_n_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M3_P_MODE", soc_enum_dmic_m3_p_mode,
        igo_ch_dmic_m3_p_mode_get, igo_ch_dmic_m3_p_mode_put),
    SOC_ENUM_EXT("IGO DMIC_M3_N_MODE", soc_enum_dmic_m3_n_mode,
        igo_ch_dmic_m3_n_mode_get, igo_ch_dmic_m3_n_mode_put),
    SOC_ENUM_EXT("IGO DMIC_S0_P_MODE", soc_enum_dmic_s0_p_mode,
        igo_ch_dmic_s0_p_mode_get, igo_ch_dmic_s0_p_mode_put),
    SOC_ENUM_EXT("IGO DMIC_S0_N_MODE", soc_enum_dmic_s0_n_mode,
        igo_ch_dmic_s0_n_mode_get, igo_ch_dmic_s0_n_mode_put),
    SOC_ENUM_EXT("IGO DMIC_S1_P_MODE", soc_enum_dmic_s1_p_mode,
        igo_ch_dmic_s1_p_mode_get, igo_ch_dmic_s1_p_mode_put),
    SOC_ENUM_EXT("IGO DMIC_S1_N_MODE", soc_enum_dmic_s1_n_mode,
        igo_ch_dmic_s1_n_mode_get, igo_ch_dmic_s1_n_mode_put),
    SOC_ENUM_EXT("IGO NR_CH0", soc_enum_nr_ch0,
        igo_ch_nr_ch0_get, igo_ch_nr_ch0_put),
    SOC_ENUM_EXT("IGO NR_CH1", soc_enum_nr_ch1,
        igo_ch_nr_ch1_get, igo_ch_nr_ch1_put),
    SOC_ENUM_EXT("IGO CH1_REF_MODE", soc_enum_ch1_ref_mode,
        igo_ch_ch1_ref_mode_get, igo_ch_ch1_ref_mode_put),
    SOC_ENUM_EXT("IGO CH1_REF_RX", soc_enum_ch1_ref_rx,
        igo_ch_ch1_ref_rx_get, igo_ch_ch1_ref_rx_put),
    SOC_ENUM_EXT("IGO CH0_FLOOR", soc_enum_ch0_floor,
        igo_ch_ch0_floor_get, igo_ch_ch0_floor_put),
    SOC_ENUM_EXT("IGO CH1_FLOOR", soc_enum_ch1_floor,
        igo_ch_ch1_floor_get, igo_ch_ch1_floor_put),
    SOC_ENUM_EXT("IGO CH0_OD", soc_enum_ch0_od,
        igo_ch_ch0_od_get, igo_ch_ch0_od_put),
    SOC_ENUM_EXT("IGO CH1_OD", soc_enum_ch1_od,
        igo_ch_ch1_od_get, igo_ch_ch1_od_put),
    SOC_ENUM_EXT("IGO CH0_THR", soc_enum_ch0_thr,
        igo_ch_ch0_thr_get, igo_ch_ch0_thr_put),
    SOC_ENUM_EXT("IGO CH1_THR", soc_enum_ch1_thr,
        igo_ch_ch1_thr_get, igo_ch_ch1_thr_put),
    SOC_ENUM_EXT("IGO HW_BYPASS_DAI_0", soc_enum_hw_bypass_dai_0,
        igo_ch_hw_bypass_dai_0_get, igo_ch_hw_bypass_dai_0_put),
    SOC_ENUM_EXT("IGO HW_BYPASS_DMIC_S0", soc_enum_hw_bypass_dmic_s0,
        igo_ch_hw_bypass_dmic_s0_get, igo_ch_hw_bypass_dmic_s0_put),
    SOC_ENUM_EXT("IGO SW_BYPASS_0_RX", soc_enum_sw_bypass_0_rx,
        igo_ch_sw_bypass_0_rx_get, igo_ch_sw_bypass_0_rx_put),
    SOC_ENUM_EXT("IGO SW_BYPASS_0_TX", soc_enum_sw_bypass_0_tx,
        igo_ch_sw_bypass_0_tx_get, igo_ch_sw_bypass_0_tx_put),
    SOC_ENUM_EXT("IGO SW_BYPASS_1_RX", soc_enum_sw_bypass_1_rx,
        igo_ch_sw_bypass_1_rx_get, igo_ch_sw_bypass_1_rx_put),
    SOC_ENUM_EXT("IGO SW_BYPASS_1_TX", soc_enum_sw_bypass_1_tx,
        igo_ch_sw_bypass_1_tx_get, igo_ch_sw_bypass_1_tx_put),
    SOC_ENUM_EXT("IGO AEC", soc_enum_aec,
        igo_ch_aec_get, igo_ch_aec_put),
    SOC_ENUM_EXT("IGO AEC_REF_RX", soc_enum_aec_ref_rx,
        igo_ch_aec_ref_rx_get, igo_ch_aec_ref_rx_put),
    SOC_ENUM_EXT("IGO VAD_STATUS", soc_enum_vad_status,
        igo_ch_vad_status_get, igo_ch_vad_status_put),
    SOC_ENUM_EXT("IGO VAD_CLEAR", soc_enum_vad_clear,
        igo_ch_vad_clear_get, igo_ch_vad_clear_put),
    SOC_ENUM_EXT("IGO VAD_INT_MOD", soc_enum_vad_int_mod,
        igo_ch_vad_int_mod_get, igo_ch_vad_int_mod_put),
    SOC_ENUM_EXT("IGO VAD_INT_PIN", soc_enum_vad_int_pin,
        igo_ch_vad_int_pin_get, igo_ch_vad_int_pin_put),
    SOC_ENUM_EXT("IGO VAD_KEYWORD", soc_enum_vad_keyword,
        igo_ch_vad_keyword_get, igo_ch_vad_keyword_put),
    SOC_ENUM_EXT("IGO VAD_KEY_GROUP_SEL", soc_enum_vad_key_group_sel,
        igo_ch_vad_key_group_sel_get, igo_ch_vad_key_group_sel_put),
    SOC_ENUM_EXT("IGO VAD_VOICE_ENHANCE", soc_enum_vad_voice_enhance,
        igo_ch_vad_voice_enhance_get, igo_ch_vad_voice_enhance_put),
    SOC_ENUM_EXT("IGO VAD_VOICE_ENROLL", soc_enum_vad_voice_enroll,
        igo_ch_vad_voice_enroll_get, igo_ch_vad_voice_enroll_put),
    SOC_ENUM_EXT("IGO VAD_ENROLL_CNT", soc_enum_vad_enroll_cnt,
        igo_ch_vad_enroll_cnt_get, igo_ch_vad_enroll_cnt_put),
    SOC_ENUM_EXT("IGO VAD_ENROLL_APPLY", soc_enum_vad_enroll_apply,
        igo_ch_vad_enroll_apply_get, igo_ch_vad_enroll_apply_put),
    SOC_SINGLE_EXT("IGO VAD_ENROLL_MD_SZ", IGO_CH_VAD_ENROLL_MD_SZ_ADDR,
        IGO_CH_SHIFT, IGO_CH_MAX, IGO_CH_NO_INVERT,
        igo_ch_vad_enroll_md_sz_get, igo_ch_vad_enroll_md_sz_put),
    SOC_SINGLE_EXT("IGO VAD_ENROLL_MD", IGO_CH_VAD_ENROLL_MD_ADDR,
        IGO_CH_SHIFT, IGO_CH_MAX, IGO_CH_NO_INVERT,
        igo_ch_vad_enroll_md_get, igo_ch_vad_enroll_md_put),
    SOC_ENUM_EXT("IGO VAD_ENROLL_RST", soc_enum_vad_enroll_rst,
        igo_ch_vad_enroll_rst_get, igo_ch_vad_enroll_rst_put),
    SOC_SINGLE_EXT("IGO VAD_KEYWORD_HIT", IGO_CH_VAD_KEYWORD_HIT_ADDR,
        IGO_CH_SHIFT, IGO_CH_MAX, IGO_CH_NO_INVERT,
        igo_ch_vad_keyword_hit_get, igo_ch_vad_keyword_hit_put),
    SOC_ENUM_EXT("IGO VAD_KEYWORD_HIT_CLEAR", soc_enum_vad_keyword_hit_clear,
        igo_ch_vad_keyword_hit_clear_get, igo_ch_vad_keyword_hit_clear_put),

//////////////////////////////////////////////////////////////////////////////////
    SOC_ENUM_EXT("IGO SCENARIOUS_SEL", soc_enum_scenarious_sel,
        igo_ch_scenarious_sel_get, igo_ch_scenarious_sel_put),
};

void debussy_add_codec_controls(struct snd_soc_codec* codec)
{
    snd_soc_add_codec_controls(codec,
        debussy_snd_controls,
        ARRAY_SIZE(debussy_snd_controls));
}