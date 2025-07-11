/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_FREQ_POLICIES_CPU_FREQ_POLICY_H
#define ZEPHYR_SUBSYS_CPU_FREQ_POLICIES_CPU_FREQ_POLICY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CPU Frequency Policy Interface - to be implemented by each policy.
 */

#include <zephyr/types.h>
#include <zephyr/cpu_freq/p_state.h>

/**
 * @brief Get the next P-state from CPU Frequency Policy
 *
 * @param p_state Pointer to the P-state struct where the next P-state is returned.
 * @param cpu_load Current CPU load percentage (0-100).
 *
 * @retval 0 In case of success, nonzero in case of failure.
 */
uint32_t cpu_freq_policy_get_p_state_next(struct p_state *p_state, uint32_t cpu_load);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_POLICIES_CPU_FREQ_POLICY_H */
