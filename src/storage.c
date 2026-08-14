/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#define STORAGE_PARTITION storage_partition

static const struct device *const storage_dev =
	PARTITION_DEVICE(STORAGE_PARTITION);

int storage_init(void)
{
	if (!device_is_ready(storage_dev)) {
		printk("Storage partition not found!\n");
		return -ENODEV;
	}

	return 0;
}

/*
 * TODO: persist the calibration via flash_area_open(PARTITION_ID(
 * STORAGE_PARTITION)) and flash_area_write(), behind a magic + version header
 * so a stale or blank partition is rejected on load. Erase is sector-granular,
 * so the record needs its own sector or a settings/NVS backend. A matching
 * storage_load_calibration() belongs here once this writes something.
 */
int storage_store_calibration(const struct accel_calibration *cal)
{
	ARG_UNUSED(cal);

	return -ENOSYS;
}
