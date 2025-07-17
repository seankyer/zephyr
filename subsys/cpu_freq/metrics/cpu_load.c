/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/kernel.h>
 #include <zephyr/logging/log.h>

 LOG_MODULE_REGISTER(cpu_freq_metrics_cpu_load, CONFIG_CPU_FREQ_LOG_LEVEL);

static uint64_t execution_cycles_init = 0;
static uint64_t total_cycles_init = 0;

int get_cpu_load(void)
{
	int ret = 0;
	uint64_t execution_cycles;
	uint64_t total_cycles;
	uint32_t load;

	struct k_thread_runtime_stats cpu_query;

	ret = k_thread_runtime_stats_cpu_get(0, &cpu_query);
	if (ret != 0) {
		LOG_ERR("Unable to retrieve CPU statistics");
		return -EINVAL;
	}

	execution_cycles = cpu_query.execution_cycles - execution_cycles_init;
	total_cycles = cpu_query.total_cycles - total_cycles_init;

	load = (uint32_t)((100 * total_cycles) / execution_cycles);

	execution_cycles_init = cpu_query.execution_cycles;
	total_cycles_init = cpu_query.total_cycles;

	return load;
}
