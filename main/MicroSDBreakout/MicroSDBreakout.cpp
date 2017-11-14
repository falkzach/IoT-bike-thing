#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

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

/*
 *	initializes the sd card in SPI mode
 */
static void init_sdcard() {

	printf("initializing card\n" );
	#ifndef USE_SPI_MODE
		//ESP_LOGI(TAG, "Using SDMMC peripheral");
		printf("Using SDMMC peripheral\n");
		sdmmc_host_t host = SDMMC_HOST_DEFAULT();

		// To use 1-line SD mode, uncomment the following line:
		// host.flags = SDMMC_HOST_FLAG_1BIT;

		// This initializes the slot without card detect (CD) and write protect (WP) signals.
		// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
		sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

		// Internal pull-ups are not sufficient. However, enabling internal pull-ups
		// does make a difference some boards, so we do that here.
		gpio_set_pull_mode(23, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
		gpio_set_pull_mode(19, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
		gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

	#else
		printf("Using SPI peripheral\n");
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
		printf("failed to mount filesystem\n");
		} else {
			printf("failed to initialize\n");
		}
		return;
	}
	return;
}

/*
 *	writes a line to the data file
 */
static void write_line_to_file(char* filename, char* line) {

	FILE* f = fopen(filename, "w");

	if (f == NULL) {
		printf("failed to open file\n");
		return;
	}

	fprintf(f, line);
	fclose(f);
	printf("file written\n");

}

/*
 *	reads the contents of the sd cards data file and prints to the console
 */
static void read_from_file(char* filename) {

	FILE* f = fopen(filename, "r");

	if (f == NULL) {
		printf("failed to open file for reading\n");
		return;
	}

	char line[64];

	while(fgets(line, sizeof(line), f) != NULL) {
		printf("%s\n",line);
	}

	fclose(f);
	printf("read from file\n");

}
	
/*
 *	unmounts the sd card
 */
static void unmount_sdcard() {

	esp_vfs_fat_sdmmc_unmount();
	printf("card unmounted\n");

}
