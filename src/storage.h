/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include "calibration.h"

/* Check that the storage partition's flash device is usable. */
int storage_init(void);

/* Not implemented yet; returns -ENOSYS. */
int storage_store_calibration(const struct accel_calibration *cal);

#endif /* STORAGE_H_ */
