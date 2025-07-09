/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_freq_sample, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Starting CPU Freq Subsystem Sample!\n");

	while (1) {
		/* Some busy work... */
		k_sleep(K_SECONDS(1));
	}
}
