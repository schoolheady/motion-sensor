/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ACCEL_H_
#define ACCEL_H_

/*
 * Application-side wrapper around the MPU6050 driver. The accel_ prefix is
 * deliberate: the driver already exports mpu6050_init(), and app and driver
 * objects are linked into the same image.
 */

/*
 * One acceleration reading, in g (1 g = 9.80665 m/s/s). The driver reports
 * m/s/s; accel.c converts on the way in so that a calibration scale factor
 * comes out near 1.0 and an offset near 0.0, i.e. the deviation from ideal is
 * the sensor error itself rather than a unit conversion folded in with it.
 */
struct accel_sample {
	float x;
	float y;
	float z;
};

/*
 * Bind the sensor and, when triggers are enabled, install the data-ready
 * handler. Returns 0 on success or a negative errno.
 */
int accel_init(void);

/*
 * Sample until a read fails. Returns immediately when CONFIG_MPU6050_TRIGGER
 * is set, because sampling then happens from the trigger handler instead.
 */
void accel_run(void);

/*
 * Copy the most recent sample into *out. Returns 0, or -EAGAIN if the sensor
 * has not produced a reading yet.
 */
int accel_get_latest(struct accel_sample *out);

#endif /* ACCEL_H_ */
