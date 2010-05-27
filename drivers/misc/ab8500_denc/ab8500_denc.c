/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * ST-Ericsson AB8500 DENC base driver
 *
 * Author: Marcel Tunnissen <marcel.tuennissen@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <mach/ab8500.h>

#include "ab8500_denc_regs.h"
#include <video/mcde_display-ab8500.h>
#include <mach/ab8500_denc.h>

#define AB8500_NAME      "ab8500"
#define AB8500_DENC_NAME "ab8500_denc"

#define AB8500_REG_BANK_NR(__reg)	((0xff00 & (__reg)) >> 8)
static inline u8 ab8500_rreg(u32 reg)
{
	return ab8500_read(AB8500_REG_BANK_NR(reg), reg);
}
static inline void ab8500_wreg(u32 reg, u8 val)
{
	ab8500_write(AB8500_REG_BANK_NR(reg), reg, val);
}

#define ab8500_set_fld(__cur_val, __reg, __fld, __val) \
	(((__cur_val) & ~__reg##_##__fld##_MASK) |\
	(((__val) << __reg##_##__fld##_SHIFT) & __reg##_##__fld##_MASK))

#define ab8500_wr_fld(__reg, __fld, __val) \
	ab8500_wreg(__reg, \
		((ab8500_rreg(__reg) & ~__reg##_##__fld##_MASK) |\
		(((__val) << __reg##_##__fld##_SHIFT) &\
						__reg##_##__fld##_MASK)))

#define AB8500_DENC_TRACE	pr_debug("%s\n", __func__)

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_ab8500_denc_dir;
static struct dentry *debugfs_ab8500_dump_regs_file;
static struct dentry *debugfs_ab8500_reg_offs_file;
static struct dentry *debugfs_ab8500_regval_file;
static u32     ab8500_debugfs_reg_nr;
static void ab8500_denc_conf_ddr(void);
static int debugfs_ab8500_open_file(struct inode *inode, struct file *file);
static int debugfs_ab8500_dump_regs(struct file *file, char __user *buf,
						size_t count, loff_t *f_pos);
static ssize_t debugfs_ab8500_write_file_dummy(struct file *file,
						const char __user *ubuf,
						size_t count, loff_t *ppos);
static int debugfs_ab8500_read_reg_offs(struct file *file, char __user *buf,
						size_t count, loff_t *f_pos);
static int debugfs_ab8500_write_reg_offs(struct file *file,
						const char __user *ubuf,
						size_t count, loff_t *ppos);
static int debugfs_ab8500_read_regval(struct file *file, char __user *buf,
						size_t count, loff_t *f_pos);
static ssize_t debugfs_ab8500_write_regval(struct file *file,
						const char __user *ubuf,
						size_t count, loff_t *ppos);

static const struct file_operations debugfs_ab8500_dump_regs_fops = {
	.owner = THIS_MODULE,
	.open  = debugfs_ab8500_open_file,
	.read  = debugfs_ab8500_dump_regs,
	.write  = debugfs_ab8500_write_file_dummy
};

static const struct file_operations debugfs_ab8500_reg_offs_fops = {
	.owner = THIS_MODULE,
	.open  = debugfs_ab8500_open_file,
	.read  = debugfs_ab8500_read_reg_offs,
	.write  = debugfs_ab8500_write_reg_offs
};

static const struct file_operations debugfs_ab8500_regval_fops = {
	.owner = THIS_MODULE,
	.open  = debugfs_ab8500_open_file,
	.read  = debugfs_ab8500_read_regval,
	.write  = debugfs_ab8500_write_regval
};
#endif /* CONFIG_DEBUG_FS */

static void setup_27mhz(bool enable);
static u32 map_tv_std(enum ab8500_denc_TV_std std);
static u32 map_cr_filter(enum ab8500_denc_cr_filter_bandwidth bw);
static u32 map_phase_rst_mode(enum ab8500_denc_phase_reset_mode mode);
static u32 map_plug_time(enum ab8500_denc_plug_time time);

static struct platform_device *ab8500_denc_dev;

static int __devinit ab8500_denc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct ab8500_denc_platform_data *pdata = pdev->dev.platform_data;

	if (pdata == NULL) {
		dev_err(&pdev->dev, "Platform data missing\n");
		ret = -EINVAL;
		goto no_pdata;
	}

	ab8500_denc_dev = pdev;
	AB8500_DENC_TRACE; /* note this needs ab8500_denc_dev to be set */

#ifdef CONFIG_DEBUG_FS
	debugfs_ab8500_denc_dir = debugfs_create_dir(pdev->name, NULL);
	debugfs_ab8500_reg_offs_file = debugfs_create_file(
			"regoffset", S_IWUGO | S_IRUGO,
			debugfs_ab8500_denc_dir, 0,
			&debugfs_ab8500_reg_offs_fops
		);
	debugfs_ab8500_regval_file = debugfs_create_file(
			"regval", S_IWUGO | S_IRUGO,
			debugfs_ab8500_denc_dir, 0,
			&debugfs_ab8500_regval_fops
		);
	debugfs_ab8500_dump_regs_file = debugfs_create_file(
			"dumpregs", S_IRUGO,
			debugfs_ab8500_denc_dir, 0,
			&debugfs_ab8500_dump_regs_fops
		);
#endif /* CONFIG_DEBUG_FS */
out:
	return ret;

no_pdata:
	goto out;
}

static void setup_27mhz(bool enable)
{
	u8 data = ab8500_rreg(AB8500_SYS_ULP_CLK_CONF);

	AB8500_DENC_TRACE;
	/* TODO: check if this field needs to be set */
	data = ab8500_set_fld(data, AB8500_SYS_ULP_CLK_CONF, CLK_27MHZ_PD_ENA,
									true);
	data = ab8500_set_fld(data, AB8500_SYS_ULP_CLK_CONF, CLK_27MHZ_BUF_ENA,
									enable);
	data = ab8500_set_fld(data, AB8500_SYS_ULP_CLK_CONF, TVOUT_CLK_INV,
									false);
	data = ab8500_set_fld(data, AB8500_SYS_ULP_CLK_CONF, TVOUT_CLK_DE_IN,
									false);
	data = ab8500_set_fld(data, AB8500_SYS_ULP_CLK_CONF, CLK_27MHZ_STRE,
									1);
	ab8500_wreg(AB8500_SYS_ULP_CLK_CONF, data);

	data = ab8500_rreg(AB8500_SYS_CLK_CTRL);
	data = ab8500_set_fld(data, AB8500_SYS_CLK_CTRL, TVOUT_CLK_VALID,
									enable);
	data = ab8500_set_fld(data, AB8500_SYS_CLK_CTRL, TVOUT_PLL_ENA,
									enable);
	ab8500_wreg(AB8500_SYS_CLK_CTRL, data);
}

static u32 map_tv_std(enum ab8500_denc_TV_std std)
{
	switch (std) {
	case TV_STD_PAL_BDGHI:
		return AB8500_DENC_CONF0_STD_PAL_BDGHI;
	case TV_STD_PAL_N:
		return AB8500_DENC_CONF0_STD_PAL_N;
	case TV_STD_PAL_M:
		return AB8500_DENC_CONF0_STD_PAL_M;
	case TV_STD_NTSC_M:
		return AB8500_DENC_CONF0_STD_NTSC_M;
	default:
		return 0;
	}
}

static u32 map_cr_filter(enum ab8500_denc_cr_filter_bandwidth bw)
{
	switch (bw) {
	case TV_CR_NTSC_LOW_DEF_FILTER:
		return AB8500_DENC_CONF1_FLT_1_1MHZ;
	case TV_CR_PAL_LOW_DEF_FILTER:
		return AB8500_DENC_CONF1_FLT_1_3MHZ;
	case TV_CR_NTSC_HIGH_DEF_FILTER:
		return AB8500_DENC_CONF1_FLT_1_6MHZ;
	case TV_CR_PAL_HIGH_DEF_FILTER:
		return AB8500_DENC_CONF1_FLT_1_9MHZ;
	default:
		return 0;
	}
}

static u32 map_phase_rst_mode(enum ab8500_denc_phase_reset_mode mode)
{
	switch (mode) {
	case TV_PHASE_RST_MOD_DISABLE:
		return AB8500_DENC_CONF8_PH_RST_MODE_DISABLED;
	case TV_PHASE_RST_MOD_FROM_PHASE_BUF:
		return AB8500_DENC_CONF8_PH_RST_MODE_UPDATE_FROM_PHASE_BUF;
	case TV_PHASE_RST_MOD_FROM_INC_DFS:
		return AB8500_DENC_CONF8_PH_RST_MODE_UPDATE_FROM_INC_DFS;
	case TV_PHASE_RST_MOD_RST:
		return AB8500_DENC_CONF8_PH_RST_MODE_RESET;
	default:
		return 0;
	}
}

static u32 map_plug_time(enum ab8500_denc_plug_time time)
{
	switch (time) {
	case TV_PLUG_TIME_0_5S:
		return AB8500_TVOUT_CTRL_PLUG_TV_TIME_0_5S;
	case TV_PLUG_TIME_1S:
		return AB8500_TVOUT_CTRL_PLUG_TV_TIME_1S;
	case TV_PLUG_TIME_1_5S:
		return AB8500_TVOUT_CTRL_PLUG_TV_TIME_1_5S;
	case TV_PLUG_TIME_2S:
		return AB8500_TVOUT_CTRL_PLUG_TV_TIME_2S;
	case TV_PLUG_TIME_2_5S:
		return AB8500_TVOUT_CTRL_PLUG_TV_TIME_2_5S;
	case TV_PLUG_TIME_3S:
		return AB8500_TVOUT_CTRL_PLUG_TV_TIME_3S;
	default:
		return 0;
	}
}

static int __devexit ab8500_denc_remove(struct platform_device *pdev)
{
	AB8500_DENC_TRACE;
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(debugfs_ab8500_dump_regs_file);
	debugfs_remove(debugfs_ab8500_regval_file);
	debugfs_remove(debugfs_ab8500_reg_offs_file);
	debugfs_remove(debugfs_ab8500_denc_dir);
#endif /* CONFIG_DEBUG_FS */

	ab8500_denc_dev = NULL;

	return 0;
}

static struct platform_driver ab8500_denc_driver = {
	.probe	= ab8500_denc_probe,
	.remove = ab8500_denc_remove,
	.driver = {
		.name	= "ab8500_denc",
	},
};

void ab8500_denc_reset(bool hard)
{
	AB8500_DENC_TRACE;
	if (hard) {
		u8 data = ab8500_rreg(AB8500_CTRL3);
		/* reset start */
		ab8500_wreg(AB8500_CTRL3,
			ab8500_set_fld(data, AB8500_CTRL3, RESET_DENC_N, 0)
		);
		/* reset done */
		ab8500_wreg(AB8500_CTRL3,
			ab8500_set_fld(data, AB8500_CTRL3, RESET_DENC_N, 1)
		);
	} else {
		ab8500_wr_fld(AB8500_DENC_CONF6, SOFT_RESET, 1);
		mdelay(10);
	}
}
EXPORT_SYMBOL(ab8500_denc_reset);

void ab8500_denc_power_up(void)
{
	setup_27mhz(true);
}
EXPORT_SYMBOL(ab8500_denc_power_up);

void ab8500_denc_power_down(void)
{
	setup_27mhz(false);
}
EXPORT_SYMBOL(ab8500_denc_power_down);

void ab8500_denc_regu_setup(bool enable_v_tv, bool enable_lp_mode)
{
	u8 data = ab8500_rreg(AB8500_REGU_MISC1);

	AB8500_DENC_TRACE;
	data = ab8500_set_fld(data, AB8500_REGU_MISC1, V_TVOUT_LP,
								enable_lp_mode);
	data = ab8500_set_fld(data, AB8500_REGU_MISC1, V_TVOUT_ENA,
								enable_v_tv);
	ab8500_wreg(AB8500_REGU_MISC1, data);
}
EXPORT_SYMBOL(ab8500_denc_regu_setup);

void ab8500_denc_conf(struct ab8500_denc_conf *conf)
{
	u8 data;

	AB8500_DENC_TRACE;

	ab8500_wreg(AB8500_DENC_CONF0,
		AB8500_VAL2REG(AB8500_DENC_CONF0, STD, map_tv_std(conf->TV_std))
		|
		AB8500_VAL2REG(AB8500_DENC_CONF0, SYNC,
			conf->test_pattern ? AB8500_DENC_CONF0_SYNC_AUTO_TEST :
					AB8500_DENC_CONF0_SYNC_F_BASED_SLAVE
		)
	);
	ab8500_wreg(AB8500_DENC_CONF1,
		AB8500_VAL2REG(AB8500_DENC_CONF1, BLK_LI,
						!conf->partial_blanking)
		|
		AB8500_VAL2REG(AB8500_DENC_CONF1, FLT,
						map_cr_filter(conf->cr_filter))
		|
		AB8500_VAL2REG(AB8500_DENC_CONF1, CO_KI, conf->suppress_col)
		|
		AB8500_VAL2REG(AB8500_DENC_CONF1, SETUP_MAIN,
						conf->black_level_setup)
		/* TODO: handle cc field: set to 0 now */
	);

	data = ab8500_rreg(AB8500_DENC_CONF2);
	data = ab8500_set_fld(data, AB8500_DENC_CONF2, N_INTRL,
						conf->progressive);
	ab8500_wreg(AB8500_DENC_CONF2, data);

	ab8500_wreg(AB8500_DENC_CONF8,
		AB8500_VAL2REG(AB8500_DENC_CONF8, PH_RST_MODE,
				map_phase_rst_mode(conf->phase_reset_mode))
		|
		AB8500_VAL2REG(AB8500_DENC_CONF8, VAL_422_MUX,
						conf->act_output)
		|
		AB8500_VAL2REG(AB8500_DENC_CONF8, BLK_ALL,
						conf->blank_all)
	);
	data = ab8500_rreg(AB8500_TVOUT_CTRL);
	data = ab8500_set_fld(data, AB8500_TVOUT_CTRL, DAC_CTRL0,
							conf->dac_enable);
	data = ab8500_set_fld(data, AB8500_TVOUT_CTRL, DAC_CTRL1,
							conf->act_dc_output);
	ab8500_wreg(AB8500_TVOUT_CTRL, data);

	/* no support for DDR in early versions */
	if (AB8500_REG2VAL(AB8500_REV, FULL_MASK, ab8500_rreg(AB8500_REV)) > 0)
		ab8500_denc_conf_ddr();
}
EXPORT_SYMBOL(ab8500_denc_conf);

void ab8500_denc_conf_plug_detect(bool enable, bool load_RC,
						enum ab8500_denc_plug_time time)
{
	u8 data = ab8500_rreg(AB8500_TVOUT_CTRL);

	AB8500_DENC_TRACE;
	data = ab8500_set_fld(data, AB8500_TVOUT_CTRL, TV_PLUG_ON,   enable);
	data = ab8500_set_fld(data, AB8500_TVOUT_CTRL, TV_LOAD_RC,   load_RC);
	data = ab8500_set_fld(data, AB8500_TVOUT_CTRL, PLUG_TV_TIME,
							map_plug_time(time));
	ab8500_wreg(AB8500_TVOUT_CTRL, data);
}
EXPORT_SYMBOL(ab8500_denc_conf_plug_detect);

void ab8500_denc_mask_int_plug_det(bool plug, bool unplug)
{
	u8 data = ab8500_rreg(AB8500_IT_MASK1);

	AB8500_DENC_TRACE;
	data = ab8500_set_fld(data, AB8500_IT_MASK1, PLUG_TV_DET,   plug);
	data = ab8500_set_fld(data, AB8500_IT_MASK1, UNPLUG_TV_DET, unplug);
	ab8500_wreg(AB8500_IT_MASK1, data);
}
EXPORT_SYMBOL(ab8500_denc_mask_int_plug_det);

static void ab8500_denc_conf_ddr(void)
{
	struct ab8500_denc_platform_data *pdata;

	AB8500_DENC_TRACE;
	if (ab8500_denc_dev) {
		pdata = (struct ab8500_denc_platform_data *)
					ab8500_denc_dev->dev.platform_data;
		ab8500_wreg(AB8500_TVOUT_CTRL2,
			AB8500_VAL2REG(AB8500_TVOUT_CTRL2,
						DENC_DDR, pdata->ddr_enable)
			|
			AB8500_VAL2REG(AB8500_TVOUT_CTRL2, SWAP_DDR_DATA_IN,
						pdata->ddr_little_endian)
		);
	}
}

#ifdef CONFIG_DEBUG_FS
static int debugfs_ab8500_open_file(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t debugfs_ab8500_write_file_dummy(struct file *file,
						const char __user *ubuf,
						size_t count, loff_t *ppos)
{
	return count;
}

static int debugfs_ab8500_dump_regs(struct file *file, char __user *buf,
						size_t count, loff_t *f_pos)
{
	int ret = 0;
	size_t data_size = 0;
	char   buffer[PAGE_SIZE];

	data_size += sprintf(buffer + data_size,
		"AB8500 DENC registers:\n"
		"CTRL3         : 0x%04x = 0x%02x\n"
		"SYSULPCLK_CONF: 0x%04x = 0x%02x\n"
		"SYSCLK_CTRL   : 0x%04x = 0x%02x\n"
		"REGU_MISC1    : 0x%04x = 0x%02x\n"
		"VAUX12_REGU   : 0x%04x = 0x%02x\n"
		"VAUX1_SEL1    : 0x%04x = 0x%02x\n"
		"DENC_CONF0    : 0x%04x = 0x%02x\n"
		"DENC_CONF1    : 0x%04x = 0x%02x\n"
		"DENC_CONF2    : 0x%04x = 0x%02x\n"
		"DENC_CONF6    : 0x%04x = 0x%02x\n"
		"DENC_CONF8    : 0x%04x = 0x%02x\n"
		"TVOUT_CTRL    : 0x%04x = 0x%02x\n"
		"TVOUT_CTRL2   : 0x%04x = 0x%02x\n"
		"IT_MASK1      : 0x%04x = 0x%02x\n"
		,
		AB8500_CTRL3,            ab8500_rreg(AB8500_CTRL3),
		AB8500_SYS_ULP_CLK_CONF, ab8500_rreg(AB8500_SYS_ULP_CLK_CONF),
		AB8500_SYS_CLK_CTRL,     ab8500_rreg(AB8500_SYS_CLK_CTRL),
		AB8500_REGU_MISC1,       ab8500_rreg(AB8500_REGU_MISC1),
		AB8500_VAUX12_REGU,      ab8500_rreg(AB8500_VAUX12_REGU),
		AB8500_VAUX1_SEL,        ab8500_rreg(AB8500_VAUX1_SEL),
		AB8500_DENC_CONF0,       ab8500_rreg(AB8500_DENC_CONF0),
		AB8500_DENC_CONF1,       ab8500_rreg(AB8500_DENC_CONF1),
		AB8500_DENC_CONF2,       ab8500_rreg(AB8500_DENC_CONF2),
		AB8500_DENC_CONF6,       ab8500_rreg(AB8500_DENC_CONF6),
		AB8500_DENC_CONF8,       ab8500_rreg(AB8500_DENC_CONF8),
		AB8500_TVOUT_CTRL,       ab8500_rreg(AB8500_TVOUT_CTRL),
		AB8500_TVOUT_CTRL2,      ab8500_rreg(AB8500_TVOUT_CTRL2),
		AB8500_IT_MASK1,         ab8500_rreg(AB8500_IT_MASK1)
	);
	if (data_size > PAGE_SIZE) {
		printk(KERN_EMERG "AB8500 DENC: Buffer overrun\n");
		ret = -EINVAL;
		goto out;
	}

	/* check if read done */
	if (*f_pos > data_size)
		goto out;

	if (*f_pos + count > data_size)
		count = data_size - *f_pos;

	if (copy_to_user(buf, buffer + *f_pos, count))
		ret = -EINVAL;
	*f_pos += count;
	ret = count;
out:
	return ret;
}

static int debugfs_ab8500_read_reg_offs(struct file *file, char __user *buf,
						size_t count, loff_t *f_pos)
{
	int    ret = 0;
	size_t data_size = 0;
	char   buffer[PAGE_SIZE];

	data_size = sprintf(buffer, "0x%08x\n", ab8500_debugfs_reg_nr);

	if (data_size > PAGE_SIZE) {
		printk(KERN_EMERG "AB8500 DENC: Buffer overrun\n");
		ret = -EINVAL;
		goto out;
	}

	/* check if read done */
	if (*f_pos > data_size)
		goto out;

	if (*f_pos + count > data_size)
		count = data_size - *f_pos;

	if (copy_to_user(buf, buffer + *f_pos, count))
		ret = -EINVAL;
	*f_pos += count;
	ret = count;

out:
	return ret;
}

static int debugfs_ab8500_write_reg_offs(struct file *file,
						const char __user *ubuf,
						size_t count, loff_t *ppos)
{
	int    buf_size;
	u32    i;
	char   buf[16] = {0};

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, ubuf, buf_size))
		goto error;

	if (sscanf(buf, "0x%x", &i) <= 0)
		goto error;

	ab8500_debugfs_reg_nr = i;

	return count;
error:
	return -EINVAL;
}

static int debugfs_ab8500_read_regval(struct file *file, char __user *buf,
						size_t count, loff_t *f_pos)
{
	int    ret = 0;
	size_t data_size = 0;
	char   buffer[PAGE_SIZE];
	u8     bank = 0;

	bank = 0xF & (ab8500_debugfs_reg_nr >> 8);
	data_size = sprintf(buffer, "0x%02x\n",
				ab8500_read(bank, ab8500_debugfs_reg_nr));

	if (data_size > PAGE_SIZE) {
		printk(KERN_EMERG "AB8500 DENC: Buffer overrun\n");
		ret = -EINVAL;
		goto out;
	}

	/* check if read done */
	if (*f_pos > data_size)
		goto out;

	if (*f_pos + count > data_size)
		count = data_size - *f_pos;

	if (copy_to_user(buf, buffer + *f_pos, count))
		ret = -EINVAL;
	*f_pos += count;
	ret = count;

out:
	return ret;
}

static ssize_t debugfs_ab8500_write_regval(struct file *file,
						const char __user *ubuf,
						size_t count, loff_t *ppos)
{
	int    buf_size;
	char   buf[16] = {0};
	u8     r = 0x0;
	u8     bank = 0;

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, ubuf, buf_size))
		goto error;

	if (sscanf(buf, "0x%hhx", &r) <= 0)
		goto error;

	bank = 0xF & (ab8500_debugfs_reg_nr >> 8);

	printk(KERN_DEBUG
		"AB8500: write to bank 0x%02x, address: 0x%08x: 0x%02x\n",
		bank,
		ab8500_debugfs_reg_nr,
		r);
	printk(KERN_DEBUG "; old value 0x%02x\n",
				ab8500_read(bank, ab8500_debugfs_reg_nr));

	ab8500_write(bank, ab8500_debugfs_reg_nr, r);

	printk(KERN_DEBUG "AB8500: write reg: new value 0x%02x\n",
				ab8500_read(bank, ab8500_debugfs_reg_nr));

	return count;
error:
	return -EINVAL;
}

#endif /* CONFIG_DEBUG_FS */

/* Module init */
static int __init ab8500_denc_init(void)
{
	return platform_driver_register(&ab8500_denc_driver);
}
module_init(ab8500_denc_init);

static void __exit ab8500_denc_exit(void)
{
	platform_driver_unregister(&ab8500_denc_driver);
}
module_exit(ab8500_denc_exit);

MODULE_AUTHOR("Marcel Tunnissen <marcel.tuennissen@stericsson.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ST-Ericsson AB8500 DENC driver");
