/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_CPU_FREQ_SOC_H__
#define ZEPHYR_CPU_FREQ_SOC_H__

#include <zephyr/cpu_freq/p_state.h>

/* Define performance-states as extern to be picked up by CPU Freq policy */
#define DECLARE_P_STATE_EXTERN(node_id)                                                            \
	extern const struct p_state _CONCAT(p_state_, DEVICE_DT_NAME_GET(node_id));

DT_FOREACH_CHILD(DT_PATH(performance_states), DECLARE_P_STATE_EXTERN)

#endif /* ZEPHYR_CPU_FREQ_SOC_H__ */
