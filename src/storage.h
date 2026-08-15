/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include "calibration.h"

/* Open the storage partition. Load/store do this themselves if needed, so
 * calling it early is only useful to surface a problem sooner.
 */
int storage_init(void);

/* Persist the calibration, replacing any previous record. Returns 0 or a
 * negative errno.
 */
int storage_store_calibration(const struct accel_calibration *cal);

/* Read the stored calibration. Returns 0 on success, -ENOENT if nothing has
 * been stored, -ENOTSUP on a version mismatch, or -EILSEQ if the record is
 * corrupt. Any non-zero return means *cal is untouched and a fresh
 * calibration is needed.
 */
int storage_load_calibration(struct accel_calibration *cal);

#endif /* STORAGE_H_ */
