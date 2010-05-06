/*----------------------------------------------------------------------------*/
/* Copyright STMicroelectronics, 2008.                                        */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify it    */
/* under the terms of the GNU General Public License as published by the Free */
/* Software Foundation; either version 2.1 of the License, or (at your option)*/
/* any later version.                                                         */
/*                                                                            */
/* This program is distributed in the hope that it will be useful, but        */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY */
/* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public            */
/* License for more details.                                                  */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.       */
/*----------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <asm/page.h>
#include <asm/fiq.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/memory.h>
#include <mach/debug.h>

#include <mach/dma.h>

#ifndef CONFIG_U8500_SECURE
/* Define this macro if in non-secure mode */
#define CONFIG_STM_SECURITY
#endif

#define DRIVER_NAME            "dma40"

/* enables/disables debug msgs */
#define DRIVER_DEBUG            0
#define DRIVER_DEBUG_PFX        DRIVER_NAME
#define DRIVER_DBG              KERN_ERR

#if DRIVER_DEBUG
struct driver_debug_st DBG_ST = { .dma = 1 };
#endif

#define MAX_BACKUP_REGS 24

struct dma_addr {
	dma_addr_t phys_addr;
	void *log_addr;
};

/**
 * struct elem_pool - Structure of element pool
 * @name:	Name of the Element pool
 * @allocation_map: Whether  a pool element is allocated or not
 * @lchan_allocation_map: Pool allocation map for Logical channels,
 *			  (for each phy res)
 * @num_blocks:	Number of elements in the pool
 * @block_size:	Size of each element in the pool
 * @allocation_size: Total memory allocated for the pool
 * @unalign_base: Unaligned Base address of pool
 * @base_addr: Aligned base address of pool
 *
 *
 **/
struct elem_pool {
	char name[20];
	spinlock_t pool_lock;
	u32 allocation_map;
	u32 lchan_allocation_map[MAX_PHYSICAL_CHANNELS];
	int num_blocks;
	int block_size;
	int allocation_size;
	struct dma_addr unalign_base;
	struct dma_addr base_addr;
};

/**
 * struct dma_driver_data - DMA driver private data structure
 * @reg_base: pointer to DMA controller Register area
 * @dma_chan_info: Array of pointer to channel info structure.
 *	It is used by DMA driver internally to access information
 *	of each channel (Physical or Logical). It is accessed in
 *	the interrupt handler also.
 * @pipe_info: Array of pointers to channel info structure, used
 *	access channel info structure when client makes a request.
 *	pipe_id used by client in all requests is an entry
 *	in this array.
 * @pipe_id_map: Map from where we allocate ID's to client drivers.
 *	If a particular bit is set in this map, it means that
 *	particular id can be given to a client.
 * @pipe_id_lock: Mutex to provide access to pipe_id_map
 * @pr_info: stores current status of all physical resources
 *	whether physical channel or Logical channel.
 *	If Logical then count of how many channels are there
 * @pr_info_lock: Lock to synchronise access to pr_info
 * @cfg_lock: Lock to synchronize access to write on DMA registers
 * @lchan_params_base: Base address where logical channel params are
 *	stored.
 * @pchan_lli_pool: Pool from where LLI's are allocated for phy channels
 * @sg_pool: Pool from where SG blocks are allocated to keep SG info
 * @lchan_lli_pool: Pool from where LLI's are allocated for Log channels
 * @backup_regs: Used to store and restore DMA regs during suspend resume.
 *
 **/

struct dma_driver_data {
	void __iomem *reg_base;
	struct clk *clk;
	struct dma_channel_info *dma_chan_info[NUM_CHANNELS];
	struct dma_channel_info *pipe_info[NUM_CHANNELS];
	 DECLARE_BITMAP(pipe_id_map, NUM_CHANNELS);
	spinlock_t pipe_id_lock;
	struct phy_res_info pr_info[MAX_AVAIL_PHY_CHANNELS];
	spinlock_t pr_info_lock;
	struct dma_addr lchan_params_base;
	struct elem_pool *pchan_lli_pool;
	struct elem_pool *sg_pool;
	struct elem_pool *lchan_lli_pool;
	spinlock_t cfg_ch_lock[MAX_AVAIL_PHY_CHANNELS];
	u32 backup_regs[MAX_BACKUP_REGS];
};
static struct dma_driver_data *dma_drv_data;

static inline void __iomem *io_addr(unsigned int offset)
{
	return dma_drv_data->reg_base + offset;
}

static inline void __iomem *cio_addr(unsigned int phys_chan_id,
				     unsigned int offset)
{
	return dma_drv_data->reg_base + 0x400 + (phys_chan_id * (8 * 4)) +
	    offset;
}

/**
 *  print_sg_info() - To Print info of the scatter gather list items
 *  @sg:	Pointer to sg List which you want to print
 *  @nents:	Number of elements in the list
 *
 **/
static void print_sg_info(struct scatterlist *sg, int nents)
{
	int i = 0;
	stm_dbg(DBG_ST.dma, "Scatter Gather info........\n");
	for (i = 0; i < nents; i++) {
		stm_dbg(DBG_ST.dma, "SG[%d] , dma_addr = %x\n", i,
			sg[i].dma_address);
		stm_dbg(DBG_ST.dma, "SG[%d] , length = %d\n", i, sg[i].length);
		stm_dbg(DBG_ST.dma, "SG[%d] , offset = %d\n", i, sg[i].offset);
	}
}

/**
 *  print_dma_lli() - To Print the Information that we stored in LLI Struct
 *  @idx:	LLI number whose info we are printing
 *  @lli:	LLI structure whose value would be printed
 *
 **/
static void print_dma_lli(int idx, struct dma_lli_info *lli)
{
	stm_dbg(DBG_ST.dma, "---------------------------------\n");
	stm_dbg(DBG_ST.dma, "##### %x\n", (u32) lli);
	stm_dbg(DBG_ST.dma, "LLI[%d]:: Link Register\t %x\n", idx,
		lli->reg_lnk);
	stm_dbg(DBG_ST.dma, "LLI[%d]:: Pointer Register\t %x\n", idx,
		lli->reg_ptr);
	stm_dbg(DBG_ST.dma, "LLI[%d]:: Element Register\t %x\n", idx,
		lli->reg_elt);
	stm_dbg(DBG_ST.dma, "LLI[%d]:: Config Register\t %x\n", idx,
		lli->reg_cfg);
	stm_dbg(DBG_ST.dma, "---------------------------------\n");
}

/**
 *  print_allocation_map() - To print Allocation map of Pipe Ids given to client
 *
 */
static void print_allocation_map(void)
{
	stm_dbg(DBG_ST.dma,
		"==================================================\n");
	stm_dbg(DBG_ST.dma, "=====pipe_id_map(0) = %x\n",
		(u32) dma_drv_data->pipe_id_map[0]);
	stm_dbg(DBG_ST.dma, "=====pipe_id_map(1) = %x\n",
		(u32) dma_drv_data->pipe_id_map[1]);
	stm_dbg(DBG_ST.dma, "=====pipe_id_map(2) = %x\n",
		(u32) dma_drv_data->pipe_id_map[2]);
	stm_dbg(DBG_ST.dma, "=====pipe_id_map(3) = %x\n",
		(u32) dma_drv_data->pipe_id_map[3]);
	stm_dbg(DBG_ST.dma, "=====pipe_id_map(4) = %x\n",
		(u32) dma_drv_data->pipe_id_map[4]);
	stm_dbg(DBG_ST.dma,
		"==================================================\n");
}

/**
 *  initialize_dma_regs() - To Initialize the DMA global registers
 *
 */
static inline void initialize_dma_regs(void)
{
	/*
	 * Some registers can only be touched in secure mode.  Normally, we
	 * will not modify these registers, as xloader would initialize these
	 * registers for us.
	 *
	 * It appears that we are allowed a write if we don't change the value
	 * of the register.
	 */

#ifndef CONFIG_STM_SECURITY
	REG_WR_BITS(io_addr(DREG_GCC), 0xff01, FULL32_MASK, NO_SHIFT);
#else
	REG_WR_BITS(io_addr(DREG_GCC), 0x0, FULL32_MASK, NO_SHIFT);
#endif

	/*All resources are standard */
	REG_WR_BITS(io_addr(DREG_PRTYP), 0x55555555, FULL32_MASK, NO_SHIFT);
	/*All in non secure mode */
	REG_WR_BITS(io_addr(DREG_PRSME), 0xaaaaaaaa, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_PRSMO), 0xaaaaaaaa, FULL32_MASK, NO_SHIFT);
	/*Basic mode */
	REG_WR_BITS(io_addr(DREG_PRMSE), 0x55555555, FULL32_MASK, NO_SHIFT);
	/*Basic mode */
	REG_WR_BITS(io_addr(DREG_PRMSO), 0x55555555, FULL32_MASK, NO_SHIFT);
	/*Basic mode */
	REG_WR_BITS(io_addr(DREG_PRMOE), 0x55555555, FULL32_MASK, NO_SHIFT);
	/*Basic mode */
	REG_WR_BITS(io_addr(DREG_PRMOO), 0x55555555, FULL32_MASK, NO_SHIFT);
	/***************************************************************/
	/*Currently dont modify as we are only using basic mode and not logical */
	REG_WR_BITS(io_addr(DREG_LCPA), 0x0, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCLA), 0x0, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_SLCPA), 0x0, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_SLCLA), 0x0, FULL32_MASK, NO_SHIFT);
	/***************************************************************/
	/*Registers to activate channels */
	REG_WR_BITS(io_addr(DREG_ACTIVE), 0x0, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_ACTIVO), 0x0, FULL32_MASK, NO_SHIFT);
	/***************************************************************/
	REG_WR_BITS(io_addr(DREG_FSEB1), 0x0, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_FSEB2), 0x0, FULL32_MASK, NO_SHIFT);
	/***************************************************************/
	/*Allow all interrupts */
	REG_WR_BITS(io_addr(DREG_PCMIS), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	/*Clear all interrupts */
	REG_WR_BITS(io_addr(DREG_PCICR), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);

	REG_WR_BITS(io_addr(DREG_SPCMIS), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_SPCICR), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);

	REG_WR_BITS(io_addr(DREG_LCMIS(0)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCMIS(1)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCMIS(2)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCMIS(3)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);

	REG_WR_BITS(io_addr(DREG_LCICR(0)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCICR(1)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCICR(2)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCICR(3)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);

	REG_WR_BITS(io_addr(DREG_LCTIS(0)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCTIS(1)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCTIS(2)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(io_addr(DREG_LCTIS(3)), 0xFFFFFFFF, FULL32_MASK, NO_SHIFT);
}

/**
 * print_channel_reg_params() - To print Channel registers of the physical
 * channel for debug
 * @channel:	Physical channel whose register values to be printed
 *
 */
static inline void print_channel_reg_params(int channel)
{
	if ((channel < 0) || (channel > MAX_AVAIL_PHY_CHANNELS)) {
		stm_dbg(DBG_ST.dma, "Invalid channel number....");
		return;
	}
	stm_dbg(DBG_ST.dma,
		"\n===================================================\n");
	stm_dbg(DBG_ST.dma, "\t\tChannel params %d\n", channel);
	stm_dbg(DBG_ST.dma,
		"\n===================================================\n");
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sscfg_excfg)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SSCFG)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sselt_exelt)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SSELT)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_ssptr_exptr)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SSPTR)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sslnk_exlnk)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SSLNK)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sdcfg_exexc)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SDCFG)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sdelt_exfrm)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SDELT)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sdptr_exrld)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SDPTR)));
	stm_dbg(DBG_ST.dma, "CHANNEL_params_regs (CHAN_REG_sdlnk_exblk)=%x\n",
		ioread32(cio_addr(channel, CHAN_REG_SDLNK)));
	stm_dbg(DBG_ST.dma,
		"\n=================================================\n");
}

/**
 * print_dma_regs() - To print DMA global registers
 *
 */
static void print_dma_regs(void)
{
	stm_dbg(DBG_ST.dma,
		"\n=====================================================\n");
	stm_dbg(DBG_ST.dma, "\t\tGLOBAL DMA REGISTERS");
	stm_dbg(DBG_ST.dma,
		"\n=====================================================\n");
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_gcc)=%x\n",
		ioread32(io_addr(DREG_GCC)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prtyp)=%x\n",
		ioread32(io_addr(DREG_PRTYP)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prsme)=%x\n",
		ioread32(io_addr(DREG_PRSME)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prsmo)=%x\n",
		ioread32(io_addr(DREG_PRSMO)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prmse)=%x\n",
		ioread32(io_addr(DREG_PRMSE)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prmso)=%x\n",
		ioread32(io_addr(DREG_PRMSO)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prmoe)=%x\n",
		ioread32(io_addr(DREG_PRMOE)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_prmoo)=%x\n",
		ioread32(io_addr(DREG_PRMOO)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcpa)=%x\n",
		ioread32(io_addr(DREG_LCPA)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcla)=%x\n",
		ioread32(io_addr(DREG_LCLA)));
#ifndef CONFIG_STM_SECURITY
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcpa)=%x\n",
		ioread32(io_addr(DREG_SLCPA)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcla)	=%x\n",
		ioread32(io_addr(DREG_SLCLA)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sseg[0])=%x\n",
		ioread32(io_addr(DREG_SSEG(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sseg[1])=%x\n",
		ioread32(io_addr(DREG_SSEG(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sseg[2])=%x\n",
		ioread32(io_addr(DREG_SSEG(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sseg[3])=%x\n",
		ioread32(io_addr(DREG_SSEG(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sceg[0])=%x\n",
		ioread32(io_addr(DREG_SCEG(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sceg[1])=%x\n",
		ioread32(io_addr(DREG_SCEG(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sceg[2])=%x\n",
		ioread32(io_addr(DREG_SCEG(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_sceg[3])=%x\n",
		ioread32(io_addr(DREG_SCEG(3))));
#endif
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_active)=%x\n",
		ioread32(io_addr(DREG_ACTIVE)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_activo)=%x\n",
		ioread32(io_addr(DREG_ACTIVO)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_fsebs1)=%x\n",
		ioread32(io_addr(DREG_FSEB1)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_fsebs2)=%x\n",
		ioread32(io_addr(DREG_FSEB2)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_pcmis)=%x\n",
		ioread32(io_addr(DREG_PCMIS)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_pcicr)=%x\n",
		ioread32(io_addr(DREG_PCICR)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_pctis)=%x\n",
		ioread32(io_addr(DREG_PCTIS)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_pceis)=%x\n",
		ioread32(io_addr(DREG_PCEIS)));
#ifndef CONFIG_STM_SECURITY
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_spcmis)=%x\n",
		ioread32(io_addr(DREG_SPCMIS)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_spcicr)=%x\n",
		ioread32(io_addr(DREG_SPCICR)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_spctis)=%x\n",
		ioread32(io_addr(DREG_SPCTIS)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_spceis)=%x\n",
		ioread32(io_addr(DREG_SPCEIS)));
#endif
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcmis[0])=%x\n",
		ioread32(io_addr(DREG_LCMIS(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcmis[1])=%x\n",
		ioread32(io_addr(DREG_LCMIS(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcmis[2])=%x\n",
		ioread32(io_addr(DREG_LCMIS(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcmis[3])=%x\n",
		ioread32(io_addr(DREG_LCMIS(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcicr[0])=%x\n",
		ioread32(io_addr(DREG_LCICR(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcicr[1])=%x\n",
		ioread32(io_addr(DREG_LCICR(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcicr[2])=%x\n",
		ioread32(io_addr(DREG_LCICR(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lcicr[3])=%x\n",
		ioread32(io_addr(DREG_LCICR(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lctis[0])=%x\n",
		ioread32(io_addr(DREG_LCTIS(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lctis[1])=%x\n",
		ioread32(io_addr(DREG_LCTIS(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lctis[2])=%x\n",
		ioread32(io_addr(DREG_LCTIS(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lctis[3])=%x\n",
		ioread32(io_addr(DREG_LCTIS(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lceis[0])=%x\n",
		ioread32(io_addr(DREG_LCEIS(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lceis[1])=%x\n",
		ioread32(io_addr(DREG_LCEIS(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lceis[2])=%x\n",
		ioread32(io_addr(DREG_LCEIS(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_lceis[3])=%x\n",
		ioread32(io_addr(DREG_LCEIS(3))));
#ifndef CONFIG_STM_SECURITY
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcmis[0])=%x\n",
		ioread32(io_addr(DREG_SLCMIS(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcmis[1])=%x\n",
		ioread32(io_addr(DREG_SLCMIS(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcmis[2])=%x\n",
		ioread32(io_addr(DREG_SLCMIS(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcmis[3])=%x\n",
		ioread32(io_addr(DREG_SLCMIS(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcicr[0])=%x\n",
		ioread32(io_addr(DREG_SLCICR(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcicr[1])=%x\n",
		ioread32(io_addr(DREG_SLCICR(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcicr[2])=%x\n",
		ioread32(io_addr(DREG_SLCICR(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slcicr[3])=%x\n",
		ioread32(io_addr(DREG_SLCICR(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slctis[0])=%x\n",
		ioread32(io_addr(DREG_SLCTIS(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slctis[1])=%x\n",
		ioread32(io_addr(DREG_SLCTIS(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slctis[2])=%x\n",
		ioread32(io_addr(DREG_SLCTIS(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slctis[3])=%x\n",
		ioread32(io_addr(DREG_SLCTIS(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slceis[0])=%x\n",
		ioread32(io_addr(DREG_SLCEIS(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slceis[1])=%x\n",
		ioread32(io_addr(DREG_SLCEIS(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slceis[2])=%x\n",
		ioread32(io_addr(DREG_SLCEIS(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_slceis[3])=%x\n",
		ioread32(io_addr(DREG_SLCEIS(3))));
#endif
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_stfu)=%x\n",
		ioread32(io_addr(DREG_STFU)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_icfg)=%x\n",
		ioread32(io_addr(DREG_ICFG)));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_mplug[0])=%x\n",
		ioread32(io_addr(DREG_MPLUG(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_mplug[1])=%x\n",
		ioread32(io_addr(DREG_MPLUG(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_mplug[2])=%x\n",
		ioread32(io_addr(DREG_MPLUG(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dmac_mplug[3])=%x\n",
		ioread32(io_addr(DREG_MPLUG(3))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_periphid0)=%x\n",
		ioread32(io_addr(DREG_PERIPHID(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_periphid1)=%x\n",
		ioread32(io_addr(DREG_PERIPHID(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_periphid2)=%x\n",
		ioread32(io_addr(DREG_PERIPHID(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_periphid3)=%x\n",
		ioread32(io_addr(DREG_PERIPHID(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_cellid0)=%x\n",
		ioread32(io_addr(DREG_CELLID(0))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_cellid1)=%x\n",
		ioread32(io_addr(DREG_CELLID(1))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_cellid2)=%x\n",
		ioread32(io_addr(DREG_CELLID(2))));
	stm_dbg(DBG_ST.dma, "DMA_REGS (dma_cellid3)=%x\n",
		ioread32(io_addr(DREG_CELLID(3))));
	stm_dbg(DBG_ST.dma,
		"\n===================================================\n");
}

/**
 * print_dma_channel_info() - To print DMA channel info that is maintained by
 * DMA driver internally
 * @info:	Info structure where information is maintained
 *
 */
static void print_dma_channel_info(struct dma_channel_info *info)
{
	stm_dbg(DBG_ST.dma, " info->phys_chan_id = %d \n", info->phys_chan_id);
	stm_dbg(DBG_ST.dma, " info->dir = %d \n", info->dir);
	stm_dbg(DBG_ST.dma, " info->security = %d \n", info->security);
	stm_dbg(DBG_ST.dma, " info->ch_status = %d \n", info->ch_status);
	stm_dbg(DBG_ST.dma, " info->src_info.endianess  = %d \n",
		info->src_info.endianess);
	stm_dbg(DBG_ST.dma, " info->src_info.data_width  = %d \n",
		info->src_info.data_width);
	stm_dbg(DBG_ST.dma, " info->src_info.burst_size  = %d \n",
		info->src_info.burst_size);
	stm_dbg(DBG_ST.dma, " info->src_info.buffer_type  = %d \n",
		info->src_info.buffer_type);
	if (info->src_addr)
		stm_dbg(DBG_ST.dma, " info->src_addr  = %x \n",
			(u32) info->src_addr);
	stm_dbg(DBG_ST.dma, " info->dst_info.endianess  = %d \n",
		info->dst_info.endianess);
	stm_dbg(DBG_ST.dma, " info->dst_info.data_width  = %d \n",
		info->dst_info.data_width);
	stm_dbg(DBG_ST.dma, " info->dst_info.burst_size  = %d \n",
		info->dst_info.burst_size);
	stm_dbg(DBG_ST.dma, " info->dst_info.buffer_type  = %d \n",
		info->dst_info.buffer_type);
	if (info->dst_addr)
		stm_dbg(DBG_ST.dma, " info->dst_addr  = %x \n",
			(u32) info->dst_addr);
}

/**
 * is_pipeid_invalid() - To check validity of client pipe_id
 * @pipe_id: pipe_id whose validity needs to be checked
 *
 * It will return -1 if pipe_id is invalid and 0 if valid
 **/
static inline int is_pipeid_invalid(int pipe_id)
{
	if ((pipe_id < 0)
	    || (pipe_id > (MAX_LOGICAL_CHANNELS + MAX_PHYSICAL_CHANNELS)))
		return -1;
	else {
		if (test_bit(pipe_id, dma_drv_data->pipe_id_map))
			return 0;
		else
			return -1;
	}
}

/**
 * get_xfer_len() - To get the length of the transfer
 * @info:	Information struct of the channel parameters.
 *
 */
static inline int get_xfer_len(struct dma_channel_info *info)
{
	return info->xfer_len;
}

/**
 * allocate_pipe_id() - To mark allocation of free pipe id.
 *
 * This function searches the pipe_id_map to find first free zero.
 * Presence of zero means we can use this position number as a pipe id.
 * After finding a position we mark it as unavailable, by writting  a 1
 * at that position in the map. This function returns -1 if it
 * cannot find a free pipe_id, or it returns pipe_id (>=0)
 *
 */
static int allocate_pipe_id(void)
{
	int id = 0;
	unsigned long flags;
	spin_lock_irqsave(&dma_drv_data->pipe_id_lock, flags);
	id = find_first_zero_bit((const unsigned long
				  *)(dma_drv_data->pipe_id_map), NUM_CHANNELS);
	if (id >= 0 && id < NUM_CHANNELS) {
		test_and_set_bit((id % BITS_PER_LONG),
				 &(dma_drv_data->pipe_id_map[BIT_WORD(id)]));
		stm_dbg(DBG_ST.dma,
			"allocate_pipe_id():: Client Id allocated = %d\n", id);
	} else {
		stm_error("Unable to allocate free channel....\n");
		id = -1;
	}
	spin_unlock_irqrestore(&dma_drv_data->pipe_id_lock, flags);
	return id;
}

/**
 * deallocate_pipe_id() - Function to deallocate already allocated pipe_id
 * @pipe_id:	Pipe id which needs to be deallocated.
 *
 * This function deallocates the pipe_id by clearing the corresponding
 * bit position in the pipe_id_map which is the allocation map for
 * DMA pipes.
 *
 */
static void deallocate_pipe_id(int pipe_id)
{
	unsigned long flags;
	spin_lock_irqsave(&dma_drv_data->pipe_id_lock, flags);
	test_and_clear_bit((pipe_id % BITS_PER_LONG),
			   &(dma_drv_data->pipe_id_map[BIT_WORD(pipe_id)]));
	spin_unlock_irqrestore(&dma_drv_data->pipe_id_lock, flags);
	print_allocation_map();
}

/**
 * get_phys_res_security() - Returns security type of the Physical resource.
 * @pr:	Physical resource whose security has to be returned
 *
 */
enum dma_chan_security get_phys_res_security(int pr)
{
#ifndef CONFIG_STM_SECURITY
	if (pr % 2)
		return REG_RD_BITS(io_addr(DREG_PRSMO),
				   PHYSICAL_RESOURCE_SECURE_MODE_MASK(pr),
				   PHYSICAL_RESOURCE_SECURE_MODE_POS(pr));
	else
		return REG_RD_BITS(io_addr(DREG_PRSME),
				   PHYSICAL_RESOURCE_SECURE_MODE_MASK(pr),
				   PHYSICAL_RESOURCE_SECURE_MODE_POS(pr));
#else
	return DMA_NONSECURE_CHAN;
#endif

}

/**
 * dma_channel_get_status() - To check the activation status of a physical channel
 * @channel:	Channel whose activation status is to be checked
 *
 */
static inline int dma_channel_get_status(int channel)
{

	if (channel % 2 == 0)
		return REG_RD_BITS(io_addr(DREG_ACTIVE),
				   ACT_PHY_RES_MASK
				   (channel), ACT_PHY_RES_POS(channel));
	else
		return REG_RD_BITS(io_addr(DREG_ACTIVO),
				   ACT_PHY_RES_MASK
				   (channel), ACT_PHY_RES_POS(channel));
}

/**
 * dma_channel_execute_command() - To execute cmds like run, stop,
 * suspend on phy channels
 * @command:	Command to be executed(run, stop, suspend, etc...)
 * @channel:	Channel on which these commands have to be executed
 *
 */
static int dma_channel_execute_command(int command, int channel)
{
	int status = 0;
	int iter = 0;

	if (get_phys_res_security(channel) == DMA_NONSECURE_CHAN) {
		switch (command) {
		case DMA_RUN:
			{
				if (channel % 2 == 0)
					REG_WR_BITS_1(io_addr
						      (DREG_ACTIVE),
						      DMA_RUN,
						      ACT_PHY_RES_MASK
						      (channel),
						      ACT_PHY_RES_POS(channel));
				else
					REG_WR_BITS_1(io_addr
						      (DREG_ACTIVO),
						      DMA_RUN,
						      ACT_PHY_RES_MASK
						      (channel),
						      ACT_PHY_RES_POS(channel));
				return 0;
			}
		case DMA_STOP:
			{
				if (channel % 2 == 0)
					REG_WR_BITS_1(io_addr
						      (DREG_ACTIVE),
						      DMA_STOP,
						      ACT_PHY_RES_MASK
						      (channel),
						      ACT_PHY_RES_POS(channel));
				else
					REG_WR_BITS_1(io_addr
						      (DREG_ACTIVO),
						      DMA_STOP,
						      ACT_PHY_RES_MASK
						      (channel),
						      ACT_PHY_RES_POS(channel));
				return 0;
			}
		case DMA_SUSPEND_REQ:
			{
				status = dma_channel_get_status(channel);
				if ((status == DMA_SUSPENDED)
				    || (status == DMA_STOP))
					return 0;
				if (channel % 2 == 0)
					REG_WR_BITS_1(io_addr
						      (DREG_ACTIVE),
						      DMA_SUSPEND_REQ,
						      ACT_PHY_RES_MASK
						      (channel),
						      ACT_PHY_RES_POS(channel));
				else
					REG_WR_BITS_1(io_addr
						      (DREG_ACTIVO),
						      DMA_SUSPEND_REQ,
						      ACT_PHY_RES_MASK
						      (channel),
						      ACT_PHY_RES_POS(channel));
				status = dma_channel_get_status(channel);
				while ((status != DMA_STOP)
				       && (status != DMA_SUSPENDED)) {
					if (iter == MAX_ITERATIONS) {
						stm_error("Unable to suspend \
						the Channel \
						%d\n", channel);
						return -1;
					}
					udelay(2);
					status =
					    dma_channel_get_status(channel);
					iter++;
				}
				return 0;
			}
		default:
			{
				stm_error("dma_channel_execute_command(): \
						Unknown command \n");
				return -1;
			}
		}
	} else {
		stm_error("Secure mode not activated yet!!\n");

	}
	return 0;
}

/**
 * get_phy_res_usage_count() - To find how many clients are sharing
 * this phy res.
 * @pchan:	id of Physical resource whose usage count has to be returned
 * This function returns the number of clients sharing a physical resource.
 * Count > 1 means, this is a logical resource.
 *
 */
static inline int get_phy_res_usage_count(int pchan)
{
	int count;
	count = dma_drv_data->pr_info[pchan].count;
	return count;
}

/**
 * release_phys_resource() - Function to deallocate a physical resource.
 * @info:  dma info staructure
 * This function deallocates a physical resource
 *
 */

static int release_phys_resource(struct dma_channel_info *info)
{
	int phy_channel = info->phys_chan_id;
	unsigned long flags;

	spin_lock_irqsave(&dma_drv_data->pr_info_lock, flags);
	if (dma_drv_data->pr_info[phy_channel].status == RESOURCE_LOGICAL) {
		if (--dma_drv_data->pr_info[phy_channel].count == 0) {
			if (dma_channel_execute_command
			    (DMA_SUSPEND_REQ, phy_channel) == -1) {
				stm_error("Unable to suspend the Channel %d\n",
					  phy_channel);
				goto error_release;
			}
			if (dma_channel_execute_command(DMA_STOP, phy_channel)
			    == -1) {
				stm_error("Unable to stop the Channel %d\n",
					  phy_channel);
				goto error_release;
			}
			dma_drv_data->pr_info[phy_channel].status =
			    RESOURCE_FREE;
			/*Resource is clean now */
			dma_drv_data->pr_info[phy_channel].dirty = 0;
		}
	} else if (dma_drv_data->pr_info[phy_channel].status ==
		   RESOURCE_PHYSICAL) {
		dma_drv_data->pr_info[phy_channel].status = RESOURCE_FREE;
		/*Resource is clean now */
		dma_drv_data->pr_info[phy_channel].dirty = 0;
	} else {
		stm_error("\n Already freed");
		goto error_release;
	}
	spin_unlock_irqrestore(&dma_drv_data->pr_info_lock, flags);
	return 0;
error_release:
	spin_unlock_irqrestore(&dma_drv_data->pr_info_lock, flags);
	return -1;
}

/**
 * allocate_free_dma_channel() - To allocate free Physical channel for a
 * dma xfer.
 * @info:	client id for which channel allocation has to be done
 * This function allocates free Physical channel for a dma xfer.
 * Allocation is done based on requirement of client and
 * availability of channels.
 * Logical channels would be used only for mem-periph, periph-mem.
 * mem-mem, periph-periph would used Physical mode only.
 * If a physical channel wants to reserve, a flag would mark
 * the physical resource.
 * If a logical channel has been requested, but corresponding
 * Physical resource(based on event line of Logical channel)is
 * reserved, request would be rejected straightaway.
 *
 */
static int allocate_free_dma_channel(struct dma_channel_info *info)
{
	int i, j, tmp;
	int event_group;
	unsigned long flags;
	if (info->dir == PERIPH_TO_PERIPH &&
	    (info->src_info.event_group != info->dst_info.event_group))
		return -EINVAL;
	if (info->dir == MEM_TO_PERIPH)
		event_group = info->dst_info.event_group;
	else
		event_group = info->src_info.event_group;

	spin_lock_irqsave(&dma_drv_data->pr_info_lock, flags);
	if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
		if (info->dir == MEM_TO_MEM) {
			for (i = 0; i < MAX_AVAIL_PHY_CHANNELS; i++) {
				if (dma_drv_data->pr_info[i].status ==
				    RESOURCE_FREE) {
					info->phys_chan_id = i;
					dma_drv_data->pr_info[i].status =
					    RESOURCE_PHYSICAL;
					info->channel_id =
					    (MAX_LOGICAL_CHANNELS +
					     info->phys_chan_id);
					goto done;
				}
			}
		} else {
			for (i = 0; i < MAX_AVAIL_PHY_CHANNELS; i += 8) {
				tmp = i * 8 + event_group * 2;
				for (j = tmp; j < tmp + 2; j++) {
					if (dma_drv_data->pr_info[j].status ==
					    RESOURCE_FREE) {
						info->phys_chan_id = j;
						info->channel_id =
						    (MAX_LOGICAL_CHANNELS +
						     info->phys_chan_id);
						dma_drv_data->
						    pr_info[j].status =
						    RESOURCE_PHYSICAL;
						goto done;
					}
				}
			}
		}
	} else {		/* Logical mode */
		if (info->dir == MEM_TO_PERIPH)
			info->channel_id = 2 * (info->dst_dev_type) + 1;
		else
			info->channel_id = 2 * (info->src_dev_type);
		if (info->dir == MEM_TO_MEM) {
			spin_unlock_irqrestore(&dma_drv_data->pr_info_lock,
					       flags);
			return -EINVAL;
		} else {
			for (i = 0; i < MAX_AVAIL_PHY_CHANNELS; i += 8) {
				tmp = i * 8 + event_group * 2;
				for (j = tmp; j < tmp + 2; j++) {
					if (dma_drv_data->pr_info[j].status ==
					    RESOURCE_FREE
					    || dma_drv_data->
					    pr_info[j].status ==
					    RESOURCE_LOGICAL) {
						if (dma_drv_data->
						    pr_info[j].count >= 16)
							continue;
						else {
							dma_drv_data->pr_info
							    [j].count++;
							info->phys_chan_id = j;
							dma_drv_data->pr_info
							    [j].status =
							    RESOURCE_LOGICAL;
							goto done;
						}
					}
				}

			}
		}
	}
	spin_unlock_irqrestore(&dma_drv_data->pr_info_lock, flags);
	return -EBUSY;
done:
	spin_unlock_irqrestore(&dma_drv_data->pr_info_lock, flags);
	return 0;

}

/**
 * get_phys_res_type() - returns Type of a Physical resource
 * (standard or extended)
 * @pr:	Physical resource whose type will be returned
 *
 */
static inline enum dma_phys_res_type get_phys_res_type(int pr)
{
	return REG_RD_BITS(io_addr(DREG_PRTYP),
			   PHYSICAL_RESOURCE_TYPE_MASK(pr),
			   PHYSICAL_RESOURCE_TYPE_POS(pr));
}

/**
 * set_phys_res_type() - To set Type of a Phys resource(standard or extended)
 * @pr:	Physical resource whose type is to be set
 * @type: standard or extended
 *
 */
static inline void set_phys_res_type(int pr, enum dma_phys_res_type type)
{
	REG_WR_BITS(io_addr(DREG_PRTYP), type,
		    PHYSICAL_RESOURCE_TYPE_MASK(pr),
		    PHYSICAL_RESOURCE_TYPE_POS(pr));
}

/**
 * set_phys_res_security() - To set security type of the Physical resource.
 * @pr:	Physical resource whose security has to be set
 * @type: type (secure, non secure)
 *
 */
static inline void set_phys_res_security(int pr,
					 enum dma_chan_security type)
{
	if (pr % 2)
		REG_WR_BITS(io_addr(DREG_PRSMO), type,
			    PHYSICAL_RESOURCE_SECURE_MODE_MASK(pr),
			    PHYSICAL_RESOURCE_SECURE_MODE_POS(pr));
	else
		REG_WR_BITS(io_addr(DREG_PRSME), type,
			    PHYSICAL_RESOURCE_SECURE_MODE_MASK(pr),
			    PHYSICAL_RESOURCE_SECURE_MODE_POS(pr));
}

/**
 * get_phys_res_mode() - To read the mode of the physical resource.
 * @pr: Physical resource whose mode is to be retrieved.
 * This function returns the current mode of the physical resources
 * by reading the DMA control registers. It returns enum
 * dma_channel_mode which is either DMA_CHAN_IN_PHYS_MODE,
 * DMA_CHAN_IN_LOG_MODE ,DMA_CHAN_IN_OPERATION_MODE.
 *
 */
static inline enum dma_channel_mode get_phys_res_mode(int pr)
{
	if (pr % 2)
		return REG_RD_BITS(io_addr(DREG_PRMSO),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_MASK
				   (pr),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_POS(pr));
	else
		return REG_RD_BITS(io_addr(DREG_PRMSE),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_MASK
				   (pr),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_POS(pr));
}

/**
 * set_phys_res_mode() - To set the mode of the Physical resource
 * @pr:	Physical resource whose mode has to be set
 * @mode: The mode in which the physical resource is to be set.
 *
 */
static inline void set_phys_res_mode(int pr, enum dma_channel_mode mode)
{
#if 0
	if (pr % 2)
		REG_WR_BITS(io_addr(DREG_PRMSO), mode,
			    PHYSICAL_RESOURCE_CHANNEL_MODE_MASK(pr),
			    PHYSICAL_RESOURCE_CHANNEL_MODE_POS(pr));
	else
		REG_WR_BITS(io_addr(DREG_PRMSE), mode,
			    PHYSICAL_RESOURCE_CHANNEL_MODE_MASK(pr),
			    PHYSICAL_RESOURCE_CHANNEL_MODE_POS(pr));
#endif

	if (get_phys_res_security(pr) == DMA_NONSECURE_CHAN) {
		if (pr % 2)
			REG_WR_BITS(io_addr(DREG_PRMSO), mode,
				    PHYSICAL_RESOURCE_CHANNEL_MODE_MASK
				    (pr),
				    PHYSICAL_RESOURCE_CHANNEL_MODE_POS(pr));
		else
			REG_WR_BITS(io_addr(DREG_PRMSE), mode,
				    PHYSICAL_RESOURCE_CHANNEL_MODE_MASK
				    (pr),
				    PHYSICAL_RESOURCE_CHANNEL_MODE_POS(pr));

	} else {
		stm_error("DMA Driver: Secure register not touched\n");
	}
}

/**
 * set_phys_res_mode_option() - To set the mode option of physical resource.
 * @pr:	Physical resource whose mode option needs to be set.
 * @option: enum dma_channel_mode_option
 *
 */
static inline void set_phys_res_mode_option(int pr,
					    enum dma_channel_mode_option
					    option)
{
#if 0
	if (pr % 2)
		REG_WR_BITS(io_addr(DREG_PRMOO), option,
			    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK
			    (pr),
			    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS(pr));
	else
		REG_WR_BITS(io_addr(DREG_PRMOE), option,
			    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK
			    (pr),
			    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS(pr));
#endif
	if (get_phys_res_security(pr) == DMA_NONSECURE_CHAN) {
		if (pr % 2)
			REG_WR_BITS(io_addr(DREG_PRMOO), option,
				    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK
				    (pr),
				    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS
				    (pr));
		else
			REG_WR_BITS(io_addr(DREG_PRMOE), option,
				    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK
				    (pr),
				    PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS
				    (pr));
	} else {
		stm_error
		    ("DMA Driver:Secure reg not touched for mode option\n");
	}

}

enum dma_channel_mode_option get_phys_res_mode_option(int pr)
{
	if (pr % 2)
		return REG_RD_BITS(io_addr(DREG_PRMOO),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK
				   (pr),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS
				   (pr));
	else
		return REG_RD_BITS(io_addr(DREG_PRMOE),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK
				   (pr),
				   PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS
				   (pr));
}

/**
 * set_event_line_security() - To set event line security
 * @line:	Event line whose security has to be set
 * @event_group: Event group of the event line
 * @security:	Type of security(enum dma_chan_security) to set
 *
 */
static inline void set_event_line_security(int line,
					   enum dma_event_group
					   event_group,
					   enum dma_chan_security security)
{
	u32 tmp;
	if (security == DMA_SECURE_CHAN) {
		tmp =
		    REG_RD_BITS(io_addr(DREG_SSEG(event_group)),
				FULL32_MASK, NO_SHIFT);
		tmp |= (0x1 << line);
		REG_WR_BITS(io_addr(DREG_SSEG(event_group)), tmp,
			    FULL32_MASK, NO_SHIFT);
	} else {
		tmp =
		    REG_RD_BITS(io_addr(DREG_SCEG(event_group)),
				FULL32_MASK, NO_SHIFT);
		tmp |= (0x1 << line);
		REG_WR_BITS(io_addr(DREG_SCEG(event_group)), tmp,
			    FULL32_MASK, NO_SHIFT);
	}
}

/**
 * create_elem_pool() - To create pool of elements
 * @name:	Name of the Element pool created
 * @block_size: Number of bytes in one element
 * @num_block:	Number of elements.
 * @allignment_bits: Allignment of memory allocated in terms of bit position
 * This function creates a pool of "num_block" blocks with fixed
 * block size of "block_size" It returns a logical address or null if
 * allocation fails. It uses dma_alloc_coherent() for allocation of pool memory.
 * The memory allocated would be alligned as given by allignment_bits argument.
 * For e.g if allignment_bits = 16, then last 16 bits of alligned
 * memory would be 0.
 * This function returns pointer to the element pool created , or NULL on error
 *
 */
static struct elem_pool *create_elem_pool(char *name, int block_size,
					  int num_block, int allignment_bits)
{
	int allignment_size;
	int alloc_size;
	struct elem_pool *pool = kzalloc(sizeof(struct elem_pool), GFP_KERNEL);
	if (!pool) {
		stm_error("Could not allocate memory for pool");
		return NULL;
	}

	pool->num_blocks = num_block;
	pool->block_size = block_size;
	pool->allocation_map = 0;
	spin_lock_init(&pool->pool_lock);
	strcpy(pool->name, name);

	if (allignment_bits <= PAGE_SHIFT) {
		/*Because dma_alloc_coherent is already page_aligned */
		void *ptr = dma_alloc_coherent(NULL,
					       num_block * block_size,
					       &pool->unalign_base.phys_addr,
					       GFP_KERNEL | GFP_DMA);
		pool->unalign_base.log_addr = ptr;
		pool->base_addr = pool->unalign_base;
		pool->allocation_size = num_block * block_size;
	} else {
		void *ptr;

		allignment_size = (0x1 << allignment_bits);
		alloc_size =
		    num_block * block_size + allignment_size + sizeof(void *);

		pool->allocation_size = alloc_size;
		ptr = dma_alloc_coherent(NULL,
					 alloc_size,
					 &pool->unalign_base.phys_addr,
					 GFP_KERNEL | GFP_DMA);
		pool->unalign_base.log_addr = ptr;
		if ((pool->unalign_base.phys_addr % allignment_size +
		     pool->unalign_base.phys_addr) ==
		    pool->unalign_base.phys_addr) {
			pool->base_addr.log_addr = pool->unalign_base.log_addr;
			pool->base_addr.phys_addr =
			    pool->unalign_base.phys_addr;
			stm_dbg(DBG_ST.dma, "No need to Allign.............\n");
			goto complete_pool;
		}

		pool->base_addr.log_addr =
		    (char *)pool->unalign_base.log_addr +
		    sizeof(void *) + allignment_size;
		pool->base_addr.log_addr =
		    (char *)pool->base_addr.log_addr -
		    ((unsigned long)pool->base_addr.log_addr) % allignment_size;

		pool->base_addr.phys_addr = pool->unalign_base.phys_addr
		    + sizeof(void *) + allignment_size;
		pool->base_addr.phys_addr = pool->base_addr.phys_addr
		    -
		    ((unsigned long)pool->base_addr.phys_addr) %
		    allignment_size;

		pool->base_addr.log_addr = pool->unalign_base.log_addr
		    + ((unsigned long)pool->base_addr.phys_addr
		       - (unsigned long)pool->unalign_base.phys_addr);
	}
complete_pool:
	stm_dbg(DBG_ST.dma,
		"UNALLIGNED BASE ADDRESS :: LOGICAL = %x, PHYSICAL = %x\n",
		(u32) pool->unalign_base.log_addr,
		(u32) pool->unalign_base.phys_addr);
	stm_dbg(DBG_ST.dma,
		"ALLIGNED BASE ADDRESS :: LOGICAL = %x, PHYSICAL = %x\n",
		(u32) pool->base_addr.log_addr,
		(u32) pool->base_addr.phys_addr);
	return pool;
}

static struct elem_pool *create_elem_pool_fixed_mem(char *name, int block_size,
						    int num_block, u32 mem_base,
						    int max_avail_mem)
{
	struct elem_pool *pool = kzalloc(sizeof(struct elem_pool), GFP_KERNEL);
	if (!pool) {
		stm_error("Could not allocate memory for pool");
		return NULL;
	}

	pool->num_blocks = num_block;
	pool->block_size = block_size;
	pool->allocation_map = 0;
	spin_lock_init(&pool->pool_lock);
	strcpy(pool->name, name);

	if ((block_size * num_block) > max_avail_mem) {
		stm_error("Too much memory requested\n");
		kfree(pool);
		return NULL;
	}
	pool->unalign_base.log_addr = ioremap(mem_base, max_avail_mem);
	if (!(pool->unalign_base.log_addr)) {
		stm_error("Could not ioremap the LCLA memory\n");
		kfree(pool);
		return NULL;
	}

	pool->unalign_base.phys_addr = mem_base;
	pool->base_addr = pool->unalign_base;
	pool->allocation_size = num_block * block_size;

	stm_dbg(DBG_ST.dma,
		"UNALLIGNED BASE ADDRESS :: LOGICAL = %x, PHYSICAL = %x\n",
		(u32) pool->unalign_base.log_addr,
		(u32) pool->unalign_base.phys_addr);
	stm_dbg(DBG_ST.dma,
		"ALLIGNED BASE ADDRESS :: LOGICAL = %x, PHYSICAL = %x\n",
		(u32) pool->base_addr.log_addr,
		(u32) pool->base_addr.phys_addr);
	return pool;
}

/**
 * destroy_elem_pool() - To destroy the Element pool created
 * @pool: Pointer to pool which needs to be destroyed
 *
 */
static void destroy_elem_pool(struct elem_pool *pool)
{
	if (pool) {
		dma_free_coherent(NULL, pool->allocation_size,
				  pool->unalign_base.log_addr,
				  pool->unalign_base.phys_addr);
		kfree(pool);
	}
}

/**
 * get_free_block() - To allocate  a Free block from an element pool
 * @pool:	Pointer to pool from which free block is required
 * This function checks in the allocation bitmap of the pool
 * and returns a free block after setting corresponding bit in the
 * allocation map of the pool.
 * It will return block id(>=0) on success and -1 on failure.
 */
static int get_free_block(struct elem_pool *pool)
{
	int i;
	stm_dbg(DBG_ST.dma, "%s :: Allocation Map: %d\n", pool->name,
		pool->allocation_map);
	spin_lock(&pool->pool_lock);
	for (i = 0; i < pool->num_blocks; i++) {
		if (!(pool->allocation_map & (0x1 << i))) {
			pool->allocation_map |= (0x1 << i);
			spin_unlock(&pool->pool_lock);
			return i;
		}
	}
	spin_unlock(&pool->pool_lock);
	return -1;
}

/**
 * get_free_log_lli_block() - To allocate a Free block for a Logical channel
 * @pool:	Element pool from where allocation has to be done
 * @pchan:	Physical resource corresponding to the Logical channel
 * There are a fixed number of blocks for Logical LLI per physical channel.
 * Hence allocation is done by searching free blocks for a particular
 * physical channel. It will return block id(>=0) on success and -1 on failure.
 *
 */
static int get_free_log_lli_block(struct elem_pool *pool, int pchan)
{
	int i = 0;
	int num_log_lli_blocks;
	num_log_lli_blocks = 126 / NUM_LLI_PER_LOG_CHANNEL;

	stm_dbg(DBG_ST.dma, "NAME:%s :: Phys_chan:: %d :: Allocation Map: %d\n",
		pool->name, pchan, pool->lchan_allocation_map[pchan]);
	spin_lock(&pool->pool_lock);
	for (i = 0; i < num_log_lli_blocks; i++) {
		if (!(pool->lchan_allocation_map[pchan] & (0x1 << i))) {
			pool->lchan_allocation_map[pchan] |= (0x1 << i);
			spin_unlock(&pool->pool_lock);
			return i;
		}
	}
	spin_unlock(&pool->pool_lock);
	return -1;
}

/**
 * rel_free_log_lli_block() - To release a Free block for a Logical channel
 * @pool:	Element pool from where relese has to be done
 * @pchan:	Physical resource corresponding to this Logical channel
 * @idx:	index of the block to be released
 *
 */
static void rel_free_log_lli_block(struct elem_pool *pool, int pchan, int idx)
{
	int num_log_lli_blocks;
	num_log_lli_blocks = 126 / NUM_LLI_PER_LOG_CHANNEL;

	stm_dbg(DBG_ST.dma, "NAME:%s :: Phys_chan:: %d :: Allocation Map: %d\n",
		pool->name, pchan, pool->lchan_allocation_map[pchan]);
	spin_lock(&pool->pool_lock);
	pool->lchan_allocation_map[pchan] &= (~(0x1 << idx));
	spin_unlock(&pool->pool_lock);
}

/**
 * release_block() - To release a Free block
 * @idx:	index of the block to be released
 * @pool:	Pool from which block is to be released
 *
 */
static void release_block(int idx, struct elem_pool *pool)
{
	if (idx < pool->num_blocks) {
		spin_lock(&pool->pool_lock);
		pool->allocation_map &= (~(0x1 << idx));
		spin_unlock(&pool->pool_lock);
	}
}

int calculate_lli_required(struct dma_channel_info *info,
			   int *lli_size_req)
{
	int lli_count = 0;
	int lli_size = 0;
	int xfer_len = get_xfer_len(info);

	u32 src_data_width = (0x1 << info->src_info.data_width);
	u32 dst_data_width = (0x1 << info->dst_info.data_width);

	lli_size = ((SIXTY_FOUR_KB - 4) *
		    ((dst_data_width >
		      src_data_width) ? src_data_width : dst_data_width));

	lli_count = xfer_len / lli_size;
	if (xfer_len % lli_size)
		lli_count++;
	*lli_size_req = lli_size;
	stm_dbg(DBG_ST.dma, "LLI_size = %d LLI Count = %d\n", lli_size,
		lli_count);
	return lli_count;
}

/**
 * generate_sg_list() - To Generate a sg list if DMA xfer len > 64K elems
 * @info:	Information of the channel parameters.
 * @type:	Whether Source or Destination half channel.
 * @lli_size:	Max size of each LLI to be generated.
 * @lli_count:	Number of LLI's to be  generated.
 * This function generates a sg list by breaking current length into
 * length of size lli_size. It will generate lli_count lli's. It will
 * finally return the sg list.
 *
 */
struct scatterlist *generate_sg_list(struct dma_channel_info *info,
				     enum dma_half_chan type, int lli_size,
				     int lli_count)
{
	struct scatterlist *sg = NULL;
	struct scatterlist *temp_sg = NULL;
	int i = 0;
	void *temp_addr;

	if (type == DMA_SRC_HALF_CHANNEL) {
		if (info->sg_block_id_src < 0)
			info->sg_block_id_src =
			    get_free_block(dma_drv_data->sg_pool);
		sg = ((struct scatterlist *)((u32) dma_drv_data->sg_pool->
					     base_addr.log_addr +
					     info->sg_block_id_src *
					     dma_drv_data->sg_pool->
					     block_size));
		info->sgcount_src = lli_count;
		info->sg_src = sg;
		temp_addr = info->src_addr;
	} else {
		if (info->sg_block_id_dest < 0)
			info->sg_block_id_dest =
			    get_free_block(dma_drv_data->sg_pool);
		sg = ((struct scatterlist *)((u32) dma_drv_data->sg_pool->
					     base_addr.log_addr +
					     info->sg_block_id_dest *
					     dma_drv_data->sg_pool->
					     block_size));
		info->sgcount_dest = lli_count;
		info->sg_dest = sg;
		temp_addr = info->dst_addr;
	}

	sg_init_table(sg, lli_count);
	for_each_sg(sg, temp_sg, lli_count, i) {
		if ((type == DMA_SRC_HALF_CHANNEL)
		    && ((info->dir == PERIPH_TO_MEM)
			|| (info->dir == PERIPH_TO_PERIPH)))
			temp_sg->dma_address = (dma_addr_t) temp_addr;
		else if ((type == DMA_DEST_HALF_CHANNEL)
			 && ((info->dir == MEM_TO_PERIPH)
			     || (info->dir == PERIPH_TO_PERIPH)))
			temp_sg->dma_address = (dma_addr_t) temp_addr;
		else
			temp_sg->dma_address =
			    (dma_addr_t) (temp_addr + i * (lli_size));

		temp_sg->length = lli_size;
		temp_sg->offset = 0;
		if (i == (lli_count - 1)) {
			int len = get_xfer_len(info) % lli_size;
			if (len)
				temp_sg->length = len;
		}
	}
	return sg;
}

/**
 * map_lli_info_log_chan() - To Map the SG info into LLI's for Log channels.
 * @info:	Information of the channel parameters.
 * @type:	Whether source or destination half channel.
 * @params:	Memory location pointer where standard Logical channel register
 *	parameters are Loaded.
 * This function maps the SG info into LLI's for the Logical channels.
 * A free block of LLI's is allocated for a particular physical
 * channel and SG info is mapped in that memory block. For a particular
 * physical channel 1Kb of memory is there for LLI's which has to be
 * shared by all Logical channels running on it.
 * It returns 0 on success, -1 on failure.
 *
 */
static int map_lli_info_log_chan(struct dma_channel_info *info, int type,
				 struct std_log_memory_param *params)
{
	int idx = 0;
	struct scatterlist *sg;
	u32 step_size = 0;
	u32 lli_count = 0;
	void *base =
	    dma_drv_data->lchan_lli_pool->base_addr.log_addr +
	    info->phys_chan_id * 1024;

	stm_dbg(DBG_ST.dma, "Setting LLI for %s\n",
		(type ==
		 DMA_SRC_HALF_CHANNEL) ? "DMA_SRC_HALF_CHANNEL" :
		"DMA_DEST_HALF_CHANNEL");

	if (type == DMA_SRC_HALF_CHANNEL) {
		struct dma_logical_src_lli_info *lli_ptr = NULL;
		u32 slos = 0;
		step_size = (0x1 << info->src_info.data_width);
		sg = info->sg_src;
		lli_count = info->sgcount_src;
		if (info->lli_block_id_src < 0)
			info->lli_block_id_src =
			    get_free_log_lli_block(dma_drv_data->lchan_lli_pool,
						   info->phys_chan_id);
		if (info->lli_block_id_src < 0) {
			stm_error("Free LLI's not available\n");
			return -1;
		}

		if (lli_count > 1)
			slos = info->lli_block_id_src * NUM_LLI_PER_LOG_CHANNEL + 1;
		/*********************************************************/
		/*Use first SG item to fill params register */
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSELT),
			    info->phys_chan_id, SREG_ELEM_LOG_LIDX_MASK,
			    SREG_ELEM_LOG_LIDX_POS);
		MEM_WRITE_BITS(params->dmac_lcsp0,
			       (sg_dma_len(sg) >> info->src_info.data_width),
			       MEM_LCSP0_ECNT_MASK, MEM_LCSP0_ECNT_POS);
		MEM_WRITE_BITS(params->dmac_lcsp1, slos, MEM_LCSP1_SLOS_MASK,
			       MEM_LCSP1_SLOS_POS);
		MEM_WRITE_BITS(params->dmac_lcsp0,
			       (0x0000FFFFUL & (u32) (sg_dma_address(sg))),
			       MEM_LCSP0_SPTR_MASK, MEM_LCSP0_SPTR_POS);
		MEM_WRITE_BITS(params->dmac_lcsp1,
			       (0xFFFF0000 & (u32) (sg_dma_address(sg))) >> 16,
			       MEM_LCSP1_SPTR_MASK, MEM_LCSP1_SPTR_POS);
		MEM_WRITE_BITS(params->dmac_lcsp1, DMA_FALSE,
			       MEM_LCSP1_SCFG_TIM_MASK,
			       MEM_LCSP1_SCFG_TIM_POS);
		sg++;
		lli_count--;
		/***********************************************************/
		if (lli_count) {
			lli_ptr = (struct dma_logical_src_lli_info *)base + slos;

			for (idx = 0; idx < lli_count; idx++) {
				lli_ptr[idx].dmac_lcsp1 = params->dmac_lcsp1;
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp0,
					       (sg_dma_len(sg) >> info->
						src_info.data_width),
					       MEM_LCSP0_ECNT_MASK, MEM_LCSP0_ECNT_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp0,
					       (0x0000FFFFUL &
						(u32) (sg_dma_address(sg))),
					       MEM_LCSP0_SPTR_MASK, MEM_LCSP0_SPTR_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp1,
					       (0xFFFF0000UL &
						(u32) (sg_dma_address(sg))) >> 16,
					       MEM_LCSP1_SPTR_MASK, MEM_LCSP1_SPTR_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp1,
					       (slos + idx + 1), MEM_LCSP1_SLOS_MASK,
					       MEM_LCSP1_SLOS_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp1,
					       DMA_FALSE,
					       MEM_LCSP1_SCFG_TIM_MASK,
					       MEM_LCSP1_SCFG_TIM_POS);
				sg++;
			}

			MEM_WRITE_BITS(lli_ptr[idx - 1].dmac_lcsp1, 0x0,
				       MEM_LCSP1_SLOS_MASK, MEM_LCSP1_SLOS_POS);
		}
	} else {
		struct dma_logical_dest_lli_info *lli_ptr = NULL;
		u32 dlos = 0;
		step_size = (0x1 << info->dst_info.data_width);
		lli_count = info->sgcount_dest;
		sg = info->sg_dest;
		info->current_sg = sg;
		if (info->lli_block_id_dest < 0)
			info->lli_block_id_dest =
			    get_free_log_lli_block(dma_drv_data->lchan_lli_pool,
						   info->phys_chan_id);
		if (info->lli_block_id_dest < 0) {
			stm_error("Free LLI's not available\n");
			return -1;
		}

		if (lli_count > 1)
			dlos = info->lli_block_id_dest * NUM_LLI_PER_LOG_CHANNEL + 1;
		/**********************************************************/
		/*Use first SG item to fill params register */
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDELT),
			    info->phys_chan_id, SREG_ELEM_LOG_LIDX_MASK,
			    SREG_ELEM_LOG_LIDX_POS);
		MEM_WRITE_BITS(params->dmac_lcsp2,
			       (sg_dma_len(sg) >> info->dst_info.data_width),
			       MEM_LCSP2_ECNT_MASK, MEM_LCSP2_ECNT_POS);
		MEM_WRITE_BITS(params->dmac_lcsp3, dlos, MEM_LCSP3_DLOS_MASK,
			       MEM_LCSP3_DLOS_POS);
		MEM_WRITE_BITS(params->dmac_lcsp2,
			       (0x0000FFFFUL & (u32) (sg_dma_address(sg))),
			       MEM_LCSP2_DPTR_MASK, MEM_LCSP2_DPTR_POS);
		MEM_WRITE_BITS(params->dmac_lcsp3,
			       (0xFFFF0000UL & (u32) (sg_dma_address(sg))) >>
			       16, MEM_LCSP3_DPTR_MASK, MEM_LCSP3_DPTR_POS);
		if (info->lli_interrupt == 1 || lli_count == 1)
			MEM_WRITE_BITS(params->dmac_lcsp3, DMA_TRUE,
				       MEM_LCSP3_DCFG_TIM_MASK,
				       MEM_LCSP3_DCFG_TIM_POS);
		else
			MEM_WRITE_BITS(params->dmac_lcsp3, DMA_FALSE,
				       MEM_LCSP3_DCFG_TIM_MASK,
				       MEM_LCSP3_DCFG_TIM_POS);
		sg++;
		lli_count--;
		/************************************************************/

		if (lli_count) {
			lli_ptr = (struct dma_logical_dest_lli_info *)base + dlos;

			for (idx = 0; idx < lli_count; idx++) {
				lli_ptr[idx].dmac_lcsp3 = params->dmac_lcsp3;
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp2,
					       (sg_dma_len(sg) >> info->
						dst_info.data_width),
					       MEM_LCSP2_ECNT_MASK, MEM_LCSP2_ECNT_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp2,
					       (0x0000FFFFUL &
						(u32) (sg_dma_address(sg))),
					       MEM_LCSP2_DPTR_MASK, MEM_LCSP2_DPTR_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp3,
					       (0xFFFF0000UL &
						(u32) (sg_dma_address(sg))) >> 16,
					       MEM_LCSP3_DPTR_MASK, MEM_LCSP3_DPTR_POS);
				MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp3,
					       (dlos + idx + 1), MEM_LCSP3_DLOS_MASK,
					       MEM_LCSP3_DLOS_POS);
				if (info->lli_interrupt == 1)
					MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp3,
						       DMA_TRUE,
						       MEM_LCSP3_DCFG_TIM_MASK,
						       MEM_LCSP3_DCFG_TIM_POS);
				else
					MEM_WRITE_BITS(lli_ptr[idx].dmac_lcsp3,
						       DMA_FALSE,
						       MEM_LCSP3_DCFG_TIM_MASK,
						       MEM_LCSP3_DCFG_TIM_POS);
				sg++;
			}

			MEM_WRITE_BITS(lli_ptr[idx - 1].dmac_lcsp3, 0x0,
				       MEM_LCSP3_DLOS_MASK, MEM_LCSP3_DLOS_POS);
			MEM_WRITE_BITS(lli_ptr[idx - 1].dmac_lcsp3, DMA_TRUE,
				       MEM_LCSP3_DCFG_TIM_MASK, MEM_LCSP3_DCFG_TIM_POS);
		}
	}
	return 0;
}

/**
 * map_lli_info_phys_chan() - To Map the SG info into LLI's for Phys channels.
 * @info:	Information of the channel parameters.
 * @type:	Whether source or destination half channel.
 * This function maps the SG info into LLI's for the Physical channels.
 * A free block of LLI's is allocated for a particular physical
 * channel and SG info is mapped in that memory block.
 * This function returns 0 on success and -1 on failure
 *
 */
static int map_lli_info_phys_chan(struct dma_channel_info *info, int type)
{
	int idx = 0;
	struct dma_lli_info *lli_ptr = NULL;
	u32 temp;
	dma_addr_t phys_lli_addr = 0;
	struct scatterlist *sg;
	u32 lli_count = 0;
	u32 fifo_ptr_incr = 0;
	stm_dbg(DBG_ST.dma, "map_lli_info_phys_chan():: Setting LLI for %s\n",
		(type ==
		 DMA_SRC_HALF_CHANNEL) ? "DMA_SRC_HALF_CHANNEL" :
		"DMA_DEST_HALF_CHANNEL");

	if (type == DMA_SRC_HALF_CHANNEL) {
		sg = info->sg_src;
		lli_count = info->sgcount_src;
		if (info->src_info.addr_inc)
			fifo_ptr_incr = (0x1 << info->src_info.data_width);
		if (info->lli_block_id_src < 0)
			info->lli_block_id_src =
			    get_free_block(dma_drv_data->pchan_lli_pool);
		if (info->lli_block_id_src < 0) {
			stm_error("Free LLI's not available\n");
			return -1;
		}
		temp = ((u32) dma_drv_data->pchan_lli_pool->base_addr.log_addr +
			info->lli_block_id_src *
			dma_drv_data->pchan_lli_pool->block_size);
		lli_ptr = (struct dma_lli_info *)temp;
		temp = (u32) (dma_drv_data->pchan_lli_pool->base_addr.phys_addr
			      +
			      info->lli_block_id_src
			      * dma_drv_data->pchan_lli_pool->block_size);
		phys_lli_addr = (dma_addr_t) temp;
		if (info->link_type == POSTLINK) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SSELT),
				    (sg_dma_len(sg) >> info->
				     src_info.data_width),
				    SREG_ELEM_PHY_ECNT_MASK,
				    SREG_ELEM_PHY_ECNT_POS);
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SSELT),
				    fifo_ptr_incr, SREG_ELEM_PHY_EIDX_MASK,
				    SREG_ELEM_PHY_EIDX_POS);
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SSPTR),
				    sg_dma_address(sg), FULL32_MASK, NO_SHIFT);
			sg++;
			lli_count--;
		}

		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSLNK),
			    (u32) (phys_lli_addr), FULL32_MASK, NO_SHIFT);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSLNK),
			    info->link_type, SREG_LNK_PHYS_PRE_MASK,
			    SREG_LNK_PHY_PRE_POS);

		for (idx = 0; idx < lli_count; idx++) {
			lli_ptr[idx].reg_cfg =
			    REG_RD_BITS(cio_addr
					(info->phys_chan_id,
					 CHAN_REG_SSCFG), FULL32_MASK,
					NO_SHIFT);
			lli_ptr[idx].reg_elt =
			    REG_RD_BITS(cio_addr
					(info->phys_chan_id,
					 CHAN_REG_SSELT), FULL32_MASK,
					NO_SHIFT);
			MEM_WRITE_BITS(lli_ptr[idx].reg_elt,
				       (sg_dma_len(sg) >> info->
					src_info.data_width),
				       SREG_ELEM_PHY_ECNT_MASK,
				       SREG_ELEM_PHY_ECNT_POS);
			MEM_WRITE_BITS(lli_ptr[idx].reg_elt, fifo_ptr_incr,
				       SREG_ELEM_PHY_EIDX_MASK,
				       SREG_ELEM_PHY_EIDX_POS);
			lli_ptr[idx].reg_ptr = sg_dma_address(sg);
			lli_ptr[idx].reg_lnk =
			    (u32) (phys_lli_addr +
				   (idx + 1) * sizeof(struct dma_lli_info));
			MEM_WRITE_BITS(lli_ptr[idx].reg_lnk, info->link_type,
				       SREG_LNK_PHYS_PRE_MASK,
				       SREG_LNK_PHY_PRE_POS);

			sg++;
			print_dma_lli(idx, &lli_ptr[idx]);
		}
		lli_ptr[idx - 1].reg_lnk = 0x0;
		MEM_WRITE_BITS(lli_ptr[idx - 1].reg_lnk, info->link_type,
			       SREG_LNK_PHYS_PRE_MASK, SREG_LNK_PHY_PRE_POS);
		MEM_WRITE_BITS(lli_ptr[idx - 1].reg_lnk, DMA_TRUE,
			       SREG_LNK_PHYS_TCP_MASK, SREG_LNK_PHY_TCP_POS);
		MEM_WRITE_BITS(lli_ptr[idx - 1].reg_cfg, DMA_TRUE,
			       SREG_CFG_PHY_TIM_MASK, SREG_CFG_PHY_TIM_POS);

	} else {
		sg = info->sg_dest;
		info->current_sg = sg;
		lli_count = info->sgcount_dest;
		if ((info->dst_info.addr_inc))
			fifo_ptr_incr = (0x1 << info->dst_info.data_width);
		info->lli_block_id_dest =
		    get_free_block(dma_drv_data->pchan_lli_pool);
		if (info->lli_block_id_dest < 0) {
			stm_error("Free LLI's not available\n");
			return -1;
		}

		lli_ptr = (struct dma_lli_info *)
		    (dma_drv_data->pchan_lli_pool->base_addr.log_addr +
		     info->lli_block_id_dest *
		     dma_drv_data->pchan_lli_pool->block_size);
		phys_lli_addr = (dma_addr_t) (((u32)
					       dma_drv_data->pchan_lli_pool->
					       base_addr.phys_addr +
					       info->lli_block_id_dest *
					       dma_drv_data->pchan_lli_pool->
					       block_size));

		if (info->link_type == POSTLINK) {
			if ((info->dir != MEM_TO_MEM)
			    && (info->lli_interrupt == 1))
				REG_WR_BITS(cio_addr
					    (info->phys_chan_id,
					     CHAN_REG_SDCFG),
					    DMA_TRUE,
					    SREG_CFG_PHY_TIM_MASK,
					    SREG_CFG_PHY_TIM_POS);
			else
				REG_WR_BITS(cio_addr
					    (info->phys_chan_id,
					     CHAN_REG_SDCFG),
					    DMA_FALSE,
					    SREG_CFG_PHY_TIM_MASK,
					    SREG_CFG_PHY_TIM_POS);
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SDELT),
				    (sg_dma_len(sg)) >> info->
				    dst_info.data_width,
				    SREG_ELEM_PHY_ECNT_MASK,
				    SREG_ELEM_PHY_ECNT_POS);
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SDELT),
				    fifo_ptr_incr, SREG_ELEM_PHY_EIDX_MASK,
				    SREG_ELEM_PHY_EIDX_POS);
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SDPTR),
				    sg_dma_address(sg), FULL32_MASK, NO_SHIFT);
			sg++;
			lli_count--;
		}

		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDLNK),
			    (u32) (phys_lli_addr), FULL32_MASK, NO_SHIFT);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDLNK),
			    info->link_type, SREG_LNK_PHYS_PRE_MASK,
			    SREG_LNK_PHY_PRE_POS);

		for (idx = 0; idx < lli_count; idx++) {
			lli_ptr[idx].reg_cfg =
			    REG_RD_BITS(cio_addr
					(info->phys_chan_id,
					 CHAN_REG_SDCFG), FULL32_MASK,
					NO_SHIFT);
			lli_ptr[idx].reg_lnk =
			    (u32) (phys_lli_addr +
				   (idx + 1) * sizeof(struct dma_lli_info));
			MEM_WRITE_BITS(lli_ptr[idx].reg_lnk, info->link_type,
				       SREG_LNK_PHYS_PRE_MASK,
				       SREG_LNK_PHY_PRE_POS);
			lli_ptr[idx].reg_elt =
			    REG_RD_BITS(cio_addr
					(info->phys_chan_id,
					 CHAN_REG_SDELT), FULL32_MASK,
					NO_SHIFT);
			MEM_WRITE_BITS(lli_ptr[idx].reg_elt,
				       (sg_dma_len(sg) >> info->
					dst_info.data_width),
				       SREG_ELEM_PHY_ECNT_MASK,
				       SREG_ELEM_PHY_ECNT_POS);
			MEM_WRITE_BITS(lli_ptr[idx].reg_elt, fifo_ptr_incr,
				       SREG_ELEM_PHY_EIDX_MASK,
				       SREG_ELEM_PHY_EIDX_POS);
			lli_ptr[idx].reg_ptr = sg_dma_address(sg);
			if ((info->dir != MEM_TO_MEM)
			    && (info->lli_interrupt == 1))
				MEM_WRITE_BITS(lli_ptr[idx].reg_cfg,
					       DMA_TRUE,
					       SREG_CFG_PHY_TIM_MASK,
					       SREG_CFG_PHY_TIM_POS);
			else
				MEM_WRITE_BITS(lli_ptr[idx].reg_cfg,
					       DMA_FALSE,
					       SREG_CFG_PHY_TIM_MASK,
					       SREG_CFG_PHY_TIM_POS);
			print_dma_lli(idx, &lli_ptr[idx]);
			sg++;
		}
		MEM_WRITE_BITS(lli_ptr[idx - 1].reg_cfg, DMA_TRUE,
			       SREG_CFG_PHY_TIM_MASK, SREG_CFG_PHY_TIM_POS);
		lli_ptr[idx - 1].reg_lnk = 0x0;
		MEM_WRITE_BITS(lli_ptr[idx - 1].reg_lnk, info->link_type,
			       SREG_LNK_PHYS_PRE_MASK, SREG_LNK_PHY_PRE_POS);
		MEM_WRITE_BITS(lli_ptr[idx - 1].reg_lnk, DMA_TRUE,
			       SREG_LNK_PHYS_TCP_MASK, SREG_LNK_PHY_TCP_POS);
	}
	return 0;
}

/**
 * set_std_phy_src_half_channel_params() - set params for src phy half channel
 * @info:	Information of the channel parameters.
 * This function sets the channel Parameters for standard source half channel.
 * Channel is configured as Physical channel. It maps sg info by calling
 * map_lli_info_phys_chan() for src channel. If transfer length is more than
 * 64K elements, it calls calculate_lli_required() and generate_sg_list()
 * API's to create LLI's.
 * It returns 0 on success and -1 on failure
 *
 */
static int set_std_phy_src_half_channel_params(struct dma_channel_info
					       *info)
{
	u32 num_elems;
	u32 step_size = 0;
	if (info->src_info.addr_inc)
		step_size = (0x1 << info->src_info.data_width);

	num_elems = info->src_xfer_elem;
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSCFG),
		    info->src_cfg, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSPTR), 0x0,
		    FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSELT), 0x0,
		    FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSLNK), 0x0,
		    FULL32_MASK, NO_SHIFT);

	if ((num_elems >= MAX_NUM_OF_ELEM_IN_A_XFER) || (info->sg_src)) {
		u32 lli_size;
		u32 lli_count;
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSCFG),
			    DMA_FALSE, SREG_CFG_PHY_TIM_MASK,
			    SREG_CFG_PHY_TIM_POS);
		if (!info->sg_src) {
			lli_count = calculate_lli_required(info, &lli_size);
			if (lli_count > NUM_SG_PER_REQUEST) {
				stm_error("Insufficient LLI's, \
					lli count very high = %d\n", lli_count);
				return -1;
			}
			if (!generate_sg_list
			    (info, DMA_SRC_HALF_CHANNEL, lli_size, lli_count)) {
				stm_error("cannot generate sg list\n");
				return -1;
			}
		}
		print_sg_info(info->sg_src, info->sgcount_src);
		if (map_lli_info_phys_chan(info, DMA_SRC_HALF_CHANNEL)) {
			stm_error("Unable to map SG info to LLI....");
			return -1;
		}
	} else {
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSELT),
			    num_elems, SREG_ELEM_PHY_ECNT_MASK,
			    SREG_ELEM_PHY_ECNT_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSELT),
			    step_size, SREG_ELEM_PHY_EIDX_MASK,
			    SREG_ELEM_PHY_EIDX_POS);

		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSPTR),
			    (u32) info->src_addr, FULL32_MASK, NO_SHIFT);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSLNK),
			    0x1, FULL32_MASK, NO_SHIFT);
	}
	return 0;
}

static int set_std_phy_src_half_channel_params_mem(struct dma_channel_info
						   *info)
{

	enum dma_master_id master_id;
	enum dma_half_chan_sync sync_type;

	info->src_cfg = 0;

	if ((info->dir == PERIPH_TO_MEM) || (info->dir == PERIPH_TO_PERIPH)) {
		master_id = DMA_MASTER_1;
		sync_type = DMA_PACKET_SYNC;
		info->src_info.addr_inc = DMA_ADR_NOINC;
	} else {
		master_id = DMA_MASTER_0;
		sync_type = DMA_NO_SYNC;
	}

	MEM_WRITE_BITS(info->src_cfg, master_id,
		       SREG_CFG_PHY_MST_MASK, SREG_CFG_PHY_MST_POS);
	MEM_WRITE_BITS(info->src_cfg, DMA_FALSE,
		       SREG_CFG_PHY_TIM_MASK, SREG_CFG_PHY_TIM_POS);
	MEM_WRITE_BITS(info->src_cfg, DMA_TRUE,
		       SREG_CFG_PHY_EIM_MASK, SREG_CFG_PHY_EIM_POS);
	MEM_WRITE_BITS(info->src_cfg, DMA_TRUE,
		       SREG_CFG_PHY_PEN_MASK, SREG_CFG_PHY_PEN_POS);
	MEM_WRITE_BITS(info->src_cfg, info->src_info.burst_size,
		       SREG_CFG_PHY_PSIZE_MASK, SREG_CFG_PHY_PSIZE_POS);
	MEM_WRITE_BITS(info->src_cfg, info->src_info.data_width,
		       SREG_CFG_PHY_ESIZE_MASK, SREG_CFG_PHY_ESIZE_POS);
	MEM_WRITE_BITS(info->src_cfg, info->priority,
		       SREG_CFG_PHY_PRI_MASK, SREG_CFG_PHY_PRI_POS);
	MEM_WRITE_BITS(info->src_cfg, info->src_info.endianess,
		       SREG_CFG_PHY_LBE_MASK, SREG_CFG_PHY_LBE_POS);
	MEM_WRITE_BITS(info->src_cfg, sync_type,
		       SREG_CFG_PHY_TM_MASK, SREG_CFG_PHY_TM_POS);
	MEM_WRITE_BITS(info->src_cfg, info->src_info.event_line,
		       SREG_CFG_PHY_EVTL_MASK, SREG_CFG_PHY_EVTL_POS);
	return 0;

}

/**
 * set_std_phy_dst_half_channel_params() - set params for dst phy half channel
 * @info:	Information of the channel parameters.
 * This function sets the channel Parameters for standard dest half channel.
 * Channel is configured as Physical channel. It maps sg info by calling
 * map_lli_info_phys_chan() for dst channel. If transfer length is more than
 * 64K elements, it calls calculate_lli_required() and generate_sg_list()
 * API's to create LLI's.
 * It returns 0 on success and -1 on failure
 *
 */
static int set_std_phy_dst_half_channel_params(struct dma_channel_info
					       *info)
{
	u32 step_size = 0;
	u32 num_elems;
	num_elems = info->dst_xfer_elem;
	if (info->dst_info.addr_inc)
		step_size = (0x1 << info->dst_info.data_width);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDCFG),
		    info->dst_cfg, FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDELT), 0x0,
		    FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDPTR), 0x0,
		    FULL32_MASK, NO_SHIFT);
	REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDLNK), 0x0,
		    FULL32_MASK, NO_SHIFT);

	if ((num_elems >= MAX_NUM_OF_ELEM_IN_A_XFER) || (info->sg_dest)) {
		u32 lli_size;
		u32 lli_count;
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDCFG),
			    DMA_FALSE, SREG_CFG_PHY_TIM_MASK,
			    SREG_CFG_PHY_TIM_POS);
		if (!info->sg_dest) {
			lli_count = calculate_lli_required(info, &lli_size);
			if (lli_count > NUM_SG_PER_REQUEST) {
				stm_error("Insufficient LLI's, \
					lli count very high = %d\n", lli_count);
				return -1;
			}
			if (!generate_sg_list
			    (info, DMA_DEST_HALF_CHANNEL, lli_size,
			     lli_count)) {
				stm_error("cannot generate sg list\n");
				return -1;
			}
		}
		print_sg_info(info->sg_dest, info->sgcount_dest);
		if (map_lli_info_phys_chan(info, DMA_DEST_HALF_CHANNEL))
			return -1;
	} else {
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDELT),
			    num_elems, SREG_ELEM_PHY_ECNT_MASK,
			    SREG_ELEM_PHY_ECNT_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDELT),
			    step_size, SREG_ELEM_PHY_EIDX_MASK,
			    SREG_ELEM_PHY_EIDX_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDPTR),
			    (u32) info->dst_addr, FULL32_MASK, NO_SHIFT);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDLNK),
			    0x1, FULL32_MASK, NO_SHIFT);
	}
	return 0;
}

static int set_std_phy_dst_half_channel_params_mem(struct dma_channel_info
						   *info)
{
	enum dma_master_id master_id;
	enum dma_half_chan_sync sync_type;

	info->dst_cfg = 0;

	if ((info->dir == MEM_TO_PERIPH) || (info->dir == PERIPH_TO_PERIPH)) {
		master_id = DMA_MASTER_1;
		sync_type = DMA_PACKET_SYNC;
		info->dst_info.addr_inc = DMA_ADR_NOINC;
	} else {
		master_id = DMA_MASTER_0;
		sync_type = DMA_NO_SYNC;
	}

	MEM_WRITE_BITS(info->dst_cfg, master_id,
		       SREG_CFG_PHY_MST_MASK, SREG_CFG_PHY_MST_POS);
	MEM_WRITE_BITS(info->dst_cfg, DMA_TRUE,
		       SREG_CFG_PHY_TIM_MASK, SREG_CFG_PHY_TIM_POS);
	MEM_WRITE_BITS(info->dst_cfg, DMA_TRUE,
		       SREG_CFG_PHY_EIM_MASK, SREG_CFG_PHY_EIM_POS);
	MEM_WRITE_BITS(info->dst_cfg, DMA_TRUE,
		       SREG_CFG_PHY_PEN_MASK, SREG_CFG_PHY_PEN_POS);
	MEM_WRITE_BITS(info->dst_cfg, info->dst_info.burst_size,
		       SREG_CFG_PHY_PSIZE_MASK, SREG_CFG_PHY_PSIZE_POS);
	MEM_WRITE_BITS(info->dst_cfg, info->dst_info.data_width,
		       SREG_CFG_PHY_ESIZE_MASK, SREG_CFG_PHY_ESIZE_POS);
	MEM_WRITE_BITS(info->dst_cfg, info->priority,
		       SREG_CFG_PHY_PRI_MASK, SREG_CFG_PHY_PRI_POS);
	MEM_WRITE_BITS(info->dst_cfg, info->dst_info.endianess,
		       SREG_CFG_PHY_LBE_MASK, SREG_CFG_PHY_LBE_POS);
	MEM_WRITE_BITS(info->dst_cfg, sync_type,
		       SREG_CFG_PHY_TM_MASK, SREG_CFG_PHY_TM_POS);
	MEM_WRITE_BITS(info->dst_cfg, info->dst_info.event_line,
		       SREG_CFG_PHY_EVTL_MASK, SREG_CFG_PHY_EVTL_POS);

	return 0;

}

/**
 * set_std_phy_channel_params() -  maps the cfg params for std phy channel
 * @info:	Information of the channel parameters.
 * This function maps the configuration parameters of source and dest
 * half channels on to the registers by calling function
 * set_std_phy_src_half_channel_params() for source half channel and
 * set_std_phy_dst_half_channel_params() for dest half channel.
 * This function returns 0 on success and -1 on failure.
 *
 */
static int set_std_phy_channel_params(struct dma_channel_info *info)
{
	if (set_std_phy_src_half_channel_params(info)) {
		stm_error("Unable to map source half channel params");
		return -1;
	}
	if (set_std_phy_dst_half_channel_params(info)) {
		stm_error("Unable to map destination half channel params");
		return -1;
	}
	return 0;
}

static int set_std_phy_channel_params_mem(struct dma_channel_info *info)
{
	if (set_std_phy_src_half_channel_params_mem(info)) {
		stm_error("Unable to map source half channel params");
		return -1;
	}
	if (set_std_phy_dst_half_channel_params_mem(info)) {
		stm_error("Unable to map destination half channel params");
		return -1;
	}
	return 0;
}

/**
 * set_std_log_dst_half_channel_params() - map channel params for dest
 * logical half channel
 * @info:	Information of the channel parameters.
 * This function sets the channel Parameters for standard Destination
 * half channel.
 * Channel is configured as Logical channel. It maps sg info by calling
 * map_lli_info_log_chan() for destination half channel. If transfer
 * length is more than 64K elements, it calls calculate_lli_required()
 * and generate_sg_list() API's to create LLI's. Activate Event Line
 * if destination is a peripheral.
 * This function returns 0 on success and -1 on failure.
 *
 */
static int set_std_log_dst_half_channel_params(struct dma_channel_info
					       *info)
{
	u32 num_elems;
	struct std_log_memory_param *params;

	num_elems = info->dst_xfer_elem;

	if (info->dir == MEM_TO_PERIPH) {
		params =
		    (struct std_log_memory_param
		     *)((void *)(dma_drv_data->lchan_params_base.log_addr)
			+ (info->dst_dev_type * 32) + 16);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDLNK),
			    ACTIVATE_EVENTLINE,
			    EVENTLINE_MASK(info->dst_info.event_line),
			    EVENTLINE_POS(info->dst_info.event_line));
	} else {
		params =
		    (struct std_log_memory_param
		     *)((void *)(dma_drv_data->lchan_params_base.log_addr)
			+ (info->src_dev_type * 32));
	}
	params->dmac_lcsp3 = info->dmac_lcsp3;
	if (info->invalid) {
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDCFG),
			    info->priority, SREG_CFG_LOG_PRI_MASK,
			    SREG_CFG_LOG_PRI_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDCFG),
			    info->dst_info.endianess,
			    SREG_CFG_LOG_LBE_MASK, SREG_CFG_LOG_LBE_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDCFG),
			    0x1, SREG_CFG_LOG_GIM_MASK, SREG_CFG_LOG_GIM_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SDCFG),
			    DMA_MASTER_0, SREG_CFG_LOG_MFU_MASK,
			    SREG_CFG_LOG_MFU_POS);
	}

	if ((num_elems >= MAX_NUM_OF_ELEM_IN_A_XFER) || (info->sg_dest)) {
		u32 lli_size;
		u32 lli_count;
		MEM_WRITE_BITS(params->dmac_lcsp3, DMA_FALSE,
			       MEM_LCSP3_DCFG_TIM_MASK, MEM_LCSP3_DCFG_TIM_POS);
		if (!info->sg_dest) {
			lli_count = calculate_lli_required(info, &lli_size);
			if (lli_count > NUM_LLI_PER_LOG_CHANNEL + 1) {
				stm_error("Insufficient LLI's, \
				lli count very high = %d\n", lli_count);
				return -1;
			}
			if (!generate_sg_list
			    (info, DMA_DEST_HALF_CHANNEL, lli_size,
			     lli_count)) {
				stm_error("cannot generate sg list\n");
				return -1;
			}
		}
		print_sg_info(info->sg_dest, info->sgcount_dest);
		if (map_lli_info_log_chan(info, DMA_DEST_HALF_CHANNEL, params))
			return -1;
	} else {
		MEM_WRITE_BITS(params->dmac_lcsp2, num_elems,
			       MEM_LCSP2_ECNT_MASK, MEM_LCSP2_ECNT_POS);
		MEM_WRITE_BITS(params->dmac_lcsp2,
			       (0x0000FFFFUL & (u32) (info->dst_addr)),
			       MEM_LCSP2_DPTR_MASK, MEM_LCSP2_DPTR_POS);
		MEM_WRITE_BITS(params->dmac_lcsp3,
			       (0xFFFF0000UL & (u32) (info->dst_addr)) >> 16,
			       MEM_LCSP3_DPTR_MASK, MEM_LCSP3_DPTR_POS);
		MEM_WRITE_BITS(params->dmac_lcsp3, 0x1, MEM_LCSP3_DTCP_MASK,
			       MEM_LCSP3_DTCP_POS);
	}
	return 0;
}

static int set_std_log_dst_half_channel_params_mem(struct dma_channel_info
						   *info)
{

	enum dma_master_id master_id;
	int tim;
	int increment_addr;
	if (info->dir == MEM_TO_PERIPH) {
		master_id = DMA_MASTER_1;
		tim = DMA_TRUE;
		increment_addr = DMA_FALSE;
	} else {
		master_id = DMA_MASTER_0;
		tim = DMA_TRUE;
		increment_addr = DMA_TRUE;
	}
	MEM_WRITE_BITS(info->dmac_lcsp3, master_id, MEM_LCSP3_DCFG_MST_MASK,
		       MEM_LCSP3_DCFG_MST_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, tim, MEM_LCSP3_DCFG_TIM_MASK,
		       MEM_LCSP3_DCFG_TIM_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, DMA_TRUE,
		       MEM_LCSP3_DCFG_EIM_MASK, MEM_LCSP3_DCFG_EIM_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, increment_addr,
		       MEM_LCSP3_DCFG_INCR_MASK, MEM_LCSP3_DCFG_INCR_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, info->dst_info.burst_size,
		       MEM_LCSP3_DCFG_PSIZE_MASK, MEM_LCSP3_DCFG_PSIZE_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, info->dst_info.data_width,
		       MEM_LCSP3_DCFG_ESIZE_MASK, MEM_LCSP3_DCFG_ESIZE_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, 0x0, MEM_LCSP3_DLOS_MASK,
		       MEM_LCSP3_DLOS_POS);
	MEM_WRITE_BITS(info->dmac_lcsp3, 0x1, MEM_LCSP3_DTCP_MASK,
		       MEM_LCSP3_DTCP_POS);

	return 0;

}

/**
 * set_std_log_src_half_channel_params() - map channel params for src
 * logical half channel
 * @info:	Information of the channel parameters.
 * This function sets the channel Parameters for standard src half channel.
 * Channel is configured as Logical channel. It maps sg info by calling
 * map_lli_info_log_chan() for source half channel. If transfer
 * length is more than 64K elements, it calls calculate_lli_required()
 * and generate_sg_list() API's to create LLI's. Activate Event Line
 * if source is a peripheral.
 * This function returns 0 on success and -1 on failure.
 *
 */
static int set_std_log_src_half_channel_params(struct dma_channel_info
					       *info)
{
	u32 num_elems;
	struct std_log_memory_param *params = NULL;

	num_elems = info->src_xfer_elem;

	if (info->dir == PERIPH_TO_MEM) {
		params =
		    (struct std_log_memory_param
		     *)((void *)(dma_drv_data->lchan_params_base.log_addr)
			+ (info->src_dev_type * 32));
		stm_dbg(DBG_ST.dma, "Params address : %x dev_indx = %d \n",
			(u32) params, info->src_dev_type);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSLNK),
			    ACTIVATE_EVENTLINE,
			    EVENTLINE_MASK(info->src_info.event_line),
			    EVENTLINE_POS(info->src_info.event_line));
	} else {
		params =
		    (struct std_log_memory_param
		     *)((void *)(dma_drv_data->lchan_params_base.log_addr)
			+ (info->dst_dev_type * 32) + 16);
	}
	params->dmac_lcsp1 = info->dmac_lcsp1;
	if (info->invalid) {
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSCFG),
			    info->priority, SREG_CFG_LOG_PRI_MASK,
			    SREG_CFG_LOG_PRI_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSCFG),
			    info->src_info.endianess,
			    SREG_CFG_LOG_LBE_MASK, SREG_CFG_LOG_LBE_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSCFG),
			    0x1, SREG_CFG_LOG_GIM_MASK, SREG_CFG_LOG_GIM_POS);
		REG_WR_BITS(cio_addr(info->phys_chan_id, CHAN_REG_SSCFG),
			    DMA_MASTER_0, SREG_CFG_LOG_MFU_MASK,
			    SREG_CFG_LOG_MFU_POS);
	}
	if ((num_elems >= MAX_NUM_OF_ELEM_IN_A_XFER) || (info->sg_src)) {
		u32 lli_size;
		u32 lli_count;
		MEM_WRITE_BITS(params->dmac_lcsp1, DMA_FALSE,
			       MEM_LCSP1_SCFG_TIM_MASK, MEM_LCSP1_SCFG_TIM_POS);
		if (!info->sg_src) {
			lli_count = calculate_lli_required(info, &lli_size);
			if (lli_count > NUM_LLI_PER_LOG_CHANNEL + 1) {
				stm_error("Insufficient LLI's, \
					lli count very high = %d\n", lli_count);
				return -1;
			}
			if (!generate_sg_list
			    (info, DMA_SRC_HALF_CHANNEL, lli_size, lli_count)) {
				stm_error("cannot generate sg list\n");
				return -1;
			}
		}
		print_sg_info(info->sg_src, info->sgcount_src);
		if (map_lli_info_log_chan(info, DMA_SRC_HALF_CHANNEL, params)) {
			stm_error("Unable to map SG info on \
			    LLI for src half channel");
			return -1;
		}
	} else {
		MEM_WRITE_BITS(params->dmac_lcsp0, num_elems,
			       MEM_LCSP0_ECNT_MASK, MEM_LCSP0_ECNT_POS);
		MEM_WRITE_BITS(params->dmac_lcsp0,
			       (0x0000FFFFUL & (u32) (info->src_addr)),
			       MEM_LCSP0_SPTR_MASK, MEM_LCSP0_SPTR_POS);
		MEM_WRITE_BITS(params->dmac_lcsp1,
			       (0xFFFF0000 & (u32) (info->src_addr)) >> 16,
			       MEM_LCSP1_SPTR_MASK, MEM_LCSP1_SPTR_POS);
	}
	return 0;
}

static int set_std_log_src_half_channel_params_mem(struct dma_channel_info
						   *info)
{

	enum dma_master_id master_id;
	int tim;
	int increment_addr;
	if (info->dir == PERIPH_TO_MEM) {
		master_id = DMA_MASTER_1;
		tim = DMA_FALSE;
		increment_addr = DMA_FALSE;
	} else {
		master_id = DMA_MASTER_0;
		tim = DMA_FALSE;
		increment_addr = DMA_TRUE;
	}
	MEM_WRITE_BITS(info->dmac_lcsp1, master_id, MEM_LCSP1_SCFG_MST_MASK,
		       MEM_LCSP1_SCFG_MST_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, tim, MEM_LCSP1_SCFG_TIM_MASK,
		       MEM_LCSP1_SCFG_TIM_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, DMA_TRUE,
		       MEM_LCSP1_SCFG_EIM_MASK, MEM_LCSP1_SCFG_EIM_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, increment_addr,
		       MEM_LCSP1_SCFG_INCR_MASK, MEM_LCSP1_SCFG_INCR_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, info->src_info.burst_size,
		       MEM_LCSP1_SCFG_PSIZE_MASK, MEM_LCSP1_SCFG_PSIZE_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, info->src_info.data_width,
		       MEM_LCSP1_SCFG_ESIZE_MASK, MEM_LCSP1_SCFG_ESIZE_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, 0x0, MEM_LCSP1_SLOS_MASK,
		       MEM_LCSP1_SLOS_POS);
	MEM_WRITE_BITS(info->dmac_lcsp1, 0x1, MEM_LCSP1_STCP_MASK,
		       MEM_LCSP1_STCP_POS);

	return 0;

}

/**
 * set_std_log_channel_params() - maps cfg params for std logical channel
 * @info:	Information of the channel parameters.
 *
 */
static int set_std_log_channel_params(struct dma_channel_info *info)
{
	if (set_std_log_src_half_channel_params(info))
		return -1;
	if (set_std_log_dst_half_channel_params(info))
		return -1;
	return 0;
}

static int set_std_log_channel_params_mem(struct dma_channel_info *info)
{
	if (set_std_log_src_half_channel_params_mem(info))
		return -1;
	if (set_std_log_dst_half_channel_params_mem(info))
		return -1;
	return 0;
}

/**
 * process_dma_pipe_info() - process the DMA channel info sent by user
 * @pipe_id:	Channel Id for which this information is to be set
 * @pipe_info:	Information struct of the given channel given by user
 * This function processes info supplied by the client driver. It stores
 * the required info in to it;s internal data structure.
 * It returns 0 on success and -1 on failure
 *
 */
static int process_dma_pipe_info(struct dma_channel_info *info,
				 struct stm_dma_pipe_info *pipe_info)
{
	unsigned long flags;

	if (info && pipe_info) {
		info->invalid = 1;
		spin_lock_irqsave(&info->cfg_lock, flags);
		if (info->active)
			stm_dbg(DBG_ST.dma,
				"Modifying the info while channel active..\n");
		info->pr_type =
		    MEM_READ_BITS(pipe_info->channel_type, 0x3,
				  INFO_CH_TYPE_POS);
		info->priority =
		    MEM_READ_BITS(pipe_info->channel_type,
				  (0x3 << INFO_PRIO_TYPE_POS),
				  INFO_PRIO_TYPE_POS);
		info->security =
		    MEM_READ_BITS(pipe_info->channel_type,
				  (0x3 << INFO_SEC_TYPE_POS),
				  INFO_SEC_TYPE_POS);
		info->chan_mode =
		    MEM_READ_BITS(pipe_info->channel_type,
				  (0x3 << INFO_CH_MODE_TYPE_POS),
				  INFO_CH_MODE_TYPE_POS);
		info->mode_option =
		    MEM_READ_BITS(pipe_info->channel_type,
				  (0x3 << INFO_CH_MODE_OPTION_POS),
				  INFO_CH_MODE_OPTION_POS);
		info->link_type =
		    MEM_READ_BITS(pipe_info->channel_type,
				  (0x1 << INFO_LINK_TYPE_POS),
				  INFO_LINK_TYPE_POS);
		info->lli_interrupt =
		    MEM_READ_BITS(pipe_info->channel_type,
				  (0x1 << INFO_TIM_POS), INFO_TIM_POS);
		info->dir = pipe_info->dir;
		info->src_dev_type = pipe_info->src_dev_type;
		info->dst_dev_type = pipe_info->dst_dev_type;
		info->src_info.endianess = pipe_info->src_info.endianess;
		info->src_info.data_width = pipe_info->src_info.data_width;
		info->src_info.burst_size = pipe_info->src_info.burst_size;
		info->src_info.buffer_type = pipe_info->src_info.buffer_type;
		if (info->src_dev_type != DMA_DEV_SRC_MEMORY) {
			info->src_info.event_group = info->src_dev_type / 16;
			info->src_info.event_line = info->src_dev_type % 16;
		} else {
			info->src_info.event_group = 0;
			info->src_info.event_line = 0;
		}
		info->dst_info.endianess = pipe_info->dst_info.endianess;
		info->dst_info.data_width = pipe_info->dst_info.data_width;
		info->dst_info.burst_size = pipe_info->dst_info.burst_size;
		info->dst_info.buffer_type = pipe_info->dst_info.buffer_type;
		if (info->dst_dev_type != DMA_DEV_DEST_MEMORY) {
			info->dst_info.event_group = info->dst_dev_type / 16;
			info->dst_info.event_line = info->dst_dev_type % 16;
		} else {
			info->dst_info.event_group = 0;
			info->dst_info.event_line = 0;
		}

		if (info->dir == MEM_TO_MEM) {
			info->src_info.addr_inc = pipe_info->src_info.addr_inc;
			info->dst_info.addr_inc = pipe_info->dst_info.addr_inc;
		}

		print_dma_channel_info(info);
		if (info->pr_type == DMA_STANDARD) {
			if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
				if (set_std_phy_channel_params_mem(info)) {
					stm_error("Could not set parameters \
						for channel in phy mode\n");
				}
			} else if (info->chan_mode == DMA_CHAN_IN_LOG_MODE) {
				if (set_std_log_channel_params_mem(info)) {
					stm_error
					    ("Could not set parameters for \
						channel in Log mode\n");
				}
			} else {
				stm_error("Operation Mode not yet supported");
			}
		} else {	/*DMA_EXTENDED */
			stm_error("Extended Channels not supported");
		}
		spin_unlock_irqrestore(&info->cfg_lock, flags);
		return 0;
	} else {
		stm_error("NULL info structure\n");
		return -1;
	}
}

static inline int validate_pipe_info(struct stm_dma_pipe_info *info)
{
	if (!info) {
		stm_error("Info is Null");
		return -1;
	}
	if (0x0 == MEM_READ_BITS(info->channel_type, 0x3, INFO_CH_TYPE_POS)) {
		stm_error("Configuration wrong:: \
		    Physical resource type incorrect\n");
		return -1;
	}
	if (0x0 == MEM_READ_BITS(info->channel_type,
				 (0x3 << INFO_PRIO_TYPE_POS),
				 INFO_PRIO_TYPE_POS)) {
		stm_error("Configuration wrong:: incorrect Priority...\n");
		return -1;
	}
	if (0x0 == MEM_READ_BITS(info->channel_type,
				 (0x3 << INFO_SEC_TYPE_POS),
				 INFO_SEC_TYPE_POS)) {
		stm_error("Configuration wrong: incorrect security type\n");
		return -1;
	}
	if (0x0 == MEM_READ_BITS(info->channel_type,
				 (0x3 << INFO_CH_MODE_TYPE_POS),
				 INFO_CH_MODE_TYPE_POS)) {
		stm_error("Configuration wrong:: Wrong channel mode\n");
		return -1;
	}
	if (0x0 == MEM_READ_BITS(info->channel_type,
				 (0x3 << INFO_CH_MODE_OPTION_POS),
				 INFO_CH_MODE_OPTION_POS)) {
		stm_error("Configuration wrong:: incorrect mode option\n");

		return -1;
	}
	return 0;
}

/**
 * get_inititalized_info_structure() - Allocate a new channel info structure
 * Returns pointer inititalized info structure(struct dma_channel_info  *),
 * or NULL in case of error
 */
static struct dma_channel_info *get_inititalized_info_structure(void)
{
	struct dma_channel_info *info = NULL;
	info = (struct dma_channel_info *)
	    kzalloc(sizeof(struct dma_channel_info), GFP_KERNEL);
	if (!info) {
		stm_error("Could not allocate memory for \
			info structure for new channel\n");
		return NULL;
	}
	info->phys_chan_id = DMA_CHAN_NOT_ALLOCATED;
	info->channel_id = -1;
	info->lli_block_id_src = -1;
	info->lli_block_id_dest = -1;
	info->sg_block_id_src = -1;
	info->sg_block_id_dest = -1;
	info->sg_dest = NULL;
	info->sg_src = NULL;
	info->current_sg = NULL;
	info->active = 0;
	info->invalid = 1;
	info->dmac_lcsp3 = 0;
	info->dmac_lcsp1 = 0;
	info->src_info.addr_inc = DMA_ADR_INC;
	info->dst_info.addr_inc = DMA_ADR_INC;
	spin_lock_init(&info->cfg_lock);
	return info;
}

/**
 * stm_request_dma() - API for requesting a DMA channel used by
 * client driver
 * @channel:	Channel Id which is returned to client on succefull allocation
 * @pipe_info:	Information struct of the given channel given by user
 * This API is called by the client driver to request for a channel. On
 * successful allocation of channel 0 is returned. This API first allocates
 * a new info structure for the channel(used by Driver to maintain internal
 * state for this pipe). A new pipe_id is allocated for this request.The channel
 * information as requested by client in pipe_info structure are then verified
 * and copied in the info structure we allocated using process_dma_pipe_info().
 * If client requests to reserve channel, it is done at this point by calling
 * acquire_physical_resource(). Reference for this info structure are then
 * stored in pipe_info array. If channel has been reserved then entry is also
 * stored in dma_chan_info array.
 *
 */
int stm_request_dma(int *channel, struct stm_dma_pipe_info *pipe_info)
{
	struct dma_channel_info *new_info;
	int pipe_id = -1;
	int retval = 0;

	if (validate_pipe_info(pipe_info)) {
		*channel = -1;
		return -EINVAL;
	}

	new_info = get_inititalized_info_structure();

	if (!new_info) {
		*channel = -1;
		return -ENOMEM;
	}

	pipe_id = allocate_pipe_id();

	if (pipe_id >= 0) {
		*channel = pipe_id;
		new_info->pipe_id = pipe_id;

		if (process_dma_pipe_info(new_info, pipe_info)) {
			retval = -EINVAL;
			goto error_request;
		}
		if (pipe_info->reserve_channel) {
			allocate_free_dma_channel(new_info);
			if (new_info->phys_chan_id == -1) {
				retval = -EINVAL;
				goto error_request;
			}
		}
		dma_drv_data->pipe_info[pipe_id] = new_info;
		if (new_info->channel_id >= 0) {
			/*i.e. for reserved channels only */
			dma_drv_data->dma_chan_info[new_info->channel_id] =
			    new_info;
		}

	} else {
		retval = -EAGAIN;
		goto error_pipeid;
	}
	return retval;

error_request:
	deallocate_pipe_id(pipe_id);
error_pipeid:
	*channel = -1;
	kfree(new_info);
	return retval;
}
EXPORT_SYMBOL(stm_request_dma);

/**
 * stm_set_callback_handler() - To set a callback handler for the
 * given channel
 * @pipe_id:	pipe id allocated to client driver from the call
 * stm_request_dma()
 * @callback_handler: Callback function to be invoked in case of
 * completion or error on xfer
 * @data:	Data pointer with which this callback function must be invoked.
 * This function returns 0 on success, -1 on error.
 * This callback handler you set here would be invoked when Xfer is complete.
 * Calling this function is optional.
 * If you do not call this function then you need to poll using
 * stm_dma_residue() API to check whether you DMA xfer has completed or not
 *
 */
int stm_set_callback_handler(int pipe_id, void *callback_handler,
				 void *data)
{
	struct dma_channel_info *info;
	unsigned long flags;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return -1;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (info) {
		if (info->active)
			stm_dbg(DBG_ST.dma,
				"Modifying the info while channel active..\n");
		spin_lock_irqsave(&info->cfg_lock, flags);
		info->callback = callback_handler;
		info->data = data;
		spin_unlock_irqrestore(&info->cfg_lock, flags);
		return 0;
	} else {
		stm_error("Info structure for this pipe is unavailable\n");
		return -EINVAL;
	}
}
EXPORT_SYMBOL(stm_set_callback_handler);

/**
 * stm_configure_dma_channel() - To configure a DMA pipe after it
 * has been requested
 * @pipe_id:	pipe id allocated to client driver from the call
 * stm_request_dma()
 * @pipe_info:	Updated pipe information structure.
 * This function returns 0 on success, negative value on error.
 * Calling this function is optional if you have already configured
 * your pipe during call to API stm_request_dma().
 * It is required only if you need to update params.
 *
 */
int stm_configure_dma_channel(int pipe_id,
				  struct stm_dma_pipe_info *pipe_info)
{
	int retval = 0;
	struct dma_channel_info *info;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return -1;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return -1;
	}

	retval = process_dma_pipe_info(info, pipe_info);
	if (retval)
		return -EINVAL;
	else
		return 0;
}
EXPORT_SYMBOL(stm_configure_dma_channel);

/**
 * stm_set_dma_addr() - To set source and destination address for
 * the given DMA channel
 * @pipe_id:	pipe_id for which dma addresses have to be set
 * @src_addr:	Source address from where DMA will start
 * @dst_addr:	Dest address at which DMA controller will copy data
 *
 */
void stm_set_dma_addr(int pipe_id, void *src_addr, void *dst_addr)
{
	struct dma_channel_info *info;
	unsigned long flags = 0;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return;
	}
	if (info->active)
		stm_dbg(DBG_ST.dma,
			"Modifying the info while channel active..\n");

	spin_lock_irqsave(&info->cfg_lock, flags);
	info->src_addr = src_addr;
	info->dst_addr = dst_addr;
	spin_unlock_irqrestore(&info->cfg_lock, flags);
}
EXPORT_SYMBOL(stm_set_dma_addr);

/**
 * stm_set_dma_count() - To set dma count(number of bytes to transfer)
 * @pipe_id:	client Pipe id for which count to transfer has to be set
 * @count:	Number of bytes to xfer
 *
 */
void stm_set_dma_count(int pipe_id, int count)
{
	struct dma_channel_info *info;
	unsigned long flags = 0;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return;
	}
	if (info->active)
		stm_dbg(DBG_ST.dma,
			"Modifying the info while channel active..\n");
	spin_lock_irqsave(&info->cfg_lock, flags);
	info->xfer_len = count;
	info->src_xfer_elem = count >> info->src_info.data_width;
	info->dst_xfer_elem = count >> info->dst_info.data_width;
	spin_unlock_irqrestore(&info->cfg_lock, flags);
}
EXPORT_SYMBOL(stm_set_dma_count);

/**
 * stm_set_dma_sg() - To set the SG list for a given pipe id.
 * @pipe_id:	client Pipe id for which sg list has to be set
 * @sg:		sg  List to be set
 * @nr_sg:	Number of items in SG list
 * @type:	Whether sg list is for src or destination(0 - src, 1 - dst)
 *
 */
void stm_set_dma_sg(int pipe_id, struct scatterlist *sg, int nr_sg,
			int type)
{
	struct dma_channel_info *info;
	unsigned long flags = 0;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return;
	}
	if (info->active)
		stm_dbg(DBG_ST.dma,
			"Modifying the info while channel active..\n");
	spin_lock_irqsave(&info->cfg_lock, flags);
	if (type == DMA_SRC_HALF_CHANNEL) {
		info->sg_src = sg;
		info->sgcount_src = nr_sg;
	} else {
		info->sg_dest = sg;
		info->sgcount_dest = nr_sg;
	}
	spin_unlock_irqrestore(&info->cfg_lock, flags);
}
EXPORT_SYMBOL(stm_set_dma_sg);

/**
 * stm_enable_dma() - To enable a DMA channel
 * @pipe_id:	client Id whose DMA channel has to be enabled
 * Unless DMA channel was reserved, at this point allocation for a
 * physical resource is done on which this request would run.
 * Once a physical resource is allocated, corresponding DMA channel
 * parameters are loaded on to the DMA registers, and physical
 * resource is put in a running state.
 * It returns 0 on success, -1 on error
 *
 */
int stm_enable_dma(int pipe_id)
{
	struct dma_channel_info *info = NULL;
	unsigned long flags;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return -1;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return -1;
	}
	if ((info->phys_chan_id) == DMA_CHAN_NOT_ALLOCATED) {
		if (allocate_free_dma_channel(info)) {
			stm_error("stm_enable_dma():: \
					No free DMA channel available.....\n");
			return -1;
		}
		dma_drv_data->dma_chan_info[info->channel_id] = info;
	}
	stm_dbg(DBG_ST.dma, "info->phys_chan_id Allocated = %d\n",
		(info->phys_chan_id));

	spin_lock_irqsave(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			  flags);
#if 0
	if (dma_channel_execute_command
	    (DMA_SUSPEND_REQ, info->phys_chan_id) == -1) {
		stm_error("Unable to suspend the Channel %d\n",
			  info->phys_chan_id);
		goto error_enable_dma;
	}
	stm_error("--->suspending the Channel %d\n", info->phys_chan_id);

#endif
	if (info->invalid) {
		if (info->chan_mode == DMA_CHAN_IN_LOG_MODE) {
			if (!dma_drv_data->pr_info[info->phys_chan_id].dirty) {
				set_phys_res_type(info->phys_chan_id,
						  info->pr_type);
				set_phys_res_mode(info->phys_chan_id,
						  info->chan_mode);
				set_phys_res_mode_option(info->phys_chan_id,
							 info->mode_option);
				set_phys_res_security(info->phys_chan_id,
						      info->security);
				/*Resource is Dirty now */
				dma_drv_data->pr_info[info->
						      phys_chan_id].dirty = 1;
			}
			if ((info->dir == PERIPH_TO_PERIPH)
			    || (info->dir == PERIPH_TO_MEM)) {
				set_event_line_security(info->src_info.
							event_line,
							info->src_info.
							event_group,
							info->security);
			}
			if ((info->dir == PERIPH_TO_PERIPH)
			    || (info->dir == MEM_TO_PERIPH))
				set_event_line_security((info->dst_info.
							 event_line + 16),
							info->dst_info.
							event_group,
							info->security);
		} else {	/*Channel in Physical mode */

			set_phys_res_type(info->phys_chan_id, info->pr_type);
			set_phys_res_mode(info->phys_chan_id, info->chan_mode);
			set_phys_res_mode_option(info->phys_chan_id,
						 info->mode_option);
			set_phys_res_security(info->phys_chan_id,
					      info->security);

			if ((info->dir == PERIPH_TO_PERIPH)
			    || (info->dir == PERIPH_TO_MEM)) {
				set_event_line_security(info->src_info.
							event_line,
							info->src_info.
							event_group,
							info->security);
			}
			if ((info->dir == PERIPH_TO_PERIPH)
			    || (info->dir == MEM_TO_PERIPH))
				set_event_line_security((info->dst_info.
							 event_line + 16),
							info->dst_info.
							event_group,
							info->security);

		}
	}
	if (info->pr_type == DMA_STANDARD) {
		if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
			if (set_std_phy_channel_params(info)) {
				stm_error("Could not set parameters \
					for channel in phy mode\n");
				goto error_enable_dma;
			}
		} else if (info->chan_mode == DMA_CHAN_IN_LOG_MODE) {
			if (dma_channel_execute_command
			    (DMA_SUSPEND_REQ, info->phys_chan_id) == -1) {
				stm_error("Unable to suspend the Channel %d\n",
					  info->phys_chan_id);
				goto error_enable_dma;
			}
			if (set_std_log_channel_params(info)) {
				stm_error("Could not set parameters \
					for channel in Log mode\n");
				goto error_enable_dma;
			}
		} else {
			stm_error("Operation Mode not yet supported");
			goto error_enable_dma;
		}
	} else {		/*DMA_EXTENDED */
		stm_error("Extended Channels not supported");
		goto error_enable_dma;
	}
	/*Now after doing all necessary settings put the channel in RUN state */
	info->active = 1;
	info->invalid = 0;
	dma_channel_execute_command(DMA_RUN, info->phys_chan_id);
	spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			       flags);

	return 0;

error_enable_dma:
	spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			       flags);
	return -1;
}
EXPORT_SYMBOL(stm_enable_dma);

/**
 * halt_dma_channel() - To halt a Running DMA channel.
 * @pipe_id:	client Id whose DMA channel has to be halted
 * We suspend the channel first. For Physical channel we just
 * stop it and return. For Logical channels, we check if some other
 * Logical channel is still running(Other than one we halted),
 * then we again put that physical resource in run state.
 *
 */
static int halt_dma_channel(int pipe_id)
{
	struct dma_channel_info *info = NULL;

	info = dma_drv_data->pipe_info[pipe_id];
	if (dma_channel_execute_command(DMA_SUSPEND_REQ, info->phys_chan_id) ==
	    -1) {
		stm_error("Unable to suspend the Channel %d\n",
			  info->phys_chan_id);
		return -1;
	}
	if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
		if (dma_channel_execute_command(DMA_STOP, info->phys_chan_id) ==
		    -1) {
			stm_error("Unable to stop the Channel %d\n",
				  info->phys_chan_id);
			return -1;
		}
	} else if (info->chan_mode == DMA_CHAN_IN_LOG_MODE) {
		/*Deactivate the event line during
		 * the time physical res is suspended */
		print_channel_reg_params(info->phys_chan_id);

		if (info->dir == MEM_TO_PERIPH) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SDLNK),
				    DEACTIVATE_EVENTLINE,
				    EVENTLINE_MASK(info->dst_info.event_line),
				    EVENTLINE_POS(info->dst_info.event_line));
		} else if (info->dir == PERIPH_TO_MEM) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SSLNK),
				    DEACTIVATE_EVENTLINE,
				    EVENTLINE_MASK(info->src_info.event_line),
				    EVENTLINE_POS(info->src_info.event_line));
		} else {
			stm_dbg(DBG_ST.dma, "Unknown request\n");
		}
		print_channel_reg_params(info->phys_chan_id);

		if (get_phy_res_usage_count(info->phys_chan_id) <= 1) {
			stm_dbg(DBG_ST.dma,
				"Stopping the Logical channel..............\n");
			if (dma_channel_execute_command
			    (DMA_STOP, info->phys_chan_id) == -1) {
				stm_error("Unable to stop the Channel %d\n",
					  info->phys_chan_id);
				return -1;
			}
			/*info->chan_mode = DMA_CHAN_IN_PHYS_MODE; */
		} else {
			stm_dbg(DBG_ST.dma,
				"since some other Logical channel might "
				"be using it" "putting "
				"it to running state again..\n");
			dma_channel_execute_command(DMA_RUN,
						    info->phys_chan_id);
		}
	} else {
		stm_error
		    ("stm_disable_dma()::Operation mode not supported\n");
	}
	info->active = 0;
	return 0;
}

/**
 * stm_disable_dma() - API to disable a DMA channel
 * @pipe_id:	client id whose channel has to be disabled
 * This function halts the concerned DMA channel and returns.
 *
 */
void stm_disable_dma(int pipe_id)
{
	struct dma_channel_info *info = NULL;
	unsigned long flags;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return;
	}

	info = dma_drv_data->pipe_info[pipe_id];
	if (info->phys_chan_id != DMA_CHAN_NOT_ALLOCATED) {
		spin_lock_irqsave(&dma_drv_data->cfg_ch_lock
				  [info->phys_chan_id], flags);
		if (halt_dma_channel(pipe_id))
			stm_error("Unable to Halt DMA pipe_id :: %d\n",
				  pipe_id);
		spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock
				       [info->phys_chan_id], flags);
	}
}
EXPORT_SYMBOL(stm_disable_dma);

/**
 * stm_free_dma() - API to release a DMA channel
 * @pipe_id:	id whose channel has to be freed
 * This function first halts the channel, then the resources
 * held by the channel are released and pipe deallocated.
 *
 */
void stm_free_dma(int pipe_id)
{
	struct dma_channel_info *info = NULL;
	unsigned long flags;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id %d", pipe_id);
		return;
	}

	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return;
	}
	stm_dbg(DBG_ST.dma, "Trying to free DMA pipe_id %d\n", pipe_id);

	if (info->phys_chan_id != DMA_CHAN_NOT_ALLOCATED) {
		spin_lock_irqsave(&dma_drv_data->cfg_ch_lock
				  [info->phys_chan_id], flags);
		if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
			if (halt_dma_channel(pipe_id)) {
				stm_error("Unable to Halt DMA pipe_id :: %d\n",
					  pipe_id);
				spin_unlock_irqrestore
				    (&dma_drv_data->cfg_ch_lock
				     [info->phys_chan_id], flags);
				return;
			}
			if (info->lli_block_id_src >= 0) {
				release_block(info->lli_block_id_src,
					      dma_drv_data->pchan_lli_pool);
				info->lli_block_id_src = -1;
			}
			if (info->lli_block_id_dest >= 0) {
				release_block(info->lli_block_id_dest,
					      dma_drv_data->pchan_lli_pool);
				info->lli_block_id_dest = -1;
			}
			if (info->sg_block_id_src >= 0) {
				release_block(info->sg_block_id_src,
					      dma_drv_data->sg_pool);
				info->sg_block_id_src = -1;
			}
			if (info->sg_block_id_dest >= 0) {
				release_block(info->sg_block_id_dest,
					      dma_drv_data->sg_pool);
				info->sg_block_id_dest = -1;
			}
			if (release_phys_resource(info) == -1) {
				stm_error
				    ("Unable to free Physical resource %d\n",
				     info->phys_chan_id);
				BUG();
			}
		} else {	/*Logical */

			halt_dma_channel(pipe_id);
			if (release_phys_resource(info) == -1) {
				stm_error
				    ("Unable to free Physical resource %d\n",
				     info->phys_chan_id);
				BUG();
			}
			if (info->lli_block_id_src >= 0) {
				rel_free_log_lli_block
				    (dma_drv_data->lchan_lli_pool,
				     info->phys_chan_id,
				     info->lli_block_id_src);
				info->lli_block_id_src = -1;
			}
			if (info->lli_block_id_dest >= 0) {
				rel_free_log_lli_block
				    (dma_drv_data->lchan_lli_pool,
				     info->phys_chan_id,
				     info->lli_block_id_dest);
				info->lli_block_id_dest = -1;
			}
			if (info->sg_block_id_src >= 0) {
				release_block(info->sg_block_id_src,
					      dma_drv_data->sg_pool);
				info->sg_block_id_src = -1;
			}
			if (info->sg_block_id_dest >= 0) {
				release_block(info->sg_block_id_dest,
					      dma_drv_data->sg_pool);
				info->sg_block_id_dest = -1;
			}
		}
		spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock
				       [info->phys_chan_id], flags);
	}
	dma_drv_data->pipe_info[pipe_id] = NULL;
	if (info->channel_id >= 0)
		dma_drv_data->dma_chan_info[info->channel_id] = NULL;
	kfree(info);
	deallocate_pipe_id(pipe_id);
	stm_dbg(DBG_ST.dma, "Succesfully freed DMA pipe_id %d\n", pipe_id);
}
EXPORT_SYMBOL(stm_free_dma);

/**
 * stm_dma_residue() - To Return remaining bytes of transfer
 * @pipe_id:	client id whose residue is to be returned
 *
 */
int stm_dma_residue(int pipe_id)
{
	struct dma_channel_info *info = NULL;
	unsigned long flags;
	int remaining = 0;
	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return -1;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return -1;
	}

	if (info->active == 0) {
		stm_error("Channel callback done... no residue\n");
		return 0;
	}
	spin_lock_irqsave(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			  flags);
	if (dma_channel_execute_command(DMA_SUSPEND_REQ, info->phys_chan_id) ==
	    -1) {
		stm_error("Unable to suspend the Channel %d\n",
			  info->phys_chan_id);
		spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock
				       [info->phys_chan_id], flags);
		return -1;
	}

	if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
		remaining =
		    REG_RD_BITS(cio_addr
				(info->phys_chan_id, CHAN_REG_SDELT),
				SREG_ELEM_PHY_ECNT_MASK,
				SREG_ELEM_PHY_ECNT_POS);
	} else {
		struct std_log_memory_param *params = NULL;

		if (info->dir == MEM_TO_PERIPH)
			params = ((void *)
				  (dma_drv_data->lchan_params_base.log_addr)
				  + (info->dst_dev_type * 32) + 16);
		else if (info->dir == PERIPH_TO_MEM)
			params = ((void *)
				  (dma_drv_data->lchan_params_base.log_addr)
				  + (info->src_dev_type * 32));
		else
			return remaining;

		remaining =
		    MEM_READ_BITS(params->dmac_lcsp2, MEM_LCSP2_ECNT_MASK,
				  MEM_LCSP2_ECNT_POS);
	}
	if (remaining)
		dma_channel_execute_command(DMA_RUN, info->phys_chan_id);
	spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			       flags);
	return remaining * (0x1 << info->src_info.data_width);
}
EXPORT_SYMBOL(stm_dma_residue);

/**
 * stm_pause_dma() - To pause an ongoing transfer on a pipe
 * @pipe_id:	client id whose transfer has to be paused
 * This function pauses the transfer on a pipe.
 *
 *
 *
 */
int stm_pause_dma(int pipe_id)
{
	struct dma_channel_info *info = NULL;
	unsigned long flags;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return -1;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return -1;
	}
	if (info->active == 0) {
		stm_dbg(DBG_ST.dma, "Channel is not active\n");
		return 0;
	}
	spin_lock_irqsave(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			  flags);
	if (dma_channel_execute_command(DMA_SUSPEND_REQ, info->phys_chan_id) ==
	    -1) {
		stm_error("Unable to suspend the Channel %d\n",
			  info->phys_chan_id);
		if (dma_channel_execute_command
		    (DMA_SUSPEND_REQ, info->phys_chan_id) == -1)
			goto error_pause;
	}

	if (info->chan_mode == DMA_CHAN_IN_PHYS_MODE) {
		if ((info->dir == PERIPH_TO_MEM)
		    || (info->dir == PERIPH_TO_PERIPH)) {
			u32 tmp;
			if (info->src_dev_type < 32) {
				tmp =
				    REG_RD_BITS(io_addr(DREG_FSEB1),
						FULL32_MASK, NO_SHIFT);
				tmp |= (0x1 << info->src_dev_type);
				REG_WR_BITS(io_addr(DREG_FSEB1), tmp,
					    FULL32_MASK, NO_SHIFT);
			} else {
				tmp =
				    REG_RD_BITS(io_addr(DREG_FSEB2),
						FULL32_MASK, NO_SHIFT);
				tmp |= (0x1 << info->src_dev_type) + 32;
				REG_WR_BITS(io_addr(DREG_FSEB2), tmp,
					    FULL32_MASK, NO_SHIFT);
			}
		}
	} else if (info->chan_mode == DMA_CHAN_IN_LOG_MODE) {
		/*Deactivate the event line during the
		 *time physical res is suspended */
		if (info->dir == MEM_TO_PERIPH) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SDLNK),
				    DEACTIVATE_EVENTLINE,
				    EVENTLINE_MASK(info->dst_info.event_line),
				    EVENTLINE_POS(info->dst_info.event_line));
		} else if (info->dir == PERIPH_TO_MEM) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SSLNK),
				    DEACTIVATE_EVENTLINE,
				    EVENTLINE_MASK(info->src_info.event_line),
				    EVENTLINE_POS(info->src_info.event_line));
		} else {
			stm_dbg(DBG_ST.dma, "Unknown request\n");
			goto error_pause;
		}
		if ((info->dir == PERIPH_TO_MEM)
		    || (info->dir == PERIPH_TO_PERIPH)) {
			u32 tmp;
			if (info->src_dev_type < 32) {
				tmp =
				    REG_RD_BITS(io_addr(DREG_FSEB1),
						FULL32_MASK, NO_SHIFT);
				tmp |= (0x1 << info->src_dev_type);
				REG_WR_BITS(io_addr(DREG_FSEB1), tmp,
					    FULL32_MASK, NO_SHIFT);
			} else {
				tmp =
				    REG_RD_BITS(io_addr(DREG_FSEB2),
						FULL32_MASK, NO_SHIFT);
				tmp |= (0x1 << info->src_dev_type) + 32;
				REG_WR_BITS(io_addr(DREG_FSEB2), tmp,
					    FULL32_MASK, NO_SHIFT);
			}
		}
		/*Restart the Physical resource */
		dma_channel_execute_command(DMA_RUN, info->phys_chan_id);
	} else {
		stm_error("Operation mode not supported\n");
		goto error_pause;
	}

	spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			       flags);
	return 0;
error_pause:
	spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			       flags);
	return -1;
}
EXPORT_SYMBOL(stm_pause_dma);

void stm_unpause_dma(int pipe_id)
{
	struct dma_channel_info *info = NULL;
	unsigned long flags;

	if (is_pipeid_invalid(pipe_id)) {
		stm_error("Invalid pipe id");
		return;
	}
	info = dma_drv_data->pipe_info[pipe_id];
	if (!info) {
		stm_error("Null Info structure....");
		return;
	}
	spin_lock_irqsave(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			  flags);
	if (dma_channel_execute_command(DMA_SUSPEND_REQ, info->phys_chan_id) ==
	    -1) {
		stm_error("Unable to suspend the Channel %d\n",
			  info->phys_chan_id);
		spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock
				       [info->phys_chan_id], flags);
		return;
	}
	if (info->chan_mode == DMA_CHAN_IN_LOG_MODE) {
		/*activate the event line */
		int line_status = 0;
		if (info->dir == MEM_TO_PERIPH) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SDLNK),
				    ACTIVATE_EVENTLINE,
				    EVENTLINE_MASK(info->dst_info.event_line),
				    EVENTLINE_POS(info->dst_info.event_line));
			line_status =
			    REG_RD_BITS(cio_addr
					(info->phys_chan_id,
					 CHAN_REG_SDLNK),
					EVENTLINE_MASK(info->
						       dst_info.event_line),
					EVENTLINE_POS(info->
						      dst_info.event_line));
			print_channel_reg_params(info->phys_chan_id);
		} else if (info->dir == PERIPH_TO_MEM) {
			REG_WR_BITS(cio_addr
				    (info->phys_chan_id, CHAN_REG_SSLNK),
				    ACTIVATE_EVENTLINE,
				    EVENTLINE_MASK(info->src_info.event_line),
				    EVENTLINE_POS(info->src_info.event_line));
			line_status =
			    REG_RD_BITS(cio_addr
					(info->phys_chan_id,
					 CHAN_REG_SSLNK),
					EVENTLINE_MASK(info->
						       src_info.event_line),
					EVENTLINE_POS(info->
						      src_info.event_line));
		} else {
			stm_dbg(DBG_ST.dma, "Unknown request\n");
		}
	}
	dma_channel_execute_command(DMA_RUN, info->phys_chan_id);
	spin_unlock_irqrestore(&dma_drv_data->cfg_ch_lock[info->phys_chan_id],
			       flags);
}
EXPORT_SYMBOL(stm_unpause_dma);

#if 0
static irqreturn_t stm_dma_secure_interrupt_handler(int irq, void *dev_id)
{
	u32 mask = 0x1;
	int i = 0;
	int k = 0;
	unsigned long flags;
	struct dma_channel_info *info = NULL;
	dbg2("stm_dma_interrupt_handler22222 called...\n");

	if (dma_drv_data->dma_regs->dmac_spcmis) {
		for (mask = 1; mask != 0x80000000; mask = mask << 1, k++) {
			if (dma_drv_data->dma_regs->dmac_spcmis & mask) {
				dma_drv_data->dma_regs->dmac_spcicr |= mask;
				spin_lock_irqsave(&dma_info_lock, flags);
				info =
				    dma_drv_data->dma_chan_info
				    [MAX_LOGICAL_CHANNELS + k];
				info->callback(info->data, 0);
				print_channel_reg_params(info->phys_chan_id);
				spin_unlock_irqrestore(&dma_info_lock, flags);
			}
			i++;
		}
	}
	return IRQ_HANDLED;
}
#endif

void process_phy_channel_inerrupt(void)
{
	int k = 0;
	struct dma_channel_info *info = NULL;
	u32 tmp;
	if (ioread32(io_addr(DREG_PCMIS))) {
		// Search Physical Interrupt source(s)
		for (k = 0; k < MAX_AVAIL_PHY_CHANNELS; k++) {
			// Is it due to an Error ?
			if (ioread32(io_addr(DREG_PCEIS)) & (0x1 << k)) {
				info =
				    dma_drv_data->dma_chan_info
				    [MAX_LOGICAL_CHANNELS + k];
				stm_dbg(DBG_ST.dma, "Error interrupt\n");
				if ((info->active) && (info->callback))
					info->callback(info->data, XFER_ERROR);
			}

			// Is it due to a Terminal Count ?
			if (ioread32(io_addr(DREG_PCTIS)) & (0x1 << k)) {
				info =
					dma_drv_data->dma_chan_info
					[MAX_LOGICAL_CHANNELS + k];
				stm_dbg(DBG_ST.dma, "TIS interrupt\n");
				if ((info->lli_interrupt == 1)
				    && (info->sg_dest)
				    && ((info->dir == PERIPH_TO_MEM)
					|| (info->dir == MEM_TO_PERIPH))) {
					info->bytes_xfred +=
						sg_dma_len(info->current_sg);
					stm_dbg(DBG_ST.dma,
						"Channel(%d) :: Transfer "
						"completed for %d bytes\n",
						info->phys_chan_id,
						info->bytes_xfred);
					info->current_sg++;
					if ((info->bytes_xfred ==
					     get_xfer_len(info))
					    && (info->active)
					    && (info->callback))
						info->callback(info->data,
							       XFER_COMPLETE);
				} else {
					if ((info->active) && (info->callback))
						info->callback(info->data,
							       XFER_COMPLETE);
				}
			}
			// Acknoledge Interrupt
			tmp =
				REG_RD_BITS(io_addr(DREG_PCICR),
					    FULL32_MASK, NO_SHIFT);
			tmp |= (0x1 << k);
			REG_WR_BITS(io_addr(DREG_PCICR), tmp,
				    FULL32_MASK, NO_SHIFT);
		}
	}
}

void process_logical_channel_inerrupt(void)
{
	int j = 0;
	int k = 0;
	struct dma_channel_info *info = NULL;
	u32 tmp;
	for (j = 0; j < 4; j++) {
		if (ioread32(io_addr(DREG_LCEIS(j)))) {
			for (k = 0; k < 32; k++) {
				if (ioread32(io_addr(DREG_LCEIS(j))) &
				    (0x1 << k)) {
					tmp =
					    REG_RD_BITS(io_addr
							(DREG_LCICR(j)),
							FULL32_MASK, NO_SHIFT);
					tmp |= (0x1 << k);
					REG_WR_BITS(io_addr
						    (DREG_LCICR(j)), tmp,
						    FULL32_MASK, NO_SHIFT);
					stm_dbg(DBG_ST.dma,
						"Error Logical "
						"Interrupt Line :: %d\n",
						(j * 32 + k));
					info =
					    dma_drv_data->dma_chan_info[j * 32 +
									k];
					if ((info->active) && (info->callback))
						info->callback(info->data,
							       XFER_ERROR);
				}
			}
		}
	}

	for (j = 0; j < 4; j++) {
		if (ioread32(io_addr(DREG_LCTIS(j)))) {
			for (k = 0; k < 32; k++) {
				if (ioread32(io_addr(DREG_LCTIS(j))) &
				    (0x1 << k)) {
					tmp =
					    REG_RD_BITS(io_addr
							(DREG_LCICR(j)),
							FULL32_MASK, NO_SHIFT);
					tmp |= (0x1 << k);
					REG_WR_BITS(io_addr
						    (DREG_LCICR(j)), tmp,
						    FULL32_MASK, NO_SHIFT);
					stm_dbg(DBG_ST.dma,
						"TIS interrupt:::: Logical "
						"Interrupt Line :: %d\n",
						(j * 32 + k));
					info =
					    dma_drv_data->dma_chan_info[j * 32 +
									k];
					if (!info)
						continue;

					if ((info->lli_interrupt == 1)
					    && (info->sg_dest)
					    && ((info->dir == PERIPH_TO_MEM)
						|| (info->dir ==
						    MEM_TO_PERIPH))) {
						info->bytes_xfred +=
						    sg_dma_len
						    (info->current_sg);
						stm_dbg(DBG_ST.dma,
							"Channel(%d) :: "
							"Transfer completed "
							"for %d bytes\n",
							info->phys_chan_id,
							info->bytes_xfred);
						info->current_sg++;
						if ((info->bytes_xfred ==
						     get_xfer_len(info))
						    && ((info->active)
							&& (info->callback)))
							info->
							    callback(info->data,
							     XFER_COMPLETE);
					} else {
						if ((info->active)
						    && (info->callback))
							info->
							    callback(info->data,
							     XFER_COMPLETE);
					}
				}
			}
		}
	}
}

static irqreturn_t stm_dma_interrupt_handler(int irq, void *dev_id)
{

	process_phy_channel_inerrupt();
	process_logical_channel_inerrupt();
	return IRQ_HANDLED;
}

static int stm_dma_probe(struct platform_device *pdev)
{
	struct resource *res = NULL;
	unsigned long lcpa_base;
	int status = 0, i;
	stm_dbg(DBG_ST.dma, "stm_dma_probe called.....\n");
	printk(KERN_INFO "In DMA PROBE \n");
	dma_drv_data = kzalloc(sizeof(struct dma_driver_data), GFP_KERNEL);

	if (!dma_drv_data) {
		stm_error("DMA driver structure cannot be allocated...\n");
		return -1;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		stm_error("IORESOURCE_MEM unavailable\n");
		goto driver_cleanup;
	}

	dma_drv_data->reg_base =
	    (void __iomem *)ioremap(res->start, res->end - res->start + 1);

	if (!dma_drv_data->reg_base) {
		stm_error("ioremap of DMA register memory failed\n");
		goto driver_cleanup;
	}
	stm_dbg(DBG_ST.dma, "DMA_REGS = %x\n", (u32) dma_drv_data->reg_base);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		stm_error("IORESOURCE_IRQ unavailable\n");
		goto driver_ioremap_cleanup;
	}

	status =
	    request_irq(res->start, stm_dma_interrupt_handler, 0, "DMA",
			NULL);
	if (status) {
		stm_error("Unable to request IRQ\n");
		goto driver_ioremap_cleanup;
	}

	dma_drv_data->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(dma_drv_data->clk)) {
		stm_error("Unable to get clock\n");
		goto driver_ioremap_cleanup;
	}

	clk_enable(dma_drv_data->clk);

#ifndef CONFIG_STM_SECURITY
	initialize_dma_regs();
	dma_drv_data->lchan_params_base.log_addr =
	    dma_alloc_coherent(NULL, 128 * 4 * sizeof(u32),
			       &dma_drv_data->lchan_params_base.phys_addr,
			       GFP_KERNEL | GFP_DMA);
	if (!dma_drv_data->lchan_params_base.log_addr) {
		stm_error("Request for Memory allocation for \
			Logical channel parameters failed\n");
		goto driver_clk_cleanup;
	}
	REG_WR_BITS(io_addr(DREG_LCPA),
		    dma_drv_data->lchan_params_base.phys_addr,
		    FULL32_MASK, NO_SHIFT);
#else
	if (u8500_is_earlydrop())
		lcpa_base = U8500_DMA_LCPA_BASE_ED;
	else
		lcpa_base = U8500_DMA_LCPA_BASE;

	dma_drv_data->lchan_params_base.log_addr = ioremap(lcpa_base, 4096);
	dma_drv_data->lchan_params_base.phys_addr = lcpa_base;

	REG_WR_BITS(io_addr(DREG_LCPA), lcpa_base, FULL32_MASK, NO_SHIFT);
#endif

	dma_drv_data->pchan_lli_pool = create_elem_pool("PCHAN_LLI_POOL",
							NUM_LLI_PER_REQUEST *
							sizeof(struct
							       dma_lli_info),
							NUM_PCHAN_LLI_BLOCKS,
							12);
	if (!dma_drv_data->pchan_lli_pool) {
		stm_error("Unable to allocate memory for Phys chan LLI pool");
		goto driver_chan_mem_cleanup;
	}
	dma_drv_data->sg_pool = create_elem_pool("SG_POOL",
						 NUM_SG_PER_REQUEST *
						 sizeof(struct scatterlist),
						 NUM_SG_BLOCKS, 12);
	if (!dma_drv_data->sg_pool) {
		stm_error("Unable to allocate memory for SG pool");
		goto pchan_lli_pool_cleanup;
	}

	/*Allocate 1Kb block for each physical channels */
	dma_drv_data->lchan_lli_pool = create_elem_pool("LCHAN_LLI_POOL",
							1024,
							MAX_PHYSICAL_CHANNELS,
							18);
	if (!dma_drv_data->lchan_lli_pool) {
		stm_error("Unable to allocate memory for lchan_lli_pool");
		goto sg_pool_cleanup;
	}

	REG_WR_BITS(io_addr(DREG_LCLA),
		    dma_drv_data->lchan_lli_pool->base_addr.phys_addr,
		    FULL32_MASK, NO_SHIFT);

	spin_lock_init(&dma_drv_data->pipe_id_lock);
	spin_lock_init(&dma_drv_data->pr_info_lock);
	for (i = 0; i < MAX_AVAIL_PHY_CHANNELS; i++)
		spin_lock_init(&dma_drv_data->cfg_ch_lock[i]);

	/* Audio is using physical channels 2 from MMDSP */
	dma_drv_data->pr_info[2].status = RESOURCE_PHYSICAL;

	print_dma_regs();
	return 0;
sg_pool_cleanup:
	destroy_elem_pool(dma_drv_data->sg_pool);
pchan_lli_pool_cleanup:
	destroy_elem_pool(dma_drv_data->pchan_lli_pool);
driver_chan_mem_cleanup:
#ifndef CONFIG_STM_SECURITY
	dma_free_coherent(NULL, 128 * 4 * sizeof(u32),
			  dma_drv_data->lchan_params_base.log_addr,
			  dma_drv_data->lchan_params_base.phys_addr);
driver_clk_cleanup:
#endif
	clk_disable(dma_drv_data->clk);
	clk_put(dma_drv_data->clk);
driver_ioremap_cleanup:
	iounmap(dma_drv_data->reg_base);
driver_cleanup:
	kfree(dma_drv_data);
	return -1;
}

/*
 * Clean up routine
 */
static int stm_dma_remove(struct platform_device *pdev)
{
	struct resource *res = NULL;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
#ifndef CONFIG_STM_SECURITY
	/*Free Memory used for Storing Logical channel cfg params */
	dma_free_coherent(NULL, 128 * 4 * sizeof(u32),
			  dma_drv_data->lchan_params_base.log_addr,
			  dma_drv_data->lchan_params_base.phys_addr);
#endif
	destroy_elem_pool(dma_drv_data->pchan_lli_pool);
	destroy_elem_pool(dma_drv_data->sg_pool);
#ifndef CONFIG_STM_SECURITY
	destroy_elem_pool(dma_drv_data->lchan_lli_pool);
#endif
	free_irq(res->start, 0);
	clk_disable(dma_drv_data->clk);
	clk_put(dma_drv_data->clk);
	iounmap(dma_drv_data->reg_base);
	kfree(dma_drv_data);
	return 0;
}

#ifdef CONFIG_PM

/**
 * stm_dma_suspend() - Function registered with Kernel Power Mgmt framework
 * @pdev:	Platform device structure for the DMA controller
 * @state:	pm_message_t state sent by PM framework
 *
 * This function will be called by the Linux Kernel Power Mgmt Framework, while
 * going to sleep. It is assumed that no active tranfer is in progress
 * at this time. Client driver should make sure of this.
 */
int stm_dma_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	 * writing back	backed-up non-secure registers
	 * FIXME : DREG_GCC/FSEBn and few others are accesible
	 *	   only	on a secure mode. this condition needs
	 *	   to be inserted before this operation
	 */
	dma_drv_data->backup_regs[0] = ioread32(io_addr(DREG_GCC));
	dma_drv_data->backup_regs[1] = ioread32(io_addr(DREG_PRTYP));
	dma_drv_data->backup_regs[2] = ioread32(io_addr(DREG_PRMSE));
	dma_drv_data->backup_regs[3] = ioread32(io_addr(DREG_PRMSO));
	dma_drv_data->backup_regs[4] = ioread32(io_addr(DREG_PRMOE));
	dma_drv_data->backup_regs[5] = ioread32(io_addr(DREG_PRMOO));
	dma_drv_data->backup_regs[6] = ioread32(io_addr(DREG_LCPA));
	dma_drv_data->backup_regs[7] = ioread32(io_addr(DREG_LCLA));
	dma_drv_data->backup_regs[8] = ioread32(io_addr(DREG_ACTIVE));
	dma_drv_data->backup_regs[9] = ioread32(io_addr(DREG_ACTIVO));
	dma_drv_data->backup_regs[10] =	ioread32(io_addr(DREG_FSEB1));
	dma_drv_data->backup_regs[11] =	ioread32(io_addr(DREG_FSEB2));
	dma_drv_data->backup_regs[12] =	ioread32(io_addr(DREG_PCICR));

	stm_dbg(DBG_ST.dma, "stm_dma_suspend: called......\n");
	return 0;
}

/**
 * stm_dma_resume() - Function registered with Kernel Power Mgmt framework
 * @pdev:	Platform device structure for the DMA controller
 *
 * This function will be called by the Linux Kernel Power Mgmt Framework, when
 * System comes out of sleep.
 *
 *
 */
int stm_dma_resume(struct platform_device *pdev)
{
	/*
	 * writing back	backed-up non-secure registers
	 * FIXME : DREG_GCC/FSEBn and few others are accesible
	 *	   only	on a secure mode. this condition needs
	 *	   to be inserted before this operation
	 */
	iowrite32(dma_drv_data->backup_regs[0],	(io_addr(DREG_GCC)));
	iowrite32(dma_drv_data->backup_regs[1],	(io_addr(DREG_PRTYP)));
	iowrite32(dma_drv_data->backup_regs[2],	(io_addr(DREG_PRMSE)));
	iowrite32(dma_drv_data->backup_regs[3],	(io_addr(DREG_PRMSO)));
	iowrite32(dma_drv_data->backup_regs[4],	(io_addr(DREG_PRMOE)));
	iowrite32(dma_drv_data->backup_regs[5],	(io_addr(DREG_PRMOO)));
	iowrite32(dma_drv_data->backup_regs[6],	(io_addr(DREG_LCPA)));
	iowrite32(dma_drv_data->backup_regs[7],	(io_addr(DREG_LCLA)));
	iowrite32(dma_drv_data->backup_regs[8],	(io_addr(DREG_ACTIVE)));
	iowrite32(dma_drv_data->backup_regs[9],	(io_addr(DREG_ACTIVO)));
	iowrite32(dma_drv_data->backup_regs[10],  (io_addr(DREG_FSEB1)));
	iowrite32(dma_drv_data->backup_regs[11],  (io_addr(DREG_FSEB2)));
	iowrite32(dma_drv_data->backup_regs[12],  (io_addr(DREG_PCICR)));


	stm_dbg(DBG_ST.dma, "stm_dma_resume: called......\n");
	return 0;
}

#else

#define stm_dma_suspend NULL
#define stm_dma_resume NULL

#endif

static struct platform_driver stm_dma_driver = {
	.probe = stm_dma_probe,
	.remove = stm_dma_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "STM-DMA",
		   },
	.suspend = stm_dma_suspend,
	.resume = stm_dma_resume,
};

static int __init stm_dma_init(void)
{
	return platform_driver_register(&stm_dma_driver);
}

module_init(stm_dma_init);
static void __exit stm_dma_exit(void)
{
	platform_driver_unregister(&stm_dma_driver);
	return;
}

module_exit(stm_dma_exit);

/* Module parameters */

MODULE_AUTHOR("ST Microelectronics");
MODULE_DESCRIPTION("STM DMA Controller");
MODULE_LICENSE("GPL");
