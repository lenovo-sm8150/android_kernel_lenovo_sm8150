/*
 * Copyright (c) 2015, 2018, The Linux Foundation. All rights reserved.
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

#include "sdhci-msm.h"

#ifdef CONFIG_MMC_SDHCI_MSM_BH201
int sdhci_bht_execute_tuning(struct sdhci_host *host, u32 opcode);
#else
inline int sdhci_bht_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	return 0;
}
#endif

