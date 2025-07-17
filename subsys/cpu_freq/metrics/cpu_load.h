/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPU_LOAD_H_
#define CPU_LOAD_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the CPU load as a percentage.
 *
 * Return the percent that the CPU has spent in the active (non-idle) state
 * between calls to this function. It's recommended to call this function at a
 * set rate so sequential measurements are consistent.
 *
 * @return The percent of time the CPU has been active since the previous call.
 * @return -EINVAL if the percent could not be calculated.
 */
int get_cpu_load(void);

#ifdef __cplusplus
}
#endif

#endif /* CPU_LOAD_H_ */
