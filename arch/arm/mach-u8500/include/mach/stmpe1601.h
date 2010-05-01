/*--------------------------------------------------------------------------------------------------*/
/*© copyright ST Ericsson 2009									    */
/*									                            */
/* This program is free software; you can redistribute it and/or modify it under	            */
/* the terms of the GNU General Public License as published by the Free	                            */
/* Software Foundation; either version 2.1 of the License, or (at your option) 	                    */
/* any later version.							                            */
/*									                            */
/* This program is distributed in the hope that it will be useful, but WITHOUT	                    */
/* ANY WARRANTY; without even the implied warranty of 			                            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See	                                    */
/* the GNU General Public License for more details.			                            */
/*									                            */
/* You should have received a copy of the GNU General Public License  	                            */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.          	            */
/*--------------------------------------------------------------------------------------------------*/



#ifndef __STMPE1601_H_
#define __STMPE1601_H_

#include <linux/gpio.h>

/*
 * STMPE interrupt numbers
 */
#if defined(CONFIG_MACH_U8500_MOP)
#define STMPE16010_INTR  		218 /*GPIO_PIN_218- TODO*/
#endif

/*Register definition*/

/*System registers Index*/
#define CHIP_ID_Index    		0x80
#define VERSION_ID_Index 		0x81
#define SYSCON_Index     		0x02
#define SYSCON_Index_2     		0x03

/*Interrupt registers Index*/
#define ICR_Msb_Index     0x10  /*Interrupt Control register*/
#define ICR_Lsb_Index     0x11
#define IER_Msb_Index     0x12  /*Interrupt Enable Mask register*/
#define IER_Lsb_Index     0x13
#define ISR_Msb_Index     0x14  /*Interrupt Status register*/
#define ISR_Lsb_Index     0x15
#define IEGPIOR_Msb_Index 0x16  /*Interrupt Enable GPIO Mask register*/
#define IEGPIOR_Lsb_Index 0x17
#define ISGPIOR_Msb_Index 0x18  /*Interrupt Status GPIO registers*/
#define ISGPIOR_Lsb_Index 0x19


/*Keypad Controller Registers*/
#define KPC_COL_Index        0x60  /*Keypad column register I2C index*/
#define KPC_ROW_Msb_Index    0x61
#define KPC_ROW_Lsb_Index    0x62
#define KPC_CTRL_Msb_Index   0x63
#define KPC_CTRL_Lsb_Index   0x64
#define KPC_COMBI_KEY_0      0x65
#define KPC_COMBI_KEY_1      0x66
#define KPC_COMBI_KEY_2      0x67
#define KPC_DATA_BYTE0_Index 0x68
#define KPC_DATA_BYTE1_Index 0x69
#define KPC_DATA_BYTE2_Index 0x6a
#define KPC_DATA_BYTE3_Index 0x6b
#define KPC_DATA_BYTE4_Index 0x6c

/*Gpio's defines*/
/*GPIO Set Pin State register Index*/
#define STMPE1601_GPIO_REG_OFFSET(offset) (0x80 + (offset))
#define GPSR_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x02) //0x82
#define GPSR_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x03) //0x85
/*GPIO Clear Pin State register Index*/
#define GPCR_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x04) //0x86
#define GPCR_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x05) //0x87
/*GPIO Monitor Pin register Index*/
#define GPMR_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x06) //0x88
#define GPMR_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x07) //0x89
/*GPIO Set Pin Direction register*/
#define GPDR_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x08) //0x8A
#define GPDR_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x09) //0x8B
/*GPIO Edge Detect Status register*/
#define GPEDR_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x0A) //0x8C
#define GPEDR_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x0B) //0x8D
/*GPIO Rising Edge register*/
#define GPRER_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x0C) //0x8E
#define GPRER_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x0D) //0x8F
/*GPIO Falling Edge register*/
#define GPFER_Msb_Index      STMPE1601_GPIO_REG_OFFSET(0x0E) //0x90
#define GPFER_Lsb_Index      STMPE1601_GPIO_REG_OFFSET(0x0F) //0x91
/*GPIO Pull Up register*/
#define GPPUR_Msb_Index     STMPE1601_GPIO_REG_OFFSET(0x10) //0x92
#define GPPUR_Lsb_Index     STMPE1601_GPIO_REG_OFFSET(0x11) //0x93

/*GPIO Alternate Function register*/
#define GPAFR_U_Msb_Index   STMPE1601_GPIO_REG_OFFSET(0x12) //0x94   /*Gpio alternate function register*/
#define GPAFR_U_Lsb_Index   STMPE1601_GPIO_REG_OFFSET(0x13) //0x95

#define GPAFR_L_Msb_Index   STMPE1601_GPIO_REG_OFFSET(0x14) //0x96
#define GPAFR_L_Lsb_Index   STMPE1601_GPIO_REG_OFFSET(0x15) //0x97
/*Level translator enable register */
#define GPLT_EN_Index   STMPE1601_GPIO_REG_OFFSET(0x16) //0x98
/*Level translator direction register */
#define GPLT_DIR_Index   STMPE1601_GPIO_REG_OFFSET(0x17) //0x99

#if defined (CONFIG_MACH_U8500_MOP)
#define	STMPE1601_GPIO_INT		15
#define KEYPAD_INT			(STMPE1601_GPIO_INT+1)
#define STMPE1601_MAX_INT		KEYPAD_INT
#endif

/*gpio_cfg related define*/
#define MAX_STMPE1601_GPIO		16	/*max number of STMPE1601 gpio allowed*/

/*keypad related define*/
#define STMPE1601_SCAN_ON		1
#define STMPE1601_SCAN_OFF		0

#define STMPE1601_MASK_NO_KEY	0x78 /*code for no key*/

#define STMPE1601_KEY(col,row)	(col + (row << 3))	/*macro for key definition*/

/**
* typedef struct t_stmpe1601_key_config
* Keypad configuration, platform specific settings
*/
typedef struct
{
	unsigned short columns;			//bit-field , 1=column used, 0=column not used
	unsigned short rows;				//bit-field , 1=row used, 0=row not used
	unsigned char  ncycles;			//number of cycles for key data updating
	unsigned char  debounce;			//de-bounce time (0-128)ms
	unsigned char  scan;				//scan status, ON or OFF

}t_stmpe1601_key_config;

/**
* typedef struct t_stmpe1601_key_status
* Data structure to save key status during last scan.
* E.g., no of keys pressed/released etc
*/
typedef struct
{
	unsigned char button_pressed;		//number of button pressed
	unsigned char button[5];			//id of buttons, 0 to 77
	unsigned char button_released;		//number of button released
	unsigned char released[5];		//id of buttons released, 0 to 77

}t_stmpe1601_key_status;

/**
* typedef struct t_stmpe1601_device_config
* General configuration
*/
typedef struct
{
	unsigned char sys_con;
	unsigned char sys_con_2;
	t_stmpe1601_key_config	key_cfg;	/*not used*/

} t_stmpe1601_device_config;

/**
* typedef struct t_stmpe1601_info
* general device info
*/
typedef struct
{
	unsigned char chip_id;
	unsigned char version_id;

} t_stmpe1601_info;
/**
* typedef struct t_stmpe1601_syscon_ds
* data structire to save control register settings
*/
typedef struct
{
	unsigned char syscon_data;
	unsigned char syscon_2_data;

} t_stmpe1601_syscon_ds;

/**
* typedef struct t_stmpe1601_interrupt_ds
* data structure to save interrupt controllder register values
*/
typedef struct
{
	/*ICR register info*/
	unsigned char icr_msb_data;
	unsigned char icr_lsb_data;

	/*IER register info*/
	unsigned char ier_msb_data;
	unsigned char ier_lsb_data;

	/*ISR register info*/
	unsigned char isr_msb_data;
	unsigned char isr_lsb_data;

	/*IEGPIOR register info*/
	unsigned char iegpior_msb_data;
	unsigned char iegpior_lsb_data;

	/*ISGPIOR register info*/
	unsigned char isgpior_msb_data;
	unsigned char isgpior_lsb_data;

} t_stmpe1601_interrupt_ds;

/**
* typedef struct t_stmpe1601_kpc_ds
* Data structure to save kpd controller resgister values
*/
typedef struct
{
	unsigned char kpc_col_data;
	unsigned char kpc_row_msb_data;
	unsigned char kpc_row_lsb_data;
	unsigned char kpc_ctrl_msb_data;
	unsigned char kpc_ctrl_lsb_data;
	unsigned char kpc_data_byte0_data;
	unsigned char kpc_data_byte1_data;
	unsigned char kpc_data_byte2_data;
	unsigned char kpc_data_byte3_data;
	unsigned char kpc_data_byte4_data;
	unsigned char kpc_data_byte5_data;
}t_stmpe1601_kpc_ds;

/**
* struct stmpe1601_platform_data
* Pltform data to save gpio base address
* @gpio_base:	start index for STMPE1601 expanded gpio: 268+24 for u8500 platform
* @irq:	Interrupt no. for STMPE1601, For U8500 platform interrupt is through GPIO218
*/
struct stmpe1601_platform_data {
	unsigned	gpio_base;
	int irq;
};

/*
* interrupt handler regiser and unregister functions
*/

/**
* stmpe1601_remove_callback() - remove a callback handler
* @irq:        gpio number
* This funtion removes the callback handler for the client device
*/
int stmpe1601_remove_callback(int irq);

/**
* stmpe1601_set_callback() - install a callback handler
* @irq:        gpio number
* @handler:    funtion pointer to the callback handler
* @data:       data pointer to be passed to the specific handler
* This funtion install the callback handler for the client device
*/
int stmpe1601_set_callback(int irq, void *handler, void *data);

/*
* keypad related functions
*/

/**
 * stmpe1601_keypad_init - initialises Keypad matrix row and columns
 * @kpconfig:    keypad configuration for a platform
 * This function configures keypad control registers of stmpe1601
 * The keypad driver should call this function to configure keypad matrix
 *
 */
int stmpe1601_keypad_init(t_stmpe1601_key_config kpconfig);
/**
 * stmpe1601_keypad_scan: start/stop keypad scannig
 * @status:    flag for enable/disable STMPE1601_SCAN_ON or STMPE1601_SCAN_OFF
 *
 */
int stmpe1601_keypad_scan(unsigned char status);

/**
 * stmpe1601_keypressed :  This function read keypad data registers
 * @keys: o/p parameter, returns keys pressed.
 * This function can be used in both polling or interrupt usage.
 */
int stmpe1601_keypressed(t_stmpe1601_key_status *keys);
/**
 * stmpe1601_read_info() - read the chip information
 * This function read stmpe1601 chip and version ID
 * and returns error if chip id or version id is not correct.
 * This function can be called to check if UIB is connected or not.
 */
int stmpe1601_read_info(void);

/**
* stmpe1601_irqen() - enables corresponding interrupt mask
* @irq:         interrupt no.
*/
int stmpe1601_irqen(int irq);
/**
* stmpe1601_irqdis() - disables corresponding interrupt mask
* @irq:         interrupt no.
**/
int stmpe1601_irqdis(int irq);




#endif


