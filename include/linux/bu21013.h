/*
 * Copyright (C) ST-Ericsson SA 2009
 * Author: Naveen Kumar G <naveen.gaddipati@stericsson.com> for ST-Ericsson
 * License terms:GNU General Public License (GPL) version 2
 */

#ifndef _BU21013_H
#define _BU21013_H

/*
 * Touch screen register offsets
 */
#define BU21013_SENSORS_BTN_0_7 0x70
#define BU21013_SENSORS_BTN_8_15 0x71
#define BU21013_SENSORS_BTN_16_23 0x72
#define BU21013_X1_POS_MSB 0x73
#define BU21013_X1_POS_LSB 0x74
#define BU21013_Y1_POS_MSB 0x75
#define BU21013_Y1_POS_LSB 0x76
#define BU21013_X2_POS_MSB 0x77
#define BU21013_X2_POS_LSB 0x78
#define BU21013_Y2_POS_MSB 0x79
#define BU21013_Y2_POS_LSB 0x7A
#define BU21013_INT_CLR 0xE8
#define BU21013_INT_MODE 0xE9
#define BU21013_GAIN 0xEA
#define BU21013_OFFSET_MODE 0xEB
#define BU21013_XY_EDGE 0xEC
#define BU21013_RESET 0xED
#define BU21013_CALIB 0xEE
#define BU21013_REG_DONE 0xEF
#define BU21013_SENSOR_0_7 0xF0
#define BU21013_SENSOR_8_15 0xF1
#define BU21013_SENSOR_16_23 0xF2
#define BU21013_POS_MODE1 0xF3
#define BU21013_POS_MODE2 0xF4
#define BU21013_CLK_MODE 0xF5
#define BU21013_IDLE 0xFA
#define BU21013_FILTER 0xFB
#define BU21013_TH_ON 0xFC
#define BU21013_TH_OFF 0xFD

/*
 * Touch screen register offset values
 */
#define BU21013_INTR_CLEAR 0x01
#define BU21013_INTR_ENABLE 0x00
#define BU21013_CALIB_ENABLE 0x01

#define BU21013_RESET_ENABLE 0x01

/* Sensors Configuration */
#define BU21013_SENSORS_EN_0_7 0x3F
#define BU21013_SENSORS_EN_8_15 0xFC
#define BU21013_SENSORS_EN_16_23 0x1F

/* Position mode1 */
#define BU21013_POS_MODE1_0 0x02
#define BU21013_POS_MODE1_1 0x04
#define BU21013_POS_MODE1_2 0x08

/* Position mode2 */
#define BU21013_POS_MODE2_ZERO 0x01
#define BU21013_POS_MODE2_AVG1 0x02
#define BU21013_POS_MODE2_AVG2 0x04
#define BU21013_POS_MODE2_EN_XY 0x08
#define BU21013_POS_MODE2_EN_RAW 0x10
#define BU21013_POS_MODE2_MULTI 0x80

/* Clock mode */
#define BU21013_CLK_MODE_DIV 0x01
#define BU21013_CLK_MODE_EXT 0x02
#define BU21013_CLK_MODE_CALIB 0x80

/* IDLE time */
#define BU21013_IDLET_0 0x01
#define BU21013_IDLET_1 0x02
#define BU21013_IDLET_2 0x04
#define BU21013_IDLET_3 0x08
#define BU21013_IDLE_INTERMIT_EN 0x10

/* FILTER reg values */
#define BU21013_DELTA_0_6 0x7F
#define BU21013_FILTER_EN 0x80

/* interrupt mode */
#define BU21013_INT_MODE_LEVEL 0x00
#define BU21013_INT_MODE_EDGE 0x01

/* Gain reg values */
#define BU21013_GAIN_0 0x01
#define BU21013_GAIN_1 0x02
#define BU21013_GAIN_2 0x04

/* OFFSET mode */
#define BU21013_OFFSET_MODE_DEFAULT 0x00
#define BU21013_OFFSET_MODE_MOVE 0x01
#define BU21013_OFFSET_MODE_DISABLE 0x02

/* Threshold ON values */
#define BU21013_TH_ON_0 0x01
#define BU21013_TH_ON_1 0x02
#define BU21013_TH_ON_2 0x04
#define BU21013_TH_ON_3 0x08
#define BU21013_TH_ON_4 0x10
#define BU21013_TH_ON_5 0x20
#define BU21013_TH_ON_6 0x40
#define BU21013_TH_ON_7 0x80

#define BU21013_TH_ON_MAX 0xFF

/* Threshold OFF values */
#define BU21013_TH_OFF_0 0x01
#define BU21013_TH_OFF_1 0x02
#define BU21013_TH_OFF_2 0x04
#define BU21013_TH_OFF_3 0x08
#define BU21013_TH_OFF_4 0x10
#define BU21013_TH_OFF_5 0x20
#define BU21013_TH_OFF_6 0x40
#define BU21013_TH_OFF_7 0x80

#define BU21013_TH_OFF_MAX 0xFF

/* FILTER reg values */
#define BU21013_X_EDGE_0 0x01
#define BU21013_X_EDGE_1 0x02
#define BU21013_X_EDGE_2 0x04
#define BU21013_X_EDGE_3 0x08
#define BU21013_Y_EDGE_0 0x10
#define BU21013_Y_EDGE_1 0x20
#define BU21013_Y_EDGE_2 0x40
#define BU21013_Y_EDGE_3 0x80

#define BU21013_DONE 0x01

/**
 * struct bu21013_platform_device - Handle the platform data
 * @cs_en:	pointer to the cs enable function
 * @cs_dis:	pointer to the cs disable function
 * @irq_init:    pointer to the irq init function
 * @irq_exit: pointer to the irq exit function
 * @pirq_en: pointer to the pen irq en function
 * @pirq_dis:	pointer to the pen irq disable function
 * @pirq_read_val:    pointer to read the pen irq value function
 * @board_check: pointer to the get the board version
 * @x_max_res: xmax resolution
 * @y_max_res: ymax resolution
 * @touch_x_max: touch x max
 * @touch_y_max: touch y max
 * @tp_cntl: controller id
 * @cs_pin: chip select pin
 * @irq: irq pin
 * @ext_clk_en: external clock flag
 * @portrait: portrait mode flag
 * @edge_mode: edge mode flag
 * This is used to handle the platform data
 **/
struct bu21013_platform_device {
	int (*cs_en)(int reset_pin);
	int (*cs_dis)(int reset_pin);
	int (*irq_init)(void (*callback)(void *parameter), void *p);
	int (*irq_exit)(void);
	int (*pirq_en) (void);
	int (*pirq_dis)(void);
	int (*pirq_read_val)(void);
	bool (*board_check)(void);
	int x_max_res;
	int y_max_res;
	int touch_x_max;
	int touch_y_max;
	int tp_cntl;
	unsigned int cs_pin;
	unsigned int irq;
	bool portrait;
	bool ext_clk;
	bool edge_mode;
};

/**
 * struct bu21013_touch_point - x and y co-ordinates of touch panel
 * @x: x co-ordinate
 * @y: y co-ordinate
 * This is used to hold the x and y co-ordinates of touch panel
 **/
struct bu21013_touch_point {
	signed short x;
	signed short y;
};

/**
 * struct bu21013_gesture_info - hold the gesture of the touch
 * @gesture_kind: gesture kind variable
 * @pt: arry to touch_point structure
 * @dir: direction variable
 * @times: touch times
 * @speed: speed of the touch
 * This is used to hold the gesture of the touch.
 **/
struct bu21013_gesture_info {
	signed short gesture_kind;
	struct bu21013_touch_point pt[2];
	signed short dir;
	signed short times;
	signed short speed;
};

/**
 * struct bu21013_ts_data - touch panel data structure
 * @client:	pointer to the i2c client
 * @chip:	pointer to the touch panel controller
 * @pin_dev:    pointer to the input device structure
 * @penirq_timer: variable to the timer list structure
 * @touch_en: variable for reporting the co-ordinates to input device.
 * @finger1_pressed:      variable to indicate the first co-ordinates.
 * @finger2_pressed:      variable to indicate the first co-ordinates.
 * @tp_timer_handler:    variable to work structure for timer
 * @tp_gpio_handler:    variable to work structure for gpio interrupt
 * @gesture_info: variable to bu21013_gesture_info structure
 * @touch_count:  variable to maintain sensors input count
 * @touchflag: variable to indicate the touch
 * @pre_tap_flag: flag to indicate the pre tap
 * @flick_flag: flickering flag
 * @touch_continue: to continue the touch flag
 * @pre_tap_flag_level: pre tap level flag
 * @x1: x1 value
 * @y1: y1 vlaue
 * @x2: x2 value
 * @y2: y2 value
 * @pinch_start: pinch start
 * @tap_start_point: variable to bu21013_touch_point structure
 * @dir_trace: array for data trace
 * @dir_idx: id for direction
 * @rotate_data: array to maintain the rotate data
 * @board_flag: variable to indicate the href v1 board
 * @finger2_count: count for finger2 touches
 * @intr_pin: interrupt pin value
 * @last_press: sensor button pressed
 * @prev_press_report: last reported flag
 * @touchp_kobj: pointer to kernel object for touch panel
 * @gain: gain value
 * @th_on: threshold on value
 * @th_off: threshold off value
 * @penup_timer: timeout value
 *
 * Tocuh panel data structure
 */
struct bu21013_ts_data {
	struct i2c_client *client;
	struct bu21013_platform_device *chip;
	struct input_dev *pin_dev;
	struct timer_list penirq_timer;
	bool touch_en;
	bool finger1_pressed;
	bool finger2_pressed;
	struct work_struct tp_timer_handler;
	struct work_struct tp_gpio_handler;
	struct bu21013_gesture_info gesture_info;
	signed long touch_count;
	bool touchflag;
	bool pre_tap_flag;
	bool flick_flag;
	bool touch_continue;
	unsigned char pre_tap_flag_level;
	unsigned char pinch_start;
	struct bu21013_touch_point tap_start_point;
	unsigned char dir_trace[38];
	unsigned char dir_idx;
	unsigned char rotate_data[5][5];
	bool board_flag;
	u8 finger2_count;
	unsigned int intr_pin;
	signed short	x1;
	signed short	y1;
	signed short	x2;
	signed short	y2;
	int last_press;
	bool prev_press_report;
	struct kobject *touchp_kobj;
	int gain;
	int th_on;
	int th_off;
	int penup_timer;
};
#endif
