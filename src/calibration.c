/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "calibration.h"
#include "accel.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

/* How long to wait for the sampling thread to produce its first reading. */
#define FIRST_SAMPLE_TIMEOUT_MS 1000
#define FIRST_SAMPLE_POLL_MS    10

/*
 * Given by the command thread when the operator confirms the board is in
 * position, taken by calibration_run(). Replaces the old shared "next" flag.
 */
static K_SEM_DEFINE(orientation_sem, 0, 1);

/*
 * Readings captured in each orientation. Orientation i is the one where axis
 * (i % CAL_AXES) is aligned with gravity, pointing up for i < CAL_AXES and
 * down for i >= CAL_AXES.
 */
static float raw[CAL_ORIENTATIONS][CAL_AXES];

/*
 * The calibration currently in force. calibration_run() writes to the caller's
 * struct, but that lives on calibration_thread's stack and dies with it, so a
 * file-scope copy is what later reads actually use.
 */
static struct accel_calibration active;
static bool calibrated = false;


static const char *const orientation_name[CAL_ORIENTATIONS] = {
	"+X up", "+Y up", "+Z up", "-X up", "-Y up", "-Z up",
};

void calibration_advance(void)
{
	k_sem_give(&orientation_sem);
}
int get_calibration(void)
{
	return calibrated;
}
/* Apply a calibration to a sample in place. */
static void calibration_apply(const struct accel_calibration *cal,
			      struct accel_sample *s)
{
	s->x = s->x * cal->scale[0] + cal->offset[0];
	s->y = s->y * cal->scale[1] + cal->offset[1];
	s->z = s->z * cal->scale[2] + cal->offset[2];
}

static int wait_for_first_sample(void)
{
	struct accel_sample s;

	for (int i = 0; i < FIRST_SAMPLE_TIMEOUT_MS / FIRST_SAMPLE_POLL_MS; i++) {
		if (accel_get_latest(&s) == 0) {
			return 0;
		}
		k_sleep(K_MSEC(FIRST_SAMPLE_POLL_MS));
	}

	return -EAGAIN;
}

static int collect(void)
{
	int rc = wait_for_first_sample();

	if (rc != 0) {
		printk("calibration: no sensor data available\n");
		return rc;
	}

	for (int i = 0; i < CAL_ORIENTATIONS; i++) {
		struct accel_sample s;

		printk("calibration: hold the board %s, then send 'n'\n",
		       orientation_name[i]);
		k_sem_take(&orientation_sem, K_FOREVER);

		rc = accel_get_latest(&s);
		if (rc != 0) {
			printk("calibration: sample unavailable: %d\n", rc);
			return rc;
		}

		raw[i][0] = s.x;
		raw[i][1] = s.y;
		raw[i][2] = s.z;
	}

	return 0;
}

static void print_table(const char *title, const struct accel_calibration *cal)
{
	printk("------------- %s -------------------\n", title);

	for (int i = 0; i < CAL_ORIENTATIONS; i++) {
		struct accel_sample s = {
			.x = raw[i][0],
			.y = raw[i][1],
			.z = raw[i][2],
		};

		if (cal != NULL) {
			calibration_apply(cal, &s);
		}
		printk("[%f] [%f] [%f]\n", (double)s.x, (double)s.y, (double)s.z);
	}
}

int calibration_run(struct accel_calibration *out)
{
	int rc = collect();

	if (rc != 0) {
		return rc;
	}

	print_table("uncalibrated", NULL);

	/*
	 * Nudge an exactly-zero gravity-aligned reading off zero; it would
	 * otherwise be the sole term of a denominator below.
	 */
	for (int i = 0; i < CAL_ORIENTATIONS; i++) {
		int axis = i % CAL_AXES;

		if (raw[i][axis] == 0.0f) {
			raw[i][axis] = 1e-7f;
		}
	}

	/*
	 * Samples arrive in g, so a healthy axis reads about +1 up and -1 down
	 * and the 2.0f below is that expected 2 g span. Scale therefore lands
	 * near 1.0 and offset near 0.0; how far each strays is the sensor's own
	 * sensitivity error and zero-g bias, both in g.
	 */
	for (int axis = 0; axis < CAL_AXES; axis++) {
		float up = raw[axis][axis];
		float down = raw[axis + CAL_AXES][axis];

		out->offset[axis] = -((up + down) / (up - down));
		out->scale[axis] = 2.0f / (up - down);
	}
	active = *out;
	calibrated = true;
	print_table("calibrated", out);

	return 0;
}

// when eeprom is not empty this has to run to get and set the calibration values
int set_calibration(struct accel_calibration *out)
{
	/* TODO: placeholders until storage_load_calibration() exists. */
	const float scale[CAL_AXES]  = {1.0f, 1.0f, 1.0f};
	const float offset[CAL_AXES] = {0.0f, 0.0f, 0.0f};

	/* Arrays are not assignable in C; copy element-wise. */
	for (int i = 0; i < CAL_AXES; i++) {
		out->offset[i] = offset[i];
		out->scale[i] = scale[i];
	}

	active = *out;
	calibrated = true;

	return 0;
}

int get_calibrated_values(void)
{
	struct accel_sample s;
	int rc;

	if (!get_calibration()) {
		return -EAGAIN;
	}

	rc = accel_get_latest(&s);
	if (rc != 0) {
		return rc;
	}

	calibration_apply(&active, &s);
	printk("[CAL] [%f] [%f] [%f] g\n", (double)s.x, (double)s.y, (double)s.z);

	return 0;
}
