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
#include <sys/unistd.h>

#include "LSM9DS1/LSM9DS1.cpp"
#include "GP20U7/GP20U7.cpp"
#include "MicroSDBreakout/MicroSDBreakout.cpp"


#define GPIO_INPUT_IO_0 GPIO_NUM_12
#define GPIO_INPUT_IO_1 GPIO_NUM_13
#define ESP_INTR_FLAG_DEFAULT 0

TaskHandle_t gps_task;
TaskHandle_t ninedof_task;
UBaseType_t num_of_tasks;
TaskHandle_t current_task;

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    gpio_num_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
	    TaskHandle_t xHandle = NULL;
            
	    if (io_num == 12){
                num_of_tasks = uxTaskGetNumberOfTasks();
		printf("Tasks Before Create: %d\n",num_of_tasks);
		if (gps_task == NULL) {
			xTaskCreate(i2c_task_who_am_i, "LSM9DS1_whoami_task", 1024 * 2, (void* ) 0, 10, &ninedof_task);
                	xTaskCreate(GP20U7_task, "GPS - GP20U7_task", 1024 * 2, (void* ) 0, 10, &gps_task);
            	}
		num_of_tasks = uxTaskGetNumberOfTasks();
		printf("Tasks After Create: %d\n",num_of_tasks);
		}
            else if (io_num == 13){
                num_of_tasks = uxTaskGetNumberOfTasks();
		printf("Tasks Before Delete: %d\n",num_of_tasks);
		if(ninedof_task != NULL) {
		
			vTaskDelete(ninedof_task);
			
			ninedof_task = NULL;
		
		}
		if(gps_task != NULL) {

		        vTaskDelete(gps_task);

			gps_task = NULL;        	
		
		}
		num_of_tasks = uxTaskGetNumberOfTasks();
		printf("Tasks After Delete: %d\n",num_of_tasks);
	}
        }
    }
}

xSemaphoreHandle print_mux;

extern "C" void app_main()
{
	/* Print chip information */
	print_mux = xSemaphoreCreateMutex();
		
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
		chip_info.cores,
		(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	printf("silicon revision %d, ", chip_info.revision);

	printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
		(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	gpio_config_t io_conf;
        //disable pull-down mode
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;

        //interrupt of rising edge
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        //set as input mode    
        io_conf.mode = GPIO_MODE_INPUT;
        //enable pull-up mode
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);

        //change gpio intrrupt type for one pin
        gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE);
        gpio_set_intr_type(GPIO_INPUT_IO_1, GPIO_INTR_POSEDGE);
        //create a queue to handle gpio event from isr
        gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        //start gpio task
        xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

        //install gpio isr service
        gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
        //hook isr handler for specific gpio pin
        gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
        gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);
        //remove isr handler for gpio number.
        gpio_isr_handler_remove(GPIO_INPUT_IO_0);
        gpio_isr_handler_remove(GPIO_INPUT_IO_1);
  
        //hook isr handler for specific gpio pin again
        gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
        gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);		
	
	/*
	 * init sensors
	 */
	init_i2c();
	LSM9DS1_init();
	GP20U7_init();
	sdmmc_card_t* sdcard = init_sdcard();

	/*
	 * register tasks
	 */
	//xTaskCreate(i2c_task_who_am_i, "LSM9DS1_whoami_task", 1024 * 2, (void* ) 0, 10, NULL);
	// xTaskCreate(i2c_task_LSM9DS1, "LSM9DS1_task", 1024 * 2, (void* ) 0, 10, NULL);
	//xTaskCreate(GP20U7_task, "GPS - GP20U7_task", 1024 * 2, (void* ) 0, 10, NULL);

	unmount_sdcard();
	
}
