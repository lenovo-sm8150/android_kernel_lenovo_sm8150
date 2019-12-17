#ifndef DEBUSSY_SCENARIOUS_H
#define DEBUSSY_SCENARIOUS_H

#include "debussy_snd_ctrl.h"

typedef struct ST_IGO_DEBUSSY_CFG {
    uint32_t    cmd_addr;
    uint32_t    cmd_data;
} st_igo_scenarious_cfg_table;

#ifdef DEBUSSY_SCENARIOUS_C

#define POWER_MODE_DELAY        (40)

struct ST_IGO_DEBUSSY_CFG dummy_cfg[] = {
    {IGO_CH_WAIT_ADDR, 0xFFFFFFFF}
};

struct ST_IGO_DEBUSSY_CFG standby_cfg[] = {
    {IGO_CH_POWER_MODE_ADDR, POWER_MODE_STANDBY},
    {IGO_CH_WAIT_ADDR, POWER_MODE_DELAY},
    {IGO_CH_WAIT_ADDR, 0xFFFFFFFF}
};

struct ST_IGO_DEBUSSY_CFG handset_nr_aec_cfg[] = {
    // keep at first
    {IGO_CH_POWER_MODE_ADDR, POWER_MODE_WORKING},
    {IGO_CH_WAIT_ADDR, POWER_MODE_DELAY},

    {IGO_CH_MCLK_ADDR, MCLK_19_2M},
    {IGO_CH_DMIC_M_CLK_SRC_ADDR, DMIC_M_CLK_SRC_MCLK},
    {IGO_CH_DMIC_M0_P_MODE_ADDR, DMIC_M0_P_MODE_ENABLE},
    {IGO_CH_DMIC_M_BCLK_ADDR, DMIC_M_BCLK_ADAPTIVE},

    {IGO_CH_DAI_0_CLK_ADDR, DAI_0_CLK_48K},
    {IGO_CH_DAI_0_MODE_ADDR, DAI_0_MODE_SLAVE},
    {IGO_CH_DAI_0_DATA_BIT_ADDR, DAI_0_DATA_BIT_32},

    {IGO_CH_CH1_RX_ADDR, CH1_RX_DMIC_M0_P},
    {IGO_CH_CH1_TX_ADDR, CH1_TX_DAI0_TX_L},

    {IGO_CH_CH1_FLOOR_ADDR, CH1_FLOOR_LVL_2},
    {IGO_CH_CH1_OD_ADDR, CH1_OD_LVL_4},
    {IGO_CH_CH1_OD_ADDR, CH1_THR_LVL_7},

    {IGO_CH_NR_CH1_ADDR, NR_CH1_ENABLE},

    {IGO_CH_AEC_ADDR, AEC_ENABLE},
    {IGO_CH_AEC_REF_RX_ADDR, AEC_REF_RX_DAI0_RX_L},
    {IGO_CH_CH0_RX_ADDR, CH0_RX_DAI0_RX_L},

    // keep at last
    {IGO_CH_OP_MODE_ADDR, OP_MODE_NR},
    {IGO_CH_WAIT_ADDR, 0xFFFFFFFF}
};

struct ST_IGO_DEBUSSY_CFG handfree_nr_aec_cfg[] = {
    // keep at first
    {IGO_CH_POWER_MODE_ADDR, POWER_MODE_WORKING},
    {IGO_CH_WAIT_ADDR, POWER_MODE_DELAY},

    {IGO_CH_MCLK_ADDR, MCLK_19_2M},
    {IGO_CH_DMIC_M_CLK_SRC_ADDR, DMIC_M_CLK_SRC_MCLK},
    {IGO_CH_DMIC_M1_P_MODE_ADDR, DMIC_M1_P_MODE_ENABLE},
    {IGO_CH_DMIC_M_BCLK_ADDR, DMIC_M_BCLK_ADAPTIVE},

    {IGO_CH_DAI_0_CLK_ADDR, DAI_0_CLK_48K},
    {IGO_CH_DAI_0_MODE_ADDR, DAI_0_MODE_SLAVE},
    {IGO_CH_DAI_0_DATA_BIT_ADDR, DAI_0_DATA_BIT_32},

    {IGO_CH_CH1_RX_ADDR, CH1_RX_DMIC_M1_P},
    {IGO_CH_CH1_TX_ADDR, CH1_TX_DAI0_TX_L},

    {IGO_CH_CH1_FLOOR_ADDR, CH1_FLOOR_LVL_2},
    {IGO_CH_CH1_OD_ADDR, CH1_OD_LVL_4},
    {IGO_CH_CH1_OD_ADDR, CH1_THR_LVL_7},

    {IGO_CH_NR_CH1_ADDR, NR_CH1_ENABLE},

    {IGO_CH_AEC_ADDR, AEC_ENABLE},
    {IGO_CH_AEC_REF_RX_ADDR, AEC_REF_RX_DAI0_RX_L},
    {IGO_CH_CH0_RX_ADDR, CH0_RX_DAI0_RX_L},

    // keep at last
    {IGO_CH_OP_MODE_ADDR, OP_MODE_NR},
    {IGO_CH_WAIT_ADDR, 0xFFFFFFFF}
};

struct ST_IGO_DEBUSSY_CFG headset_nr_aec_cfg[] = {
    // keep at first
    {IGO_CH_POWER_MODE_ADDR, POWER_MODE_WORKING},
    {IGO_CH_WAIT_ADDR, POWER_MODE_DELAY},

    {IGO_CH_DAI_0_CLK_ADDR, DAI_0_CLK_48K},
    {IGO_CH_DAI_0_MODE_ADDR, DAI_0_MODE_SLAVE},
    {IGO_CH_DAI_0_DATA_BIT_ADDR, DAI_0_DATA_BIT_32},

    {IGO_CH_CH1_RX_ADDR, CH1_RX_DAI0_RX_L},
    {IGO_CH_CH1_TX_ADDR, CH1_TX_DAI0_TX_L},

    {IGO_CH_CH1_FLOOR_ADDR, CH1_FLOOR_LVL_2},
    {IGO_CH_CH1_OD_ADDR, CH1_OD_LVL_4},
    {IGO_CH_CH1_OD_ADDR, CH1_THR_LVL_7},

    {IGO_CH_NR_CH1_ADDR, NR_CH1_ENABLE},

    // keep at last
    {IGO_CH_OP_MODE_ADDR, OP_MODE_NR},
    {IGO_CH_WAIT_ADDR, 0xFFFFFFFF}
};

// The table size is the same as the maximum ENUM of scenarious_sel.
struct ST_IGO_DEBUSSY_CFG *scenarious_mode_table[] = {
    &standby_cfg[0],                    // STANDBY_MODE
    // SCENARIOUS_SEL_HANDSET
    &handset_nr_aec_cfg[0],
    &handset_nr_aec_cfg[0],
    &handset_nr_aec_cfg[0],
    // SCENARIOUS_SEL_HANDFREE
    &handfree_nr_aec_cfg[0],
    &handfree_nr_aec_cfg[0],
    &handfree_nr_aec_cfg[0],
    // SCENARIOUS_SEL_HEADSET
    &headset_nr_aec_cfg[0],
    &headset_nr_aec_cfg[0],
    &headset_nr_aec_cfg[0],
    // SCENARIOUS_SEL_VOICE_REC
    &dummy_cfg[0],
    &dummy_cfg[0],
    &dummy_cfg[0],
    // SCENARIOUS_SEL_VIDEO_REC
    &dummy_cfg[0],
    &dummy_cfg[0],
    &dummy_cfg[0],
    // SCENARIOUS_SEL_ASR
    &dummy_cfg[0],
    &dummy_cfg[0],
    &dummy_cfg[0],
    // SCENARIOUS_SEL_BYPASS
    &dummy_cfg[0],
    &dummy_cfg[0],
    &dummy_cfg[0],
    // SCENARIOUS_SEL_RESERVED
    &dummy_cfg[0],
    &dummy_cfg[0],
    &dummy_cfg[0],
};
#else

extern struct ST_IGO_DEBUSSY_CFG *scenarious_mode_table[];

#endif

#endif
