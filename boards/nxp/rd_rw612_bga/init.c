/*
 * Copyright 2022, 2024-25 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <fsl_power.h>
#include <fsl_common.h>
#include <fsl_io_mux.h>

#define NON_AON_PINS_START      0
#define NON_AON_PINS_BREAK      21
#define NON_AON_PINS_RESTART    28
#define NON_AON_PINS_END        63
#define RF_CNTL_PINS_START      0
#define RF_CNTL_PINS_END        3

static void rdrw61x_power_init_config(void)
{
	power_init_config_t initCfg = {
		/* VCORE AVDD18 supplied from iBuck on RD board. */
		.iBuck = true,
		/* CAU_SOC_SLP_REF_CLK is needed for LPOSC. */
		.gateCauRefClk = false,
	};

	POWER_InitPowerConfig(&initCfg);
}

#if CONFIG_PM
static void rdrw61x_pm_state_exit(enum pm_state state)
{
	switch (state) {
	case PM_STATE_STANDBY:
		rdrw61x_power_init_config();
		break;
	default:
		break;
	}
}
#endif

void board_early_init_hook(void)
{
	rdrw61x_power_init_config();

#if CONFIG_PM
	static struct pm_notifier rdrw61x_pm_notifier = {
		.state_exit = rdrw61x_pm_state_exit,
	};

	pm_notifier_register(&rdrw61x_pm_notifier);

	int32_t i;

	/* Set all non-AON pins output low level in sleep mode. */
	for (i = NON_AON_PINS_START; i <= NON_AON_PINS_BREAK; i++) {
		IO_MUX_SetPinOutLevelInSleep(i, IO_MUX_SleepPinLevelLow);
	}
	for (i = NON_AON_PINS_RESTART; i <= NON_AON_PINS_END; i++) {
		IO_MUX_SetPinOutLevelInSleep(i, IO_MUX_SleepPinLevelLow);
	}

	/* Set RF_CNTL 0-3 output low level in sleep mode. */
	for (i = RF_CNTL_PINS_START; i <= RF_CNTL_PINS_END; i++) {
		IO_MUX_SetRfPinOutLevelInSleep(i, IO_MUX_SleepPinLevelLow);
	}
#endif

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/*
	 * Eventually this code should not be here
	 * but should be configured by some SYSCTL node
	 */

	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm1 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);

	/* Select Data in from Transmit I2S - Flexcomm 1 */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(1);
	/* Enable Transmit I2S - Flexcomm 1 for Shared Data Out */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_FC1DATAOUTEN(1);

	/* Set Receive I2S - Flexcomm 0 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[0] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 1 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Select Receive I2S - Flexcomm 0 Data in from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[0] |= SYSCTL1_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 1 Data out to shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] |= SYSCTL1_FCCTRLSEL_DATAOUTSEL(1);
#endif /* CONFIG_I2S_TEST_SEPARATE_DEVICES */
}
