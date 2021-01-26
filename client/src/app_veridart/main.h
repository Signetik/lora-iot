/*
 * main.h
 *
 * Created: 4/3/18 12:09:59 PM
 *  Author: scottschmitt
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#if(0)
#include "hal_i2c.h"
#include "io_config.h"
#include <asf.h>
#endif

#define pdMS_TO_TICKS(X)	(X)
#define pdS_TO_TICKS(X)		(X * 1000)

#define DEVICE_ID 0xB0001111




#if(0)
extern SemaphoreHandle_t i2c_mutex;
extern SemaphoreHandle_t spi_mutex;
extern QueueHandle_t os_job_queue;
extern SemaphoreHandle_t data_ready_semaphore;
extern SemaphoreHandle_t gps_turn_on;
extern SemaphoreHandle_t new_settings;
extern SemaphoreHandle_t modbus_rx_complete;
#endif


#if(0)
extern struct i2c_master_module i2c_master_instance;
extern struct usart_module usart_gps_instance;
extern struct usart_module usart_modbus_instance;

extern struct rtc_module rtc_instance;
#endif


#define LED_BLUE 0x01
#define LED_GREEN 0x02
#define LED_RED 0x04
#define LED_PURPLE 0x05
#define LED_OFF 0x00

void led_color_set(uint8_t color);
void save_settings_to_flash(void);
void update_flash_settings(void);
void load_settings_from_flash(void);



typedef struct settings_s
{
	uint32_t reporting_period;
	uint8_t accel_thresh;
	uint32_t gps_interval_s;
	uint32_t gps_fix_time_s;
	uint32_t move_thresh_m;
}settings_t;

typedef struct server_settings_s
{
	uint32_t reporting_period;
	uint8_t accel_thresh;
	uint32_t gps_interval_s;
	uint32_t gps_fix_time_s;
	uint32_t move_thresh_m;
}server_settings_t;

extern settings_t settings;
extern server_settings_t server_settings;










#endif /* MAIN_H_ */
