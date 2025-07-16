/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/p_state.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stm32f4_cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

struct stm32f4_config {
	int state_id;
};

int32_t cpu_freq_performance_state_set(struct p_state state)
{
	int state_id = ((struct stm32f4_config *)state.config)->state_id;

	LOG_DBG("Setting performance state: %d", state_id);

	switch (state_id) {
	case 0:
		LOG_DBG("Setting P-state 0: Nominal Mode");
		break;
	case 1:
		LOG_DBG("Setting P-state 1: Low Power Mode");
		break;
	default:
		LOG_ERR("Unsupported P-state: %d", state_id);
		return -1;
	}

	return 0;
}

#define DEFINE_STM32F4_CONFIG(node_id)                                                               \
	static const struct stm32f4_config _CONCAT(stm32f4_config_, node_id) = {                       \
		.state_id = DT_PROP(node_id, p_state_id),                                          \
	};                                                                                         \
	P_STATE_DT_DEFINE(node_id, &_CONCAT(stm32f4_config_, node_id))

DT_FOREACH_CHILD(DT_PATH(performance_states), DEFINE_STM32F4_CONFIG)
