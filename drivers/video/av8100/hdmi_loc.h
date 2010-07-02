/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Author: Per Persson <per.xb.persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __HDMI_LOC__H__
#define __HDMI_LOC__H__

#define EDID_BUF_LEN	128
#define COMMAND_BUF_LEN	128

struct edid_data {
	u8 buf_len;
	u8 buf[EDID_BUF_LEN];
};

struct hdmi_register {
	unsigned char value;
	unsigned char offset;
};

struct hdcp_loadaesone {
	u8 key[16];
	u8 result;
};

struct hdmi_driver_data {
	bool			store_as_hextext;
	struct plug_detect	plug_detect;
	bool			enable_cec_event;
	struct edid_data	edid_data;
	struct cec_rw		cec_read;
	bool			fuse_result;
	bool			loadaes_result;
};

struct hdmi_command_register {
	unsigned char cmd_id;			/* input */
	unsigned char buf_len;			/* input, output */
	unsigned char buf[COMMAND_BUF_LEN];	/* input, output */
	unsigned char return_status;		/* output */
};

/* Internal */
#define IOC_HDMI_POWER			_IOWR(HDMI_IOC_MAGIC, 31, int)
#define IOC_HDMI_ENABLE_INTERRUPTS	_IOWR(HDMI_IOC_MAGIC, 32, int)
#define IOC_HDMI_DOWNLOAD_FW		_IOWR(HDMI_IOC_MAGIC, 33, int)
#define IOC_HDMI_ONOFF			_IOWR(HDMI_IOC_MAGIC, 34, int)
#define IOC_HDMI_REGISTER_WRITE		_IOWR(HDMI_IOC_MAGIC, 35, int)
#define IOC_HDMI_REGISTER_READ		_IOWR(HDMI_IOC_MAGIC, 36, int)
#define IOC_HDMI_STATUS_GET		_IOWR(HDMI_IOC_MAGIC, 37, int)
#define IOC_HDMI_CONFIGURATION_WRITE	_IOWR(HDMI_IOC_MAGIC, 38, int)

#endif /* __HDMI_LOC__H__ */
