/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include "accel.h"

#define CAL_AXES         3
#define CAL_ORIENTATIONS 6

/* Per-axis correction: corrected = raw * scale + offset. */
struct accel_calibration {
	float offset[CAL_AXES];
	float scale[CAL_AXES];
};

/*
 * Six-point calibration. Prompts for each of the six axis-aligned
 * orientations in turn and blocks until calibration_advance() is called,
 * then derives the offsets and scales. Returns 0 on success or a negative
 * errno if no sensor data was available.
 */
int calibration_run(struct accel_calibration *out);

/*
 * Signal that the board is resting in the orientation last prompted for.
 * Safe to call from another thread; this is what unblocks calibration_run().
 */
void calibration_advance(void);

/* Ask for a (re)calibration. Safe to call from another thread; this is what
 * the operator's calibrate command and the empty-flash boot path both use.
 */
void calibration_request(void);

/* Block until calibration_request() is called. */
void calibration_wait(void);

/* Non-zero once a calibration is in force, from either calibration_run()
 * or set_calibration().
 */
int get_calibration(void);

/* Adopt a calibration loaded from flash instead of running the six-point
 * routine. Returns 0 on success.
 */
int set_calibration(const struct accel_calibration *cal);

/* Correct *s in place using the active calibration.
 * Returns 0, or -EAGAIN if nothing is calibrated yet (s is left untouched).
 */
int calibration_apply_active(struct accel_sample *s);

#endif /* CALIBRATION_H_ */
