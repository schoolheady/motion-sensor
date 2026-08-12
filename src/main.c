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
#define STACKSIZE_MPU 2048
#define PRIORITY 7
#define PRIORITY_MPU6050 7
#define PRIORITY_UART 6

K_MUTEX_DEFINE(my_mutex);

static struct accelerometer{
	float x;
	float y;
	float z;
};	

static struct accelerometer accel;
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
	struct sensor_value s_accel[3];
	struct sensor_value gyro[3];
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ,
					s_accel);
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
		accel.x = sensor_value_to_double(&s_accel[0]);
		accel.y = sensor_value_to_double(&s_accel[1]);
		accel.z = sensor_value_to_double(&s_accel[2]);
		printk("[ACCEL] - [%f] [%f] [%f] m/s\n",
		accel.x,
		accel.y,
		accel.z);
		
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
float x_scale  = 0;
float y_scale  = 0;
float z_scale  = 0;

float *xyz_raw_matrix = NULL;
int start_array = 0;
bool static next = false;

int initialize_flash(){


	return 0;
}

// store calibration
int store_calibrations(){


}
// get calibration

void get_raw_3(){
	printk("INIT get_raw\n");
	xyz_raw_matrix = malloc(sizeof(float) * 18);
	// collect first 3 
	while(start_array < 18){
		printk("start_index: %d", start_array);
		while(1){
			
			xyz_raw_matrix[start_array] 	= accel.x;  
			xyz_raw_matrix[start_array + 1] = accel.y; 
			xyz_raw_matrix[start_array + 2] = accel.z; 

			k_mutex_lock(&my_mutex, K_FOREVER);
			//printk("next %d", next);
			if(next == true){
				next = false;
				break;
			}
			k_mutex_unlock(&my_mutex);
			
		}
		
		start_array = start_array + 3;
	}
	printk("------------- uncalibrated -------------------\n");
	for(int i = 0; i < 18; i = i + 3){
		printk("[%f] [%f] [%f]\n",xyz_raw_matrix[i]
								,xyz_raw_matrix[i + 1]
								,xyz_raw_matrix[i + 2]);
	}

}

int calibration(){
	printk("entering calibration");
	get_raw_3();

	// check if 0
	int check[6] = {0 , 4, 8, 9, 13 ,17};
	for(int i = 0 ; i < 6; i++){
		int val_xyz = xyz_raw_matrix[check[i]];
		if(val_xyz == 0){
			xyz_raw_matrix[check[i]] = 0.0000001;
		}

	}

	float Xxup   = xyz_raw_matrix[0];
	float Yyup   = xyz_raw_matrix[4];
	float Zzup 	 = xyz_raw_matrix[8];
	float Xxdown = xyz_raw_matrix[9];
	float Yydown = xyz_raw_matrix[13];
	float Zzdown = xyz_raw_matrix[17];

	x_offset = -((Xxup + Xxdown ) / (Xxup - Xxdown ));
	y_offset = -((Yyup + Yydown ) / (Yyup - Yydown ));
	z_offset = -((Zzup + Zzdown ) / (Zzup - Zzdown ));
	x_scale = 2 / (Xxup - Xxdown);
	y_scale = 2 / (Yyup - Yydown);
	z_scale = 2 / (Zzup - Zzdown);
	printk("------------- calibrated -------------------\n");
		for(int i = 0; i < 18; i = i + 3){
		printk("[%f] [%f] [%f]\n",xyz_raw_matrix[i]    * x_scale + x_offset
								,xyz_raw_matrix[i + 1] * y_scale + y_offset
								,xyz_raw_matrix[i + 2] * z_scale + y_offset
							); 
	}
}

// int calculate_offset(){

 


// 	return 0;
// }

// int calibrate_accelerometer(){

// 	return 0;
// }


void read_uart()
{
	// if (!device_is_ready(uart_dev)) {
	// 	printk("UART device not found!");
	// 	return 0;
	// }
	initialize_uart();
	char *command = malloc(sizeof(char)*32); 	
	
	while(1){
		// calibrate data
		command = get_data();
		if(command[0] > 0){
			
			if(command[0] == 'n'){ // if n calibrate the next side
				//printk("next\n");
				next = true;
				
			}	
					
			memset(command, 0, sizeof(char)*32);
		}

	}

	free(command);


	

}
void read_mpu6050(){
	printk("INIT MPU\n");
	const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);

	if (!device_is_ready(mpu6050)) {
		printk("Device %s is not ready\n", mpu6050->name);
		return;
	}

#ifdef CONFIG_MPU6050_TRIGGER
	trigger = (struct sensor_trigger) {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	if (sensor_trigger_set(mpu6050, &trigger,
			       handle_mpu6050_drdy) < 0) {
		printf("Cannot configure trigger\n");
		return 0;
	}
	printk("Configured for triggered sampling.\n");
#endif

	while (!IS_ENABLED(CONFIG_MPU6050_TRIGGER)) {
		int rc = process_mpu6050(mpu6050);
		

		if (rc != 0) {
			printk("ERR");
			break;
		}
		k_sleep(K_MSEC(9));
	}

	/* triggered runs with its own thread after exit */
}



K_THREAD_DEFINE(calibration_id, STACKSIZE_MPU, calibration, NULL, NULL, NULL,
		PRIORITY_UART, K_FP_REGS, 0); // Added K_FP_REGS here

K_THREAD_DEFINE(read_mpu6050_id, STACKSIZE_MPU, read_mpu6050, NULL, NULL, NULL,
		PRIORITY_UART, K_FP_REGS, 0);

K_THREAD_DEFINE(read_uart_id, STACKSIZE, read_uart, NULL, NULL, NULL,
		PRIORITY_UART, 0, 0);