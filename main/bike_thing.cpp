/*
	Bike Thing

	Sparkfun ESP32-Thing
	https://www.sparkfun.com/products/13907

	Espressif IDF
	https://github.com/espressif/esp-idf

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "Arduino.h"

#include "LSM9DS1/LSM9DS1.cpp"
#include "GP20U7/GP20U7.cpp"

xSemaphoreHandle print_mux;

void blink(void* arg) {
	digitalWrite(13, HIGH);
	vTaskDelay(100);
	digitalWrite(13, LOW);
	vTaskDelay(100);
}
extern "C" void app_main()
{
	initArduino();
	pinMode(13,OUTPUT);
	/* Print chip information */
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
		chip_info.cores,
		(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	printf("silicon revision %d, ", chip_info.revision);

	printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
		(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	print_mux = xSemaphoreCreateMutex();


	/*
	 * init sensors
	 */
	init_i2c();
	LSM9DS1_init();
	GP20U7_init();

	/*
	 * register tasks
	 */
	xTaskCreate(blink, "blink_task", 1024*2, (void*) 0, 10, NULL);
	xTaskCreate(i2c_task_who_am_i, "LSM9DS1_whoami_task", 1024 * 2, (void* ) 0, 10, NULL);
	// xTaskCreate(i2c_task_LSM9DS1, "LSM9DS1_task", 1024 * 2, (void* ) 0, 10, NULL);
	xTaskCreate(GP20U7_task, "GPS - GP20U7_task", 1024 * 2, (void* ) 0, 10, NULL);

}
