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

        print_mux = xSemaphoreCreateMutex();
    }

}

// Read gps data from GP-20u7 gps.
static void read_gps(uint8_t *line) {
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
    char * rmc_line;
    //TODO: FIXME: http://www.freertos.org/a00111.html mallocs are not advised!
    rmc_line = (char *)malloc(256);

    while (1) {
        read_gps((uint8_t *)rmc_line);
        parse_gps_rmc(rmc_line);

        vTaskDelay(( DELAY_TIME_BETWEEN_ITEMS_MS * ( task_idx + 1 ) ) / portTICK_RATE_MS);
    }
}
