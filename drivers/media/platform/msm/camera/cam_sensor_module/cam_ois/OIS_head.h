/////////////////////////////////////////////////////////////////////////////
// File Name	: OIS_head.h
// Function		: Header file
// Rule         : Use TAB 4
// 
// Copyright(c)	Lenovo Co.,Ltd. All rights reserved 
// 
/***** Lenovo Confidential ***************************************************/
#ifndef OIS_MAIN_H
#define OIS_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* return value */
#define		ADJ_OK						 0
#define		ADJ_ERR					-1

#define		FW_DL_ERR					-2
#define		PROG_DL_ERR				-3
#define		COEF_DL_ERR				-4
#define		OIS_I2C_ERR				-5
#define		OIS_SUM_ERR				-6
#define		OIS_INIT_SERVO_ON_ERR	-7
#define		OIS_INIT_GYRO_SET_ERR	-8
#define		OIS_INIT_GYRO_ON_ERR		-9
#define		OIS_INIT_LN_COMP_ERR		-10
#define		OIS_INIT_OISMODE_ERR		-11
#define		OIS_INIT_OIS_ON_ERR		-12

typedef		short int					ADJ_STS;

typedef		char						OIS_BYTE;
typedef		short int					OIS_WORD;
typedef		int					       OIS_INT;
typedef		long int					OIS_LONG;
typedef		unsigned char				OIS_UBYTE;
typedef		unsigned short int			OIS_UWORD;
typedef		unsigned int			       OIS_UINT;
typedef		unsigned long int			OIS_ULONG;

typedef		volatile char				OIS_vBYTE;
typedef		volatile short int			OIS_vWORD;
typedef		volatile long int			OIS_vLONG;
typedef		volatile unsigned char		OIS_vUBYTE;
typedef		volatile unsigned short int	OIS_vUWORD;
typedef		volatile unsigned long int	OIS_vULONG;

#include	"OIS_defi.h"

extern OIS_INT OIS_IIC_Check(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_Reg_Check(struct camera_io_master* ois_io_master_info, uint32_t addr);
extern OIS_INT OIS_Start_DL(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_SUM_Check(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_Complete_DL(struct camera_io_master* ois_io_master_info);
extern void I2C_OIS_Single_write(struct camera_io_master* ois_io_master_info, OIS_UINT addr, OIS_UINT dat);
extern void I2C_OIS_Multi_write(struct camera_io_master* ois_io_master_info, OIS_UINT addr, OIS_UWORD size, OIS_UBYTE *dat );
extern uint32_t I2C_OIS_Single_read(struct camera_io_master* ois_io_master_info, OIS_UBYTE slvadr, OIS_UBYTE size, OIS_UINT dat );
extern OIS_INT OIS_FW_Download(struct camera_io_master* ois_io_master_info);
extern void func_download(struct camera_io_master* ois_io_master_info, OIS_UBYTE u16_type);
extern OIS_INT OIS_Calibration_Download( struct camera_io_master* ois_io_master_info );
extern void func_FADJ_Calibration_data_from_eeprom( struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_init_data( struct camera_io_master* ois_io_master_info ) ;
extern OIS_INT OIS_init_servo_on(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_init_gyro_setting(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_init_gyro_on(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_init_ois_mode(struct camera_io_master* ois_io_master_info);
extern OIS_INT OIS_init_ois_on(struct camera_io_master* ois_io_master_info);
extern OIS_INT cam_ois_fw_init(struct camera_io_master *ois_io_master_info);
extern void OIS_Gyro_Hall_Check_STS(struct camera_io_master* ois_io_master_info);

#ifdef __cplusplus
}
#endif

#endif/* OIS_MAIN_H */
