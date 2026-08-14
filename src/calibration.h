/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

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

#endif /* CALIBRATION_H_ */
