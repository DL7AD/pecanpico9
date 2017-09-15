/*
 * Registers by Arducam https://github.com/ArduCAM/Arduino/blob/master/ArduCAM/ov5640_regs.h
 * https://github.com/ArduCAM/Arduino/blob/master/ArduCAM/ArduCAM.cpp
 */

#include "ch.h"
#include "hal.h"
#include "ov5640.h"
#include "pi2c.h"
#include "board.h"
#include "debug.h"
#include "ssdv.h"
#include <string.h>

struct regval_list {
	uint16_t reg;
	uint8_t val;
};

static const struct regval_list OV5640YUV_Sensor_Dvp_Init[] =
{
	{ 0x4740, 0x24 },
		
		{ 0x4050, 0x6e },
    { 0x4051, 0x8f },
		
		{ 0x3008, 0x42 }, 
		{ 0x3103, 0x03 }, 
		{ 0x3017, 0x7f }, 
		{ 0x3018, 0xff }, 		
		{ 0x302c, 0x02 }, 		
		{ 0x3108, 0x31 }, 	
		{ 0x3630, 0x2e },//2e
		{ 0x3632, 0xe2 }, 
		{ 0x3633, 0x23 },//23 
		{ 0x3621, 0xe0 }, 
		{ 0x3704, 0xa0 }, 
		{ 0x3703, 0x5a }, 
		{ 0x3715, 0x78 }, 
		{ 0x3717, 0x01 }, 
		{ 0x370b, 0x60 }, 
		{ 0x3705, 0x1a }, 
		{ 0x3905, 0x02 }, 
		{ 0x3906, 0x10 }, 
		{ 0x3901, 0x0a }, 
		{ 0x3731, 0x12 }, 
		{ 0x3600, 0x08 }, 
		{ 0x3601, 0x33 }, 
		{ 0x302d, 0x60 }, 
		{ 0x3620, 0x52 }, 
		{ 0x371b, 0x20 }, 
		{ 0x471c, 0x50 }, 
		
		{ 0x3a18, 0x00 }, 
		{ 0x3a19, 0xf8 }, 
		
		{ 0x3635, 0x1c },//1c
		{ 0x3634, 0x40 }, 
		{ 0x3622, 0x01 }, 
 
		{ 0x3c04, 0x28 }, 
		{ 0x3c05, 0x98 }, 
		{ 0x3c06, 0x00 }, 
		{ 0x3c07, 0x08 }, 
		{ 0x3c08, 0x00 }, 
		{ 0x3c09, 0x1c }, 
		{ 0x3c0a, 0x9c }, 
		{ 0x3c0b, 0x40 },  		

		{ 0x3820, 0x41 }, 
		{ 0x3821, 0x01 }, //07

		//windows setup
		{ 0x3800, 0x00 }, 
		{ 0x3801, 0x00 }, 
		{ 0x3802, 0x00 }, 
		{ 0x3803, 0x04 }, 
		{ 0x3804, 0x0a }, 
		{ 0x3805, 0x3f }, 
		{ 0x3806, 0x07 }, 
		{ 0x3807, 0x9b }, 
		{ 0x3808, 0x05 },  
		{ 0x3809, 0x00 }, 
		{ 0x380a, 0x03 }, 
		{ 0x380b, 0xc0 }, 		
		{ 0x3810, 0x00 }, 
		{ 0x3811, 0x10 }, 
		{ 0x3812, 0x00 }, 
		{ 0x3813, 0x06 }, 
		{ 0x3814, 0x31 }, 
		{ 0x3815, 0x31 },		
		
		{ 0x3034, 0x1a }, 
		{ 0x3035, 0x11 }, //15fps
		{ 0x3036, 0x46 }, 
		{ 0x3037, 0x03 }, 
		{ 0x3038, 0x00 }, 
		{ 0x3039, 0x00 }, 
		
		{ 0x380c, 0x07 }, 
		{ 0x380d, 0x68 }, 
		{ 0x380e, 0x03 }, //03
		{ 0x380f, 0xd8 }, //d8
		 
		{ 0x3c01, 0xb4 }, 
		{ 0x3c00, 0x04 }, 
		{ 0x3a08, 0x00 }, 
		{ 0x3a09, 0x93 }, 
		{ 0x3a0e, 0x06 },
		{ 0x3a0a, 0x00 }, 
		{ 0x3a0b, 0x7b }, 		 
		{ 0x3a0d, 0x08 }, 
		
		{ 0x3a00, 0x3c }, //15fps-10fps
		{ 0x3a02, 0x05 }, 
		{ 0x3a03, 0xc4 }, 
		{ 0x3a14, 0x05 }, 
		{ 0x3a15, 0xc4 }, 
		
		{ 0x3618, 0x00 }, 
		{ 0x3612, 0x29 }, 
		{ 0x3708, 0x64 }, 
		{ 0x3709, 0x52 }, 
		{ 0x370c, 0x03 },
		
		{ 0x4001, 0x02 }, 
		{ 0x4004, 0x02 }, 
		{ 0x3000, 0x00 }, 
		{ 0x3002, 0x1c }, 
		{ 0x3004, 0xff }, 
		{ 0x3006, 0xc3 }, 
		{ 0x300e, 0x58 }, 
		{ 0x302e, 0x00 }, 
		{ 0x4300, 0x30 }, 
		{ 0x501f, 0x00 }, 
		{ 0x4713, 0x03 }, 
		{ 0x4407, 0x04 }, 
		{ 0x460b, 0x35 }, 
		{ 0x460c, 0x22 },//add by bright 
	  { 0x3824, 0x01 },//add by bright 
		{ 0x5001, 0xa3 }, 		
		
		{ 0x3406, 0x01 },//awbinit
		{ 0x3400, 0x06 },
		{ 0x3401, 0x80 },
		{ 0x3402, 0x04 },
		{ 0x3403, 0x00 },
		{ 0x3404, 0x06 },
		{ 0x3405, 0x00 },
	  //awb           
		{ 0x5180, 0xff }, 
		{ 0x5181, 0xf2 },    
		{ 0x5182, 0x00 },   
		{ 0x5183, 0x14 },    
		{ 0x5184, 0x25 },    
		{ 0x5185, 0x24 },    
		{ 0x5186, 0x16 },    
		{ 0x5187, 0x16 },    
		{ 0x5188, 0x16 },    
		{ 0x5189, 0x62 },    
		{ 0x518a, 0x62 },    
		{ 0x518b, 0xf0 },    
		{ 0x518c, 0xb2 },    
		{ 0x518d, 0x50 },    
		{ 0x518e, 0x30 },    
		{ 0x518f, 0x30 },    
		{ 0x5190, 0x50 },    
		{ 0x5191, 0xf8 },    
		{ 0x5192, 0x04 },   
		{ 0x5193, 0x70 },    
		{ 0x5194, 0xf0 },    
		{ 0x5195, 0xf0 },    
		{ 0x5196, 0x03 },   
		{ 0x5197, 0x01 },   
		{ 0x5198, 0x04 },   
		{ 0x5199, 0x12 },    
		{ 0x519a, 0x04 },   
		{ 0x519b, 0x00 },   
		{ 0x519c, 0x06 },   
		{ 0x519d, 0x82 },    
		{ 0x519e, 0x38 },  
		//color matrix  	                                        	                                  
		{ 0x5381, 0x1e },  
		{ 0x5382, 0x5b }, 
		{ 0x5383, 0x14 }, 
		{ 0x5384, 0x06 }, 
		{ 0x5385, 0x82 }, 
		{ 0x5386, 0x88 }, 
		{ 0x5387, 0x7c }, 
		{ 0x5388, 0x60 }, 
		{ 0x5389, 0x1c }, 
		{ 0x538a, 0x01 }, 
		{ 0x538b, 0x98 }, 
		//sharp&noise
		{ 0x5300, 0x08 }, 
		{ 0x5301, 0x30 }, 
		{ 0x5302, 0x3f }, 
		{ 0x5303, 0x10 },
		{ 0x5304, 0x08 }, 
		{ 0x5305, 0x30 }, 
		{ 0x5306, 0x18 }, 
		{ 0x5307, 0x28 },
		{ 0x5309, 0x08 }, 
		{ 0x530a, 0x30 }, 
		{ 0x530b, 0x04 }, 
		{ 0x530c, 0x06 }, 	 
		//gamma                         
		{ 0x5480, 0x01 },
		{ 0x5481, 0x06 }, 
		{ 0x5482, 0x12 },  
		{ 0x5483, 0x24 },  
		{ 0x5484, 0x4a }, 
		{ 0x5485, 0x58 },  
		{ 0x5486, 0x65 },  
		{ 0x5487, 0x72 },  
		{ 0x5488, 0x7d },  
		{ 0x5489, 0x88 },  
		{ 0x548a, 0x92 },  
		{ 0x548b, 0xa3 },  
		{ 0x548c, 0xb2 },  
		{ 0x548d, 0xc8 },  
		{ 0x548e, 0xdd },  
		{ 0x548f, 0xf0 }, 
		{ 0x5490, 0x15 }, 	  
		//UV adjust                              	                    
		{ 0x5580, 0x06 }, 		
		{ 0x5583, 0x40 }, 
		{ 0x5584, 0x20 }, 
		{ 0x5589, 0x10 }, 
		{ 0x558a, 0x00 }, 
		{ 0x558b, 0xf8 },                                 
		//lens shading                                      
		{ 0x5000, 0xa7 },                                      	
		{ 0x5800, 0x20 }, 
		{ 0x5801, 0x19 }, 
		{ 0x5802, 0x17 }, 
		{ 0x5803, 0x16 }, 
		{ 0x5804, 0x18 }, 
		{ 0x5805, 0x21 }, 
		{ 0x5806, 0x0F }, 
		{ 0x5807, 0x0A }, 
		{ 0x5808, 0x07 }, 
		{ 0x5809, 0x07 }, 
		{ 0x580a, 0x0A }, 
		{ 0x580b, 0x0C }, 
		{ 0x580c, 0x0A }, 
		{ 0x580d, 0x03 }, 
		{ 0x580e, 0x01 }, 
		{ 0x580f, 0x01 }, 
		{ 0x5810, 0x03 }, 
		{ 0x5811, 0x09 }, 
		{ 0x5812, 0x0A }, 
		{ 0x5813, 0x03 }, 
		{ 0x5814, 0x01 }, 
		{ 0x5815, 0x01 }, 
		{ 0x5816, 0x03 }, 
		{ 0x5817, 0x08 }, 
		{ 0x5818, 0x10 }, 
		{ 0x5819, 0x0A }, 
		{ 0x581a, 0x06 }, 
		{ 0x581b, 0x06 }, 
		{ 0x581c, 0x08 }, 
		{ 0x581d, 0x0E }, 
		{ 0x581e, 0x22 }, 
		{ 0x581f, 0x18 }, 
		{ 0x5820, 0x13 }, 
		{ 0x5821, 0x12 }, 
		{ 0x5822, 0x16 }, 
		{ 0x5823, 0x1E }, 
		{ 0x5824, 0x64 }, 
		{ 0x5825, 0x2A }, 
		{ 0x5826, 0x2C }, 
		{ 0x5827, 0x2A }, 
		{ 0x5828, 0x46 }, 
		{ 0x5829, 0x2A }, 
		{ 0x582a, 0x26 }, 
		{ 0x582b, 0x24 }, 
		{ 0x582c, 0x26 }, 
		{ 0x582d, 0x2A }, 
		{ 0x582e, 0x28 }, 
		{ 0x582f, 0x42 }, 
		{ 0x5830, 0x40 }, 
		{ 0x5831, 0x42 }, 
		{ 0x5832, 0x08 }, 
		{ 0x5833, 0x28 }, 
		{ 0x5834, 0x26 }, 
		{ 0x5835, 0x24 }, 
		{ 0x5836, 0x26 }, 
		{ 0x5837, 0x2A }, 
		{ 0x5838, 0x44 }, 
		{ 0x5839, 0x4A }, 
		{ 0x583a, 0x2C }, 
		{ 0x583b, 0x2a }, 
		{ 0x583c, 0x46 }, 
		{ 0x583d, 0xCE }, 	
		
		{ 0x5688, 0x22 }, 
		{ 0x5689, 0x22 }, 
		{ 0x568a, 0x42 }, 
		{ 0x568b, 0x24 }, 
		{ 0x568c, 0x42 }, 
		{ 0x568d, 0x24 }, 
		{ 0x568e, 0x22 }, 
		{ 0x568f, 0x22 }, 
		
		{ 0x5025, 0x00 }, 
		
		{ 0x3a0f, 0x30 },
		{ 0x3a10, 0x28 }, 
		{ 0x3a1b, 0x30 }, 
		{ 0x3a1e, 0x28 }, 
		{ 0x3a11, 0x61 }, 
		{ 0x3a1f, 0x10 }, 
		
		{ 0x4005, 0x1a },
		{ 0x3406, 0x00 },//awbinit
    { 0x3503, 0x00 },//awbinit
		{ 0x3008, 0x02 }, 
{ 0xffff, 0xff }, 
};



//2592x1944 QSXGA
static const struct regval_list OV5640_JPEG_QSXGA[]  =
{
	{0x3820 ,0x40}, 
		{0x3821 ,0x26}, 
		{0x3814 ,0x11}, 
		{0x3815 ,0x11}, 
		{0x3803 ,0x00}, 
		{0x3807 ,0x9f}, 
		{0x3808 ,0x0a}, 
		{0x3809 ,0x20}, 
		{0x380a ,0x07}, 
		{0x380b ,0x98},
		{0x380c ,0x0b},                                                                    
		{0x380d ,0x1c}, 
		{0x380e ,0x07},                                                         
		{0x380f ,0xb0},                                                          
		{0x3813 ,0x04},                                                     
		{0x3618 ,0x04},                        
		{0x3612 ,0x4b},                                               
		{0x3708 ,0x64},                
		{0x3709 ,0x12},                                    
		{0x370c ,0x00},  
		{0x3a02 ,0x07},                             
		{0x3a03 ,0xb0},                        
		{0x3a0e ,0x06},                    
		{0x3a0d ,0x08}, 
		{0x3a14 ,0x07}, 
		{0x3a15 ,0xb0}, 
		{0x4001 ,0x02}, 
		{0x4004 ,0x06}, 
		{0x3002 ,0x00}, 
		{0x3006 ,0xff}, 
		{0x3824 ,0x04}, 
		{0x5001 ,0x83}, 
		{0x3036 ,0x69}, 
		{0x3035 ,0x31}, 
		{0x4005 ,0x1A},
{0xffff, 0xff}, 
};

//5MP
static const struct regval_list OV5640_5MP_JPEG[]  =
{
	{0x3800 ,0x00},                            
	{0x3801 ,0x00},                                                            
	{0x3802 ,0x00},                            
	{0x3803 ,0x00},   
	{0x3804 ,0xA },           
	{0x3805 ,0x3f},          
	{0x3806 ,0x7 },          
	{0x3807 ,0x9f},          
	{0x3808 ,0xA },                               
	{0x3809 ,0x20},  
	{0x380a ,0x7 },           
	{0x380b ,0x98}, 
	{0x380c ,0xc },                        
	{0x380d ,0x80},                      
	{0x380e ,0x7 }, 
	{0x380f ,0xd0}, 
	{0x5001 ,0xa3}, 
	{0x5680 ,0x0 }, 
	{0x5681 ,0x0 }, 
	{0x5682 ,0xA }, 
	{0x5683 ,0x20}, 
	{0x5684 ,0x0 }, 
	{0x5685 ,0x0 }, 
	{0x5686 ,0x7 }, 
	{0x5687 ,0x98}, 	
	{0xffff, 0xff},	
};

//320x240 QVGA
static const struct regval_list OV5640_QSXGA2QVGA[]  =
{
	{0x3800 ,0x00},
	{0x3801 ,0x00},
	{0x3802 ,0x00},
	{0x3803 ,0x00},
	{0x3804 ,0xA },
	{0x3805 ,0x3f},
	{0x3806 ,0x7 },
	{0x3807 ,0x9f},
	{0x3808 ,0x1 },
	{0x3809 ,0x40},
	{0x380a ,0x0 },
	{0x380b ,0xf0},
	{0x380c ,0xc },
	{0x380d ,0x80},
	{0x380e ,0x7 },
	{0x380f ,0xd0},
	{0x5001 ,0xa3},
	{0x5680 ,0x0 },
	{0x5681 ,0x0 },
	{0x5682 ,0xA },
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 },
	{0x5686 ,0x7 },
	{0x5687 ,0x98},
	{0xffff, 0xff},
};

//320x240 QQVGA
static const struct regval_list OV5640_QSXGA2QQVGA[]  =
{
	{0x3800 ,0x00},
	{0x3801 ,0x00},
	{0x3802 ,0x00},
	{0x3803 ,0x00},
	{0x3804 ,0xA },
	{0x3805 ,0x3f},
	{0x3806 ,0x7 },
	{0x3807 ,0x9f},
	{0x3808 ,0x0 },
	{0x3809 ,0xA0},
	{0x380a ,0x0 },
	{0x380b ,0x70},
	{0x380c ,0xc },
	{0x380d ,0x80},
	{0x380e ,0x7 },
	{0x380f ,0xd0},
	{0x5001 ,0xa3},
	{0x5680 ,0x0 },
	{0x5681 ,0x0 },
	{0x5682 ,0xA },
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 },
	{0x5686 ,0x7 },
	{0x5687 ,0x98},
	{0xffff, 0xff},
};

//640x480 VGA
static const struct regval_list OV5640_QSXGA2VGA[]  =
{
		{0x3800 ,0x00}, 
		{0x3801 ,0x00}, 
		{0x3802 ,0x00}, 
		{0x3803 ,0x00},                                                                                                            
		{0x3804 ,0xA },                                  
		{0x3805 ,0x3f},                                           
		{0x3806 ,0x7 },                                     
		{0x3807 ,0x9f},                                     
		{0x3808 ,0x2 },                                                                                          
		{0x3809 ,0x80},                               
		{0x380a ,0x1 },                               
		{0x380b ,0xe0},                               
		{0x380c ,0xc },                  
		{0x380d ,0x80},                                                            
		{0x380e ,0x7 },               
		{0x380f ,0xd0},       
		{0x5001 ,0xa3},        
		{0x5680 ,0x0 },                                  
		{0x5681 ,0x0 },       
		{0x5682 ,0xA },   
		{0x5683 ,0x20},
		{0x5684 ,0x0 },
		{0x5685 ,0x0 }, 
		{0x5686 ,0x7 }, 
		{0x5687 ,0x98}, 	
{0xffff, 0xff}, 
};

//800x480 WVGA
static const struct regval_list OV5640_QSXGA2WVGA[]  =	
{
	{0x3800 ,0x00}, 
	{0x3801 ,0x00}, 
	{0x3802 ,0x00}, 
	{0x3803 ,0x00},                                                                                                            
	{0x3804 ,0xA },                                  
	{0x3805 ,0x3f},                                           
	{0x3806 ,0x7 },                                     
	{0x3807 ,0x9f},
	{0x3808 ,0x3 },
	{0x3809 ,0x20},
	{0x380a ,0x1 },                               
	{0x380b ,0xe0},                               
	{0x380c ,0xc },                  
	{0x380d ,0x80},
	{0x380e ,0x7 },               
	{0x380f ,0xd0},   
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x01},
	{0x3813, 0x48},    
	{0x5001 ,0xa3},        
	{0x5680 ,0x0 },                                  
	{0x5681 ,0x0 },       
	{0x5682 ,0xA },   
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 }, 
	{0x5686 ,0x7 }, 
	{0x5687 ,0x98}, 	
	{0xffff, 0xff},	
};

//352x288 CIF
static const struct regval_list OV5640_QSXGA2CIF[]  =	
{
	{0x3800 ,0x00}, 
	{0x3801 ,0x00}, 
	{0x3802 ,0x00}, 
	{0x3803 ,0x00},                                                                                                            
	{0x3804 ,0xA },                                  
	{0x3805 ,0x3f},                                           
	{0x3806 ,0x7 },                                     
	{0x3807 ,0x9f},                                     
	{0x3808 ,0x1 },                                                                                          
	{0x3809 ,0x60},                               
	{0x380a ,0x1 },                               
	{0x380b ,0x20},                               
	{0x380c ,0xc },                  
	{0x380d ,0x80},                                                            
	{0x380e ,0x7 },               
	{0x380f ,0xd0},   
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x70},    
	{0x5001 ,0xa3},        
	{0x5680 ,0x0 },                                  
	{0x5681 ,0x0 },       
	{0x5682 ,0xA },   
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 }, 
	{0x5686 ,0x7 }, 
	{0x5687 ,0x98}, 	
	{0xffff, 0xff},	
};

//1280x960 SXGA
static const struct regval_list OV5640_QSXGA2SXGA[]  =	
{
	{0x3800 ,0x00},
	{0x3801 ,0x00},
	{0x3802 ,0x00},
	{0x3803 ,0x00},
	{0x3804 ,0xA },
	{0x3805 ,0x3f},
	{0x3806 ,0x7 },
	{0x3807 ,0x9f},
	{0x3808 ,0x5 },
	{0x3809 ,0x0 },
	{0x380a ,0x3 },
	{0x380b ,0xc0},
	{0x380c ,0xc },
	{0x380d ,0x80},
	{0x380e ,0x7 },
	{0x380f ,0xd0},
	{0x5001 ,0xa3},
	{0x5680 ,0x0 },
	{0x5681 ,0x0 },
	{0x5682 ,0xA },
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 },
	{0x5686 ,0x7 },
	{0x5687 ,0x98},
	{0xffff, 0xff},
};

//2048x1536 QXGA
static const struct regval_list OV5640_QSXGA2QXGA[]  =	
{
	{0x3800 ,0x00},
	{0x3801 ,0x00},
	{0x3802 ,0x00},
	{0x3803 ,0x00},
	{0x3804 ,0xA },
	{0x3805 ,0x3f},
	{0x3806 ,0x7 },
	{0x3807 ,0x9f},
	{0x3808 ,0x8 },
	{0x3809 ,0x0 },
	{0x380a ,0x6 },
	{0x380b ,0x0 },
	{0x380c ,0xc },
	{0x380d ,0x80},
	{0x380e ,0x7 },
	{0x380f ,0xd0},
	{0x5001 ,0xa3},
	{0x5680 ,0x0 },
	{0x5681 ,0x0 },
	{0x5682 ,0xA },
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 },
	{0x5686 ,0x7 },
	{0x5687 ,0x98},	
	{0xffff, 0xff},
};


//1600x1200 UXGA
static const struct regval_list OV5640_QSXGA2UXGA[]  =	
{
	{0x3800 ,0x00},
	{0x3801 ,0x00},
	{0x3802 ,0x00},
	{0x3803 ,0x00},
	{0x3804 ,0xA },
	{0x3805 ,0x3f},
	{0x3806 ,0x7 },
	{0x3807 ,0x9f},
	{0x3808 ,0x6 },
	{0x3809 ,0x40},
	{0x380a ,0x4 },
	{0x380b ,0xb0},
	{0x380c ,0xc },
	{0x380d ,0x80},
	{0x380e ,0x7 },
	{0x380f ,0xd0},
	{0x5001 ,0xa3},
	{0x5680 ,0x0 },
	{0x5681 ,0x0 },
	{0x5682 ,0xA },
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 },
	{0x5686 ,0x7 },
	{0x5687 ,0x98},
	{0xffff, 0xff},	
};

//1024x768 XGA
static const struct regval_list OV5640_QSXGA2XGA[]  =	
{
	{0x3800 ,0x00},
	{0x3801 ,0x00},
	{0x3802 ,0x00},
	{0x3803 ,0x00},
	{0x3804 ,0xA },
	{0x3805 ,0x3f},
	{0x3806 ,0x7 },
	{0x3807 ,0x9f},
	{0x3808 ,0x4 },
	{0x3809 ,0x0 },
	{0x380a ,0x3 },
	{0x380b ,0x0 },
	{0x380c ,0xc },
	{0x380d ,0x80},
	{0x380e ,0x7 },
	{0x380f ,0xd0},
	{0x5001 ,0xa3},
	{0x5680 ,0x0 },
	{0x5681 ,0x0 },
	{0x5682 ,0xA },
	{0x5683 ,0x20},
	{0x5684 ,0x0 },
	{0x5685 ,0x0 },
	{0x5686 ,0x7 },
	{0x5687 ,0x98},
	{0xffff, 0xff},	
};

// TODO: Implement a state machine instead of multiple flags
static bool capture_finished;
static bool vsync;
static bool dma_error;
static uint32_t dma_flags;

static uint8_t* dma_buffer;

static uint32_t oldSpeed;
static uint32_t oldWS;

/**
  * Increase AHB clock to 48 MHz and saves the old speed
  */
void set48MHz(void)
{
	oldSpeed = RCC->CFGR & RCC_CFGR_HPRE_Msk;
	oldWS    = FLASH->ACR & FLASH_ACR_LATENCY_Msk;

	uint32_t new = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | FLASH_ACR_LATENCY_3WS;
	FLASH->ACR = new;
	while(FLASH->ACR != new);

	new = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV1;
	RCC->CFGR = new;
	while(RCC->CFGR != new);
}

/**
  * Reduce AHB clock back to the old speed which has been saved by set48MHz()
  */
void set6MHz(void)
{
	uint32_t new = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | oldSpeed;
	RCC->CFGR = new;
	while(RCC->CFGR != new);

	new = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | oldWS;
	FLASH->ACR = new;
	while(FLASH->ACR != new);
}

/**
  * Analyzes the image for JPEG errors. Returns true if the image is error free.
  */
static bool analyze_image(uint8_t *image, uint32_t image_len)
{
	#if !OV5640_USE_DMA_DBM
	if(image_len >= 65535)
	{
		TRACE_ERROR("CAM  > Camera has %d bytes allocated but DMA DBM not activated", image_len);
		TRACE_ERROR("CAM  > DMA can only use 65535 bytes only");
		image_len = 65535;
	}
	#endif

	ssdv_t ssdv;
	uint8_t pkt[SSDV_PKT_SIZE];
	uint8_t *b;
	uint32_t bi = 0;
	uint8_t c = SSDV_OK;

	ssdv_enc_init(&ssdv, SSDV_TYPE_NORMAL, "", 0, 7);
	ssdv_enc_set_buffer(&ssdv, pkt);

	while(true) // FIXME: I get caught in these loops occasionally and never return
	{
		while((c = ssdv_enc_get_packet(&ssdv)) == SSDV_FEED_ME)
		{
			b = &image[bi];
			uint8_t r = bi < image_len-128 ? 128 : image_len - bi;
			bi += r;
			if(r <= 0)
			{
				TRACE_ERROR("CAM  > Error in image");
				return false;
			}
			ssdv_enc_feed(&ssdv, b, r);
		}

		if(c == SSDV_EOI) // End of image
			return true;

		if(c != SSDV_OK) // Error in JPEG image
		{
			TRACE_ERROR("CAM  > Error in image");
			return false;
		}

		chThdSleepMilliseconds(5);
	}
}

/**
  * Captures an image from the camera.
  * @buffer Buffer in which the image can be sampled
  * @size Size of buffer
  * @res Resolution of the image
  * If resolution MAX_RES has been chosen, a highest resolution will be chosen
  * which it suitable for the amount of buffer. Due to the JPEG compression
  * that could lead to different resolutions on different method calls.
  * The method returns the size of the image.
  */
uint32_t OV5640_Snapshot2RAM(uint8_t* buffer, uint32_t size, resolution_t res, bool enableJpegValidation)
{
	uint8_t cntr = 10;
	bool status;
	bool jpegValid;
	uint32_t size_sampled;

	// Set resoultion
	if(res == RES_MAX)
	{
		OV5640_SetResolution(RES_UXGA); // FIXME: We actually have to choose the resolution which fits in the memory
	} else {
		OV5640_SetResolution(res);
	}

	// Capture image until we get a good image (max 10 tries)
	do {
		// Clearing buffer
		uint32_t i;
		for(i=0; i<size; i++)
			buffer[i] = 0;

		TRACE_INFO("CAM  > Capture image");
		status = OV5640_Capture(buffer, size);
		TRACE_INFO("CAM  > Capture finished");

		size_sampled = size - 1;
		while(!buffer[size_sampled] && size_sampled > 0)
			size_sampled--;

		TRACE_INFO("CAM  > Image size: %d bytes", size_sampled);

		// Validate JPEG image
		if(enableJpegValidation)
		{
			TRACE_INFO("CAM  > Validate integrity of JPEG");
			jpegValid = analyze_image(buffer, size);
			TRACE_INFO("CAM  > JPEG image %s", jpegValid ? "valid" : "invalid");
		} else {
			jpegValid = true;
		}

	} while((!jpegValid || !status) && cntr--);

	return size_sampled;
}

const stm32_dma_stream_t *dmastp;

#if OV5640_USE_DMA_DBM == TRUE
uint16_t dma_index;
uint16_t dma_buffers;
#define DMA_SEGMENT_SIZE 1024
#define DMA_FIFO_BURST_ALIGN 32


#if !defined(dmaStreamGetCurrentTarget)
/**
 * @brief   Get DMA stream current target.
 * @note    This function can be invoked in both ISR or thread context.
 * @pre     The stream must have been allocated using @p dmaStreamAllocate().
 * @post    After use the stream can be released using @p dmaStreamRelease().
 *
 * @param[in] dmastp    pointer to a stm32_dma_stream_t structure
 * @return  Current target index
 *
 * @special
 */
#define dmaStreamGetCurrentTarget(dmastp)                                     \
    ((uint8_t)(((dmastp)->stream->CR >> DMA_SxCR_CT_Pos) & 1U))

#endif /* !defined(dmaStreamGetCurrentTarget) */
#endif /* OV5640_USE_DMA_DBM == TRUE */

inline int32_t dma_start(void) {
  /* Clear any pending interrupts. */
  dmaStreamClearInterrupt(dmastp);
  dmaStreamEnable(dmastp);
  return 0;
}

/*
 * Stop DMA release stream and return count remaining.
 * Note that any DMA FIFO transfer will complete.
 * The Chibios DMAV2 driver waits for EN to clear before proceeding.
 */
inline uint16_t dma_stop(void) {
	dmaStreamDisable(dmastp);
	uint16_t transfer = dmaStreamGetTransactionSize(dmastp);
	dmaStreamRelease(dmastp);
	return transfer;
}

#if OV5640_USE_DMA_DBM == TRUE

static void dma_interrupt(void *p, uint32_t flags) {
	/* No parameter passed. */
	(void)p;

	dma_flags = flags;
	if(flags & (STM32_DMA_ISR_FEIF | STM32_DMA_ISR_TEIF)) {
		/*
		 * DMA transfer error or FIFO error.
		 * See 9.34.19 of RM0430.
		 *
		 * Disable timer DMA request and flag fault.
		 */
		TIM8->DIER &= ~TIM_DIER_CC1DE;
		dma_error = true;
		dmaStreamClearInterrupt(dmastp);
		return;
	}

	if(flags & STM32_DMA_ISR_HTIF) {
		/*
		 * Half transfer complete.
		 * Check if DMA is writing to the last buffer.
		 */
		if(dma_index == (dma_buffers - 1)) {
			/*
			 * This is the last buffer so we have to terminate DMA.
			 * The DBM switch is done in h/w.
			 * DMA could write beyond total buffer if not stopped.
			 *
			 * Since this is the last DMA buffer this is treated as an error.
			 * The DMA should normally be terminated by VSYNC before last buffer.
			 * Stop DMA and TIM DMA trigger and flag error.
			 */

			TIM8->DIER &= ~TIM_DIER_CC1DE;
			dma_error = true;
			dmaStreamClearInterrupt(dmastp);
			return;
		}
		/*
		 * Else Safe to allow buffer to fill.
		 * DMA DBM will switch buffers in h/w when this one is full.
		 * Just clear the interrupt and wait for TCIF.
		 */
		dmaStreamClearInterrupt(dmastp);
		return;
	}
	if (flags & STM32_DMA_ISR_TCIF) {
		/*
		 * Full buffer transfer complete.
		 * Update non-active memory address register.
		 * DMA will use new address at h/w DBM switch.
		 */


		if (dmaStreamGetCurrentTarget(dmastp) == 1) {
			dmaStreamSetMemory0(dmastp, &dma_buffer[++dma_index * DMA_SEGMENT_SIZE]);
		} else {
			dmaStreamSetMemory1(dmastp, &dma_buffer[++dma_index * DMA_SEGMENT_SIZE]);
		}
		dmaStreamClearInterrupt(dmastp);
		return;
	}
}

#else

static void dma_interrupt(void *p, uint32_t flags) {
	(void)p;

	dma_flags = flags;
	dmaStreamClearInterrupt(dmastp);
	if(flags & (STM32_DMA_ISR_FEIF | STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) {
		/*
		 * DMA transfer error, FIFO error or Direct mode error.
		 * See 9.34.19 of RM0430.
		 */
		dmaStreamClearInterrupt(dmastp);
		TIM8->DIER &= ~TIM_DIER_CC1DE;
		dma_error = true;
		return;
	}

	if((flags & STM32_DMA_ISR_TCIF) != 0) {
		/*
		 * If DMA has run to end within a frame then this is an error.
		 * In single buffer mode DMA should always be terminated by VSYNC.
		 *
		 * Disable TIM8 and DMA trigger.
		 * Dont stop the DMA here. Its going to be stopped by the  leading edge of VSYNC.
		 */
		TIM8->DIER &= ~TIM_DIER_CC1DE;
		dma_error = true;
		return;
	}
}

#endif /* USE_OV5640_DMA_DBM */

/*
 * VSYNC is asserted during a frame.
 * See OV5640 datasheet for details.
 */
CH_IRQ_HANDLER(Vector5C) {
	CH_IRQ_PROLOGUE();

	// VSYNC handling
	if(!vsync) {
		// Increase AHB clock to 48 MHz
		set48MHz();

		/*
		 * Rising edge of VSYNC after TIM8 has been initialised.
		 * Start DMA channel.
		 * Enable TIM8 trigger of DMA.
		 */
		dma_start();
		TIM8->DIER |= TIM_DIER_CC1DE;
		vsync = true;
	} else {
		// Reduce AHB clock to 6 MHz
		set6MHz();

		/* VSYNC leading with vsync true.
		 * This means end of capture for the frame.
		 * Stop & release the DMA channel.
		 * Disable TIM8 trigger of DMA.
		 * If buffer was filled in DMA then that is an error.
		 * We check that here.
		 */
		dma_stop();
		TIM8->DIER &= ~TIM_DIER_CC1DE;

		/*
		 * Disable VSYNC edge interrupts.
		 * Flag image capture complete.
		 */
		nvicDisableVector(EXTI1_IRQn);
		capture_finished = true;
	}

	EXTI->PR |= EXTI_PR_PR1;
	CH_IRQ_EPILOGUE();
}

bool OV5640_Capture(uint8_t* buffer, uint32_t size)
{
	/*
	 * Note:
	 *  If there are no Chibios devices enabled that use DMA then...
	 *  In makefile add entry to UDEFS:
	 *   UDEFS = -DSTM32_DMA_REQUIRED
	 */

	/* Setup DMA for transfer on TIM8_CH1 - DMA2 stream 2, channel 7 */
	dmastp  = STM32_DMA_STREAM(STM32_DMA_STREAM_ID(2, 2));
	uint32_t dmamode = STM32_DMA_CR_CHSEL(7) |
	STM32_DMA_CR_PL(0) |
	STM32_DMA_CR_DIR_P2M |
	STM32_DMA_CR_MSIZE_WORD |
	STM32_DMA_CR_MBURST_INCR4 |
	STM32_DMA_CR_PSIZE_BYTE |
	STM32_DMA_CR_MINC |
	STM32_DMA_CR_DMEIE |
	STM32_DMA_CR_TEIE |
#if OV5640_USE_DMA_DBM == TRUE
    STM32_DMA_CR_DBM |
    STM32_DMA_CR_HTIE |
#endif
	STM32_DMA_CR_TCIE;

	dmaStreamAllocate(dmastp, 1, (stm32_dmaisr_t)dma_interrupt, NULL);

	dmaStreamSetPeripheral(dmastp, &GPIOA->IDR); // We want to read the data from here

#if OV5640_USE_DMA_DBM == TRUE
	dma_buffer = buffer;

    /*
     * Buffer address must be word aligned.
     * Also note requirement for burst transfers from FIFO.
     * Bursts from FIFO to memory must not cross a 1K address boundary.
     * See RM0430 9.3.12
     *
     * TODO: To use DMA_FIFO_BURST_ALIGN in setting of ssdv buffer alignment.
     * Currently this is set to 32 manually in config.c.
     */

    if (((uint32_t)buffer % DMA_FIFO_BURST_ALIGN) != 0) {
      TRACE_ERROR("CAM  > Buffer not allocated on DMA burst boundary");
      return false;
    }
    /*
     * Set the initial buffer addresses.
     * The updating of DMA:MxAR is done in the the DMA interrupt function.
     */
    dmaStreamSetMemory0(dmastp, &buffer[0]);
    dmaStreamSetMemory1(dmastp, &buffer[DMA_SEGMENT_SIZE]);
    dmaStreamSetTransactionSize(dmastp, DMA_SEGMENT_SIZE);

    /*
     * Calculate the number of whole buffers.
     * TODO: Make this include remainder memory as partial buffer?
     */
    dma_buffers = (size / DMA_SEGMENT_SIZE);
    if (dma_buffers == 0) {
      TRACE_ERROR("CAM  > Capture buffer less than minimum DMA segment size");
      return false;
    }
    /* Start with buffer index 0. */
    dma_index = 0;
#else
    dmaStreamSetMemory0(dmastp, buffer);
    dmaStreamSetTransactionSize(dmastp, size);

#endif
    dmaStreamSetMode(dmastp, dmamode); // Setup DMA
    dmaStreamSetFIFO(dmastp, STM32_DMA_FCR_DMDIS | STM32_DMA_FCR_FTH_FULL | STM32_DMA_FCR_FEIE);
    dmaStreamClearInterrupt(dmastp);

    dma_error = false;
    dma_flags = 0;

	/*
	 * Setup timer for PCLK
	 * Setup timer to trigger DMA in capture mode. On rising edge, we will
	 * reach the match and on the falling edge, the timer will be reloaded.
	 */
	rccResetTIM8();
	rccEnableTIM8(FALSE);

	TIM8->ARR = 1;
	TIM8->CCR1 = 0;
	TIM8->CCER = 0;
	TIM8->CCMR1 = TIM_CCMR1_CC1S_0;
	TIM8->CCER = TIM_CCER_CC1E;

	capture_finished = false;
	vsync = false;

	I2C_Lock(); // Lock I2C because it uses the same DMA

	while(!palReadLine(LINE_CAM_VSYNC)); // Wait for current picture to finish transmission

	// Setup EXTI: EXTI1 PC for PC1 (VSYNC)
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PC;
	EXTI->IMR = EXTI_IMR_MR1; // Activate interrupt for chan1 (=>PC1)
	EXTI->RTSR = EXTI_RTSR_TR1; // Listen on rising edge
	nvicEnableVector(EXTI1_IRQn, 1); // Enable interrupt

	// Capture
	do {
		chThdSleepMilliseconds(10);
	} while(!capture_finished && !dma_error);

	// Capture done, unlock I2C
	I2C_Unlock();

	if(dma_error) {
		if(dma_flags & STM32_DMA_ISR_HTIF)
			TRACE_ERROR("CAM  > DMA abort - last buffer segment");
		if(dma_flags & STM32_DMA_ISR_FEIF)
			TRACE_ERROR("CAM  > DMA FIFO error");
		if(dma_flags & STM32_DMA_ISR_TEIF)
			TRACE_ERROR("CAM  > DMA stream transfer error");
		if(dma_flags & STM32_DMA_ISR_DMEIF)
			TRACE_ERROR("CAM  > DMA direct mode error");
		TRACE_ERROR("CAM  > Error capturing image");
		return false;
	}

    TRACE_INFO("CAM  > Capture success");

	return true;
}

/**
  * Initializes GPIO (for pseudo DCMI)
  */
void OV5640_InitGPIO(void)
{
	palSetLineMode(LINE_CAM_PCLK, PAL_MODE_ALTERNATE(3));
	palSetLineMode(LINE_CAM_VSYNC, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_XCLK, PAL_MODE_ALTERNATE(0));
	palSetLineMode(LINE_CAM_D2, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D3, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D4, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D5, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D6, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D7, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D8, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);
	palSetLineMode(LINE_CAM_D9, PAL_MODE_INPUT_PULLUP | PAL_STM32_OSPEED_HIGHEST);

	palSetLineMode(LINE_CAM_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_CAM_RESET, PAL_MODE_OUTPUT_PUSHPULL);
}

void OV5640_TransmitConfig(void)
{
	TRACE_INFO("CAM  > ... Software reset");
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3103, 0x11);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3008, 0x82);
	chThdSleepMilliseconds(100);

	TRACE_INFO("CAM  > ... Initialization");
	for(uint32_t i=0; (OV5640YUV_Sensor_Dvp_Init[i].reg != 0xffff) || (OV5640YUV_Sensor_Dvp_Init[i].val != 0xff); i++)
		I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640YUV_Sensor_Dvp_Init[i].reg, OV5640YUV_Sensor_Dvp_Init[i].val);

	chThdSleepMilliseconds(500);

	TRACE_INFO("CAM  > ... Configure JPEG");
	for(uint32_t i=0; (OV5640_JPEG_QSXGA[i].reg != 0xffff) || (OV5640_JPEG_QSXGA[i].val != 0xff); i++)
		I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_JPEG_QSXGA[i].reg, OV5640_JPEG_QSXGA[i].val);

	TRACE_INFO("CAM  > ... Light Mode: Auto");
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x03); // start group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3406, 0x00);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3400, 0x04);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3401, 0x00);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3402, 0x04);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3403, 0x00);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3404, 0x04);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3405, 0x00);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x13); // end group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0xa3); // lanuch group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5183 ,0x0 );

	TRACE_INFO("CAM  > ... Saturation: 0");
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x03); // start group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5381, 0x1c);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5382, 0x5a);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5383, 0x06);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5384, 0x1a);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5385, 0x66);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5386, 0x80);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5387, 0x82);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5388, 0x80);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5389, 0x02);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x538b, 0x98);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x538a, 0x01);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x13); // end group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0xa3); // launch group 3

	TRACE_INFO("CAM  > ... Brightness: 0");
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x03); // start group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5587, 0x00);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5588, 0x01);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x13); // end group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0xa3); // launch group 3

	TRACE_INFO("CAM  > ... Contrast: 0");
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x03); // start group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x03); // start group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5586, 0x20);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x5585, 0x00);
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0x13); // end group 3
	I2C_write8_16bitreg(OV5640_I2C_ADR, 0x3212, 0xa3); // launch group 3
}

void OV5640_SetResolution(resolution_t res)
{
	TRACE_INFO("CAM  > ... Configure Resolution");
	switch(res) {
		case RES_QQVGA:
			for(uint32_t i=0; (OV5640_QSXGA2QQVGA[i].reg != 0xffff) || (OV5640_QSXGA2QQVGA[i].val != 0xff); i++)
				I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_QSXGA2QQVGA[i].reg, OV5640_QSXGA2QQVGA[i].val);
			break;

		case RES_QVGA:
			for(uint32_t i=0; (OV5640_QSXGA2QVGA[i].reg != 0xffff) || (OV5640_QSXGA2QVGA[i].val != 0xff); i++)
				I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_QSXGA2QVGA[i].reg, OV5640_QSXGA2QVGA[i].val);
			break;

		case RES_VGA:
			for(uint32_t i=0; (OV5640_QSXGA2VGA[i].reg != 0xffff) || (OV5640_QSXGA2VGA[i].val != 0xff); i++)
				I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_QSXGA2VGA[i].reg, OV5640_QSXGA2VGA[i].val);
			break;

		case RES_XGA:
			for(uint32_t i=0; (OV5640_QSXGA2XGA[i].reg != 0xffff) || (OV5640_QSXGA2XGA[i].val != 0xff); i++)
				I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_QSXGA2XGA[i].reg, OV5640_QSXGA2XGA[i].val);
			break;

		case RES_UXGA:
			for(uint32_t i=0; (OV5640_QSXGA2UXGA[i].reg != 0xffff) || (OV5640_QSXGA2UXGA[i].val != 0xff); i++)
				I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_QSXGA2UXGA[i].reg, OV5640_QSXGA2UXGA[i].val);
			break;

		default: // Default QVGA
			for(uint32_t i=0; (OV5640_QSXGA2QVGA[i].reg != 0xffff) || (OV5640_QSXGA2QVGA[i].val != 0xff); i++)
				I2C_write8_16bitreg(OV5640_I2C_ADR, OV5640_QSXGA2QVGA[i].reg, OV5640_QSXGA2QVGA[i].val);
	}
}

void OV5640_init(void)
{
	TRACE_INFO("CAM  > Init pins");
	OV5640_InitGPIO();

	// Power on OV5640
	TRACE_INFO("CAM  > Switch on");
	palSetLine(LINE_CAM_EN); 		// Switch on camera
	palSetLine(LINE_CAM_RESET); 	// Toggle reset

	chThdSleepMilliseconds(100);

	// Send settings to OV5640
	TRACE_INFO("CAM  > Transmit config to camera");
	OV5640_TransmitConfig();

	chThdSleepMilliseconds(200);
}

void OV5640_deinit(void)
{
	// Power off OV5640
	TRACE_INFO("CAM  > Switch off");

	palSetLineMode(LINE_CAM_HREF, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_PCLK, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_VSYNC, PAL_MODE_INPUT);

	palSetLineMode(LINE_CAM_XCLK, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D2, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D3, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D4, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D5, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D6, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D7, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D8, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_D9, PAL_MODE_INPUT);

	palSetLineMode(LINE_CAM_EN, PAL_MODE_INPUT);
	palSetLineMode(LINE_CAM_RESET, PAL_MODE_INPUT);
}

bool OV5640_isAvailable(void)
{
	// Configure pins
	OV5640_InitGPIO();

	// Switch on camera
	palSetLine(LINE_CAM_EN); 		// Switch on camera
	palSetLine(LINE_CAM_RESET); // Toggle reset

	chThdSleepMilliseconds(100);

	uint8_t val, val2;
	bool ret;

	if(I2C_read8_16bitreg(OV5640_I2C_ADR, 0x300A, &val) && I2C_read8_16bitreg(OV5640_I2C_ADR, 0x300B, &val2)) {
		ret = val == 0x56 && val2 == 0x40;
	} else {
		ret = false;
	}

	palClearLine(LINE_CAM_EN); // Switch off camera
	palSetLineMode(LINE_CAM_RESET, PAL_MODE_INPUT);	// CAM_RESET

	return ret;
}

uint32_t OV5640_getLightIntensity(void)
{
	uint8_t val1,val2,val3;
	I2C_read8_16bitreg(OV5640_I2C_ADR, 0x3C1B, &val1);
	I2C_read8_16bitreg(OV5640_I2C_ADR, 0x3C1C, &val2);
	I2C_read8_16bitreg(OV5640_I2C_ADR, 0x3C1D, &val3);
	return (val1 << 16) | (val2 << 8) | val3;
}

