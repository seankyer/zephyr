/*
 * Copyright (c) 2025 Analog Devices, Inc.
 * Copyright (c) 2023 TOKITA Hiroshi
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(cpu_freq_policy_pressure, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct pstate *soc_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))};

const size_t soc_pstates_count = ARRAY_SIZE(soc_pstates);

#define DIE_TEMP_ALIAS(i) DT_ALIAS(_CONCAT(die_temp, i))
#define DIE_TEMPERATURE_SENSOR(i, _)                                                               \
	IF_ENABLED(DT_NODE_EXISTS(DIE_TEMP_ALIAS(i)), (DEVICE_DT_GET(DIE_TEMP_ALIAS(i)),))

/* support up to 16 cpu die temperature sensors */
static const struct device *const sensors[] = {LISTIFY(16, DIE_TEMPERATURE_SENSOR, ())};

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1) && !defined(CONFIG_CPU_FREQ_PER_CPU_SCALING)

/*
 * IPI tracking is needed on SMP systems where all CPUs share the same
 * frequency. The last CPU to call cpu_freq_best_pstate() sets the best
 * P-state for all CPUs.
 */

#define CPU_FREQ_IPI_TRACKING

static struct k_spinlock lock;
static const struct pstate *pstate_best;
static unsigned int num_unprocessed_cpus;

#endif /* CONFIG_SMP && (CONFIG_MP_MAX_NUM_CPUS > 1) && !CONFIG_CPU_FREQ_PER_CPU_SCALING */

#define THERMAL_CRIT_MDEG (CONFIG_CPU_FREQ_POLICY_THERMAL_CRITICAL_TEMP * 1000)

static int get_thermal_load(const struct device *dev)
{
	struct sensor_value val;
	int rc;

	/* fetch sensor samples */
	rc = sensor_sample_fetch(dev);
	if (rc) {
		printk("Failed to fetch sample (%d)\n", rc);
		return rc;
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (rc) {
		printk("Failed to get data (%d)\n", rc);
		return rc;
	}

	int load = 0;
	int temp_mdeg = sensor_value_to_milli(&val);

	if (temp_mdeg <= 0) {
		load = 0;
	} else if (temp_mdeg >= THERMAL_CRIT_MDEG) {
		load = 100;
	} else {
		load = (temp_mdeg * 100) / THERMAL_CRIT_MDEG;
	}

	return load;
}

/*
 * The pressure policy iterates through the threads currently sitting in the ready queue
 * at the time of evaluation and accumulates the sum of their priorities, normalizing them
 * around CONFIG_CPU_FREQ_POLICY_PRESSURE_LOWEST_PRIO, configured by the user. The policy
 * then iterates through the list of available P-states and selects the first P-state where
 * the current normalized system pressure is greater than or equal to the load threshold of
 * the P-state. If the calculated pressure is below all available P-state thresholds, then
 * the last P-state in the array will be selected. P-states must be defined in increasing
 * threshold order.
 */
int cpu_freq_policy_select_pstate(const struct pstate **pstate_out)
{
	int thermal_load = 0;
	int cpu_id = 0;

	if (NULL == pstate_out) {
		LOG_ERR("On-Demand Policy: pstate_out is NULL");
		return -EINVAL;
	}

#if defined(CONFIG_SMP)
	/* The caller has already ensured that the CPU is fixed */
	cpu_id = arch_curr_cpu()->id;
#endif

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			LOG_ERR("sensor: device %s not ready.\n", sensors[i]->name);
			return -1;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		thermal_load = get_thermal_load(sensors[i]);
		if (thermal_load < 0) {
			LOG_ERR("Unable to retrieve thermal load");
			thermal_load = 0;
		}
	}

	LOG_DBG("CPU%d Thermal Load: %d%%", cpu_id, thermal_load);

	for (int i = 0; i < soc_pstates_count; i++) {
		const struct pstate *state = soc_pstates[i];

		if (thermal_load >= state->load_threshold) {
			*pstate_out = state;
			LOG_DBG("Temperature Policy: Selected P-state "
				"%d with load_threshold=%d%%",
				i, state->load_threshold);
			return 0;
		}
	}

	/* No threshold matched: select the last P-state (lowest performance) */
	*pstate_out = soc_pstates[soc_pstates_count - 1];
	LOG_DBG("Temperature Policy: No threshold matched for CPU thermal load %d%%;"
		"selecting last P-state (load_threshold=%d%%)",
		thermal_load, soc_pstates[soc_pstates_count - 1]->load_threshold);

	return 0;
}

void cpu_freq_policy_reset(void)
{
#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	pstate_best = NULL;
	num_unprocessed_cpus = arch_num_cpus();

	k_spin_unlock(&lock, key);
#endif
}

const struct pstate *cpu_freq_policy_pstate_set(const struct pstate *state)
{
	int ret;

#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((pstate_best == NULL) || (state->load_threshold > pstate_best->load_threshold)) {
		pstate_best = state;
	}

	__ASSERT(num_unprocessed_cpus != 0U, "cpu_freq: Out of sync");

	num_unprocessed_cpus--;
	if (num_unprocessed_cpus > 0) {
		k_spin_unlock(&lock, key);
		return NULL;
	}
	state = pstate_best;
	k_spin_unlock(&lock, key);
#endif

	ret = cpu_freq_pstate_set(state);
	if (ret != 0) {
		LOG_ERR("Failed to set P-state: %d", ret);
		return NULL;
	}

	return state;
}
