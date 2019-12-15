#ifndef __CAM_EEPROM_DATA_H__
#define __CAM_EEPROM_DATA_H__

#if defined(CONFIG_PRODUCT_HEART)
#define EEPROM_CALIBRATION_STORE_CAM_MAX_ID 0

#define  EEPROM_ONE_BURST_SIZE   1
#define  EEPROM_POWER_MAX_SIZE 8
#define  EEPROM_DEV_NAME_MAX_SIZE 12
/*Modify following data according to eeprom*/
#define  CALIBRATION_DATA_SIZE  2048
#define  START_REGADDR_M 0x00A7
#define  SLAVE_ADDR_M (0xA0 >> 1)
#define  START_REGADDR_S 0x00A7
#define  SLAVE_ADDR_S (0xA0 >> 1)
#define  EEPROM_MASTER_CRC_ADDR_H 0x08A7
#define  EEPROM_MASTER_CRC_ADDR_L 0x08A8
#define  EEPROM_SLAVE_CRC_ADDR_H 0x08A7
#define  EEPROM_SLAVE_CRC_ADDR_L 0x08A8
#define  EEPROM_WRITE_PROTECT_ADDR 0x8000
/*value*/
#define  EEPROM_WRITE_PROTECT_ENABLE 0x0E
#define  EEPROM_WRITE_PROTECT_DISABLE 0x00

#define RECALIBRATION_FILE_NAME_MASTER "/data/vendor/camera/calibration_new.bin"
#define RECALIBRATION_FILE_NAME_SLAVE "/data/vendor/camera/calibration_slave_new.bin"

#elif defined(CONFIG_PRODUCT_ZIPPO)

#define EEPROM_CALIBRATION_STORE_CAM_MAX_ID 0

#define  EEPROM_ONE_BURST_SIZE   1
#define  EEPROM_POWER_MAX_SIZE 8
#define  EEPROM_DEV_NAME_MAX_SIZE 12
/*Modify following data according to eeprom*/

#define  CALIBRATION_DATA_SIZE  2048
#define  EEPROM_DATA_SIZE 8192
#define  EEPROM_REG_PAGE_SIZE 1024
#define  SLAVE_ADDR_M (0xB0 >> 1)
#define  SLAVE_ADDR_VCM (0x18 >> 1)
#define  START_REGADDR    0x0000
#define  START_REGADDR_M1 0x00C6
#define  START_REGADDR_M2 0x13A5
#define  START_REGADDR_M1_OFFSET 198
#define  START_REGADDR_M2_OFFSET 5029

#define  EEPROM_MASTER1_CRC_ADDR_H 0x08C6
#define  EEPROM_MASTER1_CRC_ADDR_L 0x08C7
#define  EEPROM_MASTER2_CRC_ADDR_H 0x1BA5
#define  EEPROM_MASTER2_CRC_ADDR_L 0x1BA6

#define  EEPROM_WRITE_PROTECT_ADDR 0x02
/*value*/
#define  EEPROM_WRITE_PROTECT_ENABLE 0x10
#define  EEPROM_WRITE_PROTECT_DISABLE 0x00

#define RECALIBRATION_FILE_NAME_MASTER1 "/data/vendor/camera/cal_w48_u16_new.bin"
#define RECALIBRATION_FILE_NAME_MASTER2 "/data/vendor/camera/cal_w48_t8_new.bin"
#define EEPROM_DATA_FILE_NAME_MASTER1 "/data/vendor/camera/main_cam_eeprom.bin"
#define EEPROM_DATA_REWRITE_FILE_NAME_MASTER1 "/data/vendor/camera/rewrite_main_cam_eeprom.bin"
#endif
#endif
