/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

#include "accel.h"
#include "calibration.h"
#include "storage.h"
#include "uart_cmd.h"

#define STACKSIZE        1024
#define STACKSIZE_MPU    2048
#define PRIORITY         7
#define PRIORITY_MPU6050 7
#define PRIORITY_UART    6

/* Console report period. Independent of the 9 ms sensor sampling interval. */
#define REPORT_INTERVAL_MS 100

/* UART commands. */
#define CMD_NEXT_POSE 'n'
#define CMD_CALIBRATE 'c'


static void accel_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	printk("INIT MPU\n");

	if (accel_init() != 0) {
		return;
	}

	accel_run();
}

static void calibration_thread(void *p1, void *p2, void *p3)
{
	struct accel_calibration cal;
	int rc;

	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	/*
	 * A stored calibration means no six-point routine is needed at boot.
	 * Anything else -- never written, wrong version, corrupt -- is treated
	 * the same way: calibrate now and write the result.
	 */
	rc = storage_load_calibration(&cal);
	if (rc == 0) {
		set_calibration(&cal);
		printk("calibration: loaded from flash, send '%c' to redo\n",
		       CMD_CALIBRATE);
	} else {
		printk("calibration: none stored (%d), starting one\n", rc);
		calibration_request();
	}

	while (1) {
		calibration_wait();

		rc = calibration_run(&cal);
		if (rc != 0) {
			printk("calibration failed: %d\n", rc);
			continue;
		}

		rc = storage_store_calibration(&cal);
		if (rc != 0) {
			printk("calibration: store failed: %d, "
			       "values are lost on reset\n", rc);
		} else {
			printk("calibration: stored to flash\n");
		}
	}
}

static void command_thread(void *p1, void *p2, void *p3)
{
	char command[UART_CMD_MAX_LEN];

	ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

	if (uart_cmd_init() != 0) {
		return;
	}

	while (1) {
		if (uart_cmd_get(command, sizeof(command), K_FOREVER) != 0) {
			continue;
		}

		switch (command[0]) {
		case CMD_NEXT_POSE:
			/* operator confirmed the current calibration pose */
			calibration_advance();
			break;
		case CMD_CALIBRATE:
			/* recalibrate and overwrite whatever is in flash */
			calibration_request();
			break;
		default:
			printk("unknown command: %s\n", command);
			break;
		}
	}
}

int main(void)
{
	if (storage_init() != 0) {
		printk("continuing without persistent storage\n");
	}

	/*
	 * Sole owner of the per-sample console output. Reporting from here
	 * rather than inside accel.c decouples the print rate from the sample
	 * rate, and keeps accel.c independent of calibration.c -- the sensor
	 * layer has no reason to know whether a calibration exists.
	 */
	while (1) {
		struct accel_sample s;

		if (accel_get_latest(&s) == 0) {
			bool cal = calibration_apply_active(&s) == 0;

			printk("[%s] [%f] [%f] [%f] g\n", cal ? "CAL" : "RAW",
			       (double)s.x, (double)s.y, (double)s.z);
		}

		k_sleep(K_MSEC(REPORT_INTERVAL_MS));
	}
}

K_THREAD_DEFINE(accel_tid, STACKSIZE_MPU, accel_thread, NULL, NULL, NULL,
		PRIORITY_MPU6050, K_FP_REGS, 0);

K_THREAD_DEFINE(calibration_tid, STACKSIZE_MPU, calibration_thread, NULL, NULL, NULL,
		PRIORITY, K_FP_REGS, 0);

K_THREAD_DEFINE(command_tid, STACKSIZE, command_thread, NULL, NULL, NULL,
		PRIORITY_UART, 0, 0);
