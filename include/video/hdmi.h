/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * AV8100 driver
 *
 * Author: Per Persson <per.xb.persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __HDMI__H__
#define __HDMI__H__

#define HDMI_DRIVER_MINOR_NUMBER 241

struct hdmi_register {
	unsigned char	value;
	unsigned char	offset;
};

struct hdmi_command_register {
	unsigned char cmd_id;		/* input */
	unsigned char buf_len;		/* input, output */
	unsigned char buf[128];		/* input, output */
	unsigned char return_status;	/* output */
};

/* IOCTL return status */
#define HDMI_COMMAND_RETURN_STATUS_OK			0
#define HDMI_COMMAND_RETURN_STATUS_FAIL			1

#define HDMI_IOC_MAGIC 0xcc

/** IOCTL Operations */
#define IOC_HDMI_POWER			_IOWR(HDMI_IOC_MAGIC, 1, int)
#define IOC_HDMI_ENABLE_INTERRUPTS	_IOWR(HDMI_IOC_MAGIC, 2, int)
#define IOC_HDMI_DOWNLOAD_FW		_IOWR(HDMI_IOC_MAGIC, 3, int)
#define IOC_HDMI_ONOFF			_IOWR(HDMI_IOC_MAGIC, 4, int)

#define IOC_HDMI_REGISTER_WRITE		_IOWR(HDMI_IOC_MAGIC, 5, int)
#define IOC_HDMI_REGISTER_READ		_IOWR(HDMI_IOC_MAGIC, 6, int)
#define IOC_HDMI_STATUS_GET		_IOWR(HDMI_IOC_MAGIC, 7, int)
#define IOC_HDMI_CONFIGURATION_WRITE	_IOWR(HDMI_IOC_MAGIC, 8, int)


/* HDMI driver */
int hdmi_init(void);
void hdmi_exit(void);

#endif /* __HDMI__H__ */
