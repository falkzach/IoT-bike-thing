#include "./i2c.h"
#include "./LSM9DS1_accel_gyro_register_map.h"
#include "./LSM9DS1_magnetic_register_map.h"

xSemaphoreHandle print_mux;

static void init_i2c()
{
        int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;
        i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = (gpio_num_t) I2C_EXAMPLE_MASTER_SDA_IO;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = (gpio_num_t) I2C_EXAMPLE_MASTER_SCL_IO;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;
        i2c_param_config( (i2c_port_t) i2c_master_port, &conf);
        i2c_driver_install( (i2c_port_t) i2c_master_port, conf.mode,
                        I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                        I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t i2c_who_am_i(i2c_port_t i2c_num, uint8_t* data)
{
        //WRITE COMMAND
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, GAS_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, GAS_WAI, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        int ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_FAIL) {
                return ret;
        }
        vTaskDelay(30 / portTICK_RATE_MS);

        //READ RESULT
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, GAS_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
        i2c_master_read_byte(cmd, data, NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_FAIL) {
                return ESP_FAIL;
        }
        return ESP_OK;
}

static void i2c_task_who_am_i(void* arg)
{
        init_i2c();
        int ret;
        uint32_t task_idx = (uint32_t) arg;
        uint8_t data;

	while (1) {
                ret = i2c_who_am_i( I2C_EXAMPLE_MASTER_NUM, &data);
                xSemaphoreTake(print_mux, portMAX_DELAY);
                printf("\n\n*******************\n");
                printf("MASTER READ SENSOR( MPL3115A2 ) WHO_AM_I \n");
                if (ret == ESP_OK) {
                        printf("data: %02x\n", data);
                } else {
                        printf("No ack, sensor not connected...skip...\n");
                }
                printf("*******************\n");
                xSemaphoreGive(print_mux);
                vTaskDelay(( DELAY_TIME_BETWEEN_ITEMS_MS * ( task_idx + 1 ) ) / portTICK_RATE_MS);
        }
}
