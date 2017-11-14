#pragma once
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_MOSI GPIO_NUM_23
#define PIN_NUM_CLK  GPIO_NUM_18
#define PIN_NUM_CS   GPIO_NUM_2

#define DELAY_TIME_BETWEEN_ITEMS_MS	5000 /*!< delay time between different test items */

extern xSemaphoreHandle print_mux;

/*
 *	initializes the sd card in SPI mode
 */
static sdmmc_card_t* init_sdcard() {
	
	printf("initializing card\n" );
	printf("Using SPI peripheral\n");
	print_mux = xSemaphoreCreateMutex();
	
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	slot_config.gpio_miso = PIN_NUM_MISO;
	slot_config.gpio_mosi = PIN_NUM_MOSI;
	slot_config.gpio_sck  = PIN_NUM_CLK;
	slot_config.gpio_cs   = PIN_NUM_CS;
	// This initializes the slot without card detect (CD) and write protect (WP) signals.
	// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.

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
		printf("failed to mount filesystem\n");
		} else {
			printf("failed to initialize\n");
		}
	}
	
	print_mux = xSemaphoreCreateMutex();
	
	return card;
}

/*
 *	writes a line to the data file
 */
static void write_line_to_file(char* filename, char* line) {

	FILE* f = fopen(filename, "a");

	if (f == NULL) {
		printf("failed to open file\n");
		print_mux = xSemaphoreCreateMutex();
		return;
	}

	fprintf(f, line);
	fclose(f);

}

/*
 *	reads the contents of the sd cards data file and prints to the console
 */
static void read_from_file(char* filename) {

	FILE* f = fopen(filename, "r");

	if (f == NULL) {
			return;
	}

	char line[64];

	while(fgets(line, sizeof(line), f) != NULL) {
		printf("%s\n",line);
	}
	
	print_mux = xSemaphoreCreateMutex();

	fclose(f);
	
}
	
/*
 *	unmounts the sd card
 */
static void unmount_sdcard() {

	esp_vfs_fat_sdmmc_unmount();

}
