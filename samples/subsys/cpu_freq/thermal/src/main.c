/*
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_freq_thermal_sample, LOG_LEVEL_INF);

#define SAMPLE_SLEEP_MS 1500

int main(void)
{
	LOG_INF("Starting CPU Freq Thermal Policy Sample!");

	while (1) {
		k_msleep(SAMPLE_SLEEP_MS);
	}
}
