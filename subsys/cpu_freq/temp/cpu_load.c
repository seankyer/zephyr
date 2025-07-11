/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpu_load.h"

static int select;

int get_cpu_load(void)
{
	int load;

	switch (select) {
	case 0:
		load = 25;
		break;
	case 1:
		load = 50;
		break;
	case 2:
		load = 75;
		break;
	default:
		load = 0;
		break;
	}

	select++;
	if (select > 2) {
		select = 0;
	}

	return load;
}
