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


#include "LSM9DS1/LSM9DS1.cpp"
#include "GP20U7/GP20U7.cpp"




#define GPIO_INPUT_IO_1     0
#define GPIO_INPUT_PIN_SEL  ((1<<GPIO_INPUT_IO_0) | (1<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0



xSemaphoreHandle print_mux;

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}




extern "C" void app_main()
{
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

	gpio_config_t io_conf;
    	
    	  	   	
    
    	//disable pull-down mode
    	io_conf.pull_down_en = 0;
    	//disable pull-up mode
    	io_conf.pull_up_en = 0;
    	//configure GPIO with the given settings
    	gpio_config(&io_conf);

	//interrupt of rising edge
    	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    	//bit mask of the pins, use GPIO4/5 here
    	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    	//set as input mode    
    	io_conf.mode = GPIO_MODE_INPUT;
    	//enable pull-up mode
    	io_conf.pull_up_en = 1;
    	gpio_config(&io_conf);

	//create a queue to handle gpio event from isr
    	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
   	//start gpio task
    	xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

	//hook isr handler for specific gpio pin
    	gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

	/*
	 * init sensors
	 */
	init_i2c();
	LSM9DS1_init();
	GP20U7_init();

	/*
	 * register tasks
	 */
	int recording_state

	

	switch (recording_state) {
		case 0: {
		}
		case 1: {
		}

	
	xTaskCreate(i2c_task_who_am_i, "LSM9DS1_whoami_task", 1024 * 2, (void* ) 0, 10, NULL);
	// xTaskCreate(i2c_task_LSM9DS1, "LSM9DS1_task", 1024 * 2, (void* ) 0, 10, NULL);
	xTaskCreate(GP20U7_task, "GPS - GP20U7_task", 1024 * 2, (void* ) 0, 10, NULL);

}
