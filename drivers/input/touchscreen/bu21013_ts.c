/*
 * Copyright (C) ST-Ericsson SA 2009
 * Author: Naveen Kumar G <naveen.gaddipati@stericsson.com> for ST-Ericsson
 * License terms:GNU General Public License (GPL) version 2
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/bu21013.h>

#define PEN_DOWN_INTR 0
#define PEN_UP_INTR 1
#define RESET_DELAY 30
#define POLLING_DELAY 100
#define MAX_TOOL_WIDTH 15
#define MAX_TOUCH_MAJOR 255
#define MAX_PRESSURE 1
#define PENUP_TIMEOUT (20)
#define SCALE_FACTOR 1000
#define I2C_ADDR_TP2 0x5D

#define START (0)
#define STOP (-1)

#define DIR_INVALID (0)
#define DIR_LEFT (1)
#define DIR_RIGHT (2)
#define DIR_UP (3)
#define DIR_DOWN (4)

#define PINCH_KEEP (0)
#define PINCH_IN (1)
#define PINCH_OUT (2)

#define ROTATE_INVALID (0)
#define ROTATE_R_UR (1)
#define ROTATE_R_RD (2)
#define ROTATE_R_DL (3)
#define ROTATE_R_LU (4)
#define ROTATE_L_LD (5)
#define ROTATE_L_DR (6)
#define ROTATE_L_RU (7)
#define ROTATE_L_UL (8)

#define GES_FLICK 0x01
#define GES_TAP 0x02
#define GES_PINCH 0x03
#define GES_DRAGDROP 0x04
#define GES_TOUCHSTART 0x05
#define GES_TOUCHEND 0x06
#define GES_MOVE 0x07
#define GES_ROTATE 0x08
#define GES_UNKNOWN 0xff

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
#define DIR_TRACE_VALUE 0x05
#define DRIVER_TP "bu21013_tp"

static struct i2c_driver bu21013_tp_driver;
struct bu21013_ts_data *tp_sys_data;

static int bu21013_init_chip(struct bu21013_ts_data *data);

static ssize_t touchp_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	u8 timeout;
	u8 sys_th_on;
	u8 sys_th_off;
	u8 sys_gain;
	u8 sys_ext_clock;
	u8 sys_edge_mode;

	if (strcmp(attr->name, "timeout_value") == 0) {
		timeout = tp_sys_data->penup_timer;
		sprintf(buf, "%d\n", timeout);
	} else if (strcmp(attr->name, "th_on") == 0) {
		sys_th_on = tp_sys_data->th_on;
		sprintf(buf, "%d\n", sys_th_on);
	} else if (strcmp(attr->name, "th_off") == 0) {
		sys_th_off = tp_sys_data->th_off;
		sprintf(buf, "%d\n", sys_th_off);
	} else if (strcmp(attr->name, "gain") == 0) {
		sys_gain = tp_sys_data->gain;
		sprintf(buf, "%d\n", sys_gain);
	} else if (strcmp(attr->name, "ext_clk") == 0) {
		sys_ext_clock = tp_sys_data->chip->ext_clk;
		sprintf(buf, "%d\n", sys_ext_clock);
	} else if (strcmp(attr->name, "edge_mode") == 0) {
		sys_edge_mode = tp_sys_data->chip->edge_mode;
		sprintf(buf, "%d\n", sys_edge_mode);
	}
	return strlen(buf);
}
static ssize_t touchp_attr_store(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t len)
{
	int ret = 0;
	int timeout;
	int sys_th_on;
	int sys_th_off;
	int sys_gain;
	int sys_ext_clock;
	int sys_edge_mode;

	if (strcmp(attr->name, "timeout_value") == 0) {
		ret = sscanf(buf, "%d", &timeout);
		if (ret == 1)
			tp_sys_data->penup_timer = timeout;
	} else if (strcmp(attr->name, "th_on") == 0) {
		ret = sscanf(buf, "%d", &sys_th_on);
		if ((ret == 1) && (tp_sys_data->th_on != sys_th_on)) {
			tp_sys_data->th_on = sys_th_on;
			if (tp_sys_data->th_on < BU21013_TH_ON_MAX)
				bu21013_init_chip(tp_sys_data);
		}
	} else if (strcmp(attr->name, "th_off") == 0) {
		ret = sscanf(buf, "%d", &sys_th_off);
		if ((ret == 1) && (tp_sys_data->th_off != sys_th_off)) {
			tp_sys_data->th_off = sys_th_off;
			if (tp_sys_data->th_off < BU21013_TH_OFF_MAX)
				bu21013_init_chip(tp_sys_data);
		}
	} else if (strcmp(attr->name, "gain") == 0) {
		ret = sscanf(buf, "%d", &sys_gain);
		if ((ret == 1) && (tp_sys_data->gain != sys_gain)) {
			tp_sys_data->gain = sys_gain;
			bu21013_init_chip(tp_sys_data);
		}
	} else if (strcmp(attr->name, "ext_clk") == 0) {
		ret = sscanf(buf, "%d", &sys_ext_clock);
		if ((ret == 1) && (tp_sys_data->chip->ext_clk != sys_ext_clock)) {
			tp_sys_data->chip->ext_clk = sys_ext_clock;
			bu21013_init_chip(tp_sys_data);
		}
	} else if (strcmp(attr->name, "edge_mode") == 0) {
		ret = sscanf(buf, "%d", &sys_edge_mode);
		if ((ret == 1) && (tp_sys_data->chip->edge_mode != sys_edge_mode)) {
			tp_sys_data->chip->edge_mode = sys_edge_mode;
			bu21013_init_chip(tp_sys_data);
		}
	}
	return ret == 1 ? len : -EINVAL;
}

static struct attribute touchp_timeout = {.name = "timeout_value",
						.mode = 0666};
static struct attribute touchp_th_on = {.name = "th_on", .mode = 0666};
static struct attribute touchp_th_off = {.name = "th_off", .mode = 0666};
static struct attribute touchp_gain = {.name = "gain", .mode = 0666};
static struct attribute touchp_ext_clk = {.name = "ext_clk", .mode = 0666};
static struct attribute touchp_edge_mode = {.name = "edge_mode", .mode = 0666};

static struct attribute *touchp_attribute[] = {
	&touchp_timeout,
	&touchp_th_on,
	&touchp_th_off,
	&touchp_gain,
	&touchp_ext_clk,
	&touchp_edge_mode,
	NULL
};

static struct sysfs_ops touchp_sysfs_ops = {
	.show = touchp_attr_show,
	.store = touchp_attr_store,
};

static struct kobj_type ktype_touchp = {
	.sysfs_ops = &touchp_sysfs_ops,
	.default_attrs = touchp_attribute,
};

/*
 * bu21013_do_calibrate() - Do the software Calibration
 * @i2c: i2c_client structure pointer
 * Do the soft calibration on the touch screen
 * and returns integer
 */
int bu21013_do_calibrate(struct i2c_client *i2c)
{
	int retval;
	retval = i2c_smbus_write_byte_data(i2c, BU21013_CALIB,
					BU21013_CALIB_ENABLE);
	if (retval < 0)
		dev_err(&i2c->dev, "reset failed retval=0x%x \n", retval);
	return retval;
}
/**
 * bu21013_touch_calc(): Get the exact co-ordinates of the touch interrupt
 * @data:bu21013_ts_data structure pointer
 * Transfer the position information from touch sensor
 * coordinate to display coordinate and returns none.
 */
void bu21013_touch_calc(struct bu21013_ts_data *data)
{
	signed short x1, y1;
	struct bu21013_gesture_info *p_gesture_info = &data->gesture_info;
	int tmpx, tmpy;
	signed short x2, y2;

	tmpx = (data->chip->x_max_res * SCALE_FACTOR / data->chip->touch_x_max);
	tmpy = (data->chip->y_max_res * SCALE_FACTOR / data->chip->touch_y_max);
	x1 = p_gesture_info->pt[0].x;
	y1 = p_gesture_info->pt[0].y;
	x1 = x1 * tmpx / SCALE_FACTOR;
	y1 = y1 * tmpy / SCALE_FACTOR;
	if ((data->chip->portrait) && (y1 != 0))
		y1 = data->chip->y_max_res - y1;
	p_gesture_info->pt[0].x = x1;
	p_gesture_info->pt[0].y = y1;
	x2 = p_gesture_info->pt[1].x;
	y2 = p_gesture_info->pt[1].y;
	x2 = x2 * tmpx / SCALE_FACTOR;
	y2 = y2 * tmpy / SCALE_FACTOR;
	if ((data->chip->portrait) && (y2 != 0))
		y2 = data->chip->y_max_res - y2;
	p_gesture_info->pt[1].x = x2;
	p_gesture_info->pt[1].y = y2;

}

/**
 * bu21013_get_touch(): Get the co-ordinates of the touch interrupt
 * @data:bu21013_ts_data structure pointer
 * Get touched point position
 * from touch sensor registers and write to global variables.
 */
void bu21013_get_touch(struct bu21013_ts_data *data)
{
	struct i2c_client *i2c = data->client;
	int retval;
	u8 sensors_buf[3];
	u8 finger1_buf[4];
	int count;
	signed short	tmp1x = 0;
	signed short	tmp1y = 0;
	u8 finger2_buf[4];
	signed short	tmp2x = 0;
	signed short	tmp2y = 0;
	short touch_x_max = data->chip->touch_x_max;
	short touch_y_max = data->chip->touch_y_max;

	for (count = 0; count < 2; count++) {
		if (!data->chip->edge_mode) {
			retval = i2c_smbus_read_i2c_block_data(i2c, BU21013_SENSORS_BTN_0_7, 3, sensors_buf);
			if (retval < 0)
				goto end;
			data->last_press = sensors_buf[0] | sensors_buf[1] | sensors_buf[2];
		}
		retval = i2c_smbus_read_i2c_block_data(i2c, BU21013_X1_POS_MSB, 4, finger1_buf);
		if (retval < 0)
			goto end;
		tmp1x = (finger1_buf[0] << 2) | finger1_buf[1];
		tmp1y = (finger1_buf[2] << 2) | finger1_buf[3];
		if ((tmp1x != 0) && (tmp1y != 0))
			break;
	}
	data->finger1_pressed = false;
	if ((data->chip->edge_mode || data->last_press) &
		(tmp1x != 0 && tmp1y != 0) &&
		(tmp1x < touch_x_max && tmp1y < touch_y_max))
		data->finger1_pressed = true;

	if (data->finger1_pressed) {
		data->x1 = tmp1x;
		data->y1 = tmp1y;
	} else {
		data->x1 = 0;
		data->y1 = 0;
	}
	for (count = 0; count < 2; count++) {
		retval = i2c_smbus_read_i2c_block_data(i2c, BU21013_X2_POS_MSB, 4, finger2_buf);
		if (retval < 0)
			goto end;
		tmp2x = (finger2_buf[0] << 2) | finger2_buf[1];
		tmp2y = (finger2_buf[2] << 2) | finger2_buf[3];
		if ((tmp2x != 0) && (tmp2y != 0))
			break;
	}
	data->finger2_pressed = false;
	if ((data->chip->edge_mode || data->last_press) &&
		(tmp2x != 0 && tmp2y != 0) &&
		(tmp2x < touch_x_max && tmp2y < touch_y_max))
		data->finger2_pressed = true;
	data->finger2_count = 1;
end:
	if (data->finger2_pressed) {
		data->x2 = tmp2x;
		data->y2 = tmp2y;
	} else {
		data->x2 = 0;
		data->y2 = 0;
	}
	if (data->finger1_pressed || data->finger2_pressed) {
		data->touch_en = true;
		data->touchflag = true;
	} else {
		data->touch_en = false;
		data->touchflag = false;
	}
	if ((!data->finger1_pressed) && (data->finger2_pressed)) {
		data->x1 = tmp2x;
		data->y1 = tmp2y;
		data->x2 = 0;
		data->y2 = 0;
		data->finger2_pressed = false;
	}
}
/**
 * bu21013_update_ges_no_touch(): Generate the gesture message for single touch
 * @data: a pointer to the device structure
 * Generate the gesture message for single touch.
 */
void bu21013_update_ges_st_touch(struct bu21013_ts_data *data)
{
	struct bu21013_gesture_info *p_gesture_info = &data->gesture_info;
	signed short x_delta, y_delta;
	unsigned char dir_left_right, dir_up_down;
	unsigned char tmp;
	unsigned char dir1, dir2;
	static struct bu21013_touch_point pre_gesture_point;
	x_delta = p_gesture_info->pt[0].x - pre_gesture_point.x;
	y_delta = p_gesture_info->pt[0].y - pre_gesture_point.y;
	if (x_delta < THRESHOLD_ROTATE)
		x_delta = 0;

	if (y_delta < THRESHOLD_ROTATE)
		y_delta = 0;

	p_gesture_info->dir = DIR_INVALID;
	if (x_delta != y_delta) {
		dir_left_right = DIR_RIGHT;
		if (x_delta < 0) {
			dir_left_right = DIR_LEFT;
			x_delta = -x_delta;
		}
		dir_up_down = DIR_DOWN;
		if (y_delta < 0) {
			y_delta = -y_delta;
			dir_up_down = DIR_UP;
		}
		p_gesture_info->dir = dir_up_down;
		if (x_delta > y_delta)
			p_gesture_info->dir = dir_left_right;
		tmp = p_gesture_info->dir;
		data->dir_trace[tmp]++;
		data->dir_trace[data->dir_idx] = tmp;
		if (data->dir_idx < (THRESHOLD_ROTATE_HIST + DIRHEADER))
			data->dir_idx++;
		else
			data->dir_idx = DIRHEADER;
		data->dir_trace[data->dir_trace[data->dir_idx]]--;
		dir1 = data->dir_trace[0];
		for (dir2 = 1; dir2 < 5; dir2++) {
			if (data->dir_trace[data->dir_trace[0]]
				 < data->dir_trace[dir2]) {
				data->dir_trace[0] = dir2;
				p_gesture_info->dir
					 = data->rotate_data[dir1][dir2];
				if (p_gesture_info->dir)
					p_gesture_info->gesture_kind
						= GES_ROTATE;
				break;
			}
		}
	}
	if (p_gesture_info->dir == ROTATE_INVALID) {
		p_gesture_info->gesture_kind = GES_MOVE;
		p_gesture_info->speed = data->touch_count;
	}
}
/**
 * bu21013_update_ges_no_touch(): Generate the gesture message for multi touch
 * @data: a pointer to the device structure
 * Generate the gesture message for multi touch.
 */
void bu21013_update_ges_mt_touch(struct bu21013_ts_data *data)
{
	struct bu21013_gesture_info *p_gesture_info = &data->gesture_info;
	signed short x_delta, y_delta;
	signed short area_diff;
	unsigned long	area;
	static unsigned long pre_area;

	x_delta = p_gesture_info->pt[0].x - p_gesture_info->pt[1].x;
	y_delta = p_gesture_info->pt[0].y - p_gesture_info->pt[1].y;
	if (x_delta < 0)
		x_delta = -x_delta;
	if (y_delta < 0)
		y_delta = -y_delta;
	x_delta++;
	y_delta++;
	area = x_delta * y_delta;
	p_gesture_info->dir = PINCH_KEEP;
	area_diff = area - pre_area;
	if (area_diff > 0) {
		if (area_diff > THRESHOLD_PINCH) {
			p_gesture_info->dir = PINCH_OUT;
			if (area_diff < THRESHOLD_PINCH_SPEED)
				p_gesture_info->speed = LOWSPEED;
			else
				p_gesture_info->speed = HIGHSPEED;
		}
	} else if (area_diff < 0) {
		if ((pre_area - area) > THRESHOLD_PINCH) {
			p_gesture_info->dir = PINCH_IN;
			if ((pre_area - area) < THRESHOLD_PINCH_SPEED)
				p_gesture_info->speed = LOWSPEED;
			else
				p_gesture_info->speed = HIGHSPEED;
		}
	}
	if (data->pinch_start == false) {
		p_gesture_info->dir = PINCH_KEEP;
		pre_area = 0;
		data->pinch_start = true;
	}
	p_gesture_info->gesture_kind = GES_PINCH;
	pre_area = area;
}
/**
 * bu21013_update_ges_no_touch(): Generate the gesture message for no touch
 * @data: a pointer to the device structure
 * Generate the gesture message for no touch.
 */
void bu21013_update_ges_no_touch(struct bu21013_ts_data *data)
{
	struct bu21013_gesture_info *p_gesture_info = &data->gesture_info;
	signed short x_delta, y_delta;
	bool flick_series_flag;
	signed short i;
	unsigned char dir_left_right, dir_up_down;
	static struct bu21013_touch_point pre_gesture_point;

	flick_series_flag = false;
	if (data->flick_flag) {
		x_delta = p_gesture_info->pt[0].x - data->tap_start_point.x;
		y_delta = p_gesture_info->pt[0].y - data->tap_start_point.y;
		dir_left_right = DIR_RIGHT;
		dir_up_down = DIR_DOWN;
		if (x_delta < 0) {
			dir_left_right = DIR_LEFT;
			x_delta = -x_delta;
		}
		if (y_delta < 0) {
			y_delta = -y_delta;
			dir_up_down = DIR_UP;
		}
		if ((THRESHOLD_FLICK <= x_delta) || (THRESHOLD_FLICK <= y_delta)) {
			if (THRESHOLD_DRAGDROP <= data->touch_count) {
				p_gesture_info->gesture_kind = GES_DRAGDROP;
				p_gesture_info->speed = data->touch_count;
				p_gesture_info->pt[0].x = data->tap_start_point.x;
				p_gesture_info->pt[0].y = data->tap_start_point.y;
				p_gesture_info->pt[1].x = pre_gesture_point.x;
				p_gesture_info->pt[1].y = pre_gesture_point.y;
			} else {
				p_gesture_info->speed = LOWSPEED;
				p_gesture_info->gesture_kind = GES_FLICK;
				p_gesture_info->dir = dir_up_down;
				if ((THRESHOLD_FLICK_SPEED < x_delta) ||
						(THRESHOLD_FLICK_SPEED < y_delta))
					p_gesture_info->speed = HIGHSPEED;
				if (x_delta > y_delta)
					p_gesture_info->dir = dir_left_right;
			}
			data->pre_tap_flag_level = 0;
			data->flick_flag = false;
			data->touch_count = STOP;
			flick_series_flag = true;
		}
	}
	if (flick_series_flag == false) {
		if (THRESHOLD_TAPLIMIT <= data->touch_count) {
			p_gesture_info->gesture_kind = GES_TAP;
			p_gesture_info->times = data->pre_tap_flag_level;
			p_gesture_info->speed = data->touch_count;
			data->pre_tap_flag_level = 0;
			data->flick_flag = false;
			data->touch_count = STOP;
			data->dir_idx = DIRHEADER;
			for (i = 0; i < DIRHEADER; i++)
				data->dir_trace[i] = 0x0;
			for (i = DIRHEADER; i < DIRTRACEN; i++)
				data->dir_trace[i] = DIR_TRACE_VALUE;
		}
	}
}
/**
 * bu21013_get_touch_mesg(): Generate the gesture message
 * @data: a pointer to the device structure
 * Generate the gesture message according to the
 * information collected and returns integer.
 */
int bu21013_get_touch_mesg(struct bu21013_ts_data *data)
{
	struct bu21013_gesture_info *p_gesture_info = &data->gesture_info;
	static struct bu21013_touch_point pre_gesture_point;
	p_gesture_info->gesture_kind = GES_UNKNOWN;

	bu21013_get_touch(data);
	if (!data->touchflag) {
		if (data->touch_continue) {
			data->touch_continue = false;
			p_gesture_info->gesture_kind = GES_TOUCHEND;
			if (data->pre_tap_flag_level <= 2)
				data->pre_tap_flag_level++;
			data->pre_tap_flag = false;
		} else {
			if (!data->pinch_start) {
				bu21013_update_ges_no_touch(data);
			} else {
				data->pinch_start = false;
				data->pre_tap_flag_level = 0;
				data->flick_flag = false;
				data->touch_count = STOP;
			}
		}
	} else {
		p_gesture_info->pt[0].x = data->x1;
		p_gesture_info->pt[0].y = data->y1;
		p_gesture_info->pt[1].x = data->x2;
		p_gesture_info->pt[1].y = data->y2;
		bu21013_touch_calc(data);

		if (data->touch_continue) {
			if ((data->x2 != 0) && (data->y2 != 0))
				bu21013_update_ges_mt_touch(data);
			else
				bu21013_update_ges_st_touch(data);
			data->flick_flag = true;
		} else {
			p_gesture_info->gesture_kind = GES_TOUCHSTART;
			data->tap_start_point.x = p_gesture_info->pt[0].x;
			data->tap_start_point.y = p_gesture_info->pt[0].y;
			data->touch_count = START;
		}
		data->touch_continue = true;
		pre_gesture_point.x = p_gesture_info->pt[0].x;
		pre_gesture_point.y = p_gesture_info->pt[0].y;
	}
	return 0;
}

/**
 * bu21013_timer_start() - restart the timer
 * @data:bu21013_ts_data structure pointer
 *
 * This funtion used to run the timer and returns none.
 */
static inline void bu21013_timer_start(struct bu21013_ts_data *data)
{
	mod_timer(&data->penirq_timer, jiffies + msecs_to_jiffies(data->penup_timer));
}

/**
 * tsc_clear_irq() - clear the tsc interrupt
 * @i2c:i2c_client structure pointer
 *
 * This funtion used to clear the
 * bu21013 controller interrupt for next Pen interrupts and returns none.
 */
static void tsc_clear_irq(struct i2c_client *i2c)
{
	int retval;
	retval = i2c_smbus_write_byte_data(i2c, BU21013_INT_CLR,
					BU21013_INTR_CLEAR);
	if (retval < 0)
		dev_err(&i2c->dev, "int cleared failed \n");
}

/**
 * tsc_en_irq() - Reactivate the tsc interrupt
 * @i2c:i2c_client structure pointer
 *
 * This funtion used to activate the bu21013 controller
 * interrupt for next Pen interrupts.
 */
static void tsc_en_irq(struct i2c_client *i2c)
{
	int retval;
	retval = i2c_smbus_write_byte_data(i2c, BU21013_INT_CLR,
					BU21013_INTR_ENABLE);
	if (retval < 0)
		dev_err(&i2c->dev, "interrupt enable failed\n");
}
/**
 * tsc_input_report_pen_down() - report the touch point
 * @data:bu21013_ts_data structure pointer
 * This funtion calls, when we need to report the Pen down interrupt
 */
static void tsc_input_report_pen_down(struct bu21013_ts_data *data)
{
	short pt0x;
	short pt0y;
	short pt1x;
	short pt1y;
	if (data->chip->portrait) {
		pt0x = data->gesture_info.pt[0].x;
		pt0y = data->gesture_info.pt[0].y;
		pt1x = data->gesture_info.pt[1].x;
		pt1y = data->gesture_info.pt[1].y;
	} else {
		pt0x = data->gesture_info.pt[0].y;
		pt0y = data->gesture_info.pt[0].x;
		pt1x = data->gesture_info.pt[1].y;
		pt1y = data->gesture_info.pt[1].x;
	}

	input_report_abs(data->pin_dev, ABS_X, pt0x);
	input_report_abs(data->pin_dev, ABS_Y, pt0y);

	input_report_abs(data->pin_dev, ABS_PRESSURE, 1);
	input_report_abs(data->pin_dev, ABS_TOOL_WIDTH, 1);
	input_report_key(data->pin_dev, BTN_TOUCH, 1);

	if (data->finger2_pressed) {
		input_report_key(data->pin_dev, BTN_2, 1);
		input_report_abs(data->pin_dev, ABS_HAT0X, pt1x);
		input_report_abs(data->pin_dev, ABS_HAT0Y, pt1y);
	}
	input_report_abs(data->pin_dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_key(data->pin_dev, ABS_MT_WIDTH_MAJOR, 1);
	input_report_abs(data->pin_dev, ABS_MT_POSITION_X, pt0x);
	input_report_abs(data->pin_dev, ABS_MT_POSITION_Y, pt0y);
	input_mt_sync(data->pin_dev);
	if (data->finger2_pressed) {
		input_report_abs(data->pin_dev, ABS_MT_TOUCH_MAJOR, 1);
		input_report_key(data->pin_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_report_abs(data->pin_dev, ABS_MT_POSITION_X, pt1x);
		input_report_abs(data->pin_dev, ABS_MT_POSITION_Y, pt1y);
		input_mt_sync(data->pin_dev);
	}
	input_sync(data->pin_dev);
}
/**
 * tsc_input_report_pen_up() - report the touch point
 * @data:bu21013_ts_data structure pointer
 * This funtion calls, when we need to report the Pen up interrupt
 */
static void tsc_input_report_pen_up(struct bu21013_ts_data *data)
{
	input_report_abs(data->pin_dev, ABS_PRESSURE, 0);
	input_report_abs(data->pin_dev, ABS_TOOL_WIDTH, 0);
	input_report_key(data->pin_dev, BTN_TOUCH, 0);
	if (data->finger2_count) {
		input_report_key(data->pin_dev, BTN_2, 0);
		input_report_abs(data->pin_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_key(data->pin_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_mt_sync(data->pin_dev);
		data->finger2_count = 0;
	}
	input_sync(data->pin_dev);
}
/**
 * tsc_input_report() - report the touch point
 * @data:bu21013_ts_data structure pointer
 * @value:value to be passed for en down or pen up
 * This funtion calls, when we need to report the Pen down/up interrupt
 */
static void tsc_input_report(struct bu21013_ts_data *data, bool value)
{
	if (value)
		tsc_input_report_pen_down(data);
	else
		tsc_input_report_pen_up(data);
}

/**
 * tsc_callback() - callback handler for Pen down
 * @device_data:void pointer
 *
 * This funtion calls, when we get the Pen down interrupt from Egpio Pin
 * and assigns the task and returns none.
 */

static void tsc_callback(void *device_data)
{
	struct bu21013_ts_data *data = (struct bu21013_ts_data *)device_data;
	schedule_work(&data->tp_gpio_handler);
}

/**
 * tsc_gpio_callback() - callback handler for Pen down
 * @device_data:void pointer
 * @irq: irq value
 *
 * This funtion calls for HREF v1, when we get the Pen down interrupt from GPIO Pin
 * and assigns the task and returns irqreturn_t.
 */

static irqreturn_t tsc_gpio_callback(int irq, void *device_data)
{
	struct bu21013_ts_data *data = (struct bu21013_ts_data *)device_data;
	schedule_work(&data->tp_gpio_handler);
	return IRQ_HANDLED;
}

/**
 * tsc_timer_callback() - callback handler for Timer
 * @dev_data:touch screen data
 *
 * This callback handler used to schedule the work for Pen up
 * and Pen down interrupts and returns none
 */
static void tsc_timer_callback(unsigned long dev_data)
{
	struct bu21013_ts_data *data = (struct bu21013_ts_data *)dev_data;
	schedule_work(&data->tp_timer_handler);
}
/**
 * tsc_level_mode_report() - report the touch point in level mode
 * @data:bu21013_ts_data structure pointer
 * This funtion used to report the Pen down/up interrupt for level mode.
 */
static void tsc_level_mode_report(struct bu21013_ts_data *data)
{
	if (data->touch_en) {
		data->prev_press_report = true;
		tsc_input_report(data, true);
	} else {
		if (data->prev_press_report) {
			data->prev_press_report = false;
			tsc_input_report(data, false);
		}
	}
}
/**
 * tp_timer_wq() - Work Queue for timer handler
 * @work:work_struct structure pointer
 *
 * This work queue used to get the co-ordinates
 * of the Pen up and Pen down interrupts and returns none
 */
static void tp_timer_wq(struct work_struct *work)
{
	struct bu21013_ts_data *data = container_of(work, struct bu21013_ts_data, tp_timer_handler);
	struct task_struct *tsk = current;

	set_task_state(tsk, TASK_INTERRUPTIBLE);
	data->intr_pin = data->chip->pirq_read_val();
	bu21013_get_touch_mesg(data);

	if (data->chip->edge_mode)
		tsc_input_report(data, data->touch_en);
	else
		tsc_level_mode_report(data);

	if (data->intr_pin == PEN_DOWN_INTR) {
		bu21013_timer_start(data);
	} else {
		if (!data->chip->edge_mode) {
			tsc_clear_irq(data->client);
			tsc_en_irq(data->client);
		}
		enable_irq(data->chip->irq);
	}
}

/**
 * tp_gpio_int_wq() - Work Queue for GPIO_INT handler
 * @work:work_struct structure pointer
 *
 * This work queue used to get the co-ordinates
 * of the Pen up and Pen down interrupts and returns none
 */
static void tp_gpio_int_wq(struct work_struct *work)
{
	struct bu21013_ts_data *tp_data = container_of(work, struct bu21013_ts_data, tp_gpio_handler);
	struct task_struct *tsk		= current;

	set_task_state(tsk, TASK_INTERRUPTIBLE);
	tp_data->intr_pin = tp_data->chip->pirq_read_val();
	if (tp_data->intr_pin == PEN_DOWN_INTR) {
		disable_irq(tp_data->chip->irq);
		bu21013_get_touch_mesg(tp_data);
		if (tp_data->chip->edge_mode)
			tsc_input_report(tp_data, tp_data->touch_en);
		else
			tsc_level_mode_report(tp_data);
		if (!tp_data->chip->edge_mode) {
			tsc_clear_irq(tp_data->client);
			tsc_en_irq(tp_data->client);
		}
		bu21013_timer_start(tp_data);
	} else {
		if (tp_data->chip->edge_mode) {
			tsc_input_report(tp_data, false);
		} else if (tp_data->prev_press_report) {
			tp_data->prev_press_report = false;
			tsc_input_report(tp_data, false);
		}
	}
}

/*
 * bu21013_init_config(): Initialize the Global variables
 * @data: device structure pointer
 * This function used to initialize the device variables and returns none
 */
void bu21013_init_config(struct bu21013_ts_data *data)
{
	signed int i;

	data->touch_count = STOP;
	/* turning right */
	data->rotate_data[DIR_UP][DIR_RIGHT] = ROTATE_R_UR;
	data->rotate_data[DIR_RIGHT][DIR_DOWN] = ROTATE_R_RD;
	data->rotate_data[DIR_DOWN][DIR_LEFT] = ROTATE_R_DL;
	data->rotate_data[DIR_LEFT][DIR_UP] = ROTATE_R_LU;

	/* turning left */
	data->rotate_data[DIR_LEFT][DIR_DOWN] = ROTATE_L_LD;
	data->rotate_data[DIR_RIGHT][DIR_UP] = ROTATE_L_DR;
	data->rotate_data[DIR_DOWN][DIR_RIGHT] = ROTATE_L_RU;
	data->rotate_data[DIR_UP][DIR_LEFT] = ROTATE_L_UL;

	data->dir_idx = DIRHEADER;

	for (i = DIRHEADER; i < DIRTRACEN; i++)
		data->dir_trace[i] = DIR_TRACE_VALUE;

	return;
}
/**
 * bu21013_init_chip() - Power on sequence for the bu21013 controller
 * @data: device structure pointer
 *
 * This funtion is used to power on
 * the bu21013 controller and returns integer.
 **/
static int bu21013_init_chip(struct bu21013_ts_data *data)
{
	int retval;
	struct i2c_client *i2c = data->client;

	dev_dbg(&i2c->dev, "bu21013_init_chip start\n");

	retval = i2c_smbus_write_byte_data(i2c, BU21013_RESET,
					BU21013_RESET_ENABLE);
	if (retval < 0) {
		dev_err(&i2c->dev, "ED reg write failed\n");
		goto err;
	}
	mdelay(RESET_DELAY);

	retval = i2c_smbus_write_byte_data(i2c, BU21013_SENSOR_0_7,
					BU21013_SENSORS_EN_0_7);
	if (retval < 0) {
		dev_err(&i2c->dev, "F0 reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_SENSOR_8_15,
					BU21013_SENSORS_EN_8_15);
	if (retval < 0) {
		dev_err(&i2c->dev, "F1 reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_SENSOR_16_23,
					BU21013_SENSORS_EN_16_23);
	if (retval < 0) {
		dev_err(&i2c->dev, "F2 reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_POS_MODE1,
				(BU21013_POS_MODE1_0 | BU21013_POS_MODE1_1));
	if (retval < 0) {
		dev_err(&i2c->dev, "F3 reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_POS_MODE2,
			(BU21013_POS_MODE2_ZERO | BU21013_POS_MODE2_AVG1 | BU21013_POS_MODE2_AVG2 |
				BU21013_POS_MODE2_EN_RAW | BU21013_POS_MODE2_MULTI));
	if (retval < 0) {
		dev_err(&i2c->dev, "F4 reg write failed\n");
		goto err;
	}
	if (data->chip->ext_clk) {
		retval = i2c_smbus_write_byte_data(i2c, BU21013_CLK_MODE,
					(BU21013_CLK_MODE_EXT | BU21013_CLK_MODE_CALIB));
		if (retval < 0) {
			dev_err(&i2c->dev, "F5 reg write failed\n");
			goto err;
		}
	} else {
		retval = i2c_smbus_write_byte_data(i2c, BU21013_CLK_MODE,
					(BU21013_CLK_MODE_DIV | BU21013_CLK_MODE_CALIB));
		if (retval < 0) {
			dev_err(&i2c->dev, "F5 reg write failed\n");
			goto err;
		}
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_IDLE,
					(BU21013_IDLET_0 | BU21013_IDLE_INTERMIT_EN));
	if (retval < 0) {
		dev_err(&i2c->dev, "FA reg write failed\n");
		goto err;
	}
	if (data->chip->edge_mode) {
		retval = i2c_smbus_write_byte_data(i2c, BU21013_INT_MODE,
						BU21013_INT_MODE_EDGE);
		if (retval < 0) {
			dev_err(&i2c->dev, "E9 reg write failed\n");
			goto err;
		}
	} else {
		retval = i2c_smbus_write_byte_data(i2c, BU21013_INT_MODE,
					BU21013_INT_MODE_LEVEL);
		if (retval < 0) {
			dev_err(&i2c->dev, "E9 reg write failed\n");
			goto err;
		}
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_FILTER,
					(BU21013_DELTA_0_6 | BU21013_FILTER_EN));
	if (retval < 0) {
		dev_err(&i2c->dev, "FB reg write failed\n");
		goto err;
	}

	retval = i2c_smbus_write_byte_data(i2c, BU21013_TH_ON, data->th_on);
	if (retval < 0) {
		dev_err(&i2c->dev, "FC reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_TH_OFF, data->th_off);
	if (retval < 0) {
		dev_err(&i2c->dev, "FD reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_GAIN, data->gain);
	if (retval < 0) {
		dev_err(&i2c->dev, "EA reg write failed\n");
		goto err;
	}

	retval = i2c_smbus_write_byte_data(i2c, BU21013_OFFSET_MODE,
					BU21013_OFFSET_MODE_DEFAULT);
	if (retval < 0) {
		dev_err(&i2c->dev, "EB reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_XY_EDGE,
			(BU21013_X_EDGE_0 | BU21013_X_EDGE_2 |
				BU21013_Y_EDGE_1 | BU21013_Y_EDGE_3));
	if (retval < 0) {
		dev_err(&i2c->dev, "EC reg write failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, BU21013_REG_DONE, BU21013_DONE);
	if (retval < 0) {
		dev_err(&i2c->dev, "EF reg write failed\n");
		goto err;
	}
err:
	return retval;
}
/*
 * tsc_driver_register() - Used for input driver registeration
 * @pdata: pointer to bu21013_tsc_data structure
 * This function used to register the
 * touch screen as input device and returns integer
 */
static int tsc_driver_register(struct bu21013_ts_data *pdata)
{
	int ret;
	short x_max;
	short y_max;

	struct input_dev *in_dev;
	struct i2c_client *i2c = pdata->client;
	dev_dbg(&i2c->dev, "input register start\n");

	/* register the touch screen driver to input device */
	if (pdata->chip->portrait) {
		x_max = pdata->chip->x_max_res;
		y_max = pdata->chip->y_max_res;
	} else {
		x_max = pdata->chip->y_max_res;
		y_max = pdata->chip->x_max_res;
	}
	in_dev = input_allocate_device();
	pdata->pin_dev = in_dev;
	if (!in_dev) {
		ret = -ENOMEM;
		goto err;
	}

	in_dev->name = DRIVER_TP;
	set_bit(EV_SYN, in_dev->evbit);
	set_bit(EV_KEY, in_dev->evbit);
	set_bit(EV_ABS, in_dev->evbit);
	set_bit(BTN_TOUCH, in_dev->keybit);
	input_set_abs_params(in_dev, ABS_X, 0, x_max, 0, 0);
	input_set_abs_params(in_dev, ABS_Y, 0, y_max, 0, 0);
	input_set_abs_params(in_dev, ABS_PRESSURE, 0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(in_dev, ABS_TOOL_WIDTH, 0, MAX_TOOL_WIDTH, 0, 0);
	set_bit(BTN_2, in_dev->keybit);
	input_set_abs_params(in_dev, ABS_HAT0X, 0, x_max, 0, 0);
	input_set_abs_params(in_dev, ABS_HAT0Y, 0, y_max, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_POSITION_X, 0, x_max, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_POSITION_Y, 0, y_max, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_TOUCH_MAJOR, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_WIDTH_MAJOR, 0, MAX_TOOL_WIDTH, 0, 0);
	ret = input_register_device(in_dev);
	if (ret) {
		dev_err(&i2c->dev, " input register error \n");
		goto err;
	}
	dev_dbg(&i2c->dev, "input register done \n");
err:
	return ret;
}
/**
 * tsc_config() - configure the touch screen controller
 * @pdev_data: pointer to bu21013_ts_data structure
 *
 * This funtion is used to configure
 * the bu21013 controller and returns tsc error.
 **/
static int tsc_config(struct bu21013_ts_data *pdev_data)
{
	int retval;

	retval = bu21013_init_chip(pdev_data);
	if (retval < 0)
		goto err;

	bu21013_init_config(pdev_data);
	if ((!pdev_data->board_flag) &&
		((pdev_data->chip->pirq_en) && (pdev_data->chip->pirq_dis) &&
			(pdev_data->chip->irq_init))) {
		retval = pdev_data->chip->pirq_dis();
		if (retval < 0) {
			dev_err(&pdev_data->client->dev, "irq dis failed \n");
			goto err;
		}
		retval = pdev_data->chip->pirq_en();
		if (retval < 0) {
			dev_err(&pdev_data->client->dev, "irq en failed \n");
			goto err_init_irq;
		}
		if (pdev_data->chip->irq_init(tsc_callback, pdev_data)) {
			dev_err(&pdev_data->client->dev, "irq init failed \n");
			goto err_init_irq;
		}
	}
	retval = bu21013_do_calibrate(pdev_data->client);
	if (retval < 0) {
		dev_err(&pdev_data->client->dev, "calibration not done \n");
		goto err_init_irq;
	}
	return retval;
err_init_irq:
	pdev_data->chip->pirq_dis();
	pdev_data->chip->irq_exit();
err:
	pdev_data->chip->cs_dis(pdev_data->chip->cs_pin);
	return retval;
}
#ifdef CONFIG_PM
/**
 * bu21013_tp_suspend() - suspend the touch screen controller
 * @client: pointer to i2c client structure
 * @mesg: message from power manager
 *
 * This funtion is used to suspend the
 * touch panel controller and returns integer
 **/
static int bu21013_tp_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int retval;
	struct bu21013_ts_data *tsc_data = i2c_get_clientdata(client);

	if (!tsc_data->board_flag) {
		retval = tsc_data->chip->pirq_dis();
		if (retval < 0) {
			dev_err(&client->dev, "irq disable failed \n");
			goto err;
		}
		retval = tsc_data->chip->irq_exit();
		if (retval < 0) {
			dev_err(&client->dev, "irq exit failed \n");
			goto err;
		}
	} else
		disable_irq(tsc_data->chip->irq);
	return 0;
err:
	kfree(tsc_data);
	return retval;
}

/**
 * bu21013_tp_resume() - resume the touch screen controller
 * @client: pointer to i2c client structure
 *
 * This funtion is used to resume the touch panel
 * controller and returns integer.
 **/
static int bu21013_tp_resume(struct i2c_client *client)
{
	int retval;
	struct bu21013_ts_data *tsc_data = i2c_get_clientdata(client);

	retval = tsc_config(tsc_data);
	if (retval < 0) {
		dev_err(&client->dev, "tsc config failed \n");
		goto err;
	}
	if (tsc_data->board_flag)
		enable_irq(tsc_data->chip->irq);
	return 0;
err:
	kfree(tsc_data);
	return retval;
}
#endif
/**
 * bu21013_tp_sysfs() - sys fs parameters
 * @tp_kobj: kernel object structure pointer
 *
 * This funtion uses to initialize the sysfs for touch panel
 **/
int bu21013_tp_sysfs(struct kobject *tp_kobj)
{
	int retval;
	tp_kobj = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (tp_kobj == NULL) {
		retval = -ENOMEM;
		goto err;
	}
	tp_kobj->ktype = &ktype_touchp;
	kobject_init(tp_kobj, tp_kobj->ktype);
	retval = kobject_set_name(tp_kobj, "touchp1");
	if (retval) {
		kfree(tp_kobj);
		goto err;
	}
	retval = kobject_add(tp_kobj, NULL, "touchp1");
	if (retval) {
		kfree(tp_kobj);
		goto err;
	}
	return 0;
err:
	return retval;
}
/**
 * bu21013_tp_probe() - Initialze the i2c-client touchscreen driver
 * @i2c: i2c client structure pointer
 * @id:i2c device id pointer
 *
 * This funtion uses to Initializes the i2c-client touchscreen
 * driver and returns integer.
 **/
static int bu21013_tp_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	int retval;
	struct bu21013_ts_data *tsc_data;
	struct bu21013_platform_device *pdata = i2c->dev.platform_data;
	dev_dbg(&i2c->dev, "u8500_tsc_probe:: start\n");

	tsc_data = kzalloc(sizeof(struct bu21013_ts_data), GFP_KERNEL);
	if (!tsc_data) {
		dev_err(&i2c->dev, "tp memory alloc failed \n");
		retval = -ENOMEM;
		return retval;
	}
	if (!pdata) {
		dev_err(&i2c->dev, "tp platform data not defined \n");
		retval = -EINVAL;
		goto err_alloc;
	}
	tsc_data->chip = pdata;
	tsc_data->client = i2c;
	/*
	 * Hack for dynamic detection of HREF v1 board
	 */
	tsc_data->board_flag = tsc_data->chip->board_check();
	/*
	 * Hack for not supporting touch panel controller2
	 * for the ED and MOP500 v1 boards, due to no support
	 * of shared irq callback registeration in tc35892
	 * and stmpe2401 driver.
	 */
	if ((!tsc_data->chip->tp_cntl) ||
		((tsc_data->chip->tp_cntl == 2) &&
			(!tsc_data->board_flag))) {
		dev_err(&i2c->dev, "tp is not supported \n");
		retval = -EINVAL;
		goto err_alloc;
	}
	/*
	 * Hardcoded for portrait mode
	 */
	tsc_data->chip->portrait = true;
	INIT_WORK(&tsc_data->tp_timer_handler, tp_timer_wq);
	INIT_WORK(&tsc_data->tp_gpio_handler, tp_gpio_int_wq);

	init_timer(&tsc_data->penirq_timer);
	tsc_data->penirq_timer.data = (unsigned long)tsc_data;
	tsc_data->penirq_timer.function = tsc_timer_callback;
	i2c_set_clientdata(i2c, tsc_data);
	/* configure the gpio pins */
	if (tsc_data->chip->cs_en) {
		retval = tsc_data->chip->cs_en(tsc_data->chip->cs_pin);
		if (retval != 0) {
			dev_err(&i2c->dev, "error in chip initialization\n");
			goto err;
		}
	}
	/* sys_fs value configuration */
	tsc_data->th_on 	= BU21013_TH_ON_5;
	tsc_data->th_off 	= (BU21013_TH_OFF_3 | BU21013_TH_OFF_4);
	tsc_data->gain 		= (BU21013_GAIN_0 | BU21013_GAIN_1);
	tsc_data->penup_timer 	= PENUP_TIMEOUT;

	/* configure the touch panel controller */
	retval = tsc_config(tsc_data);
	if (retval < 0) {
		dev_err(&i2c->dev, "error in tp config\n");
		goto err;
	}
	if (tsc_data->board_flag) {
		retval = request_irq(tsc_data->chip->irq, tsc_gpio_callback,
					(IRQF_TRIGGER_FALLING | IRQF_SHARED), DRIVER_TP, tsc_data);
		if (retval) {
			dev_err(&i2c->dev, "request irq %d failed\n", tsc_data->chip->irq);
			free_irq(tsc_data->chip->irq, tsc_data);
			goto err;
		}
	}
	retval = tsc_driver_register(tsc_data);
	if (retval)
		goto err_init;
	if (tsc_data->chip->tp_cntl != 2) {
		retval = bu21013_tp_sysfs(tsc_data->touchp_kobj);
		if (retval)
			goto err_init;
		else
			tp_sys_data = tsc_data;
	}
	dev_dbg(&i2c->dev, "u8500_tsc_probe : done \n");
	return retval;
err_init:
	input_free_device(tsc_data->pin_dev);
err:
	del_timer_sync(&tsc_data->penirq_timer);
err_alloc:
	kfree(tsc_data);
	return retval;
}
/**
 * bu21013_tp_remove() - Removes the i2c-client touchscreen driver
 * @client: i2c client structure pointer
 *
 * This funtion uses to remove the i2c-client
 * touchscreen driver and returns integer.
 **/
static int __exit bu21013_tp_remove(struct i2c_client *client)
{
	struct bu21013_ts_data *data = i2c_get_clientdata(client);
	del_timer_sync(&data->penirq_timer);

	if (data->chip != NULL) {
		if (!data->board_flag) {
			data->chip->irq_exit();
			data->chip->pirq_dis();
		} else
			free_irq(data->chip->irq, data);
		data->chip->cs_dis(data->chip->cs_pin);
	}
	if (data->touchp_kobj != NULL)
		kfree(data->touchp_kobj);
	input_unregister_device(data->pin_dev);
	input_free_device(data->pin_dev);
	kfree(data);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id bu21013_tp_id[] = {
	{ DRIVER_TP, 0 },
	{ DRIVER_TP, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bu21013_tp_id);

static struct i2c_driver bu21013_tp_driver = {
	.driver = {
		.name = DRIVER_TP,
		.owner = THIS_MODULE,
	},
	.probe 		= bu21013_tp_probe,
#ifdef CONFIG_PM
	.suspend	= bu21013_tp_suspend,
	.resume		= bu21013_tp_resume,
#endif
	.remove 	= __exit_p(bu21013_tp_remove),
	.id_table	= bu21013_tp_id,
};

/**
 * bu21013_tp_init() - Initialize the paltform touchscreen driver
 *
 * This funtion uses to initialize the platform
 * touchscreen driver and returns integer.
 **/
static int __init bu21013_tp_init(void){
	return i2c_add_driver(&bu21013_tp_driver);
}

/**
 * bu21013_tp_exit() - De-initialize the paltform touchscreen driver
 *
 * This funtion uses to de-initialize the platform
 * touchscreen driver and returns none.
 **/
static void __exit bu21013_tp_exit(void){
	i2c_del_driver(&bu21013_tp_driver);
}

module_init(bu21013_tp_init);
module_exit(bu21013_tp_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("NAVEEN KUMAR G");
MODULE_DESCRIPTION("Touch Screen driver for Bu21013 controller");
