/*
 * i2s_test_protocol_driver.c -- simple test protocol driver to I2S devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>

#include <linux/i2s/i2s.h>
#include <linux/i2s/i2s_test_prot.h>
#include <mach/msp.h>

#include <asm/uaccess.h>

int i2s_drv_offset = 1;
module_param(i2s_drv_offset, int, 1);
MODULE_PARM_DESC(i2s_drv_offset, "i2s driver to be opened)=(0/1/2/3/4/5)");
#define MAX_I2S_CLIENTS 6
struct i2sdrv_data {
	spinlock_t		i2s_lock;
	struct i2s_device	*i2s;
	int 			flag;
	u32 tx_status;
	u32 rx_status;
};

static struct i2sdrv_data *i2sdrv[MAX_I2S_CLIENTS];

/*API Interface */
int i2s_testprot_drv_open(int i2s_device_num)
{

	if(!i2sdrv[i2s_device_num])
		return -1;
	spin_lock_irq(&i2sdrv[i2s_device_num]->i2s_lock);

	if(!i2sdrv[i2s_device_num]->flag)
	{
		spin_unlock_irq(&i2sdrv[i2s_device_num]->i2s_lock);
		return -1;
	}
	i2sdrv[i2s_device_num]->flag = 0;
	spin_unlock_irq(&i2sdrv[i2s_device_num]->i2s_lock);
        return 0;
}
EXPORT_SYMBOL(i2s_testprot_drv_open);

int i2s_testprot_drv_configure(int i2s_device_num,struct test_prot_config *config)
{
	int error_status = 0;
	struct i2s_device *i2s_dev;
	struct msp_generic_config msp_config =
        {
                .tx_clock_sel = TX_CLK_SEL_SRG,
                .rx_clock_sel = 0x0,
                .tx_frame_sync_sel = TX_SYNC_SRG_AUTO,
                .rx_frame_sync_sel = 0x0,
                .input_clock_freq = MSP_INPUT_FREQ_48MHZ,
                .srg_clock_sel = SRG_CLK_SEL_APB,
                .rx_frame_sync_pol = RX_FIFO_SYNC_HI,
                .tx_frame_sync_pol = TX_FIFO_SYNC_HI,
                .rx_fifo_config = RX_FIFO_ENABLE,
                .tx_fifo_config = TX_FIFO_ENABLE,
                .spi_clk_mode = SPI_CLK_MODE_NORMAL,
		.tx_data_enable = 0x8000,
                .spi_burst_mode = 0,
                .loopback_enable = 0x80,
                .default_protocol_desc = 0,
        };
	if(!i2sdrv[i2s_device_num])
		return -1;
	i2s_dev = i2sdrv[i2s_device_num]->i2s;
	if(i2sdrv[i2s_device_num]->flag)
	{
		return -1;
	}
	if(!config)
	{
		return -1;
	}
	msp_config.handler = config->handler;
	msp_config.tx_callback_data =  config->tx_callback_data;
	msp_config.rx_callback_data =  config->rx_callback_data;
        msp_config.frame_freq = config->frame_freq;
        msp_config.frame_size = config->frame_size;
        msp_config.data_size = config->data_size;
        msp_config.direction = config->direction;
        msp_config.protocol = config->protocol;
        msp_config.work_mode = config->work_mode;
        msp_config.def_elem_len = 0;
	msp_config.multichannel_configured = 0;
	msp_config.protocol_desc = config->protocol_desc;

	msp_config.multichannel_configured = config->multichannel_configured;
	msp_config.multichannel_config.tx_multichannel_enable = config->multichannel_config.tx_multichannel_enable;
	msp_config.multichannel_config.tx_channel_0_enable = config->multichannel_config.tx_channel_0_enable; //Channel 1 to 32
	msp_config.multichannel_config.tx_channel_1_enable = config->multichannel_config.tx_channel_1_enable; //Channel 33 to 64
	msp_config.multichannel_config.tx_channel_2_enable = config->multichannel_config.tx_channel_2_enable; //Channel 65 to 96
	msp_config.multichannel_config.tx_channel_3_enable = config->multichannel_config.tx_channel_3_enable; //Channel 97 to 128
	msp_config.multichannel_config.rx_multichannel_enable = config->multichannel_config.rx_multichannel_enable;
	msp_config.multichannel_config.rx_channel_0_enable = config->multichannel_config.rx_channel_0_enable; //Channel 1 to 32
	msp_config.multichannel_config.rx_channel_1_enable = config->multichannel_config.rx_channel_1_enable; //Channel 33 to 64
	msp_config.multichannel_config.rx_channel_2_enable = config->multichannel_config.rx_channel_2_enable; //Channel 65 to 96
	msp_config.multichannel_config.rx_channel_3_enable = config->multichannel_config.rx_channel_3_enable; //Channel 97 to 128
	msp_config.multichannel_config.rx_comparison_enable_mode = config->multichannel_config.rx_comparison_enable_mode;
	msp_config.multichannel_config.comparison_value = config->multichannel_config.comparison_value;
	msp_config.multichannel_config.comparison_mask = config->multichannel_config.comparison_mask;


	//memcpy(&config->protocol_desc,&msp_config.protocol_desc,sizeof(struct msp_protocol_desc));
	error_status =
            i2s_setup(i2s_dev->controller, &msp_config);
	if (error_status < 0) {
                printk(KERN_ERR "error in msp enable, error_status is %d\n",
                           error_status);
                return error_status;
        }
return error_status;

}
EXPORT_SYMBOL(i2s_testprot_drv_configure);

int i2s_testprot_drv_transfer(int i2s_device_num,void *txdata, size_t txbytes, void *rxdata, size_t rxbytes)
{
	int bytes_transreceive = 0;
	struct i2s_device *i2s_dev;
	struct i2s_message message;
	if(!i2sdrv[i2s_device_num])
		return -1;
	i2s_dev = i2sdrv[i2s_device_num]->i2s;
	if(i2sdrv[i2s_device_num]->flag)
	{
		printk(KERN_ERR "msp device not get\n");
		return -1;
	}
	message.txbytes = txbytes;
        message.txdata = txdata;
        message.rxbytes = rxbytes;
        message.rxdata = rxdata;
        message.dma_flag = 1;
        bytes_transreceive = i2s_transfer(i2s_dev->controller,&message);
	if(bytes_transreceive < 0)
	{
		printk(KERN_ERR "bytes transferred%d\n",bytes_transreceive);
		return -1;
	}
	return bytes_transreceive;
}
EXPORT_SYMBOL(i2s_testprot_drv_transfer);

int i2s_testprot_drv_close(int i2s_device_num)
{
	int status = 0;
	struct i2s_device *i2s_dev;
	if(!i2sdrv[i2s_device_num])
		return -1;
	i2s_dev = i2sdrv[i2s_device_num]->i2s;
	if(i2sdrv[i2s_device_num]->flag)
	{
		return -1;
	}
	status = i2s_cleanup(i2s_dev->controller,DISABLE_ALL);
	if(status)
	{
		return -1;
	}
	i2sdrv[i2s_device_num]->flag = -1;
	return 0;
}
EXPORT_SYMBOL(i2s_testprot_drv_close);


/*-------------------------------------------------------------------------*/

static int i2sdrv_probe(struct i2s_device *i2s)
{
	int			status = 0;


	/* Allocate driver data */
	try_module_get(i2s->controller->dev.parent->driver->owner);
	i2sdrv[i2s->chip_select] = kzalloc(sizeof(*i2sdrv[i2s->chip_select]), GFP_KERNEL);
	if (!i2sdrv[i2s->chip_select])
		return -ENOMEM;

	/* Initialize the driver data */
	i2sdrv[i2s->chip_select]->i2s = i2s;
	i2sdrv[i2s->chip_select]->flag = -1;
	i2sdrv[i2s->chip_select]->tx_status = 0;
	i2sdrv[i2s->chip_select]->rx_status = 0;
	spin_lock_init(&i2sdrv[i2s->chip_select]->i2s_lock);

	i2s_set_drvdata(i2s, (void*)i2sdrv[i2s->chip_select]);
	return status;
}

static int i2sdrv_remove(struct i2s_device *i2s)
{
	spin_lock_irq(&i2sdrv[i2s->chip_select]->i2s_lock);
	i2sdrv[i2s->chip_select]->i2s = NULL;
	i2s_set_drvdata(i2s, NULL);
	module_put(i2s->controller->dev.parent->driver->owner);
	spin_unlock_irq(&i2sdrv[i2s->chip_select]->i2s_lock);

	kfree(i2sdrv[i2s->chip_select]);

	return 0;
}
static const struct i2s_device_id i2s_test_prot_id_table[] = {
#ifndef CONFIG_ARCH_U8500
	        { "i2s_device.2", 0, 0 },
	        { "i2s_device.1", 0, 0 },
#else
	        { "i2s_device.0", 0, 0 },
	        { "i2s_device.1", 0, 0 },
#endif
		        { },
};
MODULE_DEVICE_TABLE(i2s, i2s_test_prot_id_table);


static struct i2s_driver i2sdrv_i2s = {
	.driver = {
		.name =		"i2s_test_protocol_driver",
		.owner =	THIS_MODULE,
	},
	.probe =	i2sdrv_probe,
	.remove =	__devexit_p(i2sdrv_remove),
	.id_table = i2s_test_prot_id_table,

	/* NOTE:  suspend/resume methods are not necessary here.
	 */
};

/*-------------------------------------------------------------------------*/

static int __init i2sdrv_init(void)
{
	int status;
	status = i2s_register_driver(&i2sdrv_i2s);
	if (status < 0) {
		printk(KERN_ERR "Unable to register i2s driver\n");
	}
	return status;
}
module_init(i2sdrv_init);

static void __exit i2sdrv_exit(void)
{
	i2s_unregister_driver(&i2sdrv_i2s);
}
module_exit(i2sdrv_exit);

MODULE_AUTHOR("Sandeep Kaushik, <sandeep-mmc.kaushik@st.com>");
MODULE_DESCRIPTION("Test Driver module I2S device interface");
MODULE_LICENSE("GPL");
