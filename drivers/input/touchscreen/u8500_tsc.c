/*
 * Overview:
 *	Touch panel driver for u8500 platform
 *
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/types.h>
#include <mach/debug.h>
#include <mach/u8500_tsc.h>
#include <linux/timer.h>

static void tsc_timer_wq(struct work_struct *_chip);

static struct i2c_driver tp_driver;
static tsc_error tsc_write_byte(struct i2c_client *i2c, u8 reg,
				unsigned char data);
static tsc_error bu21013_tsc_init(struct i2c_client *i2c);
static tsc_error tsc_config(struct u8500_tsc_data *pdev_data);
static void tsc_clear_irq(struct i2c_client *i2c);
static void tsc_en_irq(struct i2c_client *i2c);
static void tsc_callback(void *tsc);
static irqreturn_t tsc_gpio_callback(int irq,  void *device_data);
static int tsc_driver_register(struct u8500_tsc_data *pdata);
static void tsc_timer_callback(unsigned long data);
#ifdef CONFIG_U8500_TSC_EXT_CLK_9_6
static tsc_error tsc_read_byte(struct i2c_client *i2c, u8 reg, unsigned char *val);
#endif

#if (defined CONFIG_CPU_IDLE || defined CONFIG_TOUCHP_TUNING)
u8 th_on = 0x70;
u8 th_off = 0x60;
#endif
#ifdef CONFIG_TOUCHP_TUNING
unsigned long penup_timer = PENUP_TIMEOUT;
unsigned long g_tool_width = 1;
static struct kobject *touchp_kobj;
struct u8500_tsc_data *pdev_data;
static ssize_t touchp_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	int error = 0;
	u8 timeout = 0;
	u8 sys_th_on = 0;
	u8 sys_th_off = 0;

	if (strcmp(attr->name, "timeout_value") == 0) {
		timeout = penup_timer;
		sprintf(buf, "%d\n", timeout);
	} else if (strcmp(attr->name, "th_on") == 0) {
		sys_th_on = th_on;
		sprintf(buf, "%d\n", sys_th_on);
	} else if (strcmp(attr->name, "th_off") == 0) {
		sys_th_off = th_off;
		sprintf(buf, "%d\n", sys_th_off);
	}
	return error == 0 ? strlen(buf) : 0;
}
static ssize_t touchp_attr_store(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t len)
{
	int error = 0;
	int timeout = 0;
	int sys_th_on = 0;
	int sys_th_off = 0;

	if (strcmp(attr->name, "timeout_value") == 0) {
		error = sscanf(buf, "%d", &timeout);
		if (error != 1)
			goto err;
		penup_timer = timeout;
	} else if (strcmp(attr->name, "th_on") == 0) {
		error = sscanf(buf, "%d", &sys_th_on);
		if (error != 1)
			goto err;
		if (th_on != sys_th_on) {
			th_on = sys_th_on;
			if (th_on < 0xFF)
				bu21013_tsc_init(pdev_data->client);
		}
	} else if (strcmp(attr->name, "th_off") == 0) {
		error = sscanf(buf, "%d", &sys_th_off);
		if (error != 1)
			goto err;
		if (th_off != sys_th_off) {
			th_off = sys_th_off;
			if (th_off < 0xFF)
				bu21013_tsc_init(pdev_data->client);
		}
	}
err:
	return error == 1 ? len : -EINVAL;
}

static struct attribute touchp_timeout = {.name = "timeout_value",
						.mode = 0666};
static struct attribute touchp_th_on = {.name = "th_on", .mode = 0666};
static struct attribute touchp_th_off = {.name = "th_off", .mode = 0666};

static struct attribute *touchp_attribute[] = {
	&touchp_timeout,
	&touchp_th_on,
	&touchp_th_off,
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
#endif
/**
 * tsc_write_byte() - Write a single byte to touch screen.
 * @i2c:i2c client structure
 * @reg:register offset
 * @value:data byte to be written
 *
 * This funtion uses smbus byte write API
 * to write a single byte to touch screen and returns tsc error.
 */
static tsc_error tsc_write_byte(struct i2c_client *i2c, u8 reg,
					unsigned char value)
{
	int retval = TSC_OK;
	retval = i2c_smbus_write_byte_data(i2c, reg, value);
	if (retval < 0)
		dev_err(&i2c->dev,
			"tsc_write_byte:: i2c smbus write byte failed\n");
	return retval;
}
#ifdef CONFIG_U8500_TSC_EXT_CLK_9_6
/**
 * tsc_read_byte() - read single byte from touch screen
 * @i2c:i2c client structure
 * @reg:register offset
 * @val:value to get from i2c register
 *
 * This funtion uses smbus read block API
 * to read single byte from the reg offset and returns tsc error.
 */
static tsc_error tsc_read_byte(struct i2c_client *i2c, u8 reg,
				unsigned char *val)
{
	int retval = 0;
	retval = i2c_smbus_read_byte_data(i2c, reg);
	if (retval < 0)
		return retval;
	*val = (unsigned char)retval;
	return TSC_OK;
}
#endif
/**
 * tsc_read()-read data from touch panel
 * @i2c:i2c client structure pointer
 * @reg:register offset
 * @buf:read the data in this buffer
 * @nbytes:number of bytes to read
 *
 * This funtion uses smbus read block API
 * to read multiple bytes from the reg offset.
 **/
static int tsc_read_block(struct i2c_client *i2c, u8 reg, u8 *buf, u8 nbytes)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(i2c, reg, nbytes, buf);
	if (ret < nbytes)
		return ret;
	return TSC_OK;
}
/**
 * tsc_read_10bit() - read 10 bit data from touch screen
 * @i2c:i2c client structure
 * @reg:register offset
 * @val:value to get from i2c register
 *
 * This funtion uses smbus read block API
 * to read 10-bit from the reg offset and returns tsc error.
 */
static tsc_error tsc_read_10bit(struct i2c_client *i2c,
				 u8 reg, unsigned int *val)
{
	u16 data = 0;
	int retval = 0;
	u8 buf[2];
	retval = tsc_read_block(i2c, reg, buf, 2);
	if (retval < 0)
		return retval;
	data = buf[0];
	data = (data << 2) | buf[1];
	*val = data;
	return TSC_OK;
}

/**
 * touch_calculation(): Get the exact co-ordinates of the touch interrupt
 * @p_gesture_info:  gesture_info structure pointer
 * Transfer the position information from touch sensor
 * coordinate to display coordinate and returns none.
 */
void touch_calculation(struct gesture_info *p_gesture_info)
{
	signed short x1, y1;
	int tmpx, tmpy;
#ifdef CONFIG_U8500_TSC_MULTITOUCH
	signed short x2, y2;
#endif

	tmpx = (X_MAX * 1000 / TOUCH_XMAX);
	tmpy = (Y_MAX * 1000 / TOUCH_YMAX);

	x1 = p_gesture_info->pt[0].x;
	y1 = p_gesture_info->pt[0].y;

	x1 = x1 * tmpx / 1000;
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
	x1 = X_MAX - x1;
#endif
	p_gesture_info->pt[0].x = x1;

	y1 = y1 * tmpy / 1000;
	p_gesture_info->pt[0].y = y1;
#ifdef CONFIG_U8500_TSC_MULTITOUCH
	x2 = p_gesture_info->pt[1].x;
	x2 = x2 * tmpx / 1000;
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
	x2 = X_MAX - x2;
#endif
	p_gesture_info->pt[1].x = x2;

	y2 = p_gesture_info->pt[1].y;
	y2 = y2 * tmpy / 1000;
	p_gesture_info->pt[1].y = y2;
#endif
}

/**
 * get_touch(): Get the co-ordinates of the touch interrupt
 * @data:u8500_tsc_data structure pointer
 * Get touched point position
 * from touch sensor registers and write to global variables.
 */
void get_touch(struct u8500_tsc_data *data)
{
	struct i2c_client *i2c = data->client;
	tsc_error retval = TSC_OK;
	u8 finger1_buf[4];
	u16 tmp1x = 0, tmp1y = 0;
	u8 j, i;
#ifdef CONFIG_U8500_TSC_MULTITOUCH
	u8 finger2_buf[4];
	u16 tmp2x, tmp2y;
#endif
	if (i2c == NULL)
		return;
#ifdef CONFIG_U8500_TSC_EXT_CLK_9_6
	i = 0x73;
	for (j = 0; j < 2; j++) {
		retval = tsc_read_byte(i2c, i, &finger1_buf[j]);
		if (retval < 0)
			return;
		i++;
	}
	tmp1x = finger1_buf[0];
	tmp1x = (tmp1x << 2) | finger1_buf[1];
	if (tmp1x != 0) {
		i = 0x75;
		for (j = 2; j < 4; j++) {
			retval = tsc_read_byte(i2c, i, &finger1_buf[j]);
			if (retval < 0)
				return;
			i++;
		}
		tmp1y = finger1_buf[2];
		tmp1y = (tmp1y << 2) | finger1_buf[3];
	} else
		tmp1y = 0;
#else
	retval = tsc_read_block(i2c, 0x73, finger1_buf, 4);
	if (retval != 0)
		return;

	tmp1x = finger1_buf[0];
	tmp1x = (tmp1x << 2) | finger1_buf[1];
	tmp1y = finger1_buf[2];
	tmp1y = (tmp1y << 2) | finger1_buf[3];
#endif
	if (tmp1x != 0 && tmp1y != 0)
		data->finger1_pressed = TRUE;
	else
		data->finger1_pressed = FALSE;

	if (tmp1x > TOUCH_XMAX || tmp1y > TOUCH_YMAX)
		data->finger1_pressed = FALSE;

	if (data->finger1_pressed) {
		data->x1 = tmp1x;
		data->y1 = tmp1y;
	} else {
		data->x1 = 0;
		data->y1 = 0;
		data->x2 = 0;
		data->y2 = 0;
	}

#ifndef CONFIG_U8500_TSC_MULTITOUCH
	if (data->finger1_pressed == TRUE) {
		data->touch_en = TRUE;
		data->touchflag = TRUE;
	} else {
		data->touch_en = FALSE;
		data->touchflag = FALSE;
	}
#else
#ifdef CONFIG_U8500_TSC_EXT_CLK_9_6
	i = 0x77;
	for (j = 0; j < 2; j++) {
		retval = tsc_read_byte(i2c, i, &finger2_buf[j]);
		if (retval < 0)
			return;
		i++;
	}
	tmp2x = finger2_buf[0];
	tmp2x = (tmp2x << 2) | finger2_buf[1];
	if (tmp2x != 0) {
		i = 0x79;
		for (j = 2; j < 4; j++) {
			retval = tsc_read_byte(i2c, i, &finger2_buf[j]);
			if (retval < 0)
				return;
			i++;
		}
		tmp2y = finger2_buf[2];
		tmp2y = (tmp2y << 2) | finger2_buf[3];
	} else
		tmp2y = 0;
#else
	retval = tsc_read_block(i2c, 0x77, finger2_buf, 4);
	if (retval != 0)
		return;
	tmp2x = finger2_buf[0];
	tmp2x = (tmp2x << 2) | finger2_buf[1];
	tmp2y = finger2_buf[2];
	tmp2y = (tmp2y << 2) | finger2_buf[3];
#endif

	if (tmp2x != 0 && tmp2y != 0)
		data->finger2_pressed = TRUE;
	else
		data->finger2_pressed = FALSE;

	if (tmp2x > TOUCH_XMAX || tmp2y > TOUCH_YMAX)
		data->finger2_pressed = FALSE;
	data->finger2_count = 1;
	if (data->finger2_pressed) {
		data->x2 = tmp2x;
		data->y2 = tmp2y;
	} else {
		data->x2 = 0;
		data->y2 = 0;
	}

	if ((data->finger1_pressed == TRUE)
		|| (data->finger2_pressed == TRUE)) {
		data->touch_en = TRUE;
		data->touchflag = TRUE;
	} else {
		data->touch_en = FALSE;
		data->touchflag = FALSE;
	}

	if ((data->finger1_pressed == FALSE)
		&& (data->finger2_pressed == TRUE)) {
		data->x1 = tmp2x;
		data->y1 = tmp2y;
		data->x2 = 0;
		data->y2 = 0;
		data->finger2_pressed = FALSE;
	}
#endif
}

/**
 * get_touch_message(): Generate the gesture message
 * @data: a pointer to the device structure
 * Generate the gesture message according to the
 * information collected and returns integer.
 */

int get_touch_message(struct u8500_tsc_data *data)
{
	struct gesture_info *p_gesture_info = &data->gesture_info;
	struct i2c_client *i2c = data->client;
	unsigned int ret = TRUE;

	unsigned char flick_series_flag = TRUE;
	signed short x_delta, y_delta;
	unsigned char dir_left_right, dir_up_down;
	unsigned char tmp;
	signed short i;
	unsigned char dir1, dir2;

	static struct touch_point pre_gesture_point;

	unsigned long	area;
	static unsigned long pre_area;

	p_gesture_info->gesture_kind = GES_UNKNOWN;

	if (i2c == NULL)
		return -1;

	get_touch(data);

	if (data->touchflag == FALSE) {
		if (data->touch_continue) {
			data->touch_continue = FALSE;

			/* touch end message  */
			p_gesture_info->gesture_kind = GES_TOUCHEND;

			/* tap flag less than 2 */
			if (data->pre_tap_flag_level <= 2)
				data->pre_tap_flag_level++;

			data->pre_tap_flag = CLR;
		} else {
			if (!data->pinch_start) {
				if (data->flick_flag) {
					x_delta = p_gesture_info->pt[0].x - data->tap_start_point.x;
					y_delta = p_gesture_info->pt[0].y - data->tap_start_point.y;

					if (x_delta < 0) {
						dir_left_right = DIR_LEFT;
						x_delta = -x_delta;
					} else
						dir_left_right = DIR_RIGHT;

					if (y_delta < 0) {
						y_delta = -y_delta;
						dir_up_down = DIR_UP;
					} else
						dir_up_down = DIR_DOWN;

					if ((THRESHOLD_FLICK <= x_delta) || (THRESHOLD_FLICK <= y_delta)) {
						if (THRESHOLD_DRAGDROP <= data->touch_count) {
							p_gesture_info->gesture_kind = GES_DRAGDROP;
							p_gesture_info->speed = data->touch_count;
							p_gesture_info->pt[0].x = data->tap_start_point.x;
							p_gesture_info->pt[0].y = data->tap_start_point.y;
							p_gesture_info->pt[1].x = pre_gesture_point.x;
							p_gesture_info->pt[1].y = pre_gesture_point.y;
						} else {
							if ((THRESHOLD_FLICK_SPEED < x_delta) || (THRESHOLD_FLICK_SPEED < y_delta))
								p_gesture_info->speed = HIGHSPEED;
							else
								p_gesture_info->speed = LOWSPEED;
							p_gesture_info->gesture_kind = GES_FLICK;
							if (x_delta > y_delta)
								p_gesture_info->dir = dir_left_right;
							else
								p_gesture_info->dir = dir_up_down;
						}
						data->pre_tap_flag_level = 0;
						data->flick_flag = CLR;
						data->touch_count = STOP;
					} else {
						flick_series_flag = FALSE;
					}
				} else {
					flick_series_flag = FALSE;
				}
				if (flick_series_flag == FALSE) {
					if (THRESHOLD_TAPLIMIT <= data->touch_count) {
						p_gesture_info->gesture_kind = GES_TAP;
						p_gesture_info->times = data->pre_tap_flag_level;
						p_gesture_info->speed = data->touch_count;
						data->pre_tap_flag_level = 0;
						data->flick_flag = CLR;
						data->touch_count = STOP;
						data->dir_idx = DIRHEADER;

						for (i = 0; i < DIRHEADER; i++)
							data->dir_trace[i] = 0x0;
						for (i = DIRHEADER; i < DIRTRACEN; i++)
							data->dir_trace[i] = 0x05;
					} else {
						ret = FALSE;
					}
				}
			} else {
				data->pinch_start = FALSE;
				data->pre_tap_flag_level = 0;
				data->flick_flag = CLR;
				data->touch_count = STOP;
			}
		}
	} else {
		p_gesture_info->pt[0].x = data->x1;
		p_gesture_info->pt[0].y = data->y1;
		p_gesture_info->pt[1].x = data->x2;
		p_gesture_info->pt[1].y = data->y2;
		touch_calculation(p_gesture_info);

		if (data->touch_continue) {
			if ((data->x2 != 0) && (data->y2 != 0)) {
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
				if (area != pre_area) {
					if (area > pre_area) {
						if ((area - pre_area) > THRESHOLD_PINCH) {
							p_gesture_info->dir = PINCH_OUT;
							if ((area - pre_area) < THRESHOLD_PINCH_SPEED)
								p_gesture_info->speed = LOWSPEED;
							else
								p_gesture_info->speed = HIGHSPEED;
						}
					} else {
						if ((pre_area - area) > THRESHOLD_PINCH) {
							p_gesture_info->dir = PINCH_IN;

							if ((pre_area - area) < THRESHOLD_PINCH_SPEED)
								p_gesture_info->speed = LOWSPEED;
							else
								p_gesture_info->speed = HIGHSPEED;
						}
					}
				}
				if (data->pinch_start == FALSE) {
					p_gesture_info->dir = PINCH_KEEP;
					pre_area = 0;
					data->pinch_start = TRUE;
				}
				p_gesture_info->gesture_kind = GES_PINCH;
				pre_area = area;
			} else {
				x_delta = p_gesture_info->pt[0].x - pre_gesture_point.x;
				y_delta = p_gesture_info->pt[0].y - pre_gesture_point.y;
				if (x_delta > 0) {
					if (x_delta < THRESHOLD_ROTATE)
						x_delta = 0;
				} else {
					if (-x_delta < THRESHOLD_ROTATE)
						x_delta = 0;
				}
				if (y_delta > 0) {
					if (y_delta < THRESHOLD_ROTATE)
						y_delta = 0;
				} else {
					if (-y_delta < THRESHOLD_ROTATE)
						y_delta = 0;
				}
				if (x_delta == y_delta)
					p_gesture_info->dir = DIR_INVALID;
				else {
					if (x_delta < 0) {
						dir_left_right = DIR_LEFT;
						x_delta = -x_delta;
					} else
						dir_left_right = DIR_RIGHT;
					if (y_delta < 0) {
						y_delta = -y_delta;
						dir_up_down = DIR_UP;
					} else
						dir_up_down = DIR_DOWN;
					if (x_delta > y_delta)
						p_gesture_info->dir = dir_left_right;
					else
						p_gesture_info->dir = dir_up_down;
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
					p_gesture_info->gesture_kind
						= GES_MOVE;
					p_gesture_info->speed
						= data->touch_count;
				}
			}
			data->flick_flag = SET;
		} else {
			p_gesture_info->gesture_kind = GES_TOUCHSTART;
			data->tap_start_point.x = p_gesture_info->pt[0].x;
			data->tap_start_point.y = p_gesture_info->pt[0].y;
			data->touch_count = START;
		}
		data->touch_continue = SET;
		pre_gesture_point.x = p_gesture_info->pt[0].x;
		pre_gesture_point.y = p_gesture_info->pt[0].y;
	}
	return ret;
}

/**
 * tsc_restart_pen_up_timer() - restart the timer
 * @data:u8500_tsc_data structure pointer
 *
 * This funtion used to run the timer and returns none.
 */
static inline void tsc_restart_pen_up_timer(struct u8500_tsc_data *data)
{
#ifdef CONFIG_TOUCHP_TUNING
	mod_timer(&data->penirq_timer, jiffies + msecs_to_jiffies(penup_timer));
#else
	mod_timer(&data->penirq_timer, jiffies + msecs_to_jiffies(PENUP_TIMEOUT));
#endif
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
	retval = tsc_write_byte(i2c, TSC_INT_CLR, 0X01);
	if (retval < 0)
		dev_err(&i2c->dev, \
			"TSC_INT_CLR failed retval=0x%x \n", retval);
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
	retval = tsc_write_byte(i2c, TSC_INT_CLR, 0X00);
	if (retval < 0)
		dev_err(&i2c->dev, \
			"TSC_INT_CLR failed retval=0x%x \n", retval);
}
/**
 * tsc_input_report() - report the touch point
 * @data:u8500_tsc_data structure pointer
 * @value:value to be passed for en down or pen up
 * This funtion calls, when we need to report the Pen down/up interrupt
 */
static void tsc_input_report(struct u8500_tsc_data *data, unsigned int value)
{
	if (value) {
	#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
		input_report_abs(data->pin_dev, ABS_X,
				data->gesture_info.pt[0].x);
		input_report_abs(data->pin_dev, ABS_Y,
				data->gesture_info.pt[0].y);
	#else
		input_report_abs(data->pin_dev, ABS_X,
				data->gesture_info.pt[0].y);
		input_report_abs(data->pin_dev, ABS_Y,
				data->gesture_info.pt[0].x);
	#endif
		input_report_abs(data->pin_dev, ABS_PRESSURE, 1);
		input_report_abs(data->pin_dev, ABS_TOOL_WIDTH, 1);
		input_report_key(data->pin_dev, BTN_TOUCH, 1);
#ifdef CONFIG_U8500_TSC_MULTITOUCH
		if (data->finger2_pressed) {
			input_report_key(data->pin_dev, BTN_2, 1);
		#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
			input_report_abs(data->pin_dev, ABS_HAT0X,
				data->gesture_info.pt[1].x);
			input_report_abs(data->pin_dev, ABS_HAT0Y,
				data->gesture_info.pt[1].y);
		#else
			input_report_abs(data->pin_dev, ABS_HAT0X,
					data->gesture_info.pt[1].y);
			input_report_abs(data->pin_dev, ABS_HAT0Y,
					data->gesture_info.pt[1].x);
		#endif
		}
		input_report_abs(data->pin_dev, ABS_MT_TOUCH_MAJOR, 1);
		input_report_key(data->pin_dev, ABS_MT_WIDTH_MAJOR, 1);
	#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
		input_report_abs(data->pin_dev, ABS_MT_POSITION_X,
				data->gesture_info.pt[0].x);
		input_report_abs(data->pin_dev, ABS_MT_POSITION_Y,
				data->gesture_info.pt[0].y);
	#else
		input_report_abs(data->pin_dev, ABS_MT_POSITION_X,
				data->gesture_info.pt[0].y);
		input_report_abs(data->pin_dev, ABS_MT_POSITION_Y,
				data->gesture_info.pt[0].x);
	#endif
		input_mt_sync(data->pin_dev);
		if (data->finger2_pressed) {
			input_report_abs(data->pin_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_key(data->pin_dev, ABS_MT_WIDTH_MAJOR, 1);
		#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
			input_report_abs(data->pin_dev, ABS_MT_POSITION_X,
				data->gesture_info.pt[1].x);
			input_report_abs(data->pin_dev, ABS_MT_POSITION_Y,
				data->gesture_info.pt[1].y);
		#else
			input_report_abs(data->pin_dev, ABS_MT_POSITION_X,
					data->gesture_info.pt[1].y);
			input_report_abs(data->pin_dev, ABS_MT_POSITION_Y,
					data->gesture_info.pt[1].x);
		#endif
			input_mt_sync(data->pin_dev);
		}
#endif
	} else {
		input_report_abs(data->pin_dev, ABS_PRESSURE, 0);
		input_report_abs(data->pin_dev, ABS_TOOL_WIDTH, 0);
		input_report_key(data->pin_dev, BTN_TOUCH, 0);
	#ifdef CONFIG_U8500_TSC_MULTITOUCH
		if (data->finger2_count > 0) {
			input_report_key(data->pin_dev, BTN_2, 0);
			input_report_abs(data->pin_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_key(data->pin_dev, ABS_MT_WIDTH_MAJOR, 0);
			input_mt_sync(data->pin_dev);
			data->finger2_count = 0;
		}
	#endif
	}
	input_sync(data->pin_dev);
}
/**
 * tsc_task() - called for getting the touch point
 * @data:u8500_tsc_data structure pointer
 *
 * This funtion calls, when we get the Pen down interrupt assigns the task
 * and used to calculate the x,y postitons
 */
static void tsc_task(struct u8500_tsc_data *data)
{
	get_touch_message(data);
	if (data->touch_en)
		tsc_input_report(data, 1);
	/* pen down event, (re)start the pen up timer */
	tsc_restart_pen_up_timer(data);
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
	struct u8500_tsc_data *data = (struct u8500_tsc_data *)device_data;
	data->touchp_flag = 1;
	wake_up_interruptible(&data->touchp_event);
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
	struct u8500_tsc_data *data = (struct u8500_tsc_data *)device_data;
	data->touchp_flag = 1;
	wake_up_interruptible(&data->touchp_event);
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
	struct u8500_tsc_data *data = (struct u8500_tsc_data *)dev_data;
	schedule_work(&data->workq);
}

/**
 * tsc_timer_wq() - Work Queue for timer handler
 * @work:work_struct structure pointer
 *
 * This work queue used to get the co-ordinates
 * of the Pen up and Pen down interrupts and returns none
 */
static void tsc_timer_wq(struct work_struct *work)
{
	struct u8500_tsc_data
		*data = container_of(work, struct u8500_tsc_data, workq);
	unsigned char pin_value;
	struct task_struct *tsk = current;

	set_task_state(tsk, TASK_INTERRUPTIBLE);
	pin_value = data->chip->pirq_read_val();
	if (pin_value == 1) {
		/* The pen is up */
		tsc_clear_irq(data->client);
		tsc_en_irq(data->client);
		return ;
	}

	get_touch_message(data);
	if (data->touch_en)
		tsc_input_report(data, 1);
	else
		tsc_input_report(data, 0);

	if (pin_value == 0)
		tsc_restart_pen_up_timer(data);
	return;
}

/**
 * touchp_thread() - get the touch panel co-ordinates
 * @_data:     u8500_tsc_data structure pointer
 *
 * This function will be used to
 * get the touch co-ordinates from the touch panel chip
 **/
static int touchp_thread(void *_data)
{
	struct u8500_tsc_data *tp_data = _data;
	struct task_struct *tsk = current;
	struct sched_param param = { .sched_priority = 1 };

	sched_setscheduler(tsk, SCHED_FIFO, &param);

	set_freezable();
	while (!kthread_should_stop()) {
		wait_event_freezable(tp_data->touchp_event,
				((tp_data->touchp_flag == 1) || \
				kthread_should_stop()));
		tp_data->touchp_flag = 0;
		tsc_task(tp_data);
	}

	tp_data->touchp_tsk = NULL;
	return 0;
}
/*
 * GetCaliStatus() - Get the Calibration status
 * @i2c: i2c client structure pointer
 * Get the calibration status of the touch sensor
 * and returns integer
 */
int getCalibStatus(struct i2c_client *i2c)
{
	tsc_error retval = TSC_OK;
	unsigned int value;
	u8 i;

	for (i = 0x40; i <= 0x4B; i += 2) {
		retval = tsc_read_10bit(i2c, i, &value);
		if (retval < 0)
			dev_err(&i2c->dev,
			"getCalibStatus:failed retval=0x%x \n", retval);
	}
	for (i = 0x54; i <= 0x69; i += 2) {
		retval = tsc_read_10bit(i2c, i, &value);
		if (retval < 0)
			dev_err(&i2c->dev,
			"getCalibStatus:failed retval=0x%x \n", retval);
	}
	return retval;
}
/*
 * doCalibrate() - Do the software Calibration
 * @i2c: i2c_client structure pointer
 * Do the soft calibration on the touch screen
 * and returns integer
 */
int doCalibrate(struct i2c_client *i2c)
{
	tsc_error retval = TSC_OK;
	retval = tsc_write_byte(i2c, TSC_CALIB, 0X1);
	if (retval < 0)
		dev_err(&i2c->dev, "reset failed retval=0x%x \n", retval);
	return retval;
}
/*
 * check_board(): check for href v1 board
 * @data: device structure pointer
 * This function used to check_board
 */
void check_board(struct u8500_tsc_data *data)
{
	if (data->chip->board_href_v1)
		data->href_v1_flag = data->chip->board_href_v1();
}
/*
 * init_config(): Initialize the Global variables
 * @data: device structure pointer
 * This function used to initialize the device variables and returns none
 */
void init_config(struct u8500_tsc_data *data)
{
	signed int i;

	data->touch_count = STOP;
	data->touchflag = FALSE;
	data->touch_continue = CLR;

	data->pre_tap_flag_level = 0;
	data->pre_tap_flag = 0;
	data->flick_flag = 0;
	data->finger1_pressed = 0;

#ifdef CONFIG_U8500_TSC_MULTITOUCH
	data->finger2_pressed = 0;
	data->finger2_count = 0;
#endif
	data->x1 = 0;
	data->y1 = 0;
	data->x2 = 0;
	data->y2 = 0;

	data->pinch_start = FALSE;

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

	/* invalid turning */
	data->rotate_data[DIR_UP][DIR_DOWN] = DIR_INVALID;
	data->rotate_data[DIR_DOWN][DIR_UP] = DIR_INVALID;
	data->rotate_data[DIR_LEFT][DIR_RIGHT] = DIR_INVALID;
	data->rotate_data[DIR_RIGHT][DIR_LEFT] = DIR_INVALID;
	data->rotate_data[DIR_UP][DIR_UP] = DIR_INVALID;
	data->rotate_data[DIR_DOWN][DIR_DOWN] = DIR_INVALID;
	data->rotate_data[DIR_LEFT][DIR_LEFT] = DIR_INVALID;
	data->rotate_data[DIR_RIGHT][DIR_RIGHT] = DIR_INVALID;

	data->dir_idx = DIRHEADER;

	for (i = 0; i < DIRHEADER; i++)
		data->dir_trace[i] = 0x0;

	for (i = DIRHEADER; i < DIRTRACEN; i++)
		data->dir_trace[i] = 0x05;

	data->tap_start_point.x = 0x0;
	data->tap_start_point.y = 0x0;
	data->href_v1_flag = FALSE;
	return;
}
/**
 * bu21013_tsc_init() - Power on sequence for the bu21013 controller
 * @i2c:pointer to i2c client structure
 *
 * This funtion is used to power on
 * the bu21013 controller and returns tsc error.
 **/
static tsc_error bu21013_tsc_init(struct i2c_client *i2c)
{
	tsc_error retval = TSC_OK;

	dev_dbg(&i2c->dev, "bu21013_tsc_init start\n");

	retval = i2c_smbus_write_byte_data(i2c, TSC_RESET, 0x01);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "ED reg i2c smbus write byte failed\n");
		goto err;
	}
	mdelay(30);

	retval = i2c_smbus_write_byte_data(i2c, TSC_SENSOR_0_7, 0x3F);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F0 reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_SENSOR_8_15, 0xFC);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F1 reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_SENSOR_16_23, 0x1F);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F2 reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_POS_MODE1, 0x06);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F3 reg i2c smbus write byte failed\n");
		goto err;
	}
#ifdef CONFIG_U8500_TSC_MULTITOUCH
	retval = i2c_smbus_write_byte_data(i2c, TSC_POS_MODE2, 0x97);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F4 reg i2c smbus write byte failed\n");
		goto err;
	}
#else
	retval = i2c_smbus_write_byte_data(i2c, TSC_POS_MODE2, 0x17);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F4 reg i2c smbus write byte failed\n");
		goto err;
	}
#endif
#ifdef CONFIG_U8500_TSC_EXT_CLK_9_6
	retval = i2c_smbus_write_byte_data(i2c, TSC_CLK_MODE, 0x82);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F5 reg i2c smbus write byte failed\n");
		goto err;
	}
#else
	retval = i2c_smbus_write_byte_data(i2c, TSC_CLK_MODE, 0x81);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "F5 reg i2c smbus write byte failed\n");
		goto err;
	}
#endif
	retval = i2c_smbus_write_byte_data(i2c, TSC_IDLE, 0x12);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "FA reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_INT_MODE, 0x00);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "E9 reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_FILTER, 0xFF);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "FB reg i2c smbus write byte failed\n");
		goto err;
	}
#if (defined CONFIG_CPU_IDLE || defined CONFIG_TOUCHP_TUNING)
	retval = i2c_smbus_write_byte_data(i2c, TSC_TH_ON, th_on);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "FC reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_TH_OFF, th_off);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "FD reg i2c smbus write byte failed\n");
		goto err;
	}
#else
	retval = i2c_smbus_write_byte_data(i2c, TSC_TH_ON, 0x50);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "FC i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_TH_OFF, 0x40);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "FD i2c smbus write byte failed\n");
		goto err;
	}
#endif
	retval = i2c_smbus_write_byte_data(i2c, TSC_GAIN, 0x06);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "EA reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_OFFSET_MODE, 0x00);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "EB reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_XY_EDGE, 0xA5);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "EC reg i2c smbus write byte failed\n");
		goto err;
	}
	retval = i2c_smbus_write_byte_data(i2c, TSC_DONE, 0x01);
	if (retval < TSC_OK) {
		dev_err(&i2c->dev, "EF reg i2c smbus write byte failed\n");
		goto err;
	}
err:
	return retval;
}

/*
 * tsc_driver_register() - Used for input driver registeration
 * @pdata: pointer to u8500_tsc_data structure
 * This function used to register the
 * touch screen as input device and returns integer
 */
static int tsc_driver_register(struct u8500_tsc_data *pdata)
{
	int ret = TSC_OK;

	struct input_dev *in_dev;
	struct i2c_client *i2c = pdata->client;
	dev_dbg(&i2c->dev, "tsc_driver_register start\n");

	/* register the touch screen driver to input device */
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
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
	input_set_abs_params(in_dev, ABS_X, 0, X_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_Y, 0, Y_MAX, 0, 0);
#else
	input_set_abs_params(in_dev, ABS_X, 0, Y_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_Y, 0, X_MAX, 0, 0);
#endif
	input_set_abs_params(in_dev, ABS_PRESSURE, 0, 1, 0, 0);
	input_set_abs_params(in_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);
#ifdef CONFIG_U8500_TSC_MULTITOUCH
	set_bit(BTN_2, in_dev->keybit);
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
	input_set_abs_params(in_dev, ABS_HAT0X, 0, X_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_HAT0Y, 0, Y_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_POSITION_X, 0, X_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_POSITION_Y, 0, Y_MAX, 0, 0);
#else
	input_set_abs_params(in_dev, ABS_HAT0X, 0, Y_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_HAT0Y, 0, X_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_POSITION_X, 0, Y_MAX, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_POSITION_Y, 0, X_MAX, 0, 0);
#endif
	input_set_abs_params(in_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(in_dev, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
#endif
	ret = input_register_device(in_dev);
	if (ret) {
		dev_err(&i2c->dev, " could not register error \n");
		goto err;
	}
	dev_dbg(&i2c->dev, "tsc_driver_register done \n");
err:
	return ret;
}
/**
 * tsc_config() - configure the touch screen controller
 * @pdev_data: pointer to u8500_tsc_data structure
 *
 * This funtion is used to configure
 * the bu21013 controller and returns tsc error.
 **/
static tsc_error tsc_config(struct u8500_tsc_data *pdev_data)
{
	int retval;

	retval = bu21013_tsc_init(pdev_data->client);
	if (retval == TSC_OK) {
		init_config(pdev_data);
		check_board(pdev_data);
		if (pdev_data->href_v1_flag == FALSE) {
			if ((pdev_data->chip->pirq_en) && (pdev_data->chip->pirq_dis)
					&& (pdev_data->chip->irq_init)) {
				retval = pdev_data->chip->pirq_dis();
				if (retval < 0) {
					dev_err(&pdev_data->client->dev,
						" irq disable failed \n");
					goto err;
				}
				retval = pdev_data->chip->pirq_en();
				if (retval < 0) {
					dev_err(&pdev_data->client->dev,
						" irq en failed \n");
					goto err_init_irq;
				}
				if \
				(pdev_data->chip->irq_init \
					(tsc_callback, (void *)pdev_data)) {
					dev_err(&pdev_data->client->dev, \
					" initiate the callback handler failed \n");
					goto err_init_irq;
				}
			}
		}
		retval = getCalibStatus(pdev_data->client);
		if (retval < 0) {
			dev_err(&pdev_data->client->dev,
				"u8500_tsc_probe::calibration not done \n");
			goto err_init_irq;
		}
		return retval;
	}
err_init_irq:
	pdev_data->chip->pirq_dis();
	pdev_data->chip->irq_exit();
err:
	pdev_data->chip->cs_dis();
	return retval;
}
#ifdef CONFIG_PM
/**
 * tsc_suspend() - suspend the touch screen controller
 * @client: pointer to i2c client structure
 * @mesg: message from power manager
 *
 * This funtion is used to suspend the
 * touch panel controller and returns integer
 **/
static int tsc_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int retval;
	struct u8500_tsc_data *tsc_data = i2c_get_clientdata(client);
	retval = tsc_data->chip->pirq_dis();
	if (retval < 0) {
		dev_err(&client->dev, "tsc_suspend:: irq disable failed \n");
		goto err;
	}
	retval = tsc_data->chip->irq_exit();
	if (retval < 0) {
		dev_err(&client->dev, "tsc_suspend:: remove \
				the callback handler failed \n");
		goto err;
	}
	return TSC_OK;
err:
	kfree(tsc_data);
	return retval;
}

/**
 * tsc_resume() - resume the touch screen controller
 * @client: pointer to i2c client structure
 *
 * This funtion is used to resume the touch panel
 * controller and returns integer.
 **/
static int tsc_resume(struct i2c_client *client)
{
	int retval;
	struct u8500_tsc_data *tsc_data = i2c_get_clientdata(client);

	retval = tsc_config(tsc_data);
	if (retval < 0) {
		dev_err(&client->dev, "tsc_resume:: error in \
				touch panel configuration\n");
		goto err;
	}
	if (tsc_data->touchp_tsk)
		wake_up(&tsc_data->touchp_event);
	return TSC_OK;
err:
	kfree(tsc_data);
	return retval;
}
#endif
/**
 * tp_probe() - Initialze the i2c-client touchscreen driver
 * @i2c: i2c client structure pointer
 * @id:i2c device id pointer
 *
 * This funtion uses to Initializes the i2c-client touchscreen
 * driver and returns integer.
 **/
static int tp_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	int retval = 0;
	struct u8500_tsc_data *tsc_data;
	struct tp_device *pdata = i2c->dev.platform_data;
	dev_dbg(&i2c->dev, "u8500_tsc_probe:: start\n");

	tsc_data = kzalloc(sizeof(struct u8500_tsc_data), GFP_KERNEL);
	if (!tsc_data) {
		dev_err(&i2c->dev, "tp memory allocation failed \n");
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

	init_timer(&tsc_data->penirq_timer);
	tsc_data->penirq_timer.data = (unsigned long)tsc_data;
	tsc_data->penirq_timer.function = tsc_timer_callback;

	INIT_WORK(&tsc_data->workq, tsc_timer_wq);
	i2c_set_clientdata(i2c, tsc_data);
	init_waitqueue_head(&tsc_data->touchp_event);

	/* configure the gpio pins */
	if (tsc_data->chip->cs_en) {
		retval = tsc_data->chip->cs_en();
		if (retval != TSC_OK) {
			dev_err(&tsc_data->client->dev,
				"error in tp chip initialization\n");
			goto err;
		}
	}

	/** configure the touch panel controller */
	retval = tsc_config(tsc_data);
	if (retval < 0) {
		dev_err(&i2c->dev, "error in tp configuration\n");
		goto err;
	} else {
		BUG_ON(tsc_data->touchp_tsk);
		if (tsc_data->href_v1_flag) {
			retval = request_irq(tsc_data->chip->irq, tsc_gpio_callback,
						IRQF_TRIGGER_FALLING, DRIVER_TP, tsc_data);
			if (retval) {
				dev_err(&tsc_data->client->dev,
					"unable to request for the irq %d\n", tsc_data->chip->irq);
				gpio_free(tsc_data->chip->irq);
				goto err;
			}
		}
		tsc_data->touchp_tsk = kthread_run(touchp_thread, \
					tsc_data, DRIVER_TP);
		if (IS_ERR(tsc_data->touchp_tsk)) {
			retval = PTR_ERR(tsc_data->touchp_tsk);
			tsc_data->touchp_tsk = NULL;
			goto err;
		}
		retval = tsc_driver_register(tsc_data);
		if (retval)
			goto err_init;
	}
#ifdef CONFIG_TOUCHP_TUNING
	/* Registering touchp device  for sysfs */
	pdev_data = tsc_data;
	touchp_kobj = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (touchp_kobj == NULL) {
		retval = -ENOMEM;
		goto err_init;
	}
	touchp_kobj->ktype = &ktype_touchp;
	kobject_init(touchp_kobj, touchp_kobj->ktype);
	retval = kobject_set_name(touchp_kobj, "touchp_attribute");
	if (retval) {
		kfree(touchp_kobj);
		goto err_init;
	}
	retval = kobject_add(touchp_kobj, NULL, "touchp_attribute");
	if (retval) {
		kfree(touchp_kobj);
		goto err_init;
	}
#endif
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
 * tp_remove() - Removes the i2c-client touchscreen driver
 * @client: i2c client structure pointer
 *
 * This funtion uses to remove the i2c-client
 * touchscreen driver and returns integer.
 **/
static int __exit tp_remove(struct i2c_client *client)
{
	struct u8500_tsc_data *data = i2c_get_clientdata(client);
	del_timer_sync(&data->penirq_timer);

	if (data->touchp_tsk)
		kthread_stop(data->touchp_tsk);
	if (data->chip) {
		data->chip->irq_exit();
		data->chip->pirq_dis();
		data->chip->cs_dis();
	}
	if (data->href_v1_flag)
		gpio_free(data->chip->irq);
	input_unregister_device(data->pin_dev);
	input_free_device(data->pin_dev);
	kfree(data);
	i2c_set_clientdata(client, NULL);
	return TSC_OK;
}

static const struct i2c_device_id tp_id[] = {
	{ DRIVER_TP, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tp_id);

static struct i2c_driver tp_driver = {
	.driver = {
		.name = DRIVER_TP,
		.owner = THIS_MODULE,
	},
	.probe 		= tp_probe,
#ifdef CONFIG_PM
	.suspend	= tsc_suspend,
	.resume		= tsc_resume,
#endif
	.remove 	= __exit_p(tp_remove),
	.id_table	= tp_id,
};

/**
 * tp_init() - Initialize the paltform touchscreen driver
 *
 * This funtion uses to initialize the platform
 * touchscreen driver and returns integer.
 **/
static int __init tp_init(void){
	return i2c_add_driver(&tp_driver);
}

/**
 * tp_exit() - De-initialize the paltform touchscreen driver
 *
 * This funtion uses to de-initialize the platform
 * touchscreen driver and returns none.
 **/
static void __exit tp_exit(void){
	i2c_del_driver(&tp_driver);
}

module_init(tp_init);
module_exit(tp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAVEEN KUMAR G");
MODULE_DESCRIPTION("Touch Screen driver for U8500");
