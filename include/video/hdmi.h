/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * HDMI driver
 *
 * Author: Per Persson <per.xb.persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __HDMI__H__
#define __HDMI__H__


#define HDMI_CEC_READ_MAXSIZE		16
#define HDMI_CEC_WRITE_MAXSIZE		15
#define HDMI_INFOFRAME_MAX_SIZE		27
#define HDMI_HDCP_FUSEAES_KEYSIZE	16
#define HDMI_HDCP_AES_BLOCK_START	128
#define HDMI_HDCP_KSV_BLOCK		40
#define HDMI_HDCP_AES_NR_OF_BLOCKS	18
#define HDMI_HDCP_AES_KEYSIZE		16
#define HDMI_HDCP_AES_KSVSIZE		5
#define HDMI_HDCP_AES_KSVZEROESSIZE	3

#define HDMI_STOREASTEXT_TEXT_SIZE	2
#define HDMI_STOREASTEXT_BIN_SIZE	1
#define HDMI_PLUGDETEN_TEXT_SIZE	6
#define HDMI_PLUGDETEN_BIN_SIZE		3
#define HDMI_EDIDREAD_TEXT_SIZE		4
#define HDMI_EDIDREAD_BIN_SIZE		2
#define HDMI_CECEVEN_TEXT_SIZE		2
#define HDMI_CECEVEN_BIN_SIZE		1
#define HDMI_CECSEND_TEXT_SIZE_MAX	37
#define HDMI_CECSEND_TEXT_SIZE_MIN	6
#define HDMI_CECSEND_BIN_SIZE_MAX	18
#define HDMI_CECSEND_BIN_SIZE_MIN	3
#define HDMI_INFOFRSEND_TEXT_SIZE_MIN	8
#define HDMI_INFOFRSEND_TEXT_SIZE_MAX	63
#define HDMI_INFOFRSEND_BIN_SIZE_MIN	4
#define HDMI_INFOFRSEND_BIN_SIZE_MAX	31
#define HDMI_HDCPEVEN_TEXT_SIZE		2
#define HDMI_HDCPEVEN_BIN_SIZE		1
#define HDMI_HDCP_FUSEAES_TEXT_SIZE	34
#define HDMI_HDCP_FUSEAES_BIN_SIZE	17
#define HDMI_HDCP_LOADAES_TEXT_SIZE	586
#define HDMI_HDCP_LOADAES_BIN_SIZE	293
#define HDMI_HDCPAUTHENCR_TEXT_SIZE	4
#define HDMI_HDCPAUTHENCR_BIN_SIZE	2
#define HDMI_EVCLR_TEXT_SIZE		2
#define HDMI_EVCLR_BIN_SIZE		1
#define HDMI_AUDIOCFG_TEXT_SIZE		14
#define HDMI_AUDIOCFG_BIN_SIZE		7

#define HDMI_IOC_MAGIC 0xcc

/** IOCTL Operations */
#define IOC_PLUG_DETECT_ENABLE		_IOWR(HDMI_IOC_MAGIC, 1, int)
#define IOC_EDID_READ			_IOWR(HDMI_IOC_MAGIC, 2, int)
#define IOC_CEC_EVENT_ENABLE		_IOWR(HDMI_IOC_MAGIC, 3, int)
#define IOC_CEC_READ			_IOWR(HDMI_IOC_MAGIC, 4, int)
#define IOC_CEC_SEND			_IOWR(HDMI_IOC_MAGIC, 5, int)
#define IOC_INFOFRAME_SEND		_IOWR(HDMI_IOC_MAGIC, 6, int)
#define IOC_HDCP_EVENT_ENABLE		_IOWR(HDMI_IOC_MAGIC, 7, int)
#define IOC_HDCP_CHKAESOTP		_IOWR(HDMI_IOC_MAGIC, 8, int)
#define IOC_HDCP_FUSEAES		_IOWR(HDMI_IOC_MAGIC, 9, int)
#define IOC_HDCP_LOADAES		_IOWR(HDMI_IOC_MAGIC, 10, int)
#define IOC_HDCP_AUTHENCR_REQ		_IOWR(HDMI_IOC_MAGIC, 11, int)
#define IOC_HDCP_STATE_GET		_IOWR(HDMI_IOC_MAGIC, 12, int)
#define IOC_REVOCATION_LIST_GET		_IOWR(HDMI_IOC_MAGIC, 13, int)
#define IOC_REVOCATION_LIST_SET		_IOWR(HDMI_IOC_MAGIC, 14, int)
#define IOC_EVENTS_READ			_IOWR(HDMI_IOC_MAGIC, 15, int)
#define IOC_EVENTS_CLEAR		_IOWR(HDMI_IOC_MAGIC, 16, int)
#define IOC_AUDIO_CFG			_IOWR(HDMI_IOC_MAGIC, 17, int)


/* HDMI driver */
void hdmi_event(enum av8100_hdmi_event);
int hdmi_init(void);
void hdmi_exit(void);

enum hdmi_event {
	HDMI_EVENT_NONE =		0x0,
	HDMI_EVENT_HDMI_PLUGIN =	0x1,
	HDMI_EVENT_HDMI_PLUGOUT =	0x2,
	HDMI_EVENT_CEC =		0x4,
	HDMI_EVENT_HDCP =		0x8,
};

enum hdmi_hdcp_auth_type {
	HDMI_HDCP_AUTH_OFF = 0,
	HDMI_HDCP_AUTH_START = 1,
	HDMI_HDCP_AUTH_CONT = 2,
};

enum hdmi_hdcp_encr_type {
	HDMI_HDCP_ENCR_OFF = 0,
	HDMI_HDCP_ENCR_OESS = 1,
	HDMI_HDCP_ENCR_EESS = 2,
};

struct plug_detect {
	u8 hdmi_detect_enable;
	u8 on_time;
	u8 hdmi_off_time;
};

struct edid_read {
	u8 address;
	u8 block_nr;
	u8 data_length;
	u8 data[128];
};

struct cec_rw  {
	u8 src;
	u8 dest;
	u8 length;
	u8 data[15];
};

struct info_fr {
	u8 type;
	u8 ver;
	u8 crc;
	u8 length;
	u8 data[27];
};

struct hdcp_fuseaes {
	u8 key[16];
	u8 crc;
	u8 result;
};

struct hdcp_loadaesall {
	u8 ksv[5];
	u8 key[288];
	u8 result;
};

struct hdcp_authencr {
	u8 auth_type;
	u8 encr_type;
};

struct audio_cfg {
	u8 if_format;
	u8 i2s_entries;
	u8 freq;
	u8 word_length;
	u8 format;
	u8 if_mode;
	u8 mute;
};

#endif /* __HDMI__H__ */
