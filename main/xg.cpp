#include <Wire.h>
#include <SPI.h>
//#include "LSM9DS1/LSM9DS1.cpp"
#include "LSM9DS1_Arduino/SparkFunLSM9DS1.h"

#define DELAY_TIME_BETWEEN_ITEMS_MS   5000 /*!< delay time between different test items */

extern xSemaphoreHandle print_mux;

LSM9DS1 imu;

#define LSM9DS1_M	0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG	0x6B // Would be 0x6A if SDO_AG is LOW

// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -8.58 // Declination (degrees) in Boulder, CO.


#define PRINT_CALCULATED
//#define PRINT_RAW
#define PRINT_SPEED 250 // 250 ms between prints
static unsigned long lastPrint = 0; // Keep track of print time

static void xg_setup() {
    imu.settings.device.commInterface = IMU_MODE_I2C;
    imu.settings.device.mAddress = LSM9DS1_M;
    imu.settings.device.agAddress = LSM9DS1_AG;

    if (!imu.begin()) {
        Serial.println("Failed to communicate with LSM9DS1.");
        Serial.println("Double-check wiring.");
    }
}

static void xg_task(void* arg)
{

    int ret;
    uint32_t task_idx = (uint32_t) arg;
    uint8_t data;

    while (1)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("XG_TASK\n");
        xSemaphoreGive(print_mux);
        vTaskDelay(( DELAY_TIME_BETWEEN_ITEMS_MS * ( task_idx + 1 ) ) / portTICK_RATE_MS);
    }
}
