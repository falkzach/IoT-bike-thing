/*
 *	LSM9DS1 Magnetic sensor register address map
 *	Source: Table 22
 *	https://cdn.sparkfun.com/assets/learn_tutorials/3/7/3/LSM9DS1_Datasheet.pdf
 *
 *
 *	Note: regex for defines from pdf copy pasta
 *	([A-Z0-9_]*)\s(?:[a-z/\-]*)\s([A-F0-9]*)\s.(?:[0-1]*)\s.*
 */

 #define MAG_ADDR 0x1E
 #define MAG_WAI 0x0F

#define OFFSET_X_REG_L_M 0x05
#define OFFSET_X_REG_H_M 0x06
#define OFFSET_Y_REG_H_M 0x08
#define OFFSET_Z_REG_H_M 0x0A
#define WHO_AM_I_M 0x0F	//0x3D
#define CTRL_REG1_M 0x20
#define CTRL_REG2_M 0x21
#define CTRL_REG3_M 0x22
#define CTRL_REG4_M 0x23
#define CTRL_REG5_M 0x24
#define STATUS_REG_M 0x27
#define OUT_X_L_M 0x28
#define OUT_X_H_M 0x29
#define OUT_Y_L_M 0x2A
#define OUT_Y_H_M 0x2B
#define OUT_Z_L_M 0x2C
#define OUT_Z_H_M 0x2D
#define INT_CFG_M 0x30
#define INT_SRC_M 0x31
#define INT_THS_L_M 0x32
#define INT_THS_H_M 0x33
