/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include "uart_controll.h"
#define STACKSIZE 1024
#define PRIORITY 7
// #define UART_DEVICE_NODE DT_NODELABEL(uart0)
// #define MSG_SIZE 32
// /* queue to store up to 10 messages (aligned to 4-byte boundary) */
// K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

// static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
// /* receive buffer used in UART ISR callback */
// static char rx_buf[MSG_SIZE];
// static int rx_buf_pos;


// void serial_cb(const struct device *dev, void *user_data)
// {
// 	uint8_t c;

// 	uart_irq_update(uart_dev);

// 	if (uart_irq_rx_ready(uart_dev) <= 0) {
// 		return;
// 	}

// 	/* read until FIFO empty */
// 	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
// 		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
// 			/* terminate string */
// 			rx_buf[rx_buf_pos] = '\0';

// 			/* if queue is full, message is silently dropped */
// 			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

// 			/* reset the buffer (it was copied to the msgq) */
// 			rx_buf_pos = 0;
// 		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
// 			rx_buf[rx_buf_pos++] = c;
// 		}
// 		/* else: characters beyond buffer size are dropped */
// 	}
// }


// /*
//  * Print a null-terminated string character by character to the UART interface
//  */
// void print_uart(char *buf)
// {
// 	int msg_len = strlen(buf);

// 	for (int i = 0; i < msg_len; i++) {
// 		uart_poll_out(uart_dev, buf[i]);
// 	}
// }

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

static int process_mpu6050(const struct device *dev)
{
	struct sensor_value temperature;
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ,
					gyro);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP,
					&temperature);
	}
	if (rc == 0) {
		printk("[ACCEL] - [%f] [%f] [%f] m/s\n",
		sensor_value_to_double(&accel[0]),
		sensor_value_to_double(&accel[1]),
		sensor_value_to_double(&accel[2]));
		// printk("[%s]:%g Cel\n"
		//        "  accel %f %f %f m/s/s\n"
		//        "  gyro  %f %f %f rad/s\n",
		//        now_str(),
		//        sensor_value_to_double(&temperature),
		//        sensor_value_to_double(&accel[0]),
		//        sensor_value_to_double(&accel[1]),
		//        sensor_value_to_double(&accel[2]),
		//        sensor_value_to_double(&gyro[0]),
		//        sensor_value_to_double(&gyro[1]),
		//        sensor_value_to_double(&gyro[2]));
	} else {
		printk("sample fetch/get failed: %d\n", rc);
	}

	return rc;
}

#ifdef CONFIG_MPU6050_TRIGGER
static struct sensor_trigger trigger;

static void handle_mpu6050_drdy(const struct device *dev,
				const struct sensor_trigger *trig)
{
	int rc = process_mpu6050(dev);

	if (rc != 0) {
		printf("cancelling trigger due to failure: %d\n", rc);
		(void)sensor_trigger_set(dev, trig, NULL);
		return;
	}
}
#endif /* CONFIG_MPU6050_TRIGGER */

float x_calibrated = 0;
float y_calibrated = 0;
float z_calibrated = 0;
float x_raw = 0;
float y_raw = 0;
float z_raw = 0;
float x_offset = 0;
float y_offset = 0;
float z_offset = 0;

// float *xyz_raw_matrix = malloc(sizeof(float) * 18);
// int start_array = 0;
// bool next = false;
// int get_raw_3(){

// 	// collect first 3 
// 	while(!next){
		
// 		xyz_raw_matrix[start_array]  
// 		xyz_raw_matrix[start_array + 1]  
// 		xyz_raw_matrix[start_array + 2]  
		
// 	}
// 	// next 3 values
// 	start_array = start_array + 3;

// }
// int get_all_raw(){
// 	get_raw_3();

// }

// int calculate_offset(){

 


// 	return 0;
// }

// int calibrate_accelerometer(){

// 	return 0;
// }


void gg( )
{
	const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);

	if (!device_is_ready(mpu6050)) {
		printk("Device %s is not ready\n", mpu6050->name);
		return;
	}
	// if (!device_is_ready(uart_dev)) {
	// 	printk("UART device not found!");
	// 	return 0;
	// }
	char *command = malloc(sizeof(char)*32); 	
	while(1){
		// calibrate data

		command = get_data();
		if(command[0] > 0){
			
			printk("goo: %c",command[0]);
			memset(command, 0, sizeof(command));
			
			
		}

	}

	free(command);


	



// #ifdef CONFIG_MPU6050_TRIGGER
// 	trigger = (struct sensor_trigger) {
// 		.type = SENSOR_TRIG_DATA_READY,
// 		.chan = SENSOR_CHAN_ALL,
// 	};
// 	if (sensor_trigger_set(mpu6050, &trigger,
// 			       handle_mpu6050_drdy) < 0) {
// 		printf("Cannot configure trigger\n");
// 		return 0;
// 	}
// 	printk("Configured for triggered sampling.\n");
// #endif

// 	while (!IS_ENABLED(CONFIG_MPU6050_TRIGGER)) {
// 		int rc = process_mpu6050(mpu6050);

// 		if (rc != 0) {
// 			break;
// 		}
// 		k_sleep(K_MSEC(9));
// 	}

	/* triggered runs with its own thread after exit */
}

K_THREAD_DEFINE(initialize_uart_id, STACKSIZE, initialize_uart, NULL, NULL, NULL,
		PRIORITY, 0, 0);

K_THREAD_DEFINE(gg_id, STACKSIZE, gg, NULL, NULL, NULL,
		PRIORITY, 0, 0);
