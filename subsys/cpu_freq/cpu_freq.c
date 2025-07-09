/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>

LOG_MODULE_REGISTER(cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

static void cpu_freq_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(cpu_freq_work, cpu_freq_work_handler);

/*
 * Workqueue task that runs periodically to execute the selected policy algorithm
 * and pass the next P-state to the P-state driver.
 */
static void cpu_freq_work_handler(struct k_work *work)
{
	uint32_t ret;

	/* 1. Get CPU Load */
	int load = 50;

	LOG_DBG("Current CPU Load: %d%%", load);

	/* 2. Get next P-state */
	struct p_state next_p_state;

	ret = cpu_freq_policy_get_p_state_next(&next_p_state, load);
	if (ret) {
		LOG_ERR("Failed to get next P-state: %d", ret);
		goto reschedule;
	}
	LOG_DBG("Next P-state: load_threshold=%d, config=%p", next_p_state.load_threshold,
		next_p_state.config);

	/* 3. Set P-state using P-state driver */
	ret = cpu_freq_performance_state_set(next_p_state);
	if (ret) {
		LOG_ERR("Failed to set performance state: %d", ret);
		goto reschedule;
	}

reschedule:
	/* Finally, reschedule CPU freq scale task */
	k_work_schedule(&cpu_freq_work, K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS));
}

static int cpu_freq_init(void)
{
	k_work_schedule(&cpu_freq_work, K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS));
	LOG_INF("CPU frequency subsystem initialized with interval %d ms",
		CONFIG_CPU_FREQ_INTERVAL_MS);
	return 0;
}

SYS_INIT(cpu_freq_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
