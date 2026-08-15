/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_cmd.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <string.h>

#define UART_DEVICE_NODE DT_NODELABEL(uart0)

/* Queue of up to 10 completed command lines (aligned to a 4-byte boundary). */
K_MSGQ_DEFINE(uart_msgq, UART_CMD_MAX_LEN, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* Owned by the ISR: line assembled here, then copied into the queue. */
static char rx_buf[UART_CMD_MAX_LEN];
static size_t rx_buf_pos;

static void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	ARG_UNUSED(dev); ARG_UNUSED(user_data);

	uart_irq_update(uart_dev);

	if (uart_irq_rx_ready(uart_dev) <= 0) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, rx_buf, K_NO_WAIT);

			rx_buf_pos = 0;
		} else if (rx_buf_pos < sizeof(rx_buf) - 1) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

int uart_cmd_init(void)
{
	int rc;

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!\n");
		return -ENODEV;
	}

	rc = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	if (rc < 0) {
		if (rc == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (rc == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", rc);
		}
		return rc;
	}

	uart_irq_rx_enable(uart_dev);

	return 0;
}

int uart_cmd_get(char *buf, size_t len, k_timeout_t timeout)
{
	char msg[UART_CMD_MAX_LEN];

	if (len < sizeof(msg)) {
		return -EINVAL;
	}

	if (k_msgq_get(&uart_msgq, msg, timeout) != 0) {
		return -ENOMSG;
	}

	memcpy(buf, msg, sizeof(msg));

	return 0;
}
