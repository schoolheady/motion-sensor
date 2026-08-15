/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "accel.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stdbool.h>

#define SAMPLE_INTERVAL_MS 9

/* SENSOR_G is one g expressed in micro-m/s/s. */
#define ACCEL_MS2_PER_G ((float)SENSOR_G / 1000000.0f)

static const struct device *const accel_dev =
	DEVICE_DT_GET_ONE(invensense_mpu6050);

/*
 * The sampling thread writes the latest reading and the calibration thread
 * reads it, so the fields below are only touched under the mutex.
 */
K_MUTEX_DEFINE(sample_lock);
static struct accel_sample latest;
static bool latest_valid;

static void publish(const struct accel_sample *s)
{
	k_mutex_lock(&sample_lock, K_FOREVER);
	latest = *s;
	latest_valid = true;
	k_mutex_unlock(&sample_lock);
}

int accel_get_latest(struct accel_sample *out)
{
	int rc = 0;

	k_mutex_lock(&sample_lock, K_FOREVER);
	if (latest_valid) {
		*out = latest;
	} else {
		rc = -EAGAIN;
	}
	k_mutex_unlock(&sample_lock);

	return rc;
}

static float to_g(const struct sensor_value *v)
{
	return (float)sensor_value_to_double(v) / ACCEL_MS2_PER_G;
}

static int process_sample(const struct device *dev)
{
	struct sensor_value accel[3];
	/* Read and validated alongside the accelerometer; not consumed yet. */
	struct sensor_value gyro[3];
	struct sensor_value temperature;
	struct accel_sample s;
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
	}
	if (rc != 0) {
		printk("sample fetch/get failed: %d\n", rc);
		return rc;
	}

	s.x = to_g(&accel[0]);
	s.y = to_g(&accel[1]);
	s.z = to_g(&accel[2]);
	publish(&s);

	printk("[ACCEL] - [%f] [%f] [%f] g\n",
	       (double)s.x, (double)s.y, (double)s.z);

	return 0;
}

#ifdef CONFIG_MPU6050_TRIGGER
static struct sensor_trigger trigger;

static void handle_drdy(const struct device *dev,
			const struct sensor_trigger *trig)
{
	int rc = process_sample(dev);

	if (rc != 0) {
		printk("cancelling trigger due to failure: %d\n", rc);
		(void)sensor_trigger_set(dev, trig, NULL);
	}
}
#endif /* CONFIG_MPU6050_TRIGGER */

int accel_init(void)
{
	if (!device_is_ready(accel_dev)) {
		printk("Device %s is not ready\n", accel_dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_MPU6050_TRIGGER
	trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	int rc = sensor_trigger_set(accel_dev, &trigger, handle_drdy);

	if (rc < 0) {
		printk("Cannot configure trigger: %d\n", rc);
		return rc;
	}
	printk("Configured for triggered sampling.\n");
#endif

	return 0;
}

void accel_run(void)
{
	while (!IS_ENABLED(CONFIG_MPU6050_TRIGGER)) {
		if (process_sample(accel_dev) != 0) {
			break;
		}
		k_sleep(K_MSEC(SAMPLE_INTERVAL_MS));
	}
}
