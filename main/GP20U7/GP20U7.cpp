#include "driver/uart.h"
#include "../minmea.h"
#include "esp_system.h"

// The GP-20u7 unit is only used to recieve data from the satellite.
#define GPS_TX_ESP_RX GPIO_NUM_16
#define GPS_UART_NUM UART_NUM_2
#define BUF_SIZE (2048)

#define DELAY_TIME_BETWEEN_ITEMS_MS   5000 /*!< delay time between different test items */

extern xSemaphoreHandle print_mux;

static void parse_gps_rmc(const char * line) {

    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc rmc_frame;
            if(minmea_parse_rmc(&rmc_frame, line)) {
                xSemaphoreTake(print_mux, portMAX_DELAY);
		printf("RMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",rmc_frame.latitude.value,rmc_frame.latitude.scale,rmc_frame.longitude.value,rmc_frame.longitude.scale,rmc_frame.speed.value,rmc_frame.speed.scale);
            	xSemaphoreGive(print_mux);
	    }
            break;
        }
        case MINMEA_SENTENCE_GLL: {
            break;
        }
        case MINMEA_SENTENCE_VTG: {
            break;
        }
        case MINMEA_SENTENCE_ZDA: {
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            break;
        }
        case MINMEA_SENTENCE_GSA: {
            break;
        }
        case MINMEA_UNKNOWN: {
            break;
        }
        case MINMEA_INVALID: {
            break;
        }
        default: {
            break;
        }

    }

}

// Read gps data from GP-20u7 gps.
char *read_gps() {
	static char line[256];
	int size;
	char *ptr = line;
	while(1) {
		size = uart_read_bytes(GPS_UART_NUM, (unsigned char *)ptr, 1, portMAX_DELAY);
		if (size == 1) {
			if (*ptr == '\n') {
				ptr++;
				*ptr = 0;
				return line;
			}
			ptr++;
		} // End of read a character
	} // End of loop
} // End of readLine

// init for the GP-20u7 gps
static void GP20U7_init() {

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

static void GP20U7_task(void* arg)
{
    uint32_t task_idx = (uint32_t) arg;
	char *rmc_line;
    while (1) {
        rmc_line = read_gps();
        parse_gps_rmc(rmc_line);

        vTaskDelay(( DELAY_TIME_BETWEEN_ITEMS_MS * ( task_idx + 1 ) ) / portTICK_RATE_MS);
    }
}
