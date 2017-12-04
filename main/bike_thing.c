/* SD card and FAT filesystem example.
This example code is in the Public Domain (or CC0 licensed, at your option.)
Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"


static const char *TAG = "example";

// This example can use SDMMC and SPI peripherals to communicate with SD card.
// By default, SDMMC peripheral is used.
// To enable SPI mode, uncomment the following line:

#define USE_SPI_MODE

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO GPIO_NUM_19 //was 2
#define PIN_NUM_MOSI GPIO_NUM_23 //was 15
#define PIN_NUM_CLK  GPIO_NUM_18 //was 14
#define PIN_NUM_CS   GPIO_NUM_2  // was 13
#endif //USE_SPI_MODE

#define GPIO_INPUT_IO_0 GPIO_NUM_17
#define GPIO_INPUT_IO_1 GPIO_NUM_16


#define ESP_INTR_FLAG_DEFAULT 0

TaskHandle_t gps_task;
TaskHandle_t ninedof_task;

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
            
	    if (io_num == 16){
	    	xTaskCreate(i2c_task_who_am_i, "LSM9DS1_whoami_task", 1024 * 2, (void* ) 0, 10, &ninedof_task);
  	    	xTaskCreate(GP20U7_task, "GPS - GP20U7_task", 1024 * 2, (void* ) 0, 10, &gps_task);	
	    }
	    else if (io_num == 17){
		vTaskDelete(gps_task);
		vTaskDelete(ninedof_task);
		printf(gps_task);
	    }
	}
    }
}


void app_main()
{
	

 	gpio_config_t io_conf;
    	//disable pull-down mode
    	io_conf.pull_down_en = 0;
    	//disable pull-up mode
    	io_conf.pull_up_en = 0;
    	//configure GPIO with the given settings
    	gpio_config(&io_conf);

    	//interrupt of rising edge
    	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    	//set as input mode    
    	io_conf.mode = GPIO_MODE_INPUT;
    	//enable pull-up mode
    	io_conf.pull_up_en = 1;
    	gpio_config(&io_conf);

    	//change gpio intrrupt type for one pin
    	gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);
	gpio_set_intr_type(GPIO_INPUT_IO_1, GPIO_INTR_ANYEDGE);
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


	//ESP_LOGI(TAG, "Initializing SD card");
	printf("initializing card" );
	#ifndef USE_SPI_MODE
		//ESP_LOGI(TAG, "Using SDMMC peripheral");
		printf("Using SDMMC peripheral");
		sdmmc_host_t host = SDMMC_HOST_DEFAULT();

		// To use 1-line SD mode, uncomment the following line:
		// host.flags = SDMMC_HOST_FLAG_1BIT;

		// This initializes the slot without card detect (CD) and write protect (WP) signals.
		// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
		sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

		// GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
		// Internal pull-ups are not sufficient. However, enabling internal pull-ups
		// does make a difference some boards, so we do that here.
		gpio_set_pull_mode(23, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
		gpio_set_pull_mode(19, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
		gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
		gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
		gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

	#else
		//ESP_LOGI(TAG, "Using SPI peripheral");
		printf("Using SPI peripheral");
		sdmmc_host_t host = SDSPI_HOST_DEFAULT();
		sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
		slot_config.gpio_miso = PIN_NUM_MISO;
		slot_config.gpio_mosi = PIN_NUM_MOSI;
		slot_config.gpio_sck  = PIN_NUM_CLK;
		slot_config.gpio_cs   = PIN_NUM_CS;
		// This initializes the slot without card detect (CD) and write protect (WP) signals.
		// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	#endif //USE_SPI_MODE

		// Options for mounting the filesystem.
		// If format_if_mount_failed is set to true, SD card will be partitioned and
		// formatted in case when mounting fails.
		esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5
	};

	// Use settings defined above to initialize SD card and mount FAT filesystem.
	// Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
	// Please check its source code and implement error recovery when developing
	// production applications.
	sdmmc_card_t* card;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
		//ESP_LOGE(TAG, "Failed to mount filesystem. "
		//  "If you want the card to be formatted, set format_if_mount_failed = true.");
		printf("failed to mount filesystem\n");
		} else {
			//ESP_LOGE(TAG, "Failed to initialize the card (%d). "
			//  "Make sure SD card lines have pull-up resistors in place.", ret);
			printf("failed to initialize\n");
		}
		return;
	}

	// Card has been initialized, print its properties
	sdmmc_card_print_info(stdout, card);

	// Use POSIX and C standard library functions to work with files.
	// First create a file.
	//ESP_LOGI(TAG, "Opening file");
	printf("Opening file\n");
	FILE* f = fopen("/sdcard/hello.txt", "w");
	if (f == NULL) {
		printf("failed to open file\n");
		//ESP_LOGE(TAG, "Failed to open file for writing");
		return;
	}
	fprintf(f, "Hello %s!\n", card->cid.name);
	fprintf(f, "THIS IS SOME GPS DATA\n");
	fclose(f);
	//ESP_LOGI(TAG, "File written");
	printf("file written");
	// Check if destination file exists before renaming
	struct stat st;
	if (stat("/sdcard/foo.txt", &st) == 0) {
		// Delete it if it exists
		unlink("/sdcard/foo.txt");
	}

	// Rename original file
	//ESP_LOGI(TAG, "Renaming file");
	printf("renaming file\n");
	if (rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0) {
		//ESP_LOGE(TAG, "Rename failed");
		printf("rename failed\n");
		return;
	}


	// Open renamed file for reading
	//ESP_LOGI(TAG, "Reading file");
	printf("reading file\n");
	f = fopen("/sdcard/foo.txt", "r");
	if (f == NULL) {
		//ESP_LOGE(TAG, "Failed to open file for reading");
		printf("failed to open file for reading\n");
		return;
	}
	char line[64];
	while(fgets(line, sizeof(line), f) != NULL) {
		printf("%s",line);
	}
	fclose(f);

	//ESP_LOGI(TAG, "Read from file: '%s'", line);
	printf("read from file");
	// All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	//ESP_LOGI(TAG, "Card unmounted");
	printf("card unmounted\n");
}
