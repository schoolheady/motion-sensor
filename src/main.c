/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#include "accel.h"
#include "calibration.h"
#include "storage.h"
#include "uart_cmd.h"

#define STACKSIZE        1024
#define STACKSIZE_MPU    2048
#define PRIORITY         7
#define PRIORITY_MPU6050 7
#define PRIORITY_UART    6


static void check_calibration(){

}

static void accel_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

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

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("entering calibration\n");

	rc = calibration_run(&cal);
	if (rc != 0) {
		printk("calibration failed: %d\n", rc);
		return;
	}

	rc = storage_store_calibration(&cal);
	if (rc == -ENOSYS) {
		printk("calibration: not persisted, values are lost on reset\n");
	} else if (rc != 0) {
		printk("calibration: store failed: %d\n", rc);
	}

	if(get_calibration()){
		while(1){
			get_calibrated_values();
		}

	}
}

static void command_thread(void *p1, void *p2, void *p3)
{
	char command[UART_CMD_MAX_LEN];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (uart_cmd_init() != 0) {
		return;
	}

	while (1) {
		if (uart_cmd_get(command, sizeof(command), K_FOREVER) != 0) {
			continue;
		}

		switch (command[0]) {
		case 'n':
			/* operator confirmed the current calibration pose */
			calibration_advance();
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

	return 0;
}

K_THREAD_DEFINE(accel_tid, STACKSIZE_MPU, accel_thread, NULL, NULL, NULL,
		PRIORITY_MPU6050, K_FP_REGS, 0);

K_THREAD_DEFINE(calibration_tid, STACKSIZE_MPU, calibration_thread, NULL, NULL, NULL,
		PRIORITY, K_FP_REGS, 0);

K_THREAD_DEFINE(command_tid, STACKSIZE, command_thread, NULL, NULL, NULL,
		PRIORITY_UART, 0, 0);
