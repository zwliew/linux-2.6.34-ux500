/*----------------------------------------------------------------------------*/
/*  copyright STMicroelectronics, 2007.                                       */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify it    */
/* under the terms of the GNU General Public License as published by the Free */
/* Software Foundation; either version 2.1 of the License, or (at your option)*/
/* any later version.                                                         */
/*                                                                            */
/* This program is distributed in the hope that it will be useful, but        */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY */
/* or FITNESS								      */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more      */
/* details.  								      */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.       */
/*----------------------------------------------------------------------------*/


#ifndef __I2S_TEST_PROT_DRIVER_IF_H__
#define __I2S_TEST_PROT_DRIVER_IF_H__
#include <mach/msp.h>
struct test_prot_config {
	u32 tx_config_desc;
	u32 rx_config_desc;
	u32 frame_freq;
	u32 frame_size;
	u32 data_size;
	u32 direction;
	u32 protocol;
	u32 work_mode;
	struct msp_protocol_desc protocol_desc;
	int (*handler) (void *data, int irq);
	void *tx_callback_data;
	void *rx_callback_data;
	int multichannel_configured;
        t_msp_multichannel_config multichannel_config;
};

int i2s_testprot_drv_open(int i2s_device_num);
int i2s_testprot_drv_configure(int i2s_device_num,
			       struct test_prot_config *config);
int i2s_testprot_drv_transfer(int i2s_device_num, void *txdata, size_t txbytes,
			      void *rxdata, size_t rxbytes);
int i2s_testprot_drv_close(int i2s_device_num);

#endif
