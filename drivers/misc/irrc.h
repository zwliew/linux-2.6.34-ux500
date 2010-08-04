
/*
 * Macros for register offset
 */
#define IRRC_VERSION(base_adr)		(base_adr+0x0000)
#define IRRC_MODE(base_adr)		(base_adr+0x0004)
#define IRRC_DMA(base_adr)		(base_adr+0x0008)
#define IRRC_FIFO_STAT(base_adr)	(base_adr+0x000c)
#define IRRC_MOD_INST(base_adr)		(base_adr+0x0010)
#define IRRC_CCLK_HT(base_adr)		(base_adr+0x0014)
#define IRRC_CCLK_LT(base_adr)		(base_adr+0x0018)
#define IRRC_BCLK_DIV(base_adr)		(base_adr+0x001c)
//#define IRRC_OUTP_INV(base_adr)		(base_adr+0x0020)
#define IRRC_WR_STAT(base_adr)		(base_adr+0x0020)
#define IRRC_IRQ_STAT(base_adr)		(base_adr+0x0024)
#define IRRC_IRQ_RAW_STAT(base_adr)	(base_adr+0x0028)
#define IRRC_IRQ_EN_STAT(base_adr)	(base_adr+0x002c)
#define IRRC_IRQ_EN(base_adr)		(base_adr+0x0030)
#define IRRC_IRQ_DISEN(base_adr)	(base_adr+0x0034)
#define IRRC_IRQ_CLR(base_adr)		(base_adr+0x0038)
#define IRRC_IRQ_FORCE(base_adr)	(base_adr+0x003c)
#define IRRC_SW_TEST(base_adr)		(base_adr+0x0040)

#define IRRC_IRQ_STAT_FRAME_END		(0x00000001)
#define IRRC_IRQ_STAT_FIFO_FULL		(0x00000002)
#define IRRC_IRQ_STAT_FIFO_UNDERFLOW	(0x00000004)
#define IRRC_IRQ_STAT_FIFO_EMPTY	(0x00000008)
#define IRRC_IRQ_STAT_FIFO_HALF_EMPTY	(0x00000010)

#define IRRC_IRQ_CLEAR_FRAME_END	(0x00000001)
#define IRRC_IRQ_CLEAR_FIFO_FULL	(0x00000002)
#define IRRC_IRQ_CLEAR_FIFO_UNDERFLOW	(0x00000004)
#define IRRC_IRQ_CLEAR_FIFO_EMPTY	(0x00000008)
#define IRRC_IRQ_CLEAR_FIFO_HALF_EMPTY	(0x00000010)

#define IRRC_IRQ_ON_FRAME_END		(0x00000001)
#define IRRC_IRQ_ON_FIFO_FULL		(0x00000002)
#define IRRC_IRQ_ON_FIFO_UNDERFLOW	(0x00000004)
#define IRRC_IRQ_ON_FIFO_EMPTY		(0x00000008)
#define IRRC_IRQ_ON_FIFO_HALF_EMPTY	(0x00000010)

#define IRRC_WR_STATUS_MASK_CCLK_HT	(0x00000001)
#define IRRC_WR_STATUS_MASK_CCLK_LT	(0x00000002)
#define IRRC_WR_STATUS_MASK_BCCLK	(0x00000004)

#define IRRC_BIT_SET			(0xFFFFFFFF)
#define IRRC_BIT_CLEAR			(0x00000000)



/*
 * Macro for writing and reading register
 */
#define IRRC_RD_REG_32(adr)        	   readl(adr)
#define IRRC_WR_REG_32(adr, value)        writel(adr, value)
#define IRRC_WR_FLD_32(adr, mask, value)  writel(adr, (readl(adr) & ~(mask)) | ((value) & (mask)))



/*
 * Private
 */
#define DEFAULT_CARRIER_HIGH       1000
#define DEFAULT_CARRIER_LOW        1000
#define DEFAULT_MODULATION_METHOD  HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW
#define DEFAULT_DATA_0_HIGH        5000
#define DEFAULT_DATA_0_LOW         5000
#define DEFAULT_DATA_1_HIGH        10000
#define DEFAULT_DATA_1_LOW         10000
#define DEFAULT_START_HIGH         20000
#define DEFAULT_START_LOW          20000
#define DEFAULT_STOP_HIGH          30000
#define DEFAULT_REPEAT_COUNT       1
#define DEFAULT_REPEAT_INTERVAL    100


#define IRRC_MOD_INST_MASK                                (0xE003FFFF)
//NOT USED                                                  (0x1FFC0000)
#define IRRC_MOD_INST_RWMASK                              (0x00000000)
#define IRRC_MOD_INST_VALID_MASK                          (0x80000000)
#define IRRC_MOD_INST_VALID_OK                            (0x80000000)
#define IRRC_MOD_INST_VALID_NOT_OK                        (0x00000000)
#define IRRC_MOD_INST_OUTP_STATE_MASK                     (0x40000000)
#define IRRC_MOD_INST_OUTP_STATE_ACTIVE                   (0x40000000)
#define IRRC_MOD_INST_OUTP_STATE_INACTIVE                 (0x00000000)
#define IRRC_MOD_INST_OUTP_SEL_MASK                       (0x20000000)
#define IRRC_MOD_INST_OUTP_SEL_FORCED_HIGH                (0x20000000)
#define IRRC_MOD_INST_OUTP_SEL_CARRIER                    (0x00000000)
#define IRRC_MOD_INST_PERIOD_MASK                         (0x0003FFFF)

#define ALIAS 			"irrc"
#define MINOR_DEV_NR		0
#define NR_OF_DEVICE 		1				// Not negotiable! Shall allways be 1 (code does not support more than one device instance)
#define MAJOR_DEV_NR		0				// if set to 0 the setting of major number will be dynamicaly.

#define HW_BLOCK_CLK_DURATION		21			//period time of the block HW clock, in nanosecs

#undef PDEBUG
#ifdef IRRC_DEBUG
#	define PDEBUG(fmt, args...) printk( KERN_DEBUG "irrc_"fmt, ## args)
#else
#	define PDEBUG(fmt, args...)
#endif

#undef SDEBUG
#define SDEBUG(fmt, args...)


#include <linux/timer.h>

typedef enum pulses {
	START_HIGH,
	START_LOW,
	STOP_HIGH,
	ZERO_HIGH,
	ZERO_LOW,
	ONE_HIGH,
	ONE_LOW,
	NO_OF_PULSE_TYPES
} irrc_pulses_t;

typedef enum irrc_mux {
	MUX_DEFAULT,
	MUX_IRRC,
} irrc_mux_t;

typedef enum status {
	NO_CLEANUP,
	INIT_ADD_DEVICE,
	INIT_CREATE_DEVICE,
	INIT_ALLOC_REGISTER_REGION,
//	INIT_MISC_DEVICE,
	INIT_CREATE_CLASS_DEVICE,
	EXIT_BASIC

} status_t;

struct irrc_list {
	u32 				       *encoded_buff_p;
	unsigned long int 			count;
	unsigned long int 			interval;
	unsigned long int 			carrier_high;
	unsigned long int 			carrier_low;
	unsigned long int			common_divisor;
	struct irrc_list		       *next_tx_list_p;
	struct timer_list		        timer;

};

struct irrc_device {

	struct semaphore 			mutex;			// mutual exclusion semaphore
	struct tasklet_struct			tasklet;
//	struct cdev 				cdev;	  		// Char device structure
	struct miscdevice			misc_dev;
//	struct class   			       *irrc_class_p;
	struct irrc_list 		       *last_tx_list_p;
	struct irrc_list		       *current_tx_list_p;
	int					user_count;
	int					user_owner;
};
