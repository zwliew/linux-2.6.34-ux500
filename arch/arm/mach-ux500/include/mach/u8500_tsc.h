/*
 * Overview:
 *  	 Touch panel register definitions
 *
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */



#ifndef _TOUCHP_NOMADIK_H
#define _TOUCHP_NOMADIK_H

#include <mach/hardware.h>

/**
 * Touchpanel related macros declaration
 */
#define I2C3_TOUCHP_ADDRESS 0x5C/* I2C slave address */
#define TOUCHP_IRQ	EGPIO_PIN_3	/* PENIRQNO egpio3   */
#define TOUCHP_CS0	EGPIO_PIN_2	/* Chip select egpio2 */

#define DRIVER_TP "u8500_tp"
/*
 *TOUCH SCREEN DRIVER MACROS used for Power on Sequence
 */
#define TSC_INT_CLR 0xE8
#define TSC_INT_MODE 0xE9
#define TSC_GAIN 0xEA
#define TSC_OFFSET_MODE 0xEB
#define TSC_XY_EDGE 0xEC
#define TSC_RESET 0xED
#define TSC_CALIB 0xEE
#define TSC_DONE 0xEF
#define TSC_SENSOR_0_7 0xF0
#define TSC_SENSOR_8_15 0xF1
#define TSC_SENSOR_16_23 0xF2
#define TSC_POS_MODE1 0xF3
#define TSC_POS_MODE2 0xF4
#define TSC_CLK_MODE 0xF5
#define TSC_IDLE 0xFA
#define TSC_FILTER 0xFB
#define TSC_TH_ON 0xFC
#define TSC_TH_OFF 0xFD
#define TSC_INTR_STATUS 0x7B
#define TSC_POS_X1 0x73
#define TSC_POS_Y1 0x75
#define TSC_POS_X2 0x77
#define TSC_POS_Y2 0x79

#define	MAX_10BIT ((1<<10)-1)


#define TSC_CLK_MODE_VAL1 0x82
#define TSC_CLK_MODE_VAL2 0x81
#define TSC_IDLE_VAL 0x11
#define TSC_INT_MODE_VAL 0x00
#define TSC_FILTER_VAL 0xFF
#define TSC_TH_ON_VAL 0x20
#define TSC_TH_OFF_VAL 0x18
#define TSC_GAIN_VAL 0x03
#define TSC_OFFSET_MODE_VAL 0x00
#define TSC_XY_EDGE_VAL 0xA5
#define TSC_DONE_VAL 0x01


/*
 * Resolutions
 */
/*Panel Resolution, Target size. (864*480)*/
#define X_MAX (480)
#define Y_MAX (864)

/*Touchpanel Resolution */
#define TOUCH_XMAX 384
#define TOUCH_YMAX 704

#define TRUE (1)
#define FALSE (0)

#define SET (1)
#define CLR (0)

#define START (0)
#define STOP (-1)

/*Direction Indicator */
#define DIR_INVALID (0)
#define DIR_LEFT (1)
#define DIR_RIGHT (2)
#define DIR_UP (3)
#define DIR_DOWN (4)

/* Pinch */
#define PINCH_KEEP (0)
#define PINCH_IN (1)
#define PINCH_OUT (2)

/* Rotate */
#define ROTATE_INVALID (0)
#define ROTATE_R_UR (1)
#define ROTATE_R_RD (2)
#define ROTATE_R_DL (3)
#define ROTATE_R_LU (4)
#define ROTATE_L_LD (5)
#define ROTATE_L_DR (6)
#define ROTATE_L_RU (7)
#define ROTATE_L_UL (8)

/* Gesture Information */
#define GES_FLICK 0x01
#define GES_TAP 0x02
#define GES_PINCH 0x03
#define GES_DRAGDROP 0x04
#define GES_TOUCHSTART 0x05
#define GES_TOUCHEND 0x06
#define GES_MOVE 0x07
#define GES_ROTATE 0x08
#define GES_UNKNOWN 0xff

/* Tap times */
#define TAP_SINGLE 0x01
#define TAP_DOUBLE 0x02
#define TAP_TRIPLE 0x03

/* Speed */
#define LOWSPEED 1
#define HIGHSPEED 2
#define THRESHOLD_TAPLIMIT 60
#define THRESHOLD_FLICK 60
#define THRESHOLD_FLICK_SPEED 300
#define THRESHOLD_PINCH_SPEED 3000
#define THRESHOLD_DRAGDROP 100
#define THRESHOLD_ROTATE 3
#define THRESHOLD_ROTATE_HIST 8
#define THRESHOLD_PINCH 500
#define DIRHEADER 6
#define DIRTRACEN 32
#define SMA_N 80
#define SMA_ARRAY 81
#define THRESHOLD_SMA_N SMA_N
#define MULTITOUCH_SIN_N 6
#define PENUP_TIMEOUT (10) /* msec */

/**
 * Error handling messages
 **/
typedef enum {
	TSC_OK = 0,
	TSC_BAD_PARAMETER = -2,
	TSC_FEAT_NOT_SUPPORTED = -3,
	TSC_INTERNAL_ERROR = -4,
	TSC_TIMEOUT_ERROR = -5,
	TSC_INITIALIZATION_ERROR = -6,
	TSC_I2C_ERROR = -7,
	TSC_ERROR = -8
} tsc_error;

/**
 * struct tp_device - Handle the platform data
 * @cs_en:	pointer to the cs enable function
 * @cs_dis:	pointer to the cs disable function
 * @irq_init:    pointer to the irq init function
 * @irq_exit: pointer to the irq exit function
 * @pirq_en: pointer to the pen irq en function
 * @pirq_dis:	pointer to the pen irq disable function
 * @pirq_read_val:    pointer to read the pen irq value function
 * @board_href_v1: pointer to the get the board version
 * @irq: irq variable
 * This is used to handle the platform data
 **/
struct tp_device {
	int (*cs_en)(void);
	int (*cs_dis)(void);
	int (*irq_init)(void (*callback)(void *parameter), void *p);
	int (*irq_exit)(void);
	int (*pirq_en) (void);
	int (*pirq_dis)(void);
	int (*pirq_read_val)(void);
	int (*board_href_v1)(void);
	unsigned int irq;
};

/**
 * struct touch_point - x and y co-ordinates of touch panel
 * @x: x co-ordinate
 * @y: y co-ordinate
 * This is used to hold the x and y co-ordinates of touch panel
 **/
struct touch_point {
	signed short x;
	signed short y;
};

/**
 * struct gesture_info - hold the gesture of the touch
 * @gesture_kind: gesture kind variable
 * @pt: arry to touch_point structure
 * @dir: direction variable
 * @times: touch times
 * @speed: speed of the touch
 * This is used to hold the gesture of the touch.
 **/
struct gesture_info {
	signed short gesture_kind;
	struct touch_point pt[2];
	signed short dir;
	signed short times;
	signed short speed;
};

/**
 * struct u8500_tsc_data - touch panel data structure
 * @client:	pointer to the i2c client
 * @chip:	pointer to the touch panel controller
 * @pin_dev:    pointer to the input device structure
 * @penirq_timer: variable to the timer list structure
 * @touchp_event: variable to the wait_queue_head_t structure
 * @touch_en: variable for reporting the co-ordinates to input device.
 * @finger1_pressed:      variable to indicate the first co-ordinates.
 * @finger2_pressed:      variable to indicate the first co-ordinates.
 * @m_tp_timer_int_wq:    variable to work structure for timer
 * @m_tp_timer_gpio_wq:    variable to work structure for interrupt
 * @gesture_info: variable to gesture_info structure
 * @touch_count:  variable to maintain sensors input count
 * @touchflag: variable to indicate the touch
 * @pre_tap_flag: flag to indicate the pre tap
 * @flick_flag: flickering flag
 * @touch_continue: to continue the touch flag
 * @pre_tap_flag_level: pre tap level flag
 * @x1: x1 co-ordinate
 * @y1: y1 co-ordinate
 * @x2: x2 co-ordinate
 * @y2: y2 co-ordinate
 * @pinch_start: pinch start
 * @tap_start_point: variable to touch_point structure
 * @dir_trace: array for data trace
 * @dir_idx: id for direction
 * @rotate_data: array to maintain the rotate data
 * @href_v1_flag: variable to indicate the href v1 board
 * @finger2_count: count for finger2 touches
 *
 * Tocuh panel data structure
 */
struct u8500_tsc_data {
	struct i2c_client *client;
	struct tp_device *chip;
	struct input_dev *pin_dev;
	struct task_struct *touchp_tsk;
	struct timer_list penirq_timer;
	wait_queue_head_t touchp_event;
	unsigned short touch_en;
	unsigned short finger1_pressed;
	unsigned short finger2_pressed;
	struct work_struct m_tp_timer_int_wq;
	struct work_struct m_tp_gpio_int_wq;
	struct gesture_info gesture_info;
	signed long touch_count;
	unsigned short touchflag;
	unsigned char pre_tap_flag;
	unsigned char flick_flag;
	unsigned char touch_continue;
	unsigned char pre_tap_flag_level;
	signed short x1, y1;
	signed short x2, y2;
	unsigned char pinch_start;
	struct touch_point tap_start_point;
	unsigned char dir_trace[DIRHEADER+DIRTRACEN];
	unsigned char dir_idx;
	unsigned char rotate_data[5][5];
	bool href_v1_flag;
	u8 finger2_count;
};

int doCalibrate(struct i2c_client *i2c);
int getCalibStatus(struct i2c_client *i2c);
void init_config(struct u8500_tsc_data *data);
void get_touch(struct u8500_tsc_data *data);
void touch_calculation(struct gesture_info *p_gesture_info);
int get_touch_message(struct u8500_tsc_data *data);
void check_board(struct u8500_tsc_data *data);
#endif
