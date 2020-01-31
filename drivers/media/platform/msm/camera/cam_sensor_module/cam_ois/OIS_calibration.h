/////////////////////////////////////////////////////////////////////////////
// File Name	: OIS_calibration.h
// Function		: Header file for OIS calibration data
// Rule         : Use TAB 4
//
// Copyright(c)	Lenovo Co.,Ltd. All rights reserved
//
/***** Lenovo Confidential ***************************************************/
#ifndef OIS_CALIBRATION_H
#define OIS_CALIBRATION_H

OIS_UBYTE DOWNLOAD_CALIBRATION[] = {
0xff,
0x55,
0xff,
0x43,
0x05,
0x02,
0xca,
0x01,
0x9f,
0xff,
0x2b,
0xff,
0x1b,
0x00,
0x1e,
0x00,
0xe5,
0x07,
0x1f,
0x08,
0xff,
0x71,
0x3b,
0x1a,
0xb8,
0x1a,
0xf6,
0x36,
0x0a,
0xc9,
0x00,
0x20,
0x00,
0x20,
0x00,
0x00,
0x00,
0x00,
};
/* 38 */

OIS_UWORD	DOWNLOAD_CALIBRATION_LEN = sizeof( DOWNLOAD_CALIBRATION)/sizeof(OIS_UBYTE);
#endif /* OIS_CALIBRATION_H */
