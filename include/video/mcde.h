/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * ST-Ericsson MCDE base driver
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __MCDE__H__
#define __MCDE__H__

/* Physical interface types */
enum mcde_port_type {
	MCDE_PORTTYPE_DSI = 0,
	MCDE_PORTTYPE_DPI = 1,
};

/* Interface mode */
enum mcde_port_mode {
	MCDE_PORTMODE_CMD = 0,
	MCDE_PORTMODE_VID = 1,
};

/* MCDE fifos */
enum mcde_fifo {
	MCDE_FIFO_A  = 0,
	MCDE_FIFO_B  = 1,
	MCDE_FIFO_C0 = 2,
	MCDE_FIFO_C1 = 3,
};

/* MCDE channels (pixel pipelines) */
enum mcde_chnl {
	MCDE_CHNL_A  = 0,
	MCDE_CHNL_B  = 1,
	MCDE_CHNL_C0 = 2,
	MCDE_CHNL_C1 = 3,
};

/* Channel path */
#define MCDE_CHNLPATH(__chnl, __fifo, __type, __ifc, __link) \
	(((__chnl) << 16) | ((__fifo) << 12) | \
	 ((__type) << 8) | ((__ifc) << 4) | ((__link) << 0))
enum mcde_chnl_path {
	/* Channel A */
	MCDE_CHNLPATH_CHNLA_FIFOA_DPI_0 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_A, MCDE_PORTTYPE_DPI, 0, 0),
	MCDE_CHNLPATH_CHNLA_FIFOA_DSI_IFC0_0 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_A, MCDE_PORTTYPE_DSI, 0, 0),
	MCDE_CHNLPATH_CHNLA_FIFOA_DSI_IFC0_1 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_A, MCDE_PORTTYPE_DSI, 0, 1),
	MCDE_CHNLPATH_CHNLA_FIFOC0_DSI_IFC0_2 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_C0, MCDE_PORTTYPE_DSI, 0, 2),
	MCDE_CHNLPATH_CHNLA_FIFOC0_DSI_IFC1_0 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_C0, MCDE_PORTTYPE_DSI, 1, 0),
	MCDE_CHNLPATH_CHNLA_FIFOC0_DSI_IFC1_1 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_C0, MCDE_PORTTYPE_DSI, 1, 1),
	MCDE_CHNLPATH_CHNLA_FIFOA_DSI_IFC1_2 = MCDE_CHNLPATH(MCDE_CHNL_A,
		MCDE_FIFO_A, MCDE_PORTTYPE_DSI, 1, 2),
	/* Channel B */
	MCDE_CHNLPATH_CHNLB_FIFOB_DPI_1 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_B, MCDE_PORTTYPE_DPI, 0, 1),
	MCDE_CHNLPATH_CHNLB_FIFOB_DSI_IFC0_0 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_B, MCDE_PORTTYPE_DSI, 0, 0),
	MCDE_CHNLPATH_CHNLB_FIFOB_DSI_IFC0_1 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_B, MCDE_PORTTYPE_DSI, 0, 1),
	MCDE_CHNLPATH_CHNLB_FIFOC1_DSI_IFC0_2 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_C1, MCDE_PORTTYPE_DSI, 0, 2),
	MCDE_CHNLPATH_CHNLB_FIFOC1_DSI_IFC1_0 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_C1, MCDE_PORTTYPE_DSI, 1, 0),
	MCDE_CHNLPATH_CHNLB_FIFOC1_DSI_IFC1_1 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_C1, MCDE_PORTTYPE_DSI, 1, 1),
	MCDE_CHNLPATH_CHNLB_FIFOB_DSI_IFC1_2 = MCDE_CHNLPATH(MCDE_CHNL_B,
		MCDE_FIFO_B, MCDE_PORTTYPE_DSI, 1, 2),
	/* Channel C0 */
	MCDE_CHNLPATH_CHNLC0_FIFOA_DSI_IFC0_0 = MCDE_CHNLPATH(MCDE_CHNL_C0,
		MCDE_FIFO_A, MCDE_PORTTYPE_DSI, 0, 0),
	MCDE_CHNLPATH_CHNLC0_FIFOA_DSI_IFC0_1 = MCDE_CHNLPATH(MCDE_CHNL_C0,
		MCDE_FIFO_A, MCDE_PORTTYPE_DSI, 0, 1),
	MCDE_CHNLPATH_CHNLC0_FIFOC0_DSI_IFC0_2 = MCDE_CHNLPATH(MCDE_CHNL_C0,
		MCDE_FIFO_C0, MCDE_PORTTYPE_DSI, 0, 2),
	MCDE_CHNLPATH_CHNLC0_FIFOC0_DSI_IFC1_0 = MCDE_CHNLPATH(MCDE_CHNL_C0,
		MCDE_FIFO_C0, MCDE_PORTTYPE_DSI, 1, 0),
	MCDE_CHNLPATH_CHNLC0_FIFOC0_DSI_IFC1_1 = MCDE_CHNLPATH(MCDE_CHNL_C0,
		MCDE_FIFO_C0, MCDE_PORTTYPE_DSI, 1, 1),
	MCDE_CHNLPATH_CHNLC0_FIFOA_DSI_IFC1_2 = MCDE_CHNLPATH(MCDE_CHNL_C0,
		MCDE_FIFO_A, MCDE_PORTTYPE_DSI, 1, 2),
	/* Channel C1 */
	MCDE_CHNLPATH_CHNLC1_FIFOB_DSI_IFC0_0 = MCDE_CHNLPATH(MCDE_CHNL_C1,
		MCDE_FIFO_B, MCDE_PORTTYPE_DSI, 0, 0),
	MCDE_CHNLPATH_CHNLC1_FIFOB_DSI_IFC0_1 = MCDE_CHNLPATH(MCDE_CHNL_C1,
		MCDE_FIFO_B, MCDE_PORTTYPE_DSI, 0, 1),
	MCDE_CHNLPATH_CHNLC1_FIFOC1_DSI_IFC0_2 = MCDE_CHNLPATH(MCDE_CHNL_C1,
		MCDE_FIFO_C1, MCDE_PORTTYPE_DSI, 0, 2),
	MCDE_CHNLPATH_CHNLC1_FIFOC1_DSI_IFC1_0 = MCDE_CHNLPATH(MCDE_CHNL_C1,
		MCDE_FIFO_C1, MCDE_PORTTYPE_DSI, 1, 0),
	MCDE_CHNLPATH_CHNLC1_FIFOC1_DSI_IFC1_1 = MCDE_CHNLPATH(MCDE_CHNL_C1,
		MCDE_FIFO_C1, MCDE_PORTTYPE_DSI, 1, 1),
	MCDE_CHNLPATH_CHNLC1_FIFOB_DSI_IFC1_2 = MCDE_CHNLPATH(MCDE_CHNL_C1,
		MCDE_FIFO_B, MCDE_PORTTYPE_DSI, 1, 2),
};

/* Update sync mode */
enum mcde_sync_src {
	MCDE_SYNCSRC_OFF = 0, /* No sync */
	MCDE_SYNCSRC_TE0 = 1, /* MCDE ext TE0 */
	MCDE_SYNCSRC_TE1 = 2, /* MCDE ext TE1 */
	MCDE_SYNCSRC_BTA = 3, /* DSI BTA */
};

/* Interface pixel formats (output) */
/*
* REVIEW: Define formats
* Add explanatory comments how the formats are ordered in memory
*/
enum mcde_port_pix_fmt {
	/* MIPI standard formats */

	MCDE_PORTPIXFMT_DPI_16BPP_C1 =     0x21,
	MCDE_PORTPIXFMT_DPI_16BPP_C2 =     0x22,
	MCDE_PORTPIXFMT_DPI_16BPP_C3 =     0x23,
	MCDE_PORTPIXFMT_DPI_18BPP_C1 =     0x24,
	MCDE_PORTPIXFMT_DPI_18BPP_C2 =     0x25,
	MCDE_PORTPIXFMT_DPI_24BPP =        0x26,

	MCDE_PORTPIXFMT_DSI_16BPP =        0x31,
	MCDE_PORTPIXFMT_DSI_18BPP =        0x32,
	MCDE_PORTPIXFMT_DSI_18BPP_PACKED = 0x33,
	MCDE_PORTPIXFMT_DSI_24BPP =        0x34,

	/* Custom formats */
	MCDE_PORTPIXFMT_DSI_YCBCR422 =     0x40,
};

struct mcde_col_convert {
	u16 matrix[3][3];
	u16 offset[3];
};

struct mcde_port {
	enum mcde_port_type type;
	enum mcde_port_mode mode;
	enum mcde_port_pix_fmt pixel_format;
	u8 ifc;
	u8 link;
	enum mcde_sync_src sync_src;
	bool update_auto_trig;
	union {
		struct {
			u8 virt_id;
			u8 num_data_lanes;
			u8 ui;
			bool clk_cont;
		} dsi;
		struct {
			u8 bus_width;
		} dpi;
	} phy;
};

/* Overlay pixel formats (input) *//* REVIEW: Define byte order */
enum mcde_ovly_pix_fmt {
	MCDE_OVLYPIXFMT_RGB565   = 1,
	MCDE_OVLYPIXFMT_RGBA5551 = 2,
	MCDE_OVLYPIXFMT_RGBA4444 = 3,
	MCDE_OVLYPIXFMT_RGB888   = 4,
	MCDE_OVLYPIXFMT_RGBX8888 = 5,
	MCDE_OVLYPIXFMT_RGBA8888 = 6,
	MCDE_OVLYPIXFMT_YCbCr422 = 7,/* REVIEW: Capitalize */
};

/* Display power modes */
enum mcde_display_power_mode {
	MCDE_DISPLAY_PM_OFF     = 0, /* Power off */
	MCDE_DISPLAY_PM_STANDBY = 1, /* DCS sleep mode */
	MCDE_DISPLAY_PM_ON      = 2, /* DCS normal mode, display on */
};

/* Display rotation */
enum mcde_display_rotation {
	MCDE_DISPLAY_ROT_0       = 0,
	MCDE_DISPLAY_ROT_90_CCW  = 90,
	MCDE_DISPLAY_ROT_180_CCW = 180,
	MCDE_DISPLAY_ROT_270_CCW = 270,
	MCDE_DISPLAY_ROT_90_CW   = MCDE_DISPLAY_ROT_270_CCW,
	MCDE_DISPLAY_ROT_180_CW  = MCDE_DISPLAY_ROT_180_CCW,
	MCDE_DISPLAY_ROT_270_CW  = MCDE_DISPLAY_ROT_90_CCW,
};

/* REVIEW: Verify */
#define MCDE_MIN_WIDTH  16
#define MCDE_MIN_HEIGHT 16
#define MCDE_MAX_WIDTH  2048
#define MCDE_MAX_HEIGHT 2048
#define MCDE_BUF_START_ALIGMENT 8
#define MCDE_BUF_LINE_ALIGMENT 8

#define MCDE_FIFO_AB_SIZE 640
#define MCDE_FIFO_C0C1_SIZE 160

#define MCDE_PIXFETCH_LARGE_WTRMRKLVL 128
#define MCDE_PIXFETCH_MEDIUM_WTRMRKLVL 64
#define MCDE_PIXFETCH_SMALL_WTRMRKLVL 16

/* In seconds */
#define MCDE_AUTO_SYNC_WATCHDOG 5

/* Video mode descriptor */
struct mcde_video_mode {/* REVIEW: Join 1 & 2 */
	u32 xres;
	u32 yres;
	u32 pixclock;	/* pixel clock in ps (pico seconds) */
	u32 hbp;	/* hor back porch = left_margin */
	u32 hfp;	/* hor front porch equals to right_margin */
	u32 vbp1;	/* field 1: vert back porch equals to upper_margin */
	u32 vfp1;	/* field 1: vert front porch equals to lower_margin */
	u32 vbp2;	/* field 2: vert back porch equals to upper_margin */
	u32 vfp2;	/* field 2: vert front porch equals to lower_margin */
	bool interlaced;
};

struct mcde_rectangle {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

struct mcde_overlay_info {
	u32 paddr;
	u16 stride; /* buffer line len in bytes */
	enum mcde_ovly_pix_fmt fmt;

	u16 src_x;
	u16 src_y;
	u16 dst_x;
	u16 dst_y;
	u16 dst_z;
	u16 w;
	u16 h;
	struct mcde_rectangle dirty;
};

struct mcde_overlay {
	struct kobject kobj;
	struct list_head list; /* mcde_display_device.ovlys */

	struct mcde_display_device *ddev;
	struct mcde_overlay_info info;
	struct mcde_ovly_state *state;
};

struct mcde_chnl_state;

struct mcde_chnl_state *mcde_chnl_get(enum mcde_chnl chnl_id,
			enum mcde_fifo fifo, const struct mcde_port *port);
int mcde_chnl_set_pixel_format(struct mcde_chnl_state *chnl,
					enum mcde_port_pix_fmt pix_fmt);
void mcde_chnl_set_col_convert(struct mcde_chnl_state *chnl,
					struct mcde_col_convert *col_convert);
int mcde_chnl_set_video_mode(struct mcde_chnl_state *chnl,
					struct mcde_video_mode *vmode);
/* TODO: Remove rotbuf* parameters when ESRAM allocator is implemented*/
int mcde_chnl_set_rotation(struct mcde_chnl_state *chnl,
		enum mcde_display_rotation rotation, u32 rotbuf1, u32 rotbuf2);
int mcde_chnl_enable_synchronized_update(struct mcde_chnl_state *chnl,
								bool enable);
int mcde_chnl_set_power_mode(struct mcde_chnl_state *chnl,
				enum mcde_display_power_mode power_mode);

int mcde_chnl_apply(struct mcde_chnl_state *chnl);
int mcde_chnl_update(struct mcde_chnl_state *chnl,
					struct mcde_rectangle *update_area);
void mcde_chnl_put(struct mcde_chnl_state *chnl);

void mcde_chnl_stop_flow(struct mcde_chnl_state *chnl);

/* MCDE overlay */
struct mcde_ovly_state;

struct mcde_ovly_state *mcde_ovly_get(struct mcde_chnl_state *chnl);
void mcde_ovly_set_source_buf(struct mcde_ovly_state *ovly,
	u32 paddr);
void mcde_ovly_set_source_info(struct mcde_ovly_state *ovly,
	u32 stride, enum mcde_ovly_pix_fmt pix_fmt);
void mcde_ovly_set_source_area(struct mcde_ovly_state *ovly,
	u16 x, u16 y, u16 w, u16 h);
void mcde_ovly_set_dest_pos(struct mcde_ovly_state *ovly,
	u16 x, u16 y, u8 z);
void mcde_ovly_apply(struct mcde_ovly_state *ovly);
void mcde_ovly_put(struct mcde_ovly_state *ovly);

/* MCDE dsi */

#define DCS_CMD_ENTER_IDLE_MODE       0x39
#define DCS_CMD_ENTER_INVERT_MODE     0x21
#define DCS_CMD_ENTER_NORMAL_MODE     0x13
#define DCS_CMD_ENTER_PARTIAL_MODE    0x12
#define DCS_CMD_ENTER_SLEEP_MODE      0x10
#define DCS_CMD_EXIT_IDLE_MODE        0x38
#define DCS_CMD_EXIT_INVERT_MODE      0x20
#define DCS_CMD_EXIT_SLEEP_MODE       0x11
#define DCS_CMD_GET_ADDRESS_MODE      0x0B
#define DCS_CMD_GET_BLUE_CHANNEL      0x08
#define DCS_CMD_GET_DIAGNOSTIC_RESULT 0x0F
#define DCS_CMD_GET_DISPLAY_MODE      0x0D
#define DCS_CMD_GET_GREEN_CHANNEL     0x07
#define DCS_CMD_GET_PIXEL_FORMAT      0x0C
#define DCS_CMD_GET_POWER_MODE        0x0A
#define DCS_CMD_GET_RED_CHANNEL       0x06
#define DCS_CMD_GET_SCANLINE          0x45
#define DCS_CMD_GET_SIGNAL_MODE       0x0E
#define DCS_CMD_NOP                   0x00
#define DCS_CMD_READ_DDB_CONTINUE     0xA8
#define DCS_CMD_READ_DDB_START        0xA1
#define DCS_CMD_READ_MEMORY_CONTINE   0x3E
#define DCS_CMD_READ_MEMORY_START     0x2E
#define DCS_CMD_SET_ADDRESS_MODE      0x36
#define DCS_CMD_SET_COLUMN_ADDRESS    0x2A
#define DCS_CMD_SET_DISPLAY_OFF       0x28
#define DCS_CMD_SET_DISPLAY_ON        0x29
#define DCS_CMD_SET_GAMMA_CURVE       0x26
#define DCS_CMD_SET_PAGE_ADDRESS      0x2B
#define DCS_CMD_SET_PARTIAL_AREA      0x30
#define DCS_CMD_SET_PIXEL_FORMAT      0x3A
#define DCS_CMD_SET_SCROLL_AREA       0x33
#define DCS_CMD_SET_SCROLL_START      0x37
#define DCS_CMD_SET_TEAR_OFF          0x34
#define DCS_CMD_SET_TEAR_ON           0x35
#define DCS_CMD_SET_TEAR_SCANLINE     0x44
#define DCS_CMD_SOFT_RESET            0x01
#define DCS_CMD_WRITE_LUT             0x2D
#define DCS_CMD_WRITE_CONTINUE        0x3C
#define DCS_CMD_WRITE_START           0x2C

#define MCDE_MAX_DCS_READ   4
#define MCDE_MAX_DCS_WRITE 15

int mcde_dsi_dcs_write(struct mcde_chnl_state *chnl, u8 cmd, u8* data, int len);
int mcde_dsi_dcs_read(struct mcde_chnl_state *chnl, u8 cmd, u8* data, int *len);

/* MCDE */

/* Driver data */
#define MCDE_IRQ     "MCDE IRQ"
#define MCDE_IO_AREA "MCDE I/O Area"

struct mcde_platform_data {
	/* DSI */
	int num_dsilinks;

	/* DPI */
	u8 outmux[5]; /* MCDE_CONF0.OUTMUXx */
	u8 syncmux;   /* MCDE_CONF0.SYNCMUXx */

	const char *regulator_id;
	const char *clock_dsi_id;
	const char *clock_dsi_lp_id;
	const char *clock_mcde_id;
};

int __init mcde_init(void);
void mcde_exit(void);

#endif /* __MCDE__H__ */

