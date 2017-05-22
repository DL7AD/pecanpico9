/**
  * This is the OV2640 driver
  * I2C configuring concept has been taken from
  * https://github.com/iqyx/ov2640-stm32/blob/master/F4discovery/main.c
  */

/*
 * ov2640 Camera Driver
 *
 * Copyright (C) 2010 Alberto Panizzo <maramaopercheseimorto@gmail.com>
 *
 * Based on ov772x, ov9640 drivers and previous non merged implementations.
 *
 * Copyright 2005-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006, OmniVision
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ch.h"
#include "hal.h"
#include "ov2640.h"
#include "pi2c.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_dcmi.h"
#include "board.h"
#include "defines.h"
#include "debug.h"
#include <string.h>

#define OV2640_I2C_ADR			0x30
#define DCMI_BASE_ADR			((uint32_t)0x50050000)
#define DCMI_REG_DR_OFFSET		0x28
#define DCMI_REG_DR_ADDRESS		(DCMI_BASE_ADR | DCMI_REG_DR_OFFSET)


#define VAL_SET(x, mask, rshift, lshift)  \
		((((x) >> rshift) & mask) << lshift)
/*
 * DSP registers
 * register offset for BANK_SEL == BANK_SEL_DSP
 */
#define R_BYPASS					0x05 /* Bypass DSP */
#define R_BYPASS_DSP_BYPAS			0x01 /* Bypass DSP, sensor out directly */
#define   R_BYPASS_USE_DSP			0x00 /* Use the internal DSP */
#define QS							0x44 /* Quantization Scale Factor */
#define CTRLI						0x50
#define   CTRLI_LP_DP				0x80
#define   CTRLI_ROUND				0x40
#define   CTRLI_V_DIV_SET(x)		VAL_SET(x, 0x3, 0, 3)
#define   CTRLI_H_DIV_SET(x)		VAL_SET(x, 0x3, 0, 0)
#define HSIZE						0x51 /* H_SIZE[7:0] (real/4) */
#define   HSIZE_SET(x)				VAL_SET(x, 0xFF, 2, 0)
#define VSIZE						0x52 /* V_SIZE[7:0] (real/4) */
#define   VSIZE_SET(x)				VAL_SET(x, 0xFF, 2, 0)
#define XOFFL						0x53 /* OFFSET_X[7:0] */
#define   XOFFL_SET(x)				VAL_SET(x, 0xFF, 0, 0)
#define YOFFL						0x54 /* OFFSET_Y[7:0] */
#define   YOFFL_SET(x)				VAL_SET(x, 0xFF, 0, 0)
#define VHYX						0x55 /* Offset and size completion */
#define   VHYX_VSIZE_SET(x)			VAL_SET(x, 0x1, (8+2), 7)
#define   VHYX_HSIZE_SET(x)			VAL_SET(x, 0x1, (8+2), 3)
#define   VHYX_YOFF_SET(x)			VAL_SET(x, 0x3, 8, 4)
#define   VHYX_XOFF_SET(x)			VAL_SET(x, 0x3, 8, 0)
#define DPRP						0x56
#define TEST						0x57 /* Horizontal size completion */
#define   TEST_HSIZE_SET(x)			VAL_SET(x, 0x1, (9+2), 7)
#define ZMOW						0x5A /* Zoom: Out Width  OUTW[7:0] (real/4) */
#define   ZMOW_OUTW_SET(x)			VAL_SET(x, 0xFF, 2, 0)
#define ZMOH						0x5B /* Zoom: Out Height OUTH[7:0] (real/4) */
#define   ZMOH_OUTH_SET(x)			VAL_SET(x, 0xFF, 2, 0)
#define ZMHH						0x5C /* Zoom: Speed and H&W completion */
#define   ZMHH_ZSPEED_SET(x)		VAL_SET(x, 0x0F, 0, 4)
#define   ZMHH_OUTH_SET(x)			VAL_SET(x, 0x1, (8+2), 2)
#define   ZMHH_OUTW_SET(x)			VAL_SET(x, 0x3, (8+2), 0)
#define BPADDR						0x7C /* SDE Indirect Register Access: Address */
#define BPDATA						0x7D /* SDE Indirect Register Access: Data */
#define CTRL2						0x86 /* DSP Module enable 2 */
#define   CTRL2_DCW_EN				0x20
#define   CTRL2_SDE_EN				0x10
#define   CTRL2_UV_ADJ_EN			0x08
#define   CTRL2_UV_AVG_EN			0x04
#define   CTRL2_CMX_EN				0x01
#define CTRL3						0x87 /* DSP Module enable 3 */
#define   CTRL3_BPC_EN				0x80
#define   CTRL3_WPC_EN				0x40
#define SIZEL						0x8C /* Image Size Completion */
#define   SIZEL_HSIZE8_11_SET(x)	VAL_SET(x, 0x1, 11, 6)
#define   SIZEL_HSIZE8_SET(x)		VAL_SET(x, 0x7, 0, 3)
#define   SIZEL_VSIZE8_SET(x)		VAL_SET(x, 0x7, 0, 0)
#define HSIZE8						0xC0 /* Image Horizontal Size HSIZE[10:3] */
#define   HSIZE8_SET(x)				VAL_SET(x, 0xFF, 3, 0)
#define VSIZE8						0xC1 /* Image Vertical Size VSIZE[10:3] */
#define   VSIZE8_SET(x)				VAL_SET(x, 0xFF, 3, 0)
#define CTRL0						0xC2 /* DSP Module enable 0 */
#define   CTRL0_AEC_EN				0x80
#define   CTRL0_AEC_SEL				0x40
#define   CTRL0_STAT_SEL			0x20
#define   CTRL0_VFIRST				0x10
#define   CTRL0_YUV422				0x08
#define   CTRL0_YUV_EN				0x04
#define   CTRL0_RGB_EN				0x02
#define   CTRL0_RAW_EN				0x01
#define CTRL1						0xC3 /* DSP Module enable 1 */
#define   CTRL1_CIP					0x80
#define   CTRL1_DMY					0x40
#define   CTRL1_RAW_GMA				0x20
#define   CTRL1_DG					0x10
#define   CTRL1_AWB					0x08
#define   CTRL1_AWB_GAIN			0x04
#define   CTRL1_LENC				0x02
#define   CTRL1_PRE					0x01
#define R_DVP_SP					0xD3 /* DVP output speed control */
#define   R_DVP_SP_AUTO_MODE		0x80
#define   R_DVP_SP_DVP_MASK			0x3F /* DVP PCLK = sysclk (48)/[6:0] (YUV0);
										  *          = sysclk (48)/(2*[6:0]) (RAW);*/
#define IMAGE_MODE					0xDA /* Image Output Format Select */
#define   IMAGE_MODE_Y8_DVP_EN		0x40
#define   IMAGE_MODE_JPEG_EN		0x10
#define   IMAGE_MODE_YUV422			0x00
#define   IMAGE_MODE_RAW10			0x04 /* (DVP) */
#define   IMAGE_MODE_RGB565			0x08
#define   IMAGE_MODE_HREF_VSYNC		0x02 /* HREF timing select in DVP JPEG output
										  * mode (0 for HREF is same as sensor) */
#define   IMAGE_MODE_LBYTE_FIRST	0x01 /* Byte swap enable for DVP
										   *    1: Low byte first UYVY (C2[4] =0)
										   *        VYUY (C2[4] =1)
										   *    0: High byte first YUYV (C2[4]=0)
										   *        YVYU (C2[4] = 1) */
#define RESET						0xE0 /* Reset */
#define   RESET_MICROC				0x40
#define   RESET_SCCB				0x20
#define   RESET_JPEG				0x10
#define   RESET_DVP					0x04
#define   RESET_IPU					0x02
#define   RESET_CIF					0x01
#define REGED						0xED /* Register ED */
#define   REGED_CLK_OUT_DIS			0x10
#define MS_SP						0xF0 /* SCCB Master Speed */
#define SS_ID						0xF7 /* SCCB Slave ID */
#define SS_CTRL						0xF8 /* SCCB Slave Control */
#define   SS_CTRL_ADD_AUTO_INC		0x20
#define   SS_CTRL_EN				0x08
#define   SS_CTRL_DELAY_CLK			0x04
#define   SS_CTRL_ACC_EN			0x02
#define   SS_CTRL_SEN_PASS_THR		0x01
#define MC_BIST						0xF9 /* Microcontroller misc register */
#define   MC_BIST_RESET				0x80 /* Microcontroller Reset */
#define   MC_BIST_BOOT_ROM_SEL		0x40
#define   MC_BIST_12KB_SEL			0x20
#define   MC_BIST_12KB_MASK			0x30
#define   MC_BIST_512KB_SEL			0x08
#define   MC_BIST_512KB_MASK		0x0C
#define   MC_BIST_BUSY_BIT_R		0x02
#define   MC_BIST_MC_RES_ONE_SH_W	0x02
#define   MC_BIST_LAUNCH			0x01
#define BANK_SEL					0xFF /* Register Bank Select */
#define   BANK_SEL_DSP				0x00
#define   BANK_SEL_SENS				0x01

/*
 * Sensor registers
 * register offset for BANK_SEL == BANK_SEL_SENS
 */
#define GAIN						0x00 /* AGC - Gain control gain setting */
#define COM1						0x03 /* Common control 1 */
#define   COM1_1_DUMMY_FR			0x40
#define   COM1_3_DUMMY_FR			0x80
#define   COM1_7_DUMMY_FR			0xC0
#define   COM1_VWIN_LSB_UXGA		0x0F
#define   COM1_VWIN_LSB_SVGA		0x0A
#define   COM1_VWIN_LSB_CIF			0x06
#define REG04						0x04 /* Register 04 */
#define   REG04_DEF					0x20 /* Always set */
#define   REG04_HFLIP_IMG			0x80 /* Horizontal mirror image ON/OFF */
#define   REG04_VFLIP_IMG			0x40 /* Vertical flip image ON/OFF */
#define   REG04_VREF_EN				0x10
#define   REG04_HREF_EN				0x08
#define   REG04_AEC_SET(x)			VAL_SET(x, 0x3, 0, 0)
#define REG08						0x08 /* Frame Exposure One-pin Control Pre-charge Row Num */
#define COM2						0x09 /* Common control 2 */
#define   COM2_SOFT_SLEEP_MODE		0x10 /* Soft sleep mode */
										 /* Output drive capability */
#define   COM2_OCAP_Nx_SET(N)		(((N) - 1) & 0x03) /* N = [1x .. 4x] */
#define PID							0x0A /* Product ID Number MSB */
#define VER							0x0B /* Product ID Number LSB */
#define COM3						0x0C /* Common control 3 */
#define   COM3_BAND_50H				0x04 /* 0 For Banding at 60H */
#define   COM3_BAND_AUTO			0x02 /* Auto Banding */
#define   COM3_SING_FR_SNAPSH		0x01 /* 0 For enable live video output after the
										  * snapshot sequence*/
#define AEC							0x10 /* AEC[9:2] Exposure Value */
#define CLKRC						0x11 /* Internal clock */
#define   CLKRC_EN					0x80
#define   CLKRC_DIV_SET(x)			(((x) - 1) & 0x1F) /* CLK = XVCLK/(x) */
#define COM7						0x12 /* Common control 7 */
#define   COM7_SRST					0x80 /* Initiates system reset. All registers are
										  * set to factory default values after which
										  * the chip resumes normal operation */
#define   COM7_RES_UXGA				0x00 /* Resolution selectors for UXGA */
#define   COM7_RES_SVGA				0x40 /* SVGA */
#define   COM7_RES_CIF				0x20 /* CIF */
#define   COM7_ZOOM_EN				0x04 /* Enable Zoom mode */
#define   COM7_COLOR_BAR_TEST		0x02 /* Enable Color Bar Test Pattern */
#define COM8						0x13 /* Common control 8 */
#define   COM8_DEF					0xC0 /* Banding filter ON/OFF */
#define   COM8_BNDF_EN				0x20 /* Banding filter ON/OFF */
#define   COM8_AGC_EN				0x04 /* AGC Auto/Manual control selection */
#define   COM8_AEC_EN				0x01 /* Auto/Manual Exposure control */
#define COM9						0x14 /* Common control 9
										  * Automatic gain ceiling - maximum AGC value [7:5]*/
#define   COM9_AGC_GAIN_2x			0x00 /* 000 :   2x */
#define   COM9_AGC_GAIN_4x			0x20 /* 001 :   4x */
#define   COM9_AGC_GAIN_8x			0x40 /* 010 :   8x */
#define   COM9_AGC_GAIN_16x			0x60 /* 011 :  16x */
#define   COM9_AGC_GAIN_32x			0x80 /* 100 :  32x */
#define   COM9_AGC_GAIN_64x			0xA0 /* 101 :  64x */
#define   COM9_AGC_GAIN_128x		0xC0 /* 110 : 128x */
#define COM10						0x15 /* Common control 10 */
#define   COM10_PCLK_HREF			0x20 /* PCLK output qualified by HREF */
#define   COM10_PCLK_RISE			0x10 /* Data is updated at the rising edge of
										  * PCLK (user can latch data at the next
										  * falling edge of PCLK).
										  * 0 otherwise. */
#define   COM10_HREF_INV			0x08 /* Invert HREF polarity:
										  * HREF negative for valid data*/
#define   COM10_VSINC_INV			0x02 /* Invert VSYNC polarity */
#define HSTART						0x17 /* Horizontal Window start MSB 8 bit */
#define HEND						0x18 /* Horizontal Window end MSB 8 bit */
#define VSTART						0x19 /* Vertical Window start MSB 8 bit */
#define VEND						0x1A /* Vertical Window end MSB 8 bit */
#define MIDH						0x1C /* Manufacturer ID byte - high */
#define MIDL						0x1D /* Manufacturer ID byte - low  */
#define AEW							0x24 /* AGC/AEC - Stable operating region (upper limit) */
#define AEB							0x25 /* AGC/AEC - Stable operating region (lower limit) */
#define VV							0x26 /* AGC/AEC Fast mode operating region */
#define   VV_HIGH_TH_SET(x)			VAL_SET(x, 0xF, 0, 4)
#define   VV_LOW_TH_SET(x)			VAL_SET(x, 0xF, 0, 0)
#define REG2A						0x2A /* Dummy pixel insert MSB */
#define FRARL						0x2B /* Dummy pixel insert LSB */
#define ADDVFL						0x2D /* LSB of insert dummy lines in Vertical direction */
#define ADDVFH						0x2E /* MSB of insert dummy lines in Vertical direction */
#define YAVG						0x2F /* Y/G Channel Average value */
#define REG32						0x32 /* Common Control 32 */
#define   REG32_PCLK_DIV_2			0x80 /* PCLK freq divided by 2 */
#define   REG32_PCLK_DIV_4			0xC0 /* PCLK freq divided by 4 */
#define ARCOM2						0x34 /* Zoom: Horizontal start point */
#define REG45						0x45 /* Register 45 */
#define FLL							0x46 /* Frame Length Adjustment LSBs */
#define FLH							0x47 /* Frame Length Adjustment MSBs */
#define COM19						0x48 /* Zoom: Vertical start point */
#define ZOOMS						0x49 /* Zoom: Vertical start point */
#define COM22						0x4B /* Flash light control */
#define COM25						0x4E /* For Banding operations */
#define BD50						0x4F /* 50Hz Banding AEC 8 LSBs */
#define BD60						0x50 /* 60Hz Banding AEC 8 LSBs */
#define REG5D						0x5D /* AVGsel[7:0],   16-zone average weight option */
#define REG5E						0x5E /* AVGsel[15:8],  16-zone average weight option */
#define REG5F						0x5F /* AVGsel[23:16], 16-zone average weight option */
#define REG60						0x60 /* AVGsel[31:24], 16-zone average weight option */
#define HISTO_LOW					0x61 /* Histogram Algorithm Low Level */
#define HISTO_HIGH					0x62 /* Histogram Algorithm High Level */

#define MANUFACTURER_ID				0x7FA2
#define PID_OV2640					0x2626
#define VERSION(pid, ver)			((pid << 8) | (ver & 0xFF))

struct regval_list {
	uint8_t reg;
	uint8_t val;
};

/* Supported resolutions */
enum ov2640_width {
	W_QCIF	= 176,
	W_QVGA	= 320,
	W_CIF	= 352,
	W_VGA	= 640,
	W_SVGA	= 800,
	W_XGA	= 1024,
	W_SXGA	= 1280,
	W_UXGA	= 1600,
};

enum ov2640_height {
	H_QCIF	= 144,
	H_QVGA	= 240,
	H_CIF	= 288,
	H_VGA	= 480,
	H_SVGA	= 600,
	H_XGA	= 768,
	H_SXGA	= 1024,
	H_UXGA	= 1200,
};

struct ov2640_win_size {
	char				*name;
	enum ov2640_width		width;
	enum ov2640_height		height;
	const struct regval_list	*regs;
};


/*
 * Registers settings. Most of them are undocumented. Some documentation is
 * is available in the OV2640 datasheet, the OV2640 hardware app notes and
 * the OV2640 software app notes documents.
 */

#define ENDMARKER { 0xff, 0xff }

static const struct regval_list ov2640_init_regs[] = {
	{ BANK_SEL, BANK_SEL_DSP },
	{ 0x2c,   0xff },
	{ 0x2e,   0xdf },
	{ BANK_SEL, BANK_SEL_SENS },
	{ 0x3c,   0x32 },
	{ CLKRC, CLKRC_DIV_SET(1) },
	{ COM2, COM2_OCAP_Nx_SET(3) },
	{ REG04, REG04_DEF | REG04_HREF_EN },
	{ COM8,  COM8_DEF | COM8_AGC_EN | COM8_AEC_EN | COM8_BNDF_EN },
	//~ { AEC,    0x00 },
	{ COM9, COM9_AGC_GAIN_8x | 0x08},
	{ 0x2c,   0x0c },
	{ 0x33,   0x78 },
	{ 0x3a,   0x33 },
	{ 0x3b,   0xfb },
	{ 0x3e,   0x00 },
	{ 0x43,   0x11 },
	{ 0x16,   0x10 },
	{ 0x39,   0x02 },
	{ 0x35,   0x88 },
	{ 0x22,   0x0a },
	{ 0x37,   0x40 },
	{ 0x23,   0x00 },
	{ ARCOM2, 0xa0 },
	{ 0x06,   0x02 },
	{ 0x06,   0x88 },
	{ 0x07,   0xc0 },
	{ 0x0d,   0xb7 },
	{ 0x0e,   0x01 },
	{ 0x4c,   0x00 },
	{ 0x4a,   0x81 },
	{ 0x21,   0x99 },
	{ AEW,    0x40 },
	{ AEB,    0x38 },
	{ VV,     VV_HIGH_TH_SET(0x08) | VV_LOW_TH_SET(0x02) },
	{ 0x5c,   0x00 },
	{ 0x63,   0x00 },
	{ FLL,    0x22 },
	{ COM3,   0x38 | COM3_BAND_AUTO },
	{ REG5D,  0x55 },
	{ REG5E,  0x7d },
	{ REG5F,  0x7d },
	{ REG60,  0x55 },
	{ HISTO_LOW,   0x70 },
	{ HISTO_HIGH,  0x80 },
	{ 0x7c,   0x05 },
	{ 0x20,   0x80 },
	{ 0x28,   0x30 },
	{ 0x6c,   0x00 },
	{ 0x6d,   0x80 },
	{ 0x6e,   0x00 },
	{ 0x70,   0x02 },
	{ 0x71,   0x94 },
	{ 0x73,   0xc1 },
	{ 0x3d,   0x34 },
	{ COM7, COM7_RES_UXGA | COM7_ZOOM_EN },
	{ 0x5a,   0x57 },
	{ BD50,   0xbb },
	{ BD60,   0x9c },
	{ BANK_SEL, BANK_SEL_DSP },
	{ 0xe5,   0x7f },
	{ MC_BIST, MC_BIST_RESET | MC_BIST_BOOT_ROM_SEL },
	{ 0x41,   0x24 },
	{ RESET, RESET_JPEG | RESET_DVP },
	{ 0x76,   0xff },
	{ 0x33,   0xa0 },
	{ 0x42,   0x20 },
	{ 0x43,   0x18 },
	{ 0x4c,   0x00 },
	{ CTRL3, CTRL3_BPC_EN | CTRL3_WPC_EN | 0x10 },
	{ 0x88,   0x3f },
	{ 0xd7,   0x03 },
	{ 0xd9,   0x10 },
	{ R_DVP_SP , R_DVP_SP_AUTO_MODE | 0x2 },
	{ 0xc8,   0x08 },
	{ 0xc9,   0x80 },
	{ BPADDR, 0x00 },
	{ BPDATA, 0x00 },
	{ BPADDR, 0x03 },
	{ BPDATA, 0x48 },
	{ BPDATA, 0x48 },
	{ BPADDR, 0x08 },
	{ BPDATA, 0x20 },
	{ BPDATA, 0x10 },
	{ BPDATA, 0x0e },
	{ 0x90,   0x00 },
	{ 0x91,   0x0e },
	{ 0x91,   0x1a },
	{ 0x91,   0x31 },
	{ 0x91,   0x5a },
	{ 0x91,   0x69 },
	{ 0x91,   0x75 },
	{ 0x91,   0x7e },
	{ 0x91,   0x88 },
	{ 0x91,   0x8f },
	{ 0x91,   0x96 },
	{ 0x91,   0xa3 },
	{ 0x91,   0xaf },
	{ 0x91,   0xc4 },
	{ 0x91,   0xd7 },
	{ 0x91,   0xe8 },
	{ 0x91,   0x20 },
	{ 0x92,   0x00 },
	{ 0x93,   0x06 },
	{ 0x93,   0xe3 },
	{ 0x93,   0x03 },
	{ 0x93,   0x03 },
	{ 0x93,   0x00 },
	{ 0x93,   0x02 },
	{ 0x93,   0x00 },
	{ 0x93,   0x00 },
	{ 0x93,   0x00 },
	{ 0x93,   0x00 },
	{ 0x93,   0x00 },
	{ 0x93,   0x00 },
	{ 0x93,   0x00 },
	{ 0x96,   0x00 },
	{ 0x97,   0x08 },
	{ 0x97,   0x19 },
	{ 0x97,   0x02 },
	{ 0x97,   0x0c },
	{ 0x97,   0x24 },
	{ 0x97,   0x30 },
	{ 0x97,   0x28 },
	{ 0x97,   0x26 },
	{ 0x97,   0x02 },
	{ 0x97,   0x98 },
	{ 0x97,   0x80 },
	{ 0x97,   0x00 },
	{ 0x97,   0x00 },
	{ 0xa4,   0x00 },
	{ 0xa8,   0x00 },
	{ 0xc5,   0x11 },
	{ 0xc6,   0x51 },
	{ 0xbf,   0x80 },
	{ 0xc7,   0x10 }, /* white balance */
	{ 0xb6,   0x66 },
	{ 0xb8,   0xA5 },
	{ 0xb7,   0x64 },
	{ 0xb9,   0x7C },
	{ 0xb3,   0xaf },
	{ 0xb4,   0x97 },
	{ 0xb5,   0xFF },
	{ 0xb0,   0xC5 },
	{ 0xb1,   0x94 },
	{ 0xb2,   0x0f },
	{ 0xc4,   0x5c },
	{ 0xa6,   0x00 },
	{ 0xa7,   0x20 },
	{ 0xa7,   0xd8 },
	{ 0xa7,   0x1b },
	{ 0xa7,   0x31 },
	{ 0xa7,   0x00 },
	{ 0xa7,   0x18 },
	{ 0xa7,   0x20 },
	{ 0xa7,   0xd8 },
	{ 0xa7,   0x19 },
	{ 0xa7,   0x31 },
	{ 0xa7,   0x00 },
	{ 0xa7,   0x18 },
	{ 0xa7,   0x20 },
	{ 0xa7,   0xd8 },
	{ 0xa7,   0x19 },
	{ 0xa7,   0x31 },
	{ 0xa7,   0x00 },
	{ 0xa7,   0x18 },
	{ 0x7f,   0x00 },
	{ 0xe5,   0x1f },
	{ 0xe1,   0x77 },
	{ 0xdd,   0x7f },
	{ QS,     0x0C },
	{ CTRL0,  CTRL0_YUV422 | CTRL0_YUV_EN },
	ENDMARKER,
};

/*
 * Register settings for window size
 * The preamble, setup the internal DSP to input an UXGA (1600x1200) image.
 * Then the different zooming configurations will setup the output image size.
 */
static const struct regval_list ov2640_size_change_preamble_regs[] = {
	{ BANK_SEL, BANK_SEL_DSP },
	{ RESET, RESET_DVP },
	{ HSIZE8, HSIZE8_SET(W_UXGA) },
	{ VSIZE8, VSIZE8_SET(H_UXGA) },
	{ CTRL2, CTRL2_DCW_EN | CTRL2_SDE_EN |
		 CTRL2_UV_AVG_EN | CTRL2_CMX_EN | CTRL2_UV_ADJ_EN },
	{ HSIZE, HSIZE_SET(W_UXGA) },
	{ VSIZE, VSIZE_SET(H_UXGA) },
	{ XOFFL, XOFFL_SET(0) },
	{ YOFFL, YOFFL_SET(0) },
	{ VHYX, VHYX_HSIZE_SET(W_UXGA) | VHYX_VSIZE_SET(H_UXGA) |
		VHYX_XOFF_SET(0) | VHYX_YOFF_SET(0)},
	{ TEST, TEST_HSIZE_SET(W_UXGA) },
	ENDMARKER,
};

#define PER_SIZE_REG_SEQ(x, y, v_div, h_div, pclk_div)	\
	{ CTRLI, CTRLI_LP_DP | CTRLI_V_DIV_SET(v_div) |	\
		 CTRLI_H_DIV_SET(h_div)},		\
	{ ZMOW, ZMOW_OUTW_SET(x) },			\
	{ ZMOH, ZMOH_OUTH_SET(y) },			\
	{ ZMHH, ZMHH_OUTW_SET(x) | ZMHH_OUTH_SET(y) },	\
	{ R_DVP_SP, pclk_div },				\
	{ RESET, 0x00}

static const struct regval_list ov2640_qcif_regs[] = {
	PER_SIZE_REG_SEQ(W_QCIF, H_QCIF, 3, 3, 4),
	ENDMARKER,
};

static const struct regval_list ov2640_qvga_regs[] = {
	PER_SIZE_REG_SEQ(W_QVGA, H_QVGA, 2, 2, 4),
	ENDMARKER,
};

static const struct regval_list ov2640_cif_regs[] = {
	PER_SIZE_REG_SEQ(W_CIF, H_CIF, 2, 2, 8),
	ENDMARKER,
};

static const struct regval_list ov2640_vga_regs[] = {
	PER_SIZE_REG_SEQ(W_VGA, H_VGA, 0, 0, 2),
	ENDMARKER,
};

static const struct regval_list ov2640_svga_regs[] = {
	PER_SIZE_REG_SEQ(W_SVGA, H_SVGA, 1, 1, 2),
	ENDMARKER,
};

static const struct regval_list ov2640_xga_regs[] = {
	PER_SIZE_REG_SEQ(W_XGA, H_XGA, 0, 0, 2),
	{ CTRLI,    0x00},
	ENDMARKER,
};

static const struct regval_list ov2640_sxga_regs[] = {
	PER_SIZE_REG_SEQ(W_SXGA, H_SXGA, 0, 0, 2),
	{ CTRLI,    0x00},
	{ R_DVP_SP, 2 | R_DVP_SP_AUTO_MODE },
	ENDMARKER,
};

static const struct regval_list ov2640_uxga_regs[] = {
	PER_SIZE_REG_SEQ(W_UXGA, H_UXGA, 0, 0, 0),
	{ CTRLI,    0x00},
	{ R_DVP_SP, 0 | R_DVP_SP_AUTO_MODE },
	ENDMARKER,
};

#define OV2640_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static const struct ov2640_win_size ov2640_supported_win_sizes[] = {
	OV2640_SIZE("QCIF", W_QCIF, H_QCIF, ov2640_qcif_regs),
	OV2640_SIZE("QVGA", W_QVGA, H_QVGA, ov2640_qvga_regs),
	OV2640_SIZE("CIF", W_CIF, H_CIF, ov2640_cif_regs),
	OV2640_SIZE("VGA", W_VGA, H_VGA, ov2640_vga_regs),
	OV2640_SIZE("SVGA", W_SVGA, H_SVGA, ov2640_svga_regs),
	OV2640_SIZE("XGA", W_XGA, H_XGA, ov2640_xga_regs),
	OV2640_SIZE("SXGA", W_SXGA, H_SXGA, ov2640_sxga_regs),
	OV2640_SIZE("UXGA", W_UXGA, H_UXGA, ov2640_uxga_regs),
};

/*
 * Register settings for pixel formats
 */
static const struct regval_list ov2640_format_change_preamble_regs[] = {
	{ BANK_SEL, BANK_SEL_DSP },
	{ R_BYPASS, R_BYPASS_USE_DSP },
	ENDMARKER,
};

static const struct regval_list ov2640_yuyv_regs[] = {
	{ IMAGE_MODE, IMAGE_MODE_YUV422 },
	{ 0xd7, 0x03 },
	{ 0x33, 0xa0 },
	{ 0xe5, 0x1f },
	{ 0xe1, 0x67 },
	{ RESET,  0x00 },
	{ R_BYPASS, R_BYPASS_USE_DSP },
	ENDMARKER,
};

static const struct regval_list ov2640_uyvy_regs[] = {
	{ IMAGE_MODE, IMAGE_MODE_LBYTE_FIRST | IMAGE_MODE_YUV422 },
	{ 0xd7, 0x01 },
	{ 0x33, 0xa0 },
	{ 0xe1, 0x67 },
	{ RESET,  0x00 },
	{ R_BYPASS, R_BYPASS_USE_DSP },
	ENDMARKER,
};

static const struct regval_list ov2640_rgb565_be_regs[] = {
	{ IMAGE_MODE, IMAGE_MODE_RGB565 },
	{ 0xd7, 0x03 },
	{ RESET,  0x00 },
	{ R_BYPASS, R_BYPASS_USE_DSP },
	ENDMARKER,
};

static const struct regval_list ov2640_rgb565_le_regs[] = {
	{ IMAGE_MODE, IMAGE_MODE_LBYTE_FIRST | IMAGE_MODE_RGB565 },
	{ 0xd7, 0x03 },
	{ RESET,  0x00 },
	{ R_BYPASS, R_BYPASS_USE_DSP },
	ENDMARKER,
};

static const struct regval_list ov2640_jpeg_regs[] = {
	{ BANK_SEL, BANK_SEL_DSP },
	{ 0xe0, 0x14 },
	{ 0xe1, 0x77 },
	{ 0xe5, 0x1f },
	{ 0xd7, 0x03 },
	{ IMAGE_MODE, IMAGE_MODE_JPEG_EN },
	{ 0xe0, 0x00 },
	{ BANK_SEL, BANK_SEL_SENS },
	{ 0x04, 0x08 },
	ENDMARKER,
};


bool ov2640_samplingFinished;
ssdv_config_t *ov2640_config;

/**
  * Captures an image from the camera.
  */
bool OV2640_Snapshot2RAM(void)
{
	palClearPad(PORT(IO_LED2), PIN(IO_LED2)); // Yellow LED shows when image is captured

	// Capture enable
	TRACE_INFO("CAM  > Capture image");
	OV2640_CaptureDCMI();
	chThdSleepMilliseconds(1000);

	palSetPad(PORT(IO_LED2), PIN(IO_LED2));

	return true;
}

bool OV2640_BufferOverflow(void)
{
	return OV2640_getBuffer(NULL) > ov2640_config->ram_size-3;
}

uint32_t OV2640_getBuffer(uint8_t** buffer) {
	*buffer = ov2640_config->ram_buffer;

	// Detect size
	uint32_t size = ov2640_config->ram_size;
	while(!ov2640_config->ram_buffer[size-1])
		size--;

	return size;
}

void OV2640_dma_avail(uint32_t flags)
{
	(void)flags;
	TRACE_DEBUG("DMA Interrupt");
	ov2640_samplingFinished = true;
	dmaStreamDisable(STM32_DMA2_STREAM1);
}

/**
  * Initializes DMA
  */
void OV2640_InitDMA(void)
{
	TRACE_INFO("CAM  > Available buffer %d byte", ov2640_config->ram_size);
	const stm32_dma_stream_t *stream = STM32_DMA2_STREAM1;
	dmaStreamAllocate(stream, 2, (stm32_dmaisr_t)OV2640_dma_avail, NULL);
	dmaStreamSetPeripheral(stream, ((uint32_t*)DCMI_REG_DR_ADDRESS));
	dmaStreamSetMemory0(stream, (uint32_t)ov2640_config->ram_buffer);
	dmaStreamSetTransactionSize(stream, ov2640_config->ram_size / sizeof(uint32_t));
	dmaStreamSetMode(stream, STM32_DMA_CR_CHSEL(1) | STM32_DMA_CR_DIR_P2M |
							 STM32_DMA_CR_MINC | STM32_DMA_CR_PSIZE_WORD |
							 STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_MBURST_SINGLE |
							 STM32_DMA_CR_PBURST_SINGLE | STM32_DMA_CR_TEIE |
							 STM32_DMA_CR_PL(3));
	dmaStreamSetFIFO(stream, STM32_DMA_FCR_FTH_FULL);
	dmaStreamEnable(stream);
}

void OV2640_DeinitDMA(void)
{
	const stm32_dma_stream_t *stream = STM32_DMA2_STREAM1;
	dmaStreamDisable(stream);
}

/**
  * Initializes DCMI
  */
void OV2640_InitDCMI(void)
{
	// Clock enable
	RCC->AHB2ENR |= RCC_AHB2Periph_DCMI;
	DCMI->CR &= ~(DCMI_CR_ENABLE | DCMI_CR_CAPTURE);

	// Configure DCMI
	DCMI->CR = DCMI_CaptureMode_SnapShot | DCMI_CR_JPEG | DCMI_CR_PCKPOL;

	// DCMI enable
	DCMI->CR |= (uint32_t)DCMI_CR_ENABLE;
}

void OV2640_CaptureDCMI(void)
{
	// DCMI capture
	DCMI->CR |= (uint32_t)DCMI_CR_CAPTURE;
}

void OV2640_DeinitDCMI(void)
{
	// Clock enable
	RCC->AHB2ENR &= ~RCC_AHB2Periph_DCMI;
	DCMI->CR &= ~(DCMI_CR_ENABLE | DCMI_CR_CAPTURE);
}

/**
  * Initializes GPIO (for DCMI)
  * The high speed clock supports communication by I2C (XCLK = 16MHz)
  */
void OV2640_InitGPIO(void)
{
	palSetPadMode(PORT(CAM_HREF), PIN(CAM_HREF), PAL_MODE_ALTERNATE(13));		// HSYNC -> PA4
	palSetPadMode(PORT(CAM_PCLK), PIN(CAM_PCLK), PAL_MODE_ALTERNATE(13));		// PCLK  -> PA6
	palSetPadMode(PORT(CAM_VSYNC), PIN(CAM_VSYNC), PAL_MODE_ALTERNATE(13));		// VSYNC -> PB7
	palSetPadMode(PORT(CAM_D2), PIN(CAM_D2), PAL_MODE_ALTERNATE(13));			// D0    -> PC6
	palSetPadMode(PORT(CAM_D3), PIN(CAM_D3), PAL_MODE_ALTERNATE(13));			// D1    -> PC7
	palSetPadMode(PORT(CAM_D4), PIN(CAM_D4), PAL_MODE_ALTERNATE(13));			// D2    -> PC8
	palSetPadMode(PORT(CAM_D5), PIN(CAM_D5), PAL_MODE_ALTERNATE(13));			// D3    -> PC9
	palSetPadMode(PORT(CAM_D6), PIN(CAM_D6), PAL_MODE_ALTERNATE(13));			// D4    -> PE4
	palSetPadMode(PORT(CAM_D7), PIN(CAM_D7), PAL_MODE_ALTERNATE(13));			// D5    -> PB6
	palSetPadMode(PORT(CAM_D8), PIN(CAM_D8), PAL_MODE_ALTERNATE(13));			// D6    -> PE5
	palSetPadMode(PORT(CAM_D9), PIN(CAM_D9), PAL_MODE_ALTERNATE(13));			// D7    -> PE6

	palSetPadMode(PORT(CAM_EN), PIN(CAM_EN), PAL_MODE_OUTPUT_PUSHPULL);			// CAM_EN
	palSetPadMode(PORT(CAM_RESET), PIN(CAM_RESET), PAL_MODE_OUTPUT_PUSHPULL);	// CAM_RESET
}

void OV2640_TransmitConfig(void)
{
	// Set to page 1
	I2C_write8_locked(OV2640_I2C_ADR, 0xff, 0x01);
	I2C_write8_locked(OV2640_I2C_ADR, 0x12, 0x80);
	chThdSleepMilliseconds(50);

	/* Write selected arrays to the camera to initialize it and set the
	 * desired output format. */
	for(uint32_t i=0; (ov2640_init_regs[i].reg != 0xff) || (ov2640_init_regs[i].val != 0xff); i++)
		I2C_write8_locked(OV2640_I2C_ADR, ov2640_init_regs[i].reg, ov2640_init_regs[i].val);

	for(uint32_t i=0; (ov2640_size_change_preamble_regs[i].reg != 0xff) || (ov2640_size_change_preamble_regs[i].val != 0xff); i++)
		I2C_write8_locked(OV2640_I2C_ADR, ov2640_size_change_preamble_regs[i].reg, ov2640_size_change_preamble_regs[i].val);

	switch(ov2640_config->res) {
		case RES_QCIF:
			for(uint32_t i=0; (ov2640_qcif_regs[i].reg != 0xff) || (ov2640_qcif_regs[i].val != 0xff); i++)
				I2C_write8_locked(OV2640_I2C_ADR, ov2640_qcif_regs[i].reg, ov2640_qcif_regs[i].val);
			break;

		case RES_QVGA:
			for(uint32_t i=0; (ov2640_qvga_regs[i].reg != 0xff) || (ov2640_qvga_regs[i].val != 0xff); i++)
				I2C_write8_locked(OV2640_I2C_ADR, ov2640_qvga_regs[i].reg, ov2640_qvga_regs[i].val);
			break;

		case RES_VGA:
			for(uint32_t i=0; (ov2640_vga_regs[i].reg != 0xff) || (ov2640_vga_regs[i].val != 0xff); i++)
				I2C_write8_locked(OV2640_I2C_ADR, ov2640_vga_regs[i].reg, ov2640_vga_regs[i].val);
			break;

		case RES_XGA:
			for(uint32_t i=0; (ov2640_xga_regs[i].reg != 0xff) || (ov2640_xga_regs[i].val != 0xff); i++)
				I2C_write8_locked(OV2640_I2C_ADR, ov2640_xga_regs[i].reg, ov2640_xga_regs[i].val);
			break;

		case RES_UXGA:
			for(uint32_t i=0; (ov2640_uxga_regs[i].reg != 0xff) || (ov2640_uxga_regs[i].val != 0xff); i++)
				I2C_write8_locked(OV2640_I2C_ADR, ov2640_uxga_regs[i].reg, ov2640_uxga_regs[i].val);
			break;

		default: // Default QVGA
			for(uint32_t i=0; (ov2640_qvga_regs[i].reg != 0xff) || (ov2640_qvga_regs[i].val != 0xff); i++)
				I2C_write8_locked(OV2640_I2C_ADR, ov2640_qvga_regs[i].reg, ov2640_qvga_regs[i].val);
	}

	for(uint32_t i=0; (ov2640_format_change_preamble_regs[i].reg != 0xff) || (ov2640_format_change_preamble_regs[i].val != 0xff); i++)
		I2C_write8_locked(OV2640_I2C_ADR, ov2640_format_change_preamble_regs[i].reg, ov2640_format_change_preamble_regs[i].val);
	for(uint32_t i=0; (ov2640_yuyv_regs[i].reg != 0xff) || (ov2640_yuyv_regs[i].val != 0xff); i++)
		I2C_write8_locked(OV2640_I2C_ADR, ov2640_yuyv_regs[i].reg, ov2640_yuyv_regs[i].val);

	for(uint32_t i=0; (ov2640_jpeg_regs[i].reg != 0xff) || (ov2640_jpeg_regs[i].val != 0xff); i++)
		I2C_write8_locked(OV2640_I2C_ADR, ov2640_jpeg_regs[i].reg, ov2640_jpeg_regs[i].val);
}

void OV2640_init(ssdv_config_t *config) {
	ov2640_config = config;

	// Take I2C (due to silicon bug of OV2640, it interferes if byte 0x30 transmitted on I2C bus)
	I2C_lock();

	// Clearing buffer
	uint32_t i;
	for(i=0; i<ov2640_config->ram_size; i++)
		ov2640_config->ram_buffer[i] = 0;

	TRACE_INFO("CAM  > Init pins");
	OV2640_InitGPIO();

	// Power on OV2640
	TRACE_INFO("CAM  > Switch on");
	palSetPad(PORT(CAM_EN), PIN(CAM_EN)); 		// Switch on camera
	palSetPad(PORT(CAM_RESET), PIN(CAM_RESET)); // Toggle reset

	// Send settings to OV2640
	TRACE_INFO("CAM  > Transmit config to camera");
	OV2640_TransmitConfig();

	// DCMI DMA
	TRACE_INFO("CAM  > Init DMA");
	OV2640_InitDMA();

	// DCMI Init
	TRACE_INFO("CAM  > Init DCMI");
	OV2640_InitDCMI();

	chThdSleepMilliseconds(3000);
}

void OV2640_deinit(void) {
	// DCMI Init
	TRACE_INFO("CAM  > Deinit DCMI");
	OV2640_DeinitDCMI();

	// DCMI DMA
	TRACE_INFO("CAM  > Deinit DMA");
	OV2640_DeinitDMA();

	// Power off OV2640
	TRACE_INFO("CAM  > Switch off");
	palClearPad(PORT(CAM_EN), PIN(CAM_EN)); // Switch off camera

	// Deinit pins
	palSetPadMode(PORT(CAM_RESET), PIN(CAM_RESET), PAL_MODE_INPUT);	// CAM_RESET

	// Release I2C (due to silicon bug of OV2640, it interferes if byte 0x30 transmitted on I2C bus)
	I2C_unlock();
}

bool OV2640_isAvailable(void)
{
	I2C_lock();

	// Configure pins
	palSetPadMode(PORT(CAM_EN), PIN(CAM_EN), PAL_MODE_OUTPUT_PUSHPULL);			// CAM_EN
	palSetPadMode(PORT(CAM_RESET), PIN(CAM_RESET), PAL_MODE_OUTPUT_PUSHPULL);	// CAM_RESET

	// Switch on camera
	palSetPad(PORT(CAM_EN), PIN(CAM_EN)); 		// Switch on camera
	palSetPad(PORT(CAM_RESET), PIN(CAM_RESET)); // Toggle reset

	chThdSleepMilliseconds(100);

	uint16_t val;
	bool ret;
	if(I2C_read16_locked(OV2640_I2C_ADR, 0x0A, &val))
		ret = val == PID_OV2640;
	else
		ret = false;

	palClearPad(PORT(CAM_EN), PIN(CAM_EN)); // Switch off camera
	palSetPadMode(PORT(CAM_RESET), PIN(CAM_RESET), PAL_MODE_INPUT);	// CAM_RESET
	I2C_unlock();

	return ret;
}

