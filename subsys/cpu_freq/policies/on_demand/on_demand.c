/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <metrics/cpu_load.h>
#include "cpu_freq_soc.h"

LOG_MODULE_REGISTER(cpu_freq_policy_on_demand, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct p_state *soc_p_states[] = {
	DT_FOREACH_CHILD(DT_PATH(performance_states), P_STATE_DT_GET)};

/*
 * On-demand policy scans the list of P-states from the devicetree and selects the
 * first P-state where the cpu_load is greater than or equal to the trigger threshold
 * of the P-state.
 */
int cpu_freq_policy_get_p_state_next(struct p_state *p_state_out)
{
	int ret;
	uint32_t cpu_load;

	ret = get_cpu_load(&cpu_load);
	if (ret) {
		LOG_ERR("Unable to retrieve CPU load");
		return ret;
	}

	LOG_DBG("Current CPU Load: %d%%", cpu_load);

	for (int i = 0; i < ARRAY_SIZE(soc_p_states); i++) {
		const struct p_state *state = soc_p_states[i];

		if (cpu_load >= state->load_threshold) {
			*p_state_out = *state;
			LOG_DBG("On-Demand Policy: Selected P-state %d with load_threshold=%d%%", i,
				state->load_threshold);
			return 0;
		}
	}

	LOG_ERR("On-Demand Policy: No suitable P-state found for CPU load %d%%", cpu_load);

	return -ENOTSUP;
}
