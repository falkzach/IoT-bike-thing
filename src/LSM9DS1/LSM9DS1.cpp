#include "i2c.h"
#include "LSM9DS1_xg_registers.h"
#include "LSM9DS1_mag_registers.h"
#include "LSM9DS1.h"
#include "LSM9DS1_types.h"

extern xSemaphoreHandle print_mux;

#define DELAY_TIME_BETWEEN_ITEMS_MS   5000 /*!< delay time between different test items */

static void init_i2c()
{
	int i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = (gpio_num_t) I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = (gpio_num_t) I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	i2c_param_config( (i2c_port_t) i2c_master_port, &conf);
	i2c_driver_install( (i2c_port_t) i2c_master_port, conf.mode,
	I2C_MASTER_RX_BUF_DISABLE,
	I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t xg_write_i2c(uint8_t* data, size_t data_len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, XG_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write(cmd, data, data_len, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	int ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_FAIL) {
		return ret;
	}
	return ESP_OK;
}

static esp_err_t xg_read_i2c(uint8_t address, uint8_t* data, uint8_t count)//TODO: use count?
{
	int ret;
	uint8_t d[1];
	d[0] = address;
	ret = xg_write_i2c(d, 1);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, XG_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, data, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}
	return ESP_OK;
}

static void enableFIFO(bool enable)
{
	int res;
	uint8_t data[1];

	xg_read_i2c(CTRL_REG9, data,1);
	uint8_t temp = data[0];
	if (enable) temp |= (1<<1);
	else temp &= ~(1<<1);

	data[1] = CTRL_REG9;
	data[2] = temp;
	res = xg_write_i2c(data, 2);
}

static esp_err_t setFIFO(FIFOMode_t fifoMode, uint8_t fifoThs)
{
	int ret;
	uint8_t data[2];

	uint8_t threshold = fifoThs <= 0x1F ? fifoThs : 0x1F;
	data[0] = FIFO_CTRL;
	data[1] = ((fifoMode & 0x7) << 5) | (threshold & 0x1F);
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	return ESP_OK;
}

static esp_err_t init_gyro()
{
	int ret;
	uint8_t tmp_reg = 0;
	uint8_t data[2];

	tmp_reg = (6 & 0x07) << 5;
	// gyro scale can be 245, 500, or 2000, 0x01 for 245
	tmp_reg |= (0x1 << 3);

	tmp_reg |= (0 & 0x3);


	data[0] = CTRL_REG1_G;
	data[1] = tmp_reg;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	data[0] = CTRL_REG2_G;
	data[1] = 0x00;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	data[0] = CTRL_REG3_G;
	data[1] = 0x00;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	tmp_reg = 0;
	tmp_reg |= (1<<5);// enable Z
	tmp_reg |= (1<<4);// enable Y
	tmp_reg |= (1<<3);// enable Z
	// tmp_reg |= (1<<1);//enable latch interupt

	data[0] = CTRL_REG4;
	data[1] = tmp_reg;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	tmp_reg = 0;
	tmp_reg |= (1<<5);// flip X
	tmp_reg |= (1<<4);// flip Y
	tmp_reg |= (1<<3);// flip Z

	data[0] = ORIENT_CFG_G;
	data[1] = tmp_reg;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	return ESP_OK;
}

static esp_err_t init_accel()
{
	int ret;
	uint8_t tmp_reg = 0;
	uint8_t data[2];

	tmp_reg |= (1<<5);// enable Z
	tmp_reg |= (1<<4);// enable Y
	tmp_reg |= (1<<3);// enable Z

	data[0] = CTRL_REG5_XL;
	data[1] = tmp_reg;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	tmp_reg = 0;

	data[0] = CTRL_REG6_XL;
	data[1] = tmp_reg;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	tmp_reg = 0;

	data[0] = CTRL_REG7_XL;
	data[1] = tmp_reg;
	ret = xg_write_i2c(data, 2);
	if (ret == ESP_FAIL) {
		return ESP_FAIL;
	}

	return ESP_OK;
}

static esp_err_t calibrate()
{
	uint8_t data[6] = {0, 0, 0, 0, 0, 0};
	uint8_t samples = 0;
	int ii;
	int32_t aBiasRawTemp[3] = {0, 0, 0};
	int32_t gBiasRawTemp[3] = {0, 0, 0};

	// Turn on FIFO and set threshold to 32 samples
	enableFIFO(true);
	setFIFO(FIFO_THS, 0x1F);

	//TODO: finish implementing
//	while (samples < 0x1F)
//	{
//		xg_read_i2c(FIFO_SRC);
//	}

	enableFIFO(false);
	setFIFO(FIFO_OFF, 0x00);

	return ESP_OK;
}

static void LSM9DS1_init()
{
	init_gyro();
	init_accel();
	calibrate();
}

static esp_err_t i2c_who_am_i(i2c_port_t i2c_num, uint8_t* data)
{
	//WRITE COMMAND
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, XG_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, WHO_AM_I, ACK_CHECK_EN);
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
	i2c_master_write_byte(cmd, XG_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
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

	int ret;
	uint32_t task_idx = (uint32_t) arg;
	uint8_t data;

	while (1)
	{
		ret = i2c_who_am_i( I2C_MASTER_NUM, &data);
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

//static void i2c_task_LSM9DS1(void* arg)
//{
//
//}
