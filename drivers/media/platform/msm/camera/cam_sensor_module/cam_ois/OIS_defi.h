/////////////////////////////////////////////////////////////////////////////
// File Name	: OIS_defi.h
// Function		: Header file for OIS controller
// Rule         : Use TAB 4
//
// Copyright(c)	Lenovo Co.,Ltd. All rights reserved 
// 
/***** Lenovo Confidential ***************************************************/
#ifndef OIS_DEFINITION_H
#define OIS_DEFINITION_H

// =================================================================
// OIS controller assign
// =================================================================
/* Slave Address */
/* 0x7C>>1 = 0x3E */
#define _SLV_OIS_SENSOR	0x3E
/* 0xb0>>1 = 0x58 */
#define _SLV_OIS_EEPROM	0x58

/* ois sensor addr */
#define _OIS_I2C_CHECK_STS 		0x6024
#define _OIS_GYRO_YOUT 		0x6042
#define _OIS_GYRO_XOUT 		0x6044
#define _OIS_HALL_X 				0x6058
#define _OIS_HALL_Y 				0x6059
#define _OIS_START_DL 			0xF010
#define _OIS_SUM_CHECK			0xF008
#define _OIS_COMP_DL			0xF006
#define _OIS_FW_DL_PROG_ADDR	0x0000
#define _OIS_FW_DL_COEF_ADDR	0x1C00
#define _OIS_CAL_DATA_DL_ADDR	0x1DC0

#define _OIS_SERVO_ON_6020H 	0x6020
#define _OIS_GYRO_SET_6023H 	0x6023
#define _OIS_GYRO_SET_602CH 	0x602c
#define _OIS_GYRO_SET_602DH 	0x602d
#define _OIS_LN_COMP_614FH 	0x614f
#define _OIS_OIS_MODE_6021H 	0x6021
#define _OIS_OIS_ON_6020H 		0x6020

/* read data value */
#define		OIS_I2C_CHECK_VALUE			0x01
#define		OIS_SUM_CHECK_VALUE		0x0002D385

/* Factory Eeprom data */
#define OIS_CAL_DATA_SIZE 38
#define OSCILLATOR_OFFSET 21
#define OIS_CAL_DATA_ADDR_START 5029

#define OIS_RETRY_NUMBER 100

typedef struct{
	OIS_UBYTE	ois_cal_reserved_0;
	OIS_UBYTE	ois_cal_hall_current_x;
	OIS_UBYTE	ois_cal_reserved_1;
	OIS_UBYTE	ois_cal_hall_current_y;
	OIS_UBYTE	ois_cal_hall_offset_x_hb;
	OIS_UBYTE	ois_cal_hall_offset_x_lb;
	OIS_UBYTE	ois_cal_hall_offset_y_hb;
	OIS_UBYTE	ois_cal_hall_offset_y_lb;
	OIS_UBYTE	ois_cal_gyro_offset_x_hb;
	OIS_UBYTE	ois_cal_gyro_offset_x_lb;
	OIS_UBYTE	ois_cal_gyro_offset_y_hb;
	OIS_UBYTE	ois_cal_gyro_offset_y_lb;
	OIS_UBYTE	ois_cal_servo_loop_gain_x_hb;
	OIS_UBYTE	ois_cal_servo_loop_gain_x_lb;
	OIS_UBYTE	ois_cal_servo_loop_gain_y_hb;
	OIS_UBYTE	ois_cal_servo_loop_gain_y_lb;
	OIS_UBYTE	ois_cal_optical_offset_x_hb;
	OIS_UBYTE	ois_cal_optical_offset_x_lb;
	OIS_UBYTE	ois_cal_optical_offset_y_hb;
	OIS_UBYTE	ois_cal_optical_offset_y_lb;
	OIS_UBYTE	ois_cal_reserved_2;
	OIS_UBYTE	ois_cal_oscillator;
	OIS_UBYTE	ois_cal_gyro_gain_x_hb;
	OIS_UBYTE	ois_cal_gyro_gain_x_lb;
	OIS_UBYTE	ois_cal_gyro_gain_y_hb;
	OIS_UBYTE	ois_cal_gyro_gain_y_lb;
	OIS_UBYTE	ois_cal_gyro_sensitivity_x_hb;
	OIS_UBYTE	ois_cal_gyro_sensitivity_x_lb;
	OIS_UBYTE	ois_cal_gyro_sensitivity_y_hb;
	OIS_UBYTE	ois_cal_gyro_sensitivity_y_lb;
	OIS_UBYTE	ois_cal_gyro_mix_1_hb;
	OIS_UBYTE	ois_cal_gyro_mix_1_lb;
	OIS_UBYTE	ois_cal_gyro_mix_2_hb;
	OIS_UBYTE	ois_cal_gyro_mix_2_lb;
	OIS_UBYTE	ois_cal_gyro_mix_3_hb;
	OIS_UBYTE	ois_cal_gyro_mix_3_lb;
	OIS_UBYTE	ois_cal_gyro_mix_4_hb;
	OIS_UBYTE	ois_cal_gyro_mix_4_lb;
	OIS_UBYTE	ois_cal_reserved_3;
	OIS_UBYTE	ois_cal_reserved_4;
}_FACT_ADJ;

#endif/* OIS_DEFINITION_H */
