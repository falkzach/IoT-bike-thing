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
#include "driver/uart.h"
#include "minmea.h"

#include "./LSM9DS1_accel_gyro_register_map.hpp"
#include "./LSM9DS1_magnetic_register_map.hpp"

// The GP-20u7 unit is only used to recieve data from the satellite.
#define GPS_TX_ESP_RX GPIO_NUM_16
#define GPS_UART_NUM UART_NUM_2
#define BUF_SIZE (2048)

void parse_gps_rmc(const char * line) {

	switch (minmea_sentence_id(line, false)) {
		case MINMEA_SENTENCE_RMC: {
			struct minmea_sentence_rmc rmc_frame;
			if(minmea_parse_rmc(&rmc_frame, line)) {
				printf("RMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",rmc_frame.latitude.value,rmc_frame.latitude.scale,rmc_frame.longitude.value,rmc_frame.longitude.scale,rmc_frame.speed.value,rmc_frame.speed.scale);
			}
			break;
		}
		case MINMEA_SENTENCE_GLL: {
			printf("GLL SENTENCE\n");
			break;
		}
		case MINMEA_SENTENCE_VTG: {
			printf("VTG SENTENCE\n");
			break;
		}
		case MINMEA_SENTENCE_ZDA: {
			printf("ZDA SENTENCE\n");
			break;
		}
		case MINMEA_SENTENCE_GGA: {
			printf("GGA SENTENCE\n");
			break;
		}
		case MINMEA_SENTENCE_GSA: {
			printf("GSA SENTENCE\n");
			break;
		}
		case MINMEA_UNKNOWN: {
			printf("UNKNOWN SENTENCE\n");
			break;
		}
		case MINMEA_INVALID: {
			printf("INVALID SENTENCE\n");
			break;
		}
		default: {
			printf("NONE\n");
			break;
		}
	}

}

// Read gps data from GP-20u7 gps.
void read_gps(uint8_t *line) {
  int size; unsigned int count = 0;
	uint8_t * ptr = line;
	do {
		size = uart_read_bytes(GPS_UART_NUM, ptr, 1, 1000/portMAX_DELAY);
    printf("%d\n",size);
			if (*ptr == '\n') {
				ptr++;
				*ptr = 0;
				break;
			}
			ptr++;
			++count;
  } while (size > 0 || count > 256);
}

// init for the GP-20u7 gps
void gps_init() {

	uart_config_t bikeGpsConfig;

	bikeGpsConfig.baud_rate           = 9600;
	bikeGpsConfig.data_bits           = UART_DATA_8_BITS;
	bikeGpsConfig.parity              = UART_PARITY_DISABLE;
	bikeGpsConfig.stop_bits           = UART_STOP_BITS_1;
	bikeGpsConfig.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
	bikeGpsConfig.rx_flow_ctrl_thresh = 120;

	uart_param_config(GPS_UART_NUM, &bikeGpsConfig);

	uart_set_pin(GPS_UART_NUM, UART_PIN_NO_CHANGE, GPS_TX_ESP_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	uart_driver_install(GPS_UART_NUM, BUF_SIZE, BUF_SIZE, 10, NULL, 0);

}

void app_main()
{
    gps_init();

      char * rmc_line;
	rmc_line = (char *)malloc(256);

  while(1) {
		read_gps((uint8_t *)rmc_line);
		parse_gps_rmc(rmc_line);
		read_9dof()
  }
}
