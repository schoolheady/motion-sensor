/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_CMD_H_
#define UART_CMD_H_

#include <zephyr/kernel.h>
#include <stddef.h>

/* Longest command line accepted, terminating NUL included. */
#define UART_CMD_MAX_LEN 32

/* Bind the console UART and enable interrupt-driven receive. */
int uart_cmd_init(void);

/*
 * Copy the next complete line (CR- or LF-terminated, delimiter stripped) into
 * buf. Returns 0 on success, -ENOMSG if none arrived before the timeout, or
 * -EINVAL if buf is too small to hold a full command.
 */
int uart_cmd_get(char *buf, size_t len, k_timeout_t timeout);

#endif /* UART_CMD_H_ */
