/*
 * drivers/mmc/host/sdhci-msm-bh201.c -Bayhub Technologies, Inc. BH201 SDHCI
 * bridge IC for MSM SDHCI platform driver source file
 *
 * Copyright (c) 2012-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "sdhci-msm-bh201.h"

#define TRUE	1
#define FALSE	0

#define TUNING_PHASE_SIZE  (11)

#define DbgErr pr_err
#define PrintMsg  pr_err
#define DbgInfo(m, f, r, _x_,...)
#define os_memcpy memcpy
#define os_memset memset

extern const u32 tuning_block_64[16];
extern const u32 tuning_block_128[32];

typedef struct t_gg_reg_strt {
	u32 ofs;
	u32 mask;
	u32 value;
} t_gg_reg_strt, *p_gg_reg_strt;

typedef struct rl_bit_lct {
	u8 bits;
	u8 rl_bits;
} rl_bit_lct;

static rl_bit_lct cfg_bit_map[6] = {
	{0, 6},
	{1, 5},
	{2, 4},
	{3, 2},
	{4, 1},
	{5, 0}
};

// read
t_gg_reg_strt pha_stas_rx_low32 = { 14, 0xffffffff, 0 };
t_gg_reg_strt pha_stas_rx_high32 = { 46, 0xffffffff, 0 };
t_gg_reg_strt pha_stas_tx_low32 = { 205, 0xffffffff, 0 };
t_gg_reg_strt pha_stas_tx_high32 = { 237, 0xffffffff, 0 };
t_gg_reg_strt dll_sela_after_mask = { 141, 0xf, 0 };
t_gg_reg_strt dll_selb_after_mask = { 145, 0xf, 0 };

t_gg_reg_strt lp3p3v_ocb = { 4, 0x1, 0 };
t_gg_reg_strt analog_ip_read_volatge_mask = { 154, 0xf, 0 };
t_gg_reg_strt dll_delay_100m_backup = { 83, 0xfff, 0 };
t_gg_reg_strt dll_delay_200m_backup = { 95, 0xfff, 0 };
t_gg_reg_strt ssc_bypass = { 171, 1, 0 };
t_gg_reg_strt ssc_enable = { 191, 1, 0 };
t_gg_reg_strt sd_freq_cur = { 151, 0x7, 0 };

// write
t_gg_reg_strt dll_sela_100m_cfg = { 126, 0xf, 0 };
t_gg_reg_strt dll_sela_200m_cfg = { 130, 0xf, 0 };
t_gg_reg_strt dll_selb_100m_cfg = { 140, 0xf, 0 };
t_gg_reg_strt dll_selb_200m_cfg = { 144, 0xf, 0 };

t_gg_reg_strt dll_selb_100m_cfg_en = { 183, 0x1, 0 };
t_gg_reg_strt dll_selb_200m_cfg_en = { 184, 0x1, 0 };
t_gg_reg_strt internl_tuning_en_100m = { 171, 0x1, 0 };
t_gg_reg_strt internl_tuning_en_200m = { 172, 0x1, 0 };
t_gg_reg_strt force_dll_lock = { 63, 0x1, 0 };
t_gg_reg_strt analog_ip_write_volatge_mask = { 167, 0xf, 0 };

t_gg_reg_strt dll_phase_adjust_en = { 105, 0x1, 0 };
t_gg_reg_strt dll_delay_config_en = { 64, 0x1, 0 };
t_gg_reg_strt dll_delay_config_wr = { 106, 0x1, 0 };
t_gg_reg_strt dll_delay_100m_cfg = { 81, 0xfff, 0 };
t_gg_reg_strt dll_delay_200m_cfg = { 93, 0xfff, 0 };

//inject
t_gg_reg_strt inject_failure_for_tuning_enable_cfg = { 357, 0x1, 0 };
t_gg_reg_strt inject_failure_for_200m_tuning_cfg = { 93, 0x7ff, 0 };
t_gg_reg_strt inject_failure_for_100m_tuning_cfg = { 81, 0x7ff, 0 };

t_gg_reg_strt cclk_ds_18 = { 3, 0x7, 0 };

#define MAX_CFG_BIT_VAL (383)
#define BIT_VALID_NUM_INBYTE 6
static void cfg_bit_2_bt(int max_bit, int tar, int *byt, int *bit)
{
	*byt = (max_bit - tar) / 6;
	*bit = cfg_bit_map[(max_bit - tar) % 6].rl_bits;
}

static u32 cfg_read_bits_ofs_mask(u8 * cfg, t_gg_reg_strt * bts)
{
	u32 rv = 0;
	u32 msk = bts->mask;
	int byt = 0, bit = 0;
	int i = 0;
	do {
		cfg_bit_2_bt(MAX_CFG_BIT_VAL, bts->ofs + i, &byt, &bit);
		if (cfg[byt] & (1 << bit)) {
			rv |= 1 << i;
		}
		//
		i++;
		msk >>= 1;
		if (0 == msk)
			break;

	}
	while (1);
	return rv;
}

static void cfg_write_bits_ofs_mask(u8 * cfg, t_gg_reg_strt * bts, u32 w_value)
{
	u32 wv = w_value & bts->mask;
	u32 msk = bts->mask;
	int byt = 0, bit = 0;
	int i = 0;
	do {
		cfg_bit_2_bt(MAX_CFG_BIT_VAL, bts->ofs + i, &byt, &bit);
		if (wv & 1) {
			cfg[byt] |= (u8) (1 << bit);
		} else {
			cfg[byt] &= (u8) (~(1 << bit));
		}
		//
		i++;
		wv >>= 1;
		msk >>= 1;
		if (0 == msk)
			break;

	}
	while (1);
}

void ram_bit_2_bt(int tar, int *byt, int *bit)
{
	*byt = tar / 8;
	*bit = tar % 8;
}

static u32 read_ram_bits_ofs_mask(u8 * cfg, t_gg_reg_strt * bts)
{
	u32 rv = 0;
	u32 msk = bts->mask;
	int byt = 0, bit = 0;
	int i = 0;
	do {
		ram_bit_2_bt(bts->ofs + i, &byt, &bit);
		if (cfg[byt] & (1 << bit)) {
			rv |= 1 << i;
		}
		//
		i++;
		msk >>= 1;
		if (0 == msk)
			break;

	}
	while (1);
	return rv;
}

int card_deselect_card(struct sdhci_host *host)
{
	int ret = -1;
	int err;
	struct mmc_host *mmc = host->mmc;
	struct mmc_command cmd = { 0 };
	BUG_ON(!host);
	cmd.opcode = MMC_SELECT_CARD;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_NONE | MMC_CMD_AC;

	err = mmc_wait_for_cmd(mmc, &cmd, 3);
	if (err) {
		pr_err("---- CMD7 FAILE0 ----\n");
	} else {
		ret = 0;
	}
	return ret;
}

#define GG_ENTER_CMD7_DE_TIMES 2
#define GG_EXIT_CMD7_DE_TIMES 1
bool enter_exit_emulator_mode(struct sdhci_host * host, bool b_enter)
{
	bool ret = FALSE;
	u8 times = b_enter ? GG_ENTER_CMD7_DE_TIMES : GG_EXIT_CMD7_DE_TIMES;
	u8 i = 0;

	for (i = 0; i < times; i++) {

		ret = card_deselect_card(host);
		if (ret) {
			break;
		}
	}
	return ret;
}

static bool _gg_emulator_read_only(struct sdhci_host *host,
				   u8 * in_data, u32 datalen)
{
	struct mmc_host *mmc = host->mmc;
	int rc = 0;
	u8 *data1 = kzalloc(PAGE_SIZE, GFP_KERNEL);
	struct mmc_request mrq = { 0 };
	struct mmc_command cmd = { 0 };
	struct mmc_data data = { 0 };
	struct scatterlist sg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;

	if (!data1) {
		PrintMsg("gg read no memory\n");
		rc = -ENOMEM;
		goto out;
	}

	sg_init_one(&sg, data1, 512);

	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	cmd.sw_cmd_timeout = 150;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	data.blksz = 512;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.timeout_ns = 1000 * 1000 * 1000;	/* 1 sec */
	data.sg = &sg;
	data.sg_len = 1;
	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = NULL;

	mmc_wait_for_req(mmc, &mrq);
	memcpy(in_data, data1, datalen);
	kfree(data1);

	if (cmd.error || data.error) {

		if (cmd.error) {
			if (cmd.err_int_mask & 0xa0000) {
				//pr_err("sdr50_notuning_crc_error_flag=1\n");
				msm_host->sdr50_notuning_crc_error_flag = 1;
			}
		}

		if (data.error) {
			if (data.err_int_mask & 0x200000) {
				//pr_err("sdr50_notuning_crc_error_flag=1\n");
				msm_host->sdr50_notuning_crc_error_flag = 1;
			}
		}
		rc = -1;
		goto out;
	}

out:
	return rc;
}

void host_cmddat_line_reset(struct sdhci_host *host)
{
	if (host->ops->reset) {
		host->ops->reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
	}
}

static int gg_select_card_spec(struct sdhci_host *host)
{
	int err;
	struct mmc_command cmd = { 0 };
	struct mmc_card *card = host->mmc->card;

	cmd.opcode = MMC_SELECT_CARD;
	cmd.sw_cmd_timeout = 150;

	if (card) {
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

	} else {
		cmd.arg = 0;
		cmd.flags = MMC_RSP_NONE | MMC_CMD_AC;
	}

	err = mmc_wait_for_cmd(host->mmc, &cmd, 0);
	if (err) {

		if (-EILSEQ == err && (0x20000 == (0x30000 & cmd.err_int_mask))) {
			host_cmddat_line_reset(host);
			{
				struct mmc_command cmd = { 0 };
				cmd.opcode = 5;
				cmd.arg = 0;
				cmd.flags =
				    MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR;

				mmc_wait_for_cmd(host->mmc, &cmd, 0);
			}
			host_cmddat_line_reset(host);
			return 0;
		}
		if (-ETIMEDOUT == err && (0x10000 & cmd.err_int_mask)) {

			host_cmddat_line_reset(host);
			return err;
		}
		return 0;
	}

	return 0;
}

void dump_u32_buf(u8 * ptb, int len)
{
	int i = 0;
	u32 *tb = (u32 *) ptb;
	for (i = 0; i < len / 4; i++) {
		PrintMsg(" [%d]:%08xh\n", i, tb[i]);
	}
}

bool gg_emulator_read(struct sdhci_host *host, u8 * data, u32 datalen)
{
	bool ret = FALSE;
	bool rd_ret = FALSE;

	// twice CMD7 deselect
	ret = enter_exit_emulator_mode(host, TRUE);
	if (ret)
		goto exit;

	// CMD17

	rd_ret = _gg_emulator_read_only(host, data, datalen);

	// CMD7 deselect
	ret = enter_exit_emulator_mode(host, FALSE);
	if (ret)
		goto exit;

	// CMD7 select

	ret = gg_select_card_spec(host);

exit:
	if (rd_ret) {
		DbgErr("GGC read status error\n");
	}

	if (ret)
		DbgErr("GGC Emulator exit Fail!!\n");

	if (0 == rd_ret && ret) {
		DbgErr("data read ok, but exit NG\n");
		ret = 0;
	}

	if (rd_ret && 0 == ret) {
		DbgErr("data read NG, but exit ok\n");
		ret = -1;
	}

	return ret ? FALSE : TRUE;
}

static bool _ggc_emulator_write_only(struct sdhci_host *host,
				     u8 * in_data, u32 datalen)
{
	struct mmc_host *mmc = host->mmc;
	int rc = 0;
	u8 *data1 = kzalloc(PAGE_SIZE, GFP_KERNEL);
	struct mmc_request mrq = { 0 };
	struct mmc_command cmd = { 0 };
	struct mmc_data data = { 0 };
	struct scatterlist sg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;

	if (!data1) {
		PrintMsg("gg write no memory\n");
		rc = -ENOMEM;
		goto out;
	}

	memcpy(data1, in_data, datalen);
	sg_init_one(&sg, data1, 512);

	cmd.opcode = MMC_WRITE_BLOCK;
	cmd.sw_cmd_timeout = 150;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	data.blksz = 512;
	data.blocks = 1;
	data.flags = MMC_DATA_WRITE;
	data.timeout_ns = 1000 * 1000 * 1000;	/* 1 sec */
	data.sg = &sg;
	data.sg_len = 1;
	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = NULL;

	mmc_wait_for_req(mmc, &mrq);

	if (cmd.error) {
		if (cmd.err_int_mask & 0xa0000) {

			msm_host->sdr50_notuning_crc_error_flag = 1;
		}
	}
	kfree(data1);
out:
	return rc;
}

bool gg_emulator_write(struct sdhci_host * host, u8 * data, u32 datalen)
{
	bool ret = FALSE;
	bool wr_ret = FALSE;

	// twice CMD7 deselect
	ret = enter_exit_emulator_mode(host, TRUE);
	if (ret)
		goto exit;

	// CMD24 ignore CMD timeout
	_ggc_emulator_write_only(host, data, datalen);
	wr_ret = TRUE;		//force ignore
	// CMD7 deselect
	ret = enter_exit_emulator_mode(host, FALSE);
	if (ret)
		goto exit;

	ret = gg_select_card_spec(host);

exit:
	if (wr_ret)
		ret = FALSE;

	if (ret)
		DbgErr("GGC Emulator Write Fail!!\n");

	return ret ? FALSE : TRUE;
}

#define MAX_GG_REG_NUM 16
u32 gg_sw_def[MAX_GG_REG_NUM] = GGC_CFG_DATA;

void get_gg_reg_def(struct sdhci_host *host, u8 * data)
{
	os_memcpy(data, (u8 *) & (gg_sw_def[0]), sizeof(gg_sw_def));
}

static u32 g_gg_reg_cur[16] = { 0 };
static u32 g_gg_reg_tmp[16] = { 0 };

void set_gg_reg_tmp(u8 * data)
{
	os_memcpy(&g_gg_reg_tmp[0], data, sizeof(g_gg_reg_tmp));
}

void set_gg_reg_cur_val(u8 * data)
{
	os_memcpy(&g_gg_reg_cur[0], data, sizeof(g_gg_reg_cur));
}

void get_gg_reg_cur_val(u8 * data)
{
	os_memcpy(data, &g_gg_reg_cur[0], sizeof(g_gg_reg_cur));
}

void bht_gg_status(struct mmc_host *mmc_host)
{
	struct sdhci_host *host = mmc_priv(mmc_host);
	u32 data[128] = { 0 };
	int i = 0;
	os_memset(data, 0, 512);
	gg_emulator_read(host, (u8 *) data, 512);
	for (i = 0; i < 40; i++)
		PrintMsg("[%d]:%x\n", i, data[i]);

}

EXPORT_SYMBOL(bht_gg_status);
bool get_gg_reg_cur(struct sdhci_host *host, u8 * data,
		    t_gg_reg_strt * gg_reg_arr, u8 num)
{
	u8 get_idx = 0;
	bool ret = FALSE;

	// read ggc register
	os_memset(data, 0, 512);
	ret = gg_emulator_read(host, data, 512);

	if (ret == FALSE)
		goto exit;

	// read the offset bits value
	for (get_idx = 0; get_idx < num; get_idx++) {
		(gg_reg_arr + get_idx)->value =
		    read_ram_bits_ofs_mask(data, (gg_reg_arr + get_idx));
	}
exit:
	return ret;
}

bool chg_gg_reg(struct sdhci_host * host, u8 * data, t_gg_reg_strt * gg_reg_arr,
		u8 num)
{
	u8 chg_idx = 0;
	os_memset(data, 0, 512);
	get_gg_reg_def(host, data);

	for (chg_idx = 0; chg_idx < num; chg_idx++) {
		// modify the ggc register bit value
		cfg_write_bits_ofs_mask(data, (gg_reg_arr + chg_idx),
					(gg_reg_arr + chg_idx)->value);
	}

	// write ggc register
	return gg_emulator_write(host, data, 512);
}

void chg_gg_reg_cur_val(u8 * data, t_gg_reg_strt * gg_reg_arr, u8 num,
			bool b_sav_chg)
{
	u8 chg_idx = 0;

	for (chg_idx = 0; chg_idx < num; chg_idx++) {
		// modify the ggc register bit value
		cfg_write_bits_ofs_mask(data, (gg_reg_arr + chg_idx),
					(gg_reg_arr + chg_idx)->value);
	}

	if (b_sav_chg)
		set_gg_reg_cur_val(data);
}

void generate_gg_reg_val(u8 * data, t_gg_reg_strt * gg_reg_arr, u8 num)
{
	u8 chg_idx = 0;

	for (chg_idx = 0; chg_idx < num; chg_idx++) {
		// modify the ggc register bit value
		cfg_write_bits_ofs_mask(data, (gg_reg_arr + chg_idx),
					(gg_reg_arr + chg_idx)->value);
	}
}

void chg_gg_reg_tmp(u8 * data, t_gg_reg_strt * gg_reg_arr, u8 num)
{
	u8 chg_idx = 0;
	for (chg_idx = 0; chg_idx < num; chg_idx++) {
		cfg_write_bits_ofs_mask(data, (gg_reg_arr + chg_idx),
					(gg_reg_arr + chg_idx)->value);
	}
	set_gg_reg_tmp(data);
}

bool b_output_tuning;

bool is_output_tuning(void)
{
	return b_output_tuning;
}

void log_bin(u32 n)
{
	int i = 0;
	u8 tb[33] = { 0 };
	for (i = 0; i < 32; i++) {
		if (n & (1 << i)) {
			tb[i] = '1';
		} else
			tb[i] = '0';
	}
	PrintMsg("bin:%s\n", tb);
}

void phase_str(u8 * tb, u32 n)
{
	int i = 0;

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (n & (1 << i)) {
			tb[i] = '1';
		} else
			tb[i] = '0';
	}
	tb[TUNING_PHASE_SIZE] = 0;

}

int get_bit_number(u32 n)
{
	int i = 0;
	int cnt = 0;
	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (n & (1 << i)) {
			cnt++;
		}
	}
	return cnt;
}

bool ggc_set_output_tuning_phase(struct sdhci_host * host, int sela, int selb)
{
	bool ret = TRUE;
	u8 data[512] = { 0 };

	t_gg_reg_strt gg_reg_arr[8];

	DbgInfo(MODULE_SD_HOST, FEATURE_SDREG_TRACEW, NOT_TO_RAM,
		"%s sela:%xh,selb:%xh\n", __FUNCTION__, sela, selb);

	get_gg_reg_cur_val(data);
	os_memcpy(&gg_reg_arr[0], &dll_sela_100m_cfg, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[1], &dll_sela_200m_cfg, sizeof(t_gg_reg_strt));
	gg_reg_arr[0].value = sela;
	gg_reg_arr[1].value = sela;
	os_memcpy(&gg_reg_arr[2], &dll_selb_100m_cfg, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[3], &dll_selb_200m_cfg, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[4], &dll_selb_100m_cfg_en, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[5], &dll_selb_200m_cfg_en, sizeof(t_gg_reg_strt));
	gg_reg_arr[2].value = selb;
	gg_reg_arr[3].value = selb;
	gg_reg_arr[4].value = 1;
	gg_reg_arr[5].value = 1;
	os_memcpy(&gg_reg_arr[6], &internl_tuning_en_100m,
		  sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[7], &internl_tuning_en_200m,
		  sizeof(t_gg_reg_strt));
	gg_reg_arr[6].value = 1;
	gg_reg_arr[7].value = 1;
	chg_gg_reg_cur_val(data, gg_reg_arr, 8, TRUE);
	ret = gg_emulator_write(host, data, 512);
	return ret;
}

bool gg_fix_output_tuning_phase(struct sdhci_host * host, int sela, int selb)
{
	u8 data[512] = { 0 };
	t_gg_reg_strt gg_reg_arr[10];

	PrintMsg("### %s - sela dll: %x, selb dll: %x \n", __FUNCTION__, sela,
		 selb);

	get_gg_reg_cur_val(data);

	os_memcpy(&gg_reg_arr[0], &dll_sela_100m_cfg, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[1], &dll_sela_200m_cfg, sizeof(t_gg_reg_strt));
	gg_reg_arr[0].value = sela;
	gg_reg_arr[1].value = sela;
	os_memcpy(&gg_reg_arr[2], &dll_selb_100m_cfg, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[3], &dll_selb_200m_cfg, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[4], &dll_selb_100m_cfg_en, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[5], &dll_selb_200m_cfg_en, sizeof(t_gg_reg_strt));
	gg_reg_arr[2].value = selb;
	gg_reg_arr[3].value = selb;
	gg_reg_arr[4].value = 1;
	gg_reg_arr[5].value = 1;
	os_memcpy(&gg_reg_arr[6], &internl_tuning_en_100m,
		  sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[7], &internl_tuning_en_200m,
		  sizeof(t_gg_reg_strt));
	gg_reg_arr[6].value = 0;
	gg_reg_arr[7].value = 0;

	chg_gg_reg_cur_val(data, gg_reg_arr, 8, TRUE);

	return gg_emulator_write(host, data, 512);
}

// generate 11 bit data
void gen_array_data(u32 low32, u32 high32, u32 * ptw)
{

#define MAX_TUNING_RESULT_COMBO_SIZE (6)
	u8 tu_res_per[MAX_TUNING_RESULT_COMBO_SIZE][TUNING_PHASE_SIZE];
	u8 i = 0, j = 0;
	u8 i_mode = 0;
	u32 tw = 0;

	os_memset(tu_res_per, 1, sizeof(tu_res_per));
	for (i = 0; i < 64; i++) {
		u32 tmp_data = (i < 32) ? low32 : high32;
		tu_res_per[i / TUNING_PHASE_SIZE][i % TUNING_PHASE_SIZE] =
		    (tmp_data & (1 << (i % 32))) >> (i % 32);
	}
//
	for (i_mode = 0; i_mode < TUNING_PHASE_SIZE; i_mode++) {
		for (j = 0; j < MAX_TUNING_RESULT_COMBO_SIZE; j++) {
			if (tu_res_per[j][i_mode] == 0) {
				tw &= ~(1 << i_mode);
				break;
			} else
				tw |= (1 << i_mode);
		}
	}
	if (ptw)
		*ptw = tw;
}

bool sw_calc_tuning_result(struct sdhci_host *host, u32 * tx_selb,
			   u32 * all_selb)
{
	bool ret = FALSE;
	u8 data[512] = { 0 };
	u32 selb_status_tx_low32 = 0, selb_status_tx_high32 = 0;
	u32 selb_status_ggc_low32 = 0, selb_status_ggc_high32 = 0;
	t_gg_reg_strt gg_reg_arr[6];

	os_memcpy(&gg_reg_arr[0], &pha_stas_tx_low32, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[1], &pha_stas_tx_high32, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[2], &pha_stas_rx_low32, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[3], &pha_stas_rx_high32, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[4], &dll_sela_after_mask, sizeof(t_gg_reg_strt));
	os_memcpy(&gg_reg_arr[5], &dll_selb_after_mask, sizeof(t_gg_reg_strt));

	ret = get_gg_reg_cur(host, data, gg_reg_arr, 6);

	if ((TRUE == ret)) {
		//
		selb_status_tx_low32 = gg_reg_arr[0].value;
		PrintMsg("[205-236]:\n");
		log_bin(selb_status_tx_low32);
		PrintMsg("[237-268]:\n");
		selb_status_tx_high32 = gg_reg_arr[1].value;
		log_bin(selb_status_tx_high32);

		PrintMsg("[14-45]:\n");
		selb_status_ggc_low32 = gg_reg_arr[2].value;
		log_bin(selb_status_ggc_low32);
		PrintMsg("[46-77]:\n");
		selb_status_ggc_high32 = gg_reg_arr[3].value;
		log_bin(selb_status_ggc_high32);
		PrintMsg("dll  sela after mask=%xh", gg_reg_arr[4].value);
		PrintMsg("dll  selb after mask=%xh", gg_reg_arr[5].value);

		if (tx_selb) {
			gen_array_data(gg_reg_arr[0].value, gg_reg_arr[1].value,
				       tx_selb);
			PrintMsg("tx_selb:%xh\n", *tx_selb);
		}
		if (all_selb) {
			gen_array_data(gg_reg_arr[2].value, gg_reg_arr[3].value,
				       all_selb);
			PrintMsg("all_selb:%xh\n", *all_selb);
		}
	}

	return ret;
}

bool gg_tuning_result(struct sdhci_host * host, u32 * tx_selb, u32 * all_selb)
{

	host_cmddat_line_reset(host);
	return sw_calc_tuning_result(host, tx_selb, all_selb);
}

/** bit 1means pass, 0 means failed**/
#define BIT_PASS_MASK 0x7ff
u32 g_tx_selb_failed_history_sdr104 = BIT_PASS_MASK;
u32 g_tx_selb_failed_history_sdr50 = BIT_PASS_MASK;

bool is_bus_mode_sdr104(struct sdhci_host * host)
{
	bool ret = FALSE;
	if (host->timing == MMC_TIMING_UHS_SDR104)
		ret = TRUE;

	return ret;
}

void tx_selb_failed_history_update(struct sdhci_host *host, u32 val)
{
	if (is_bus_mode_sdr104(host)) {
		g_tx_selb_failed_history_sdr104 &= val;
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh %xh\n", __FUNCTION__, val,
			g_tx_selb_failed_history_sdr104);
	} else {
		g_tx_selb_failed_history_sdr50 &= val;
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh %xh\n", __FUNCTION__, val,
			g_tx_selb_failed_history_sdr50);
	}
}

void tx_selb_failed_history_reset(struct sdhci_host *host)
{
	g_tx_selb_failed_history_sdr104 = BIT_PASS_MASK;
	g_tx_selb_failed_history_sdr50 = BIT_PASS_MASK;
}

u32 tx_selb_failed_history_get(struct sdhci_host *host)
{

	if (is_bus_mode_sdr104(host)) {

		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh\n", __FUNCTION__,
			g_tx_selb_failed_history_sdr104);

		return g_tx_selb_failed_history_sdr104;
	} else {

		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh\n", __FUNCTION__,
			g_tx_selb_failed_history_sdr50);
		return g_tx_selb_failed_history_sdr50;
	}

}

u32 g_tx_selb_tb_sdr104[TUNING_PHASE_SIZE] = {
	BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK
};

u32 g_tx_selb_tb_sdr50[TUNING_PHASE_SIZE] = {
	BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK
};

void tx_selb_failed_tb_update(struct sdhci_host *host, int sela, u32 val)
{
	if (is_bus_mode_sdr104(host)) {
		g_tx_selb_tb_sdr104[sela] &= val;
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh %xh\n", __FUNCTION__, val,
			g_tx_selb_tb_sdr104[sela]);
	} else {
		g_tx_selb_tb_sdr50[sela] &= val;
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh %xh\n", __FUNCTION__, val,
			g_tx_selb_tb_sdr50[sela]);
	}

}

void tx_selb_failed_tb_reset(struct sdhci_host *host)
{
	os_memset(g_tx_selb_tb_sdr104, 0xff, sizeof(g_tx_selb_tb_sdr104));
	os_memset(g_tx_selb_tb_sdr50, 0xff, sizeof(g_tx_selb_tb_sdr50));
}

u32 tx_selb_failed_tb_get(struct sdhci_host *host, int sela)
{
	if (is_bus_mode_sdr104(host)) {
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh\n", __FUNCTION__, g_tx_selb_tb_sdr104[sela]);
		return g_tx_selb_tb_sdr104[sela];
	} else {
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh\n", __FUNCTION__, g_tx_selb_tb_sdr50[sela]);
		return g_tx_selb_tb_sdr50[sela];
	}
}

u32 g_all_selb_tb_sdr104[TUNING_PHASE_SIZE] = {
	BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK
};

u32 g_all_selb_tb_sdr50[TUNING_PHASE_SIZE] = {
	BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK, BIT_PASS_MASK, BIT_PASS_MASK,
	BIT_PASS_MASK
};

void all_selb_failed_history_update(struct sdhci_host *host, int sela, u32 val)
{
	if (is_bus_mode_sdr104(host)) {
		g_all_selb_tb_sdr104[sela] &= val;
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh %xh\n", __FUNCTION__, val,
			g_all_selb_tb_sdr104[sela]);
	} else {
		g_all_selb_tb_sdr50[sela] &= val;
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh %xh\n", __FUNCTION__, val,
			g_all_selb_tb_sdr50[sela]);
	}

}

void all_selb_failed_history_reset(struct sdhci_host *host)
{
	os_memset(g_all_selb_tb_sdr104, 0xff, sizeof(g_all_selb_tb_sdr104));
	os_memset(g_all_selb_tb_sdr50, 0xff, sizeof(g_all_selb_tb_sdr50));
}

u32 all_selb_failed_history_get(struct sdhci_host *host, int sela)
{
	if (is_bus_mode_sdr104(host)) {
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh\n", __FUNCTION__, g_all_selb_tb_sdr104[sela]);

		return g_all_selb_tb_sdr104[sela];
	} else {
		DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM,
			"%s %xh\n", __FUNCTION__, g_all_selb_tb_sdr50[sela]);

		return g_all_selb_tb_sdr50[sela];
	}
}

void chk_phase_window(u8 * tuning_win, u8 * mid_val, u8 * max_pass_win)	// type[0] = 0: if 011110, right 1 index is the middle value, type[1] = 1: first_i valid
{
	u8 tuning_pass[TUNING_PHASE_SIZE + 32];
	u8 tuning_pass_start[TUNING_PHASE_SIZE + 32];
	u8 tuning_pass_num_max = 0;
	u8 first_0 = 0;
	u8 i = 0, j = 0;
	u8 i_mode = 0, selb_mode = 0;

	os_memset(tuning_pass, 1, sizeof(tuning_pass));
	os_memset(tuning_pass_start, 1, sizeof(tuning_pass_start));

	{
		for (i = 0; i < TUNING_PHASE_SIZE; i++) {
			if (tuning_win[i] == 0) {
				first_0 = i;
				break;
			}
		}
	}

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		i_mode = (first_0 + i) % TUNING_PHASE_SIZE;
		if (tuning_win[i_mode] == 1)
			tuning_pass[j]++;
		else if (tuning_pass[j])
			j++;
		if (tuning_pass[j] == 1)
			tuning_pass_start[j] = i_mode;
	}

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (tuning_pass_num_max < tuning_pass[i]) {
			tuning_pass_num_max = tuning_pass[i];
			i_mode = i;
		}
	}

	if (tuning_pass_num_max == 0)
		DbgErr
		    ("###### Get max pass window fail, there is no any pass phase!!\n");
	else {
		*max_pass_win = tuning_pass_num_max - 1;
		tuning_pass_num_max /= 2;
		selb_mode = tuning_pass_start[i_mode] + tuning_pass_num_max;
		if ((*max_pass_win % 2 == 0))
			selb_mode += 1;
		selb_mode %= TUNING_PHASE_SIZE;
	}

	*mid_val = selb_mode;

	return;
}

void dump_array(u8 * tb)
{
	int i = 0;
	u8 str[12] = { 0 };

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		str[i] = tb[i] + '0';

	}
	DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM, "%s\n", str);
}

void bits_generate_array(u8 * tb, u32 v)
{
	int i = 0;
	DbgInfo(MODULE_SD_HOST, FEATURE_VENREG_TRACEW, NOT_TO_RAM, "%s %xh\n",
		__FUNCTION__, v);

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if ((v & (1 << i))) {
			tb[i] = 1;
		} else
			tb[i] = 0;
	}
	dump_array(tb);
}

typedef struct {
	u8 right_valid:1;
	u8 first_valid:1;
	u8 record_valid:1;
} chk_type_t;

// normal check continue 1
void chk_arr_max_win(u8 * tuning_win, u8 first_i, u8 * mid_val, u8 * max_pass_win, chk_type_t type)	// type[0] = 0: if 011110, right 1 index is the middle value, type[1] = 1: first_i valid
{
	u8 tuning_pass[TUNING_PHASE_SIZE];
	u8 tuning_pass_start[TUNING_PHASE_SIZE];
	u8 tuning_pass_num_max = 0;
	u8 first_0 = 0;
	u8 i = 0, j = 0;
	u8 i_mode = 0, selb_mode = 0;

	os_memset(tuning_pass, 1, sizeof(tuning_pass));
	os_memset(tuning_pass_start, 1, sizeof(tuning_pass_start));

	if (type.first_valid)
		first_0 = first_i;
	else {
		for (i = 0; i < TUNING_PHASE_SIZE; i++) {
			if (tuning_win[i] == 0) {
				first_0 = i;
				break;
			}
		}
	}

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		i_mode = (first_0 + i) % TUNING_PHASE_SIZE;
		if (tuning_win[i_mode] == 1)
			tuning_pass[j]++;
		else if (tuning_pass[j])
			j++;
		if (tuning_pass[j] == 1)
			tuning_pass_start[j] = i_mode;
	}

	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (tuning_pass_num_max < tuning_pass[i]) {
			tuning_pass_num_max = tuning_pass[i];
			i_mode = i;
		}
	}

	if (tuning_pass_num_max == 0)
		DbgErr
		    ("###### Get max pass window fail, there is no any pass phase!!\n");
	else {
		*max_pass_win = tuning_pass_num_max - 1;
		tuning_pass_num_max /= 2;
		selb_mode = tuning_pass_start[i_mode] + tuning_pass_num_max;
		if ((*max_pass_win % 2 == 0) && (type.right_valid)
		    )
			selb_mode += 1;
		selb_mode %= TUNING_PHASE_SIZE;
	}

	*mid_val = selb_mode;
	return;
}

// 0 no fail point
void no_fail_p(u8 * tuning_win, u8 * mid_val, u8 * max_pass_win)
{
	chk_type_t type;
	u8 first_0 = 0;

	os_memset((u8 *) & type, 0, sizeof(chk_type_t));

	type.first_valid = 0;
	type.right_valid = 1;
	type.record_valid = 0;

	chk_arr_max_win(tuning_win, first_0, mid_val, max_pass_win, type);

}

int sdhci_bht_sdr104_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	unsigned long flags;
	int tuning_seq_cnt = 3;
	u8 phase, *data_buf, tuned_phases[NUM_TUNING_PHASES], tuned_phase_cnt;
	const u32 *tuning_block_pattern = tuning_block_64;
	int size = sizeof(tuning_block_64);	/* Tuning pattern size in bytes */
	int rc;
	struct mmc_host *mmc = host->mmc;
	struct mmc_ios ios = host->mmc->ios;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;
	u8 drv_type = 0;
	bool drv_type_changed = false;
	struct mmc_card *card = host->mmc->card;
	/*
	 * Tuning is required for SDR104, HS200 and HS400 cards and
	 * if clock frequency is greater than 100MHz in these modes.
	 */
	if (host->clock <= CORE_FREQ_100MHZ ||
	    !((ios.timing == MMC_TIMING_MMC_HS400) ||
	      (ios.timing == MMC_TIMING_MMC_HS200) ||
	      (ios.timing == MMC_TIMING_UHS_SDR104))) {

		return 0;
	}

	pr_debug("%s: Enter %s\n", mmc_hostname(mmc), __func__);

	/* CDC/SDC4 DLL HW calibration is only required for HS400 mode */
	if (msm_host->tuning_done && !msm_host->calibration_done &&
	    (mmc->ios.timing == MMC_TIMING_MMC_HS400)) {
		rc = sdhci_msm_hs400_dll_calibration(host);
		spin_lock_irqsave(&host->lock, flags);
		if (!rc)
			msm_host->calibration_done = true;
		spin_unlock_irqrestore(&host->lock, flags);
		goto out;
	}

	spin_lock_irqsave(&host->lock, flags);

	if ((opcode == MMC_SEND_TUNING_BLOCK_HS200) &&
	    (mmc->ios.bus_width == MMC_BUS_WIDTH_8)) {
		tuning_block_pattern = tuning_block_128;
		size = sizeof(tuning_block_128);
	}
	spin_unlock_irqrestore(&host->lock, flags);

	data_buf = kmalloc(size, GFP_KERNEL);
	if (!data_buf) {
		rc = -ENOMEM;
		goto out;
	}

retry:
	tuned_phase_cnt = 0;

	/* first of all reset the tuning block */
	rc = msm_init_cm_dll(host);
	if (rc)
		goto kfree;

	phase = 0;
	do {
		struct mmc_command cmd = { 0 };
		struct mmc_data data = { 0 };
		struct mmc_request mrq = {
			.cmd = &cmd,
			.data = &data
		};
		struct scatterlist sg;
		rc = msm_config_cm_dll_phase(host, phase);
		if (rc)
			goto kfree;

		cmd.opcode = opcode;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

		data.blksz = size;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.timeout_ns = 30 * 1000 * 1000;	/* 30 ms */

		data.sg = &sg;
		data.sg_len = 1;
		sg_init_one(&sg, data_buf, size);
		memset(data_buf, 0, size);
		mmc_wait_for_req(mmc, &mrq);

		if (!cmd.error && !data.error &&
		    !memcmp(data_buf, tuning_block_pattern, size)) {
			/* tuning is successful at this tuning point */
			tuned_phases[tuned_phase_cnt++] = phase;
			pr_debug("%s: %s: found *** good *** phase = %d\n",
				 mmc_hostname(mmc), __func__, phase);
		} else {
			pr_debug("%s: %s: found ## bad ## phase = %d\n",
				 mmc_hostname(mmc), __func__, phase);
		}
	} while (++phase < 16);

	if ((tuned_phase_cnt == NUM_TUNING_PHASES) &&
	    card && mmc_card_mmc(card)) {
		/*
		 * If all phases pass then its a problem. So change the card's
		 * drive type to a different value, if supported and repeat
		 * tuning until at least one phase fails. Then set the original
		 * drive type back.
		 *
		 * If all the phases still pass after trying all possible
		 * drive types, then one of those 16 phases will be picked.
		 * This is no different from what was going on before the
		 * modification to change drive type and retune.
		 */
		pr_debug("%s: tuned phases count: %d\n", mmc_hostname(mmc),
			 tuned_phase_cnt);

		/* set drive type to other value . default setting is 0x0 */
		while (++drv_type <= MAX_DRV_TYPES_SUPPORTED_HS200) {
			pr_debug("%s: trying different drive strength (%d)\n",
				 mmc_hostname(mmc), drv_type);
			if (card->ext_csd.raw_driver_strength & (1 << drv_type)) {
				sdhci_msm_set_mmc_drv_type(host, opcode,
							   drv_type);
				if (!drv_type_changed)
					drv_type_changed = true;
				goto retry;
			}
		}
	}

	/* reset drive type to default (50 ohm) if changed */
	if (drv_type_changed)
		sdhci_msm_set_mmc_drv_type(host, opcode, 0);

	if (tuned_phase_cnt) {
		rc = msm_find_most_appropriate_phase(host, tuned_phases,
						     tuned_phase_cnt);
		if (rc < 0)
			goto kfree;
		else
			phase = (u8) rc;

		/*
		 * Finally set the selected phase in delay
		 * line hw block.
		 */
		rc = msm_config_cm_dll_phase(host, phase);
		if (rc)
			goto kfree;
		msm_host->saved_tuning_phase = phase;
		pr_debug("%s: %s: finally setting the tuning phase to %d\n",
			 mmc_hostname(mmc), __func__, phase);
	} else {
		if (--tuning_seq_cnt)
			goto retry;
		/* tuning failed */
		pr_err("%s: %s: no tuning point found\n",
		       mmc_hostname(mmc), __func__);
		rc = -EIO;
	}

kfree:
	kfree(data_buf);
out:
	spin_lock_irqsave(&host->lock, flags);
	if (!rc)
		msm_host->tuning_done = true;
	spin_unlock_irqrestore(&host->lock, flags);

	pr_debug("%s: Exit %s, err(%d)\n", mmc_hostname(mmc), __func__, rc);
	return rc;
}

int sdhci_bht_sdr50_execute_tuning(struct sdhci_host *host, u32 opcode)
{

	u8 phase, *data_buf;
	int size = sizeof(tuning_block_64);	/* Tuning pattern size in bytes */
	int rc = 0;
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;

	pr_debug("%s: Enter %s\n", mmc_hostname(mmc), __func__);

	data_buf = kmalloc(size, GFP_KERNEL);
	if (!data_buf) {
		PrintMsg("tuning no memory\n");
		rc = -ENOMEM;
		goto out;
	}

	phase = 0;
	do {
		struct mmc_command cmd = { 0 };
		struct mmc_data data = { 0 };
		struct mmc_request mrq = {
			.cmd = &cmd,
			.data = &data
		};
		struct scatterlist sg;

		cmd.opcode = opcode;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

		data.blksz = size;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.timeout_ns = 30 * 1000 * 1000;	/* 30ms */

		data.sg = &sg;
		data.sg_len = 1;
		sg_init_one(&sg, data_buf, size);
		memset(data_buf, 0, size);
		host_cmddat_line_reset(host);
		mmc_wait_for_req(mmc, &mrq);

		if (cmd.error) {
			if (cmd.err_int_mask & 0xa0000) {
				//pr_err("sdr50_notuning_crc_error_flag=1\n");
				msm_host->sdr50_notuning_crc_error_flag = 1;
			}
		}

		if (data.error) {
			if (data.err_int_mask & 0x200000) {
				//pr_err("sdr50_notuning_crc_error_flag=1\n");
				msm_host->sdr50_notuning_crc_error_flag = 1;
			}
		}

	} while (++phase < 16);

//kfree:
	kfree(data_buf);
out:

	return rc;
}

int sd_tuning_sw(struct sdhci_host *host)
{
	int ret = 0;
	if (is_bus_mode_sdr104(host)) {

		ret = sdhci_bht_sdr104_execute_tuning(host, 0x13);
	} else {
		ret = sdhci_bht_sdr50_execute_tuning(host, 0x13);
	}
	return !ret;
}

#define SD_TUNING_TIMEOUT_MS 150
bool sd_gg_tuning_status(struct sdhci_host * host,
			 u32 * tx_selb, u32 * all_selb, bool * status_ret)
{
	bool ret = FALSE;

	{

		ret = sd_tuning_sw(host);

		{
			if (status_ret) {
				*status_ret =
				    gg_tuning_result(host, tx_selb, all_selb);
			} else {
				gg_tuning_result(host, 0, 0);
			}
		}
	}

	DbgInfo(MODULE_SD_CARD, FEATURE_CARD_INIT, NOT_TO_RAM, "Exit(%d) %s\n",
		ret, __FUNCTION__);
	return ret;
}

int g_def_sela_100m = 0, g_def_sela_200m = 0;
int g_def_selb_100m = 0, g_def_selb_200m = 0;
u32 g_def_delaycode_100m = 0, g_def_delaycode_200m = 0;
u32 g_def_cclk_ds_18 = 0;
int get_config_sela_setting(struct sdhci_host *host)
{
	if (is_bus_mode_sdr104(host))
		return g_def_sela_200m;
	else
		return g_def_sela_100m;
}

int get_config_selb_setting(struct sdhci_host *host)
{
	if (is_bus_mode_sdr104(host))
		return g_def_selb_200m;
	else
		return g_def_selb_100m;
}

void get_defualt_setting(u8 * data)
{
	g_def_sela_100m = cfg_read_bits_ofs_mask(data, &dll_sela_100m_cfg);
	g_def_sela_200m = cfg_read_bits_ofs_mask(data, &dll_sela_200m_cfg);
	g_def_selb_100m = cfg_read_bits_ofs_mask(data, &dll_sela_100m_cfg);
	g_def_selb_200m = cfg_read_bits_ofs_mask(data, &dll_sela_200m_cfg);
	g_def_delaycode_100m =
	    cfg_read_bits_ofs_mask(data, &dll_delay_100m_backup);
	g_def_delaycode_200m =
	    cfg_read_bits_ofs_mask(data, &dll_delay_200m_backup);
	g_def_cclk_ds_18 = cfg_read_bits_ofs_mask(data, &cclk_ds_18);
}

u32 get_all_sela_status(struct sdhci_host *host, u32 target_selb)
{
	u32 all_sela = 0;
	int i = 0;
	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		u32 all_selb = all_selb_failed_history_get(host, i);

		if (all_selb & (1 << target_selb)) {
			all_sela |= 1 << i;
		}

	}
	return all_sela;
}

#define  GET_IDX_VALUE(tb, x)  (tb &(1 << x))
#define  GENERATE_IDX_VALUE( x)  (1 << x)
int get_pass_window_weight(u32 val)
{
	int i = 0;
	int cnt = 0;
	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (GET_IDX_VALUE(val, i)) {
			cnt++;
		}
	}
	return cnt;
}

int get_sela_nearby_pass_window(u32 sela, u32 base)
{

	int i = 0;
	int idx = base;
	int cnt = 0;

	if (0 == GET_IDX_VALUE(sela, idx))
		return 0;
//get first 0
	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (GET_IDX_VALUE(sela, idx)) {
			idx++;
			idx %= TUNING_PHASE_SIZE;
		} else {
			break;
		}
	}
//get first 1 idx
	if (0 == idx)
		idx = 0xa;
	else
		idx--;

//
	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		if (GET_IDX_VALUE(sela, idx)) {
			cnt++;
			if (0 == idx)
				idx = 0xa;
			else
				idx--;
		} else {
			break;
		}

	}
	return cnt;
}

int get_left_one_sel(int base)
{
	if (base == 0)
		return 0xa;
	else
		return base - 1;

}

int get_right_one_sel(int base)
{
	if (base == 0xa)
		return 0x0;
	else
		return base + 1;

}

int get_dif(int x, int y)
{
	int dif = 0;
	if (y > x) {
		dif = y - x;
	} else {
		dif = x - y;
	}
	return dif;
}

#define  NEARBY_THD  2
#define  NEARBY_2_THD  3
bool get_refine_sel(struct sdhci_host * host, u32 tga, u32 tgb, u32 * rfa,
		    u32 * rfb)
{
	int len_tb[TUNING_PHASE_SIZE] = { 0 };
	u32 sela = 0;
	int i = 0;
	u32 selb = 0;
	for (i = 0; i < TUNING_PHASE_SIZE; i++) {
		sela = get_all_sela_status(host, selb);
		len_tb[selb] = get_sela_nearby_pass_window(sela, tga);
		DbgInfo(MODULE_SD_HOST, FEATURE_CARD_INIT, NOT_TO_RAM,
			"%x len:%xh\n", selb, len_tb[selb]);
		selb++;
		selb %= TUNING_PHASE_SIZE;
	}
	PrintMsg("tgb:%xh, tgb len:%x\n", tgb, len_tb[tgb]);
	if (len_tb[tgb] < 6) {
		int lft1 = get_left_one_sel(tgb);
		int lft2 = get_left_one_sel(lft1);
		int rt1 = get_right_one_sel(tgb);
		int rt2 = get_right_one_sel(rt1);

		if (0 == len_tb[lft1] || 0 == len_tb[lft2]) {
			//left fail
			len_tb[lft1] = 0;
			len_tb[lft2] = 0;
			PrintMsg("over boundary case\n");
			goto next;
		}
		if (0 == len_tb[rt1] || 0 == len_tb[rt2]) {
			len_tb[rt1] = 0;
			len_tb[rt2] = 0;
			PrintMsg("over boundary case\n");
			goto next;
		}
		{
			int dif = get_dif(len_tb[lft1], len_tb[tgb]);
			if (dif > NEARBY_THD) {
				len_tb[lft1] = len_tb[tgb];
			}
			dif = get_dif(len_tb[rt1], len_tb[tgb]);
			if (dif > NEARBY_THD) {
				len_tb[rt1] = len_tb[tgb];
			}
			dif = get_dif(len_tb[lft2], len_tb[tgb]);
			if (dif > NEARBY_2_THD) {
				len_tb[lft2] = len_tb[tgb];
			}
			dif = get_dif(len_tb[rt2], len_tb[tgb]);
			if (dif > NEARBY_2_THD) {
				len_tb[rt2] = len_tb[tgb];
			}
		}
next:
		if ((len_tb[lft1] + len_tb[lft2]) >=
		    (len_tb[rt1] + len_tb[rt2])) {
			*rfb = lft2;
			*rfa = get_left_one_sel(tga);
		} else {
			*rfb = rt2;
			*rfa = get_right_one_sel(tga);
		}

		//check it exist
		sela = get_all_sela_status(host, *rfb);
		if (0 == GET_IDX_VALUE(sela, *rfa)) {
			PrintMsg
			    ("refine point is failed point, so no change\n");
			*rfa = tga;
			*rfb = tgb;
		}
	} else {
		*rfa = tga;
		*rfb = tgb;
	}
	PrintMsg("tg sela:%xh, selb:%x\n", tga, tgb);
	PrintMsg("rf sela:%xh, selb:%x\n", *rfa, *rfb);
	return TRUE;
}

#define OUTPUT_PASS_TYPE  0
#define SET_PHASE_FAIL_TYPE  1
#define TUNING_FAIL_TYPE 2
#define READ_STATUS_FAIL_TYPE 3

u8 *op_dbg_str[] = { "pass",
	"set_phase_fail",
	"tuning fail",
	"read status fail"
};

int update_selb(struct sdhci_host *host, int target_selb)
{

	return target_selb;
}

bool _ggc_output_tuning(struct sdhci_host * host, u8 * selb_pass_win)
{
#define DLL_SEL_RESULT_SIZE  16
	int cur_dll_sela = 0, dll_sela_cnt = 0;
	int dll_sela_basis = 0;
	bool ret = FALSE;

	int target_sela = 0;
	int target_selb = 0;
	u32 tx_selb, all_selb;
	bool status_ret = FALSE;
	int selb_cur = 0;
	int tuning_error_type[DLL_SEL_RESULT_SIZE] = { 0 };
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;
	//struct mmc_ios    ios = host->mmc->ios;

	msm_host->sdr50_notuning_crc_error_flag = 0;
	tx_selb_failed_history_reset(host);
	tx_selb_failed_tb_reset(host);
	all_selb_failed_history_reset(host);	//init

	{
		u32 inj_tx_selb = BIT_PASS_MASK;

		if (is_bus_mode_sdr104(host)) {
			inj_tx_selb &= 0x7ff;
		} else {
			inj_tx_selb &= 0x77f;
		}
		tx_selb_failed_history_update(host, inj_tx_selb);
		PrintMsg("###inj_tx_selb[%xh] \n", inj_tx_selb);
	}
	{
		u8 tuning_win[TUNING_PHASE_SIZE + 1] = { 0 };
		u8 selb_tuning_pass_num = 0;
		u8 first_selb = 0;

		status_ret = FALSE;
		ret =
		    sd_gg_tuning_status(host, &tx_selb, &all_selb, &status_ret);
		if (ret == FALSE) {
			DbgErr
			    ("Error when output_tuning, first sd_tuning fail\n");
			goto sel_test;
		}

		if (TRUE == status_ret) {

			tx_selb &= tx_selb_failed_history_get(host);
		} else {
			tx_selb = tx_selb_failed_history_get(host);
		}

		phase_str(tuning_win, tx_selb);
		PrintMsg("### tx selb[%xh] : %s \n", tx_selb, tuning_win);
		bits_generate_array(tuning_win, tx_selb);
		no_fail_p(tuning_win, &first_selb, &selb_tuning_pass_num);
		selb_cur = first_selb;
	}

sel_test:
//init
	dll_sela_basis = get_config_sela_setting(host);

	for (dll_sela_cnt = 0; dll_sela_cnt < TUNING_PHASE_SIZE; dll_sela_cnt++) {
		host_cmddat_line_reset(host);
		cur_dll_sela =
		    (dll_sela_cnt + dll_sela_basis) % TUNING_PHASE_SIZE;
		PrintMsg("=== ot select sela: %x, selb: %x ===\n", cur_dll_sela,
			 selb_cur);
		if ((FALSE == is_bus_mode_sdr104(host))
		    && 1 == msm_host->sdr50_notuning_sela_inject_flag
		    && GET_IDX_VALUE(msm_host->sdr50_notuning_sela_rx_inject,
				     cur_dll_sela)) {
			PrintMsg("skip %d\n", cur_dll_sela);
			tuning_error_type[cur_dll_sela] = READ_STATUS_FAIL_TYPE;
			goto cur_sela_failed;
		}
		ret =
		    ggc_set_output_tuning_phase(host, cur_dll_sela,
						update_selb(host, selb_cur));

		if (ret == FALSE) {
			DbgErr
			    ("Error when output_tuning,  sd_tuning set sela,selb fail at phase %d\n",
			     cur_dll_sela);
			tuning_error_type[cur_dll_sela] = SET_PHASE_FAIL_TYPE;
			goto cur_sela_failed;
		}
		status_ret = FALSE;
		ret =
		    sd_gg_tuning_status(host, &tx_selb, &all_selb, &status_ret);

		if (ret == FALSE) {
			DbgErr
			    ("Error when output_tuning,  sd_tuning fail at phase %d\n",
			     cur_dll_sela);
			tuning_error_type[cur_dll_sela] = TUNING_FAIL_TYPE;
			goto cur_sela_failed;
		}
		//update tx history
		if (TRUE == status_ret) {
			u8 tuning_win[TUNING_PHASE_SIZE + 1] = { 0 };
			u8 selb_tuning_pass_num = 0;
			u8 mid_val = 0;

			all_selb_failed_history_update(host, cur_dll_sela,
						       all_selb);
			if (get_bit_number(tx_selb) >= 6)	//skip bad tx_selb
			{
				tx_selb_failed_tb_update(host, cur_dll_sela,
							 tx_selb);
				tx_selb_failed_history_update(host, tx_selb);
				tx_selb = tx_selb_failed_history_get(host);
				phase_str(tuning_win, tx_selb);
				PrintMsg("### tx selb[%xh]: %s \n", tx_selb,
					 tuning_win);
				bits_generate_array(tuning_win, tx_selb);
				no_fail_p(tuning_win, &mid_val,
					  &selb_tuning_pass_num);
				selb_cur = mid_val;
			} else {
				DbgErr("bad tx_selb:%xh\n", tx_selb);

			}
		} else {
			PrintMsg("read status failedA!!\n");
			tuning_error_type[cur_dll_sela] = READ_STATUS_FAIL_TYPE;
			goto cur_sela_failed;
		}

		PrintMsg("===ot  sela:%xh pass ===\n", cur_dll_sela);
		tuning_error_type[cur_dll_sela] = OUTPUT_PASS_TYPE;
		goto next_dll_sela;

cur_sela_failed:
		PrintMsg("read status failedB\n");
		all_selb = 0;
		all_selb_failed_history_update(host, cur_dll_sela, all_selb);	//mark failed case to failed.

		PrintMsg("===ot  sela:%xh failed ===\n", cur_dll_sela);
next_dll_sela:
		if ((FALSE == is_bus_mode_sdr104(host))
		    && msm_host->sdr50_notuning_crc_error_flag) {
			u32 fp = 0;
			fp = GENERATE_IDX_VALUE(cur_dll_sela);
			fp |=
			    GENERATE_IDX_VALUE((cur_dll_sela +
						1) % TUNING_PHASE_SIZE);
			fp |=
			    GENERATE_IDX_VALUE((cur_dll_sela +
						10) % TUNING_PHASE_SIZE);
			msm_host->sdr50_notuning_sela_rx_inject = fp;
			msm_host->sdr50_notuning_sela_inject_flag = 1;
			PrintMsg("sdr50_notuning_sela_rx_inject:%x\n",
				 msm_host->sdr50_notuning_sela_rx_inject);
			ret = FALSE;
			goto exit;
		};
	}

	//
	{
		int i = 0;

		for (i = 0; i < TUNING_PHASE_SIZE; i++) {
			u8 all_str[TUNING_PHASE_SIZE + 1],
			    tx_str[TUNING_PHASE_SIZE + 1];
			phase_str(all_str,
				  all_selb_failed_history_get(host, i));
			phase_str(tx_str, tx_selb_failed_tb_get(host, i));
			PrintMsg
			    ("DLL sela[%x]  all selb: %s   tx selb: %s [%xh,%xh] %s\n",
			     i, all_str, tx_str,
			     all_selb_failed_history_get(host, i),
			     tx_selb_failed_tb_get(host, i),
			     op_dbg_str[tuning_error_type[i]]);

		}
		{
			u32 tx_selb = tx_selb_failed_history_get(host);
			u32 cfg = 0;
			PrintMsg
			    ("inject sw selb & merge tx_selb failed point to all_selb\n");
			for (i = 0; i < TUNING_PHASE_SIZE; i++) {
				all_selb_failed_history_update(host, i, tx_selb);	//inject tx_selb failed
			}
#if  1
			PrintMsg("inject sw sela failed point to all_selb\n");
			if (is_bus_mode_sdr104(host)) {
				cfg = 0x7ff;
			} else {
				cfg = 0x7ff;
			}
			for (i = 0; i < TUNING_PHASE_SIZE; i++) {
				if (0 == GET_IDX_VALUE(cfg, i)) {
					all_selb_failed_history_update(host, i, 0);	//inject sela failed
				}
			}
#endif
			for (i = 0; i < TUNING_PHASE_SIZE; i++) {
				u8 all_str[TUNING_PHASE_SIZE + 1],
				    tx_str[TUNING_PHASE_SIZE + 1];
				phase_str(all_str,
					  all_selb_failed_history_get(host, i));
				phase_str(tx_str,
					  tx_selb_failed_tb_get(host, i));
				PrintMsg
				    ("DLL sela[%x]  all selb: %s   tx selb: %s [%xh,%xh] %s\n",
				     i, all_str, tx_str,
				     all_selb_failed_history_get(host, i),
				     tx_selb_failed_tb_get(host, i),
				     op_dbg_str[tuning_error_type[i]]);

			}
		}
		// calculate selb
		{
			u8 win_tb[12] = { 0 };
			u8 win_mid = 0;
			u8 win_max = 0;
			u32 tx_tmp = 0;
			u32 tx_selb = tx_selb_failed_history_get(host);
			tx_selb &= 0x7ff;
			tx_tmp = tx_selb;
			//if((tx_selb & 0x7ff )== 0x7ff) //all pass
			PrintMsg("---selb merge---\n");
			{
#if 1
				if (is_bus_mode_sdr104(host)) {
					u32 cfg = 0x7ff;
					tx_selb &= cfg;
					PrintMsg
					    ("tx selb:%xh SDR104 inject:%xh merge tx_selb:%xh\n",
					     tx_tmp, cfg, tx_selb);
				} else {
					u32 cfg = 0x7ff;
					tx_selb &= cfg;
					PrintMsg
					    ("tx selb:%xh SDR50 inject:%xh merge tx_selb:%xh\n",
					     tx_tmp, cfg, tx_selb);
				}
#endif
			}

			{
				if (0 == tx_selb) {
					DbgErr
					    ("no valid  selb window, all failed, so force fixed phase sela selb to default\n");
					target_selb =
					    get_config_selb_setting(host);
					target_sela =
					    get_config_sela_setting(host);
					goto final;
				}
				phase_str(win_tb, tx_selb);
				PrintMsg("######### tx selb[%xh] 11 bit: %s \n",
					 tx_selb, win_tb);
				bits_generate_array(win_tb, tx_selb);
				chk_phase_window(win_tb, &win_mid, &win_max);
				target_selb = win_mid;

			}
			{
				//sela

				u32 all_sela = 0;

				for (i = 0; i < TUNING_PHASE_SIZE; i++) {
					u32 all_selb =
					    all_selb_failed_history_get(host,
									i);
					phase_str(win_tb, all_selb);
					PrintMsg
					    ("######### all_selb[%xh] 11 bit: %s \n",
					     all_selb, win_tb);
					bits_generate_array(win_tb, all_selb);
					chk_phase_window(win_tb, &win_mid,
							 &win_max);
					*selb_pass_win = win_max;
					if (all_selb & (1 << target_selb)) {
						all_sela |= 1 << i;
					}

				}

				phase_str(win_tb, all_sela);
				PrintMsg
				    ("######### all_sela[%xh] 11 bit: %s \n",
				     all_sela, win_tb);
				bits_generate_array(win_tb, all_sela);
				chk_phase_window(win_tb, &win_mid, &win_max);
				target_sela = win_mid;
#if 0
				if (host->info.sw_cur_setting.sd_access_mode ==
				    SD_FNC_AM_SDR50
				    || (host->info.sw_cur_setting.
					sd_access_mode == SD_FNC_AM_SDR104
					&&
					(get_pass_window_weight
					 (host->host->cfg->output_pfm.
					  sw_sela_sdr104.fail_phase) != 11))) {
					// move for host
					PrintMsg("base sela:%xh, selb:%x\n",
						 target_sela, target_selb);
					get_refine_sel(host, target_sela,
						       target_selb,
						       &target_sela,
						       &target_selb);
				}
#endif
			}

		}

	}

final:
	{
		if (0) {
			gg_fix_output_tuning_phase(host, target_sela,
						   target_selb);
		} else {
			PrintMsg
			    ("######### hw select mode - sela dll: %x, selb dll: %x \n",
			     target_sela, target_selb);
			ggc_set_output_tuning_phase(host, target_sela,
						    target_selb);
		}
		if (1) {
			ret =
			    sd_gg_tuning_status(host, &tx_selb, &all_selb,
						&status_ret);
			if (ret == FALSE) {
				DbgErr
				    ("Error when output_tuning,  sd_tuning fail\n");
				ret = FALSE;
				goto exit;
			}

			//use final pass windows
			{
				u8 win_tb[12] = { 0 };
				u8 win_mid = 0;
				u8 win_max = 0;
				phase_str(win_tb, all_selb);
				PrintMsg
				    ("######### all_selb[%xh] 11 bit: %s \n",
				     all_selb, win_tb);
				bits_generate_array(win_tb, all_selb);
				chk_phase_window(win_tb, &win_mid, &win_max);
				*selb_pass_win = win_max;
			}

		}

	}

exit:
	PrintMsg("exit:%s  %d\n", __FUNCTION__, ret);
	return ret;
}

void ggc_chip_init(struct sdhci_host *host)
{
	static int init_flg = 0;
	if (0 == init_flg) {
		u8 data[512] = { 0 };
		get_gg_reg_def(host, data);
		get_defualt_setting(data);
		set_gg_reg_cur_val(data);	//save it to cur
		init_flg = 1;
	}
}

int sdhci_bht_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	if (sdhci_bht_target_host(host)) {	//wangpengpeng@wind-mobi.com modify for sd at 20180223
		u8 tw = 0;
		int ret = 0;
		struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
		struct sdhci_msm_host *msm_host = pltfm_host->priv;
		PrintMsg("enter bht tuning\n");

		if (host->clock < CORE_FREQ_100MHZ) {
			PrintMsg("%d less 100Mhz,no tuning\n", host->clock);
			return 0;
		}

		if (msm_host->tuning_in_progress) {
			PrintMsg("tuning_in_progress\n");
			return 0;
		}
		msm_host->tuning_in_progress = true;

		ggc_chip_init(host);
		udelay(1200);
		if (is_bus_mode_sdr104(host)) {
			sdhci_bht_sdr104_execute_tuning(host, opcode);
		} else {
			sdhci_bht_sdr50_execute_tuning(host, opcode);
		}
		ret = ! ! !_ggc_output_tuning(host, &tw);

		msm_host->tuning_in_progress = false;
		return ret;
	} else
		return sdhci_msm_execute_tuning(host, opcode);
}
