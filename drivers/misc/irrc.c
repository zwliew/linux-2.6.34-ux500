#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/io.h>
//#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
//#include <linux/timer.h>
//#include <linux/jiffies.h>
#include <linux/sched.h>
//#include <linux/device.h>
//#include <asm/semaphore.h>

#include "irrc_ioctl.h"
#include "irrc.h"
//#include "tranceiver.h"

/*
 * Parameters that can be passed at module load time
 */
//#include <linux/moduleparam.h>
//static char *string = "IRRC DRIVER";
//module_param(string, charp, S_IRUGO);

//if (down_interrutible(&device_p->mutex)) {return -ERESTARTSYS}

//up(&device_p->mutex);


#define IRRC_TEMPORARY_IRQ 55	// temp as long as we don't know the interrupt number
u32 iobase_virtual[17] = {0};	// temp as long as we don't have HW
int *iobase = (int *)&iobase_virtual;

/*
 * prototypes to functions used internal
 */
static int irrc_cleanup(status_t status);
static int irrc_open(struct inode *inode, struct file *file);
static int irrc_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
static int irrc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long argp);
static int irrc_release(struct inode *inode, struct file *filp);
static int irrc_init(void);
static void irrc_exit(void);
static unsigned short int irrc_encode_command(const unsigned int common_divisor,
					u32 * const encoded_buffer_p,
					const unsigned int cmd_size,
					const unsigned char * const cmd_buffer_p,
					const struct irrc_start_stop_pulse_data start_stop_pulse_data,
					const struct irrc_logical_pulse_data logical_pulse_data);
static unsigned long int irrc_calculate_common_divisor(const struct irrc_pulse_data * const pulse_data);

static void irrc_start_timer(struct irrc_list *const tx_list_p);
//static void irrc_conf_hw(const struct irrc_list * const tx_list_p);
static void irrc_tx_dma(const struct irrc_list * const tx_list_p);
static void irrc_expired_timer(unsigned long arg);
static irqreturn_t irrc_interrupt(int irq, void *dev_id);
static void irrc_do_pend_cmd(unsigned long data);
static int irrc_do_xmit(const struct irrc_list * const tx_list_p);

static int irrc_disable_tranceiver(void);
static int irrc_enable_tranceiver(void);
static int irrc_mux_rx(irrc_mux_t mode);
static int irrc_mux_tx(irrc_mux_t mode);

/*
 * file global variables, access should be considered to be critical sections
 */

static struct irrc_device *irrc_device_p;

//static int major_dev_nr = MAJOR_DEV_NR;
//static int minor_dev_nr = MINOR_DEV_NR;

//DECLARE_MUTEX(init_mutex);	// semaphore variable initialized to 1, unlocked state

static const struct file_operations irrc_fops = {
	.owner 		= THIS_MODULE,
	.open 		= irrc_open,
	.write 		= irrc_write,
	.ioctl 		= irrc_ioctl,
	.release 	= irrc_release,
};

static int irrc_mux_tx(irrc_mux_t mode) {

// Todo: add input parameter to pin-port used for TX and SD

	int ret = 0;

	PDEBUG("mux_tx: enter \'mux_tx\'\n");

	if (MUX_IRRC == mode) {
		// mux
	}
	else { // MUX DEFAULT
		//unmux
	}

	return ret;
}

static int irrc_mux_rx(irrc_mux_t mode) {

// Todo: add input parameter to pin-port used for TX and SD

	int ret = 0;

	PDEBUG("mux_rx: enter \'mux_rx\'\n");

	if (MUX_IRRC == mode) {
		// mux
	}
	else { // MUX DEFAULT
		//unmux
	}

	return ret;
}

static int irrc_disable_tranceiver(void) {
// Todo: add input parameter to pin-port used for SD

	int ret = 0;

	PDEBUG("disable_tranceiver: enter \'disable_tranceiver\'\n");

// 1) set SD output to logic "High" (active)

	return ret;
}

// Actually enables FIR on the tranceiver
static int irrc_enable_tranceiver(void) {
// Todo: add input parameter to pin-port used for TX and SD

	int ret = 0;

	PDEBUG("enable_tranceiver: enter \'enable_tranceiver\'\n");

// 1) set SD output to logic "High" (active)
// 2) set TX output to logic "High" (active), wait >= 200 ns
// 3) set SD output to logic "Low" (negative edge latches tx state which determines speed settings)

// 4) after waiting >= 200 ns TX can be set to logic "Low"

	return ret;
}

/*
 *
 */
static void irrc_start_timer(struct irrc_list *const tx_list_p) {

	unsigned long int n;
	n = tx_list_p->interval;

	PDEBUG("start_timer: enter \'start_timer\'\n");

	PDEBUG("start_timer: jiffies = %lu\n", jiffies);
	PDEBUG("start_timer: HZ = %d\n", HZ);

	if (HZ != 100) {
		PDEBUG("start_timer: WARNING: timer ticks changed!");
	}

	tx_list_p->timer.function = irrc_expired_timer;
	tx_list_p->timer.data = (unsigned long)tx_list_p;
	tx_list_p->timer.expires = jiffies + (n * HZ / 1000); 	//ticks of 10 ms when HZ is 100

	init_timer(&tx_list_p->timer);
	add_timer(&tx_list_p->timer);

//del_timer_sync(timer_p);
}


/*
 * Note: Timers must run in an atomic way meaning that no sleep is allowed.
 */
static void irrc_expired_timer(unsigned long arg) {

	struct irrc_list *tx_list_p;

	PDEBUG("timer_expired: enter \'timer_expired\'\n");

	tx_list_p = (struct irrc_list *)arg;

	// decrement object pending counter
	tx_list_p->count--;
	//
	irrc_do_xmit(tx_list_p);
}


/*
 *
 */
//static void irrc_conf_hw(const struct irrc_list *const tx_list_p) {

//}


/*
 *
 */
static void irrc_tx_dma(const struct irrc_list *const tx_list_p) {

}



/* Calculates the common divisor for the pulse data input, returns the common divisor.
 *
 * Note, function itself is reentrant but take care of the INPUT pointer, if the pointer points to an object
 * that is global the call to this function must be considered as a READ critical section.
 */
static unsigned long int irrc_calculate_common_divisor(const struct irrc_pulse_data* const pulse_data_p)
{
	unsigned long int pulse_period[NO_OF_PULSE_TYPES];
	unsigned long int temp = 0;
	unsigned long int i,j,flag = 1;

	PDEBUG("calculate_common_divisior: enter \'calculate_common_divisor\'\n");

	pulse_period[START_HIGH] = pulse_data_p->start_stop_pulse_data.start_high;
	pulse_period[START_LOW] = pulse_data_p->start_stop_pulse_data.start_low;
	pulse_period[STOP_HIGH] = pulse_data_p->start_stop_pulse_data.stop_high;
	pulse_period[ZERO_HIGH] = pulse_data_p->logical_pulse_data.data0high;
	pulse_period[ZERO_LOW] = pulse_data_p->logical_pulse_data.data0low;
	pulse_period[ONE_HIGH] = pulse_data_p->logical_pulse_data.data1high;
	pulse_period[ONE_LOW] = pulse_data_p->logical_pulse_data.data1low;
	/*
	 * order the size of the pulse using bubble sort algarithm
	 */

	for (i = 1; (i <= NO_OF_PULSE_TYPES) && flag; i++) {
		flag = 0;
		for (j=0; j < (NO_OF_PULSE_TYPES -1); j++) {
			if (pulse_period[j+1] < pulse_period[j]) {       // ascending order simply changes to <
				temp = pulse_period[j];             	 // swap elements
				pulse_period[j] = pulse_period[j+1];
				pulse_period[j+1] = temp;
				flag = 1;               		 // indicates that a swap occurred.
			}
		}
	}

	/*
	 * order the size of the pulse using bubble sort algarithm
	 */
	for (i = 0; (i < NO_OF_PULSE_TYPES); i++) {			// bebugging purpose
		PDEBUG("calculate_common_divisor: pulse_period in ascending order; %lu\n", pulse_period[i]);
	}

	/*
	 * finds the common divisor
	 */
	for(i = pulse_period[0]; i > 0; i--) {
		if (pulse_period[0]%i==0) {				 // statement allways true first loop
			temp=0;
			for(j=0; j<NO_OF_PULSE_TYPES; j++) {
				if(pulse_period[j]%i==0) {
					temp+=1;  			 // statement allways true first loop
				}
			}
		}
		if(temp==NO_OF_PULSE_TYPES) {
			break;
		}
	}
	PDEBUG("calculate_common_divisior: finds the common divisor, found: %lu\n", i);

	/*
	 * adjust the value incase lower than clocking period)
	 */
	//if(i < HW_BLOCK_CLK_DURATION) {
	//	i = HW_BLOCK_CLK_DURATION;
	//	PDEBUG("calculate_common_divisor: divisor less than (21 ns): set: %lu\n", i);
	//}
	 if(i && (i < (HW_BLOCK_CLK_DURATION*2)))
	{
		i = i * ((HW_BLOCK_CLK_DURATION*2)/i);
		PDEBUG("calculate_common_divisor: divisor less than (21*2 ns): set: %lu\n", i);
	}

	PDEBUG("calculate_common_divisior: returns: %lu\n", i);
	return i;
}


/*
 * Encodes the received remote control command to a driver format for creating the pulse sequences specified in the request.
 * Returns the length of the encoded command.
 *
 * Note, function itself is reentrant but take care of the in/out pointers, if the pointers points to an object
 * that is global the call to this function must be considered as a critical section.
 */
static unsigned short int irrc_encode_command(const unsigned int common_divisor,
					u32 * const encoded_buffer_p,
					const unsigned int cmd_buffer_size,
					const unsigned char * const cmd_buffer_p,
					const struct irrc_start_stop_pulse_data start_stop_pulse_data,
					const struct irrc_logical_pulse_data logical_pulse_data)
{
	unsigned long int i;
	unsigned short int encoded_length;
	unsigned char logic0modulationoffset;
	unsigned char logic1modulationoffset;
	u8 bit_mask;
//	unsigned long frame_duration, intermidiate;

	PDEBUG("encode_command: enter \'encode_command\'\n");

	/*
	 * Start pulse information
	 */
	encoded_length = 0;
	encoded_buffer_p[START_HIGH] = (start_stop_pulse_data.start_high / common_divisor) | IRRC_MOD_INST_OUTP_STATE_ACTIVE | IRRC_MOD_INST_VALID_OK;
//	frame_duration  = start_stop_pulse_data.start_high;
	encoded_buffer_p[START_LOW]  = (start_stop_pulse_data.start_low / common_divisor)   | IRRC_MOD_INST_OUTP_STATE_INACTIVE | IRRC_MOD_INST_VALID_OK;
//	frame_duration += start_stop_pulse_data.start_low;
	encoded_length += 2;


	/*
	 * Which period is output active, decided by the modulation scheme.
	 */
	switch(logical_pulse_data.modulation_method) {
		case HAL_IRRC_MODULATION_METHOD_HIGHLOW_LOWHIGH:
			logic0modulationoffset = 1;	// origin
			logic1modulationoffset = 0;
			PDEBUG("encode_command: modulation method = %s\n", "HAL_IRRC_MODULATION_METHOD_HIGHLOW_LOWHIGH");
			break;
		case HAL_IRRC_MODULATION_METHOD_LOWHIGH_HIGHLOW:
			logic0modulationoffset = 0;
			logic1modulationoffset = 1;	// origin
			PDEBUG("encode_command: modulation method = %s\n", "HAL_IRRC_MODULATION_METHOD_LOWHIGH_HIGHLOW");
			break;
		case HAL_IRRC_MODULATION_METHOD_LOWHIGH_LOWHIGH:
			logic0modulationoffset = 1;
			logic1modulationoffset = 1;
			PDEBUG("encode_command: modulation method = %s\n", "HAL_IRRC_MODULATION_METHOD_LOWHIGH_LOWHIGH");
			break;
		case HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW:
			logic0modulationoffset = 0;
			logic1modulationoffset = 0;
			PDEBUG("encode_command: modulation method = %s\n", "HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW");
			break;
		default:
			// modulation method was set incorrectly, just pick one.
			logic0modulationoffset = 0;
			logic1modulationoffset = 0;
			PDEBUG("encode_command: default modulation method = %s\n", "HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW");
			break;
	}

	/*
	 * Encode the raw data into instruction periods.
	 *
	 */
	for (i = 0; i < cmd_buffer_size; i++) {
		// Set Mask for each bit.
		bit_mask = (0x1 << (((cmd_buffer_size - 1)-(i % 8)))%8);

		if(cmd_buffer_p[i/8] & bit_mask) { //bit is a one
			//high section
			encoded_buffer_p[encoded_length+logic1modulationoffset] =
				 (logical_pulse_data.data1high / common_divisor) |
						 IRRC_MOD_INST_OUTP_STATE_ACTIVE |
							   IRRC_MOD_INST_VALID_OK;
//			frame_duration += logical_pulse_data.data1high;
			//low section
			encoded_buffer_p[encoded_length+1-logic1modulationoffset] =
				(logical_pulse_data.data1low / common_divisor)  |
					      IRRC_MOD_INST_OUTP_STATE_INACTIVE |
							  IRRC_MOD_INST_VALID_OK;
//			frame_duration += logical_pulse_data.data1low
		}
		else { //bit is a zero
			//high section
			encoded_buffer_p[encoded_length+logic0modulationoffset] =
				(logical_pulse_data.data0high / common_divisor) |
						IRRC_MOD_INST_OUTP_STATE_ACTIVE |
							  IRRC_MOD_INST_VALID_OK;
//			frame_duration += logical_pulse_data.data0high;
			//low section
			encoded_buffer_p[encoded_length+1-logic0modulationoffset] =
				(logical_pulse_data.data0low / common_divisor)  |
					      IRRC_MOD_INST_OUTP_STATE_INACTIVE |
							  IRRC_MOD_INST_VALID_OK;
//			frame_duration += logical_pulse_data.data0low;
		}
		encoded_length+=2;
	}

	/*
	 * End pulse high section
	 */
	encoded_buffer_p[encoded_length++] = (start_stop_pulse_data.stop_high / common_divisor) | IRRC_MOD_INST_OUTP_STATE_ACTIVE | IRRC_MOD_INST_VALID_OK;
//	frame_duration += start_stop_pulse_data.stop_high;

	/*
	 * End pulse low section
	 */
//	frame_duration = start_stop_pulse_data.frame_duration - frame_duration;

	// how many instrucions must be added to keep the line low?
//	intermidiate = frame_duration/MAX_PERIOD;

//	frame_duration%MAX_PERIOD;

//	if (frame_duration)

//	encoded_buffer_p[encoded_length++] = (start_stop_pulse_data.frame_duration / common_divisior | IRRC_MOD_INST_OUTP_STATE_INACTIVE | IRRC_MOD_INST_VALID_OK;

	/*
	 * Putting an invalid instruction command at the end of the buffer triggers an interrupt from HW.
	 */
	encoded_buffer_p[encoded_length++] = (IRRC_MOD_INST_VALID_NOT_OK | IRRC_MOD_INST_OUTP_STATE_INACTIVE);

	// Return final encoded remote command length.
	return encoded_length;
}


/*
 * Cleans all activities this driver has accomplished so far in its execution.
 * It is important that the case statements is in the same order to keep the level of cleanup.
 */
static int irrc_cleanup(status_t status)
{
	PDEBUG("cleanup: enter: \'cleanup\' \n");

//	if (down_interruptible(&irrc_device_p->mutex)) {return -ERESTARTSYS;}

	switch (status) {

		case EXIT_BASIC:
			misc_deregister(&irrc_device_p->misc_dev);	// returns minus value if unregister fails, add failour handling..
			tasklet_disable(&irrc_device_p->tasklet);	// release tasklet
			kfree(irrc_device_p);
			//break;
		default:
			break;

//	up(&irrc_device_p->mutex);
	}
/*
	dev_t dev = MKDEV(major_dev_nr, minor_dev_nr);

	PDEBUG("cleanup: enter: \'cleanup\' \n");

	if (down_interruptible(&irrc_device_p->mutex)) {return -ERESTARTSYS;}

	switch (status) {

		case EXIT_BASIC:

			device_destroy(irrc_device_p->irrc_class_p,dev);
			class_destroy(irrc_device_p->irrc_class_p);
//			misc_deregister(&irrc_misc);			// returns minus value if unregister fails, add failour handling..
			//break;

		//case INIT_MISC_DEVICE:
		case INIT_CREATE_CLASS_DEVICE:

			cdev_del(&irrc_device_p->cdev);			// returns void
			//break;

		case INIT_ADD_DEVICE:

			kfree(irrc_device_p);
			//break;

		case INIT_CREATE_DEVICE:

			unregister_chrdev_region(dev, NR_OF_DEVICE);	// returns void
			//break;

		case INIT_ALLOC_REGISTER_REGION:

			// no cleanup tp do
			break;
		default:
			break;

	up(&irrc_device_p->mutex);
	}
*/

	return 0;
}

/*
 *
 */
static int irrc_release(struct inode *inode, struct file *filp)
{

//	struct irrc_device *device_p = filp->private_data;

	if (down_interruptible(&irrc_device_p->mutex)) {return -ERESTARTSYS;}
	irrc_device_p->user_count--;
	up(&irrc_device_p->mutex);

	PDEBUG("release: user count %d\n", irrc_device_p->user_count);



	/* Todo: Detach Ir tranceiver */

	/* Todo: Disable the irrc port control */

	/* Todo: Reset block and turn off clock */

	/* free interrupt line */
	free_irq(IRRC_TEMPORARY_IRQ, irrc_device_p);	// returns void


	return 0;
}

/*
 *
 */
static int irrc_open(struct inode *inode, struct file *filp)
{
	// In open function typically to be done is to initilize hardware

//	struct irrc_device *device_p;

	int ret;
	ret = 0;

	PDEBUG("open: enter: \'open\' \n");

	/*
	 * Restricting access to one single user at the time, several user with the same pid is ok
	 */
	if (down_interruptible(&irrc_device_p->mutex)) {return -ERESTARTSYS;}
	if (irrc_device_p->user_count &&
		(irrc_device_p->user_owner != current->uid)) {
		up(&irrc_device_p->mutex);
		return -EBUSY;
	}

	if (irrc_device_p->user_count == 0) {
		irrc_device_p->user_owner = current->uid;		// save users pid
		PDEBUG("open: pid %d is the owner of the device\n", current->uid);
	}
	irrc_device_p->user_count++;
	up(&irrc_device_p->mutex);

	PDEBUG("open: user count %d\n", irrc_device_p->user_count);

//	device_p = container_of(inode->i_cdev, struct irrc_device, cdev);
//	filp->private_data = device_p; 					// to be used later on


	/*
	 * Attach interrupt handler
	 */
	ret = request_irq(IRRC_TEMPORARY_IRQ, irrc_interrupt, 0, "irrc", irrc_device_p);
	if (ret) {
		PDEBUG("open:unable to attach interrupt handler, irq = %d\n", IRRC_TEMPORARY_IRQ);
		return ret;
	}

/*
  // Enable the output inverter if required
#ifdef PD_IRRC_INVERTED_OUTPUT
  IRRC_RD_OUTP_INV_VALUE_ENABLE();
#else
  IRRC_WR_OUTP_INV_VALUE_DISABLE();
#endif // PD_IRRC_INVERT_OUTPUT

// Flush the fifo first before enabling interrupts
  IRRC_WR_FIFO_STAT_FLUSH(IRRC_FIFO_STAT_FLUSH_ENABLE);
  IRRC_WR_FIFO_STAT_FLUSH(IRRC_FIFO_STAT_FLUSH_NORMAL);

  // Enable the irrc port control
  IRRC_WR_MODE_IRRC_EN_ENABLE();


  // GPIO CONFIGUATION

  //Force IRRC pin high (to give the pin a value it most be set to output).
  Do_PD_GPIO_PinConfigSet(IRRC_OUT_GPIO_PORT,
                          IRRC_OUT_GPIO_BIT,
                          PD_GPIO_CONFIG_OUT_OS);//lint !e534 return value ignored.
  Do_PD_GPIO_PortWrite(IRRC_OUT_GPIO_PORT,
                       IRRC_OUT_GPIO_BIT, 0);//lint !e534 return value ignored.

  // Set the mux routing
  if (PD_MISCCON_RESULT_OK != Do_PD_MISCCON_AllocAndConfigurePadmux(IRRC_OUT_GPIO_MUX))
  {
    A_(printf("PD_IRRC[Do_PD_IRRC_Allocate]: Failed to mux IRRC_OUT\n");)
    return PD_IRRC_RESULT_FAILED_GPIOERR;
  }
*/

	/*
	 * ToDo:
	 * 1) check padmuxer if IrDA has control off the tx-pin, the idea is to check if the IR transiver is ready to be used by IrRC.
	 * 2) mux pad to IrRC HW block.
	 * 3) put SD (shut down) pin in shout down mode.
	 * 4) make sure power to transiver is on.
	 */

	/*
	 * Todo:
	 * Request clock sw-wise in order to configure block
	 */



	/*
	 * initialize IrRC HW-block
	 */

	return ret;
}

/*
 *
 */
static ssize_t irrc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{

	PDEBUG("write: enter: \'write\' \n");

	return (ssize_t)count;
}
/*
static ssize_t irrc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{

	unsigned int encoded_buffer_size;
	unsigned int cmd_buffer_size;
	unsigned long int * encoded_buffer_p;
        unsigned char * cmd_buffer_p;
	int i;

	struct irrc_device *device_p = filp->private_data;

	PDEBUG("write: enter: \'write\' \n");

	cmd_buffer_size = count;
	PDEBUG("write: bytes to write: %d\n", cmd_buffer_size);

	encoded_buffer_size = (count*16)+4;	// count *(eight bits in a byte) * (two intructions per bit pulse) +
						// (two instructions per start pulse) +
						// (one instructions per stop pulse) +
						// (one  instructions per invalid instruction)

	PDEBUG("write: encoded buffer size: %d\n", encoded_buffer_size);


	cmd_buffer_p = (unsigned char *)kmalloc(cmd_buffer_size, GFP_KERNEL);

	if (!cmd_buffer_p) {
		PDEBUG("write: memory allocation failed!\n");
		return -ENOMEM;
	}

	if (copy_from_user(cmd_buffer_p, buf, count)) { 		// > 0 means not all data have been copied
		PDEBUG("write: copy_from_user returned non zero.\n");
		kfree(cmd_buffer_p);
		return -EINVAL;
	}

	encoded_buffer_p = (unsigned long int *)kmalloc(encoded_buffer_size, GFP_KERNEL);

	if (!encoded_buffer_p) {
		PDEBUG("write: memory allocation failed!\n");
		kfree(cmd_buffer_p);	// don't forget to free already allocated memory
		return -ENOMEM;
	}

	memset(encoded_buffer_p, 0, encoded_buffer_size);

	(void)irrc_encode_command(device_p->common_divisor, encoded_buffer_p,
				cmd_buffer_size, cmd_buffer_p, device_p->start_stop_pulse_data, device_p->logical_pulse_data);

	PDEBUG("write: encoded buffer content:\n");
	for (i = 0; i < encoded_buffer_size; i++) {
		PDEBUG("buffer [%d]: 0x%lx\n", i, encoded_buffer_p[i]);
	}

	// Todo: Start DMA transfer to transfer encoded buffer to HW

	*f_pos = 0;

	kfree(cmd_buffer_p);	// free allocated memory

	kfree(encoded_buffer_p);// free allocated momory

	return (ssize_t)count;
}
*/


/*
 *
 */
static int irrc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long argp)
{
//	struct irrc_device *device_p = filp->private_data;
	struct irrc_ioctl  *ioctl_p;
	struct irrc_list   *tx_list_p;

	unsigned long int common_divisor;
	int size, encoded_buff_size;
	u32 *encoded_buff_p;

	PDEBUG("ioctl: enter: \'ioctl\' \n");

	if (argp == 0) {
		PDEBUG("ioctl: argument is null\n");
		return -EINVAL;
	}
//   	if (cmd !=IRRC_IOC_CONFIGURE_PULSE_PARAM) {
//    		PDEBUG("ioctl: unknown ioctl command\n");
//		return -EINVAL;
//	}

	size = sizeof(struct irrc_ioctl);
	PDEBUG("ioctl: sizeof irrc_ioctl = %d\n", size );

	ioctl_p = (struct irrc_ioctl *)kmalloc(sizeof(struct irrc_ioctl), GFP_KERNEL);
	if (!ioctl_p) {
		PDEBUG("ioctl: memory allocation failed!\n");
		return -ENOMEM;
	}
	if (copy_from_user(ioctl_p, (void*) argp, size)) { 		// > 0 means not all data have been copied
		PDEBUG("ioctl: copy_from_user returned non zero.\n");
		kfree(ioctl_p);
		return -EINVAL;
	}

	PDEBUG("ioctl: carrier high = %lu us\n",      	ioctl_p->pulse_data.carrier_data.carrier_high);
	PDEBUG("ioctl: carrier low = %lu us\n",	   	ioctl_p->pulse_data.carrier_data.carrier_low);
	PDEBUG("ioctl: modulation method = %d\n",	ioctl_p->pulse_data.logical_pulse_data.modulation_method);
	PDEBUG("ioctl: data0high = %lu us\n", 		ioctl_p->pulse_data.logical_pulse_data.data0high);
	PDEBUG("ioctl: data0low = %lu us\n", 		ioctl_p->pulse_data.logical_pulse_data.data0low);
	PDEBUG("ioctl: data1high = %lu us\n", 		ioctl_p->pulse_data.logical_pulse_data.data1high);
	PDEBUG("ioctl: data1low = %lu us\n", 		ioctl_p->pulse_data.logical_pulse_data.data1low);
	PDEBUG("ioctl: start high = %lu us\n", 		ioctl_p->pulse_data.start_stop_pulse_data.start_high);
	PDEBUG("ioctl: start low = %lu us\n", 		ioctl_p->pulse_data.start_stop_pulse_data.start_low);
	PDEBUG("ioctl: stop high = %lu us\n", 		ioctl_p->pulse_data.start_stop_pulse_data.stop_high);
//	PDEBUG("ioctl: frame duration = %lu ms\n", 	ioctl_p->pulse_data.start_stop_pulse_data.frame_duration);
	PDEBUG("ioctl: count = %lu\n", 			ioctl_p->pulse_data.repeat_data.count);
	PDEBUG("ioctl: interval = %lu ms\n", 		ioctl_p->pulse_data.repeat_data.interval);
	PDEBUG("ioctl: size (in bits) = %lu\n",   	ioctl_p->cmd_data.size );

	tx_list_p = (struct irrc_list *)kmalloc(sizeof(struct irrc_list), GFP_KERNEL);
	if (!tx_list_p) {
		PDEBUG("ioctl: memory allocation failed!\n");
		kfree(ioctl_p);
		return -ENOMEM;
	}
	memset(tx_list_p, 0, sizeof(struct irrc_list));

	/*
	 * find out how many bytes should be allocated to the encode buffer
	 *
	 * (two intructions per bit pulse) +
	 * (two instructions per start pulse) +
	 * (one instructions per stop pulse) +
	 * (one instructions per invalid instruction)
	 */
	encoded_buff_size = (ioctl_p->cmd_data.size << 2) + 4; 	// multiply by two and add 4
//	encoded_buff_size = ioctl_p->cmd_data.size/8;
//	if (encoded_buff_size%8)
//		encoded_buff_size++;

	PDEBUG("ioctl: encoded_buff_size = %d\n", encoded_buff_size);
	PDEBUG("ioctl: encoded_buff_size in bytes = %d\n", encoded_buff_size * sizeof(u32));

	encoded_buff_p = (u32 *)kmalloc(encoded_buff_size * sizeof(u32), GFP_KERNEL);
	if (!encoded_buff_p) {
		PDEBUG("ioctl: memory allocation failed!\n");
		kfree(ioctl_p);
		kfree(tx_list_p);
		return -ENOMEM;
	}
	memset(encoded_buff_p, 0, encoded_buff_size * sizeof(u32));


	/*
	 * Everyting is ok, start using the pulse parameters
	 */

	common_divisor = irrc_calculate_common_divisor(&ioctl_p->pulse_data);	// no need to use a mutex ioctl_p is an object within this function.

	(void)irrc_encode_command(common_divisor, encoded_buff_p,
				ioctl_p->cmd_data.size, ioctl_p->cmd_data.data,
				ioctl_p->pulse_data.start_stop_pulse_data, ioctl_p->pulse_data.logical_pulse_data);


	tx_list_p->encoded_buff_p 	= encoded_buff_p;
	tx_list_p->common_divisor 	= common_divisor;
	tx_list_p->carrier_high 	= ioctl_p->pulse_data.carrier_data.carrier_high;
	tx_list_p->carrier_low 		= ioctl_p->pulse_data.carrier_data.carrier_low;
	tx_list_p->count 		= ioctl_p->pulse_data.repeat_data.count;
	tx_list_p->interval 		= ioctl_p->pulse_data.repeat_data.interval;

	if (irrc_device_p->current_tx_list_p == NULL) { // No tx objects in list, start transmission.

		int ret;

		irrc_device_p->current_tx_list_p = tx_list_p;
		// update last list ithem
		irrc_device_p->last_tx_list_p = tx_list_p;

		// Transmit
		ret = irrc_do_xmit(tx_list_p);
		if (ret) { PDEBUG("ioctl: irrc_do_xmit failed!\n"); }
	}
	else { // Add the tx object to pending  list.

		// add the object to the list
		irrc_device_p->current_tx_list_p->next_tx_list_p = tx_list_p;
		// update last list ithem
		irrc_device_p->last_tx_list_p = tx_list_p;
	}

	kfree(ioctl_p);		// ioctl object not used anymore, free.

	return 0;
}


/*
 * Note: Tasklet runs in an atomic way meaning that no sleep is allowed.
 */
static void irrc_do_pend_cmd(unsigned long data) {

	struct irrc_device *device_p;
	struct irrc_list   *temp_list_p;

	PDEBUG("do_pend_cmd: enter \'do_pend_cmd\'\n");

	/*
	 * free the already sent object in list and set current to the object to be sent.
	 */
	device_p = (struct irrc_device *) data;
	temp_list_p = device_p->current_tx_list_p->next_tx_list_p;

	kfree(device_p->current_tx_list_p);	// kfree is never a sleepy call

	if (temp_list_p != NULL) { // pending tx transfer

		int ret;

		device_p->current_tx_list_p = temp_list_p;

		// Transmit
		ret = irrc_do_xmit(temp_list_p);
		if (ret) { PDEBUG("do_pend_cmd: irrc_do_xmit failed!\n"); }

	}
	else { //no pending tx transfer
		device_p->current_tx_list_p = NULL;
	}
}


/*
 *	Configures HW modulation parameters
 */
static int irrc_do_xmit(const struct irrc_list * const tx_list_p) {

	unsigned long ht_reg, lt_reg, bclk_reg;
	unsigned long carrier_high, carrier_low, divisor;

	carrier_high = tx_list_p->carrier_high;
	carrier_low  = tx_list_p->carrier_low;
	divisor      = tx_list_p->common_divisor;

	ht_reg  = (carrier_high/HW_BLOCK_CLK_DURATION) - 1;
	lt_reg  = (carrier_low/HW_BLOCK_CLK_DURATION) - 1;
	bclk_reg = (divisor/HW_BLOCK_CLK_DURATION) - 1;

	do { /*loop until register synchronization is finished */ } while (!(IRRC_RD_REG_32(IRRC_WR_STAT(iobase) && IRRC_WR_STATUS_MASK_CCLK_HT)));
	IRRC_WR_REG_32(IRRC_CCLK_HT(iobase), ht_reg);

	do { /*loop until register synchronization is finished */ } while (!(IRRC_RD_REG_32(IRRC_WR_STAT(iobase) && IRRC_WR_STATUS_MASK_CCLK_LT)));
	IRRC_WR_REG_32(IRRC_CCLK_LT(iobase), lt_reg);

	do { /*loop until register synchronization is finished */ } while (!(IRRC_RD_REG_32(IRRC_WR_STAT(iobase) && IRRC_WR_STATUS_MASK_BCCLK)));
	IRRC_WR_REG_32(IRRC_BCLK_DIV(iobase), bclk_reg);


	if (tx_list_p->count) { // repeat interval, start timer
		irrc_start_timer(tx_list_p);
	}
	else { // enable invalid instruction interrupt
		IRRC_WR_FLD_32(IRRC_IRQ_EN(iobase),IRRC_IRQ_ON_FRAME_END, IRRC_BIT_SET);
	}

	// ToDo, start DMA transfer
	irrc_tx_dma(tx_list_p);
}

/*
 * 	Interrupt handler
 */
static irqreturn_t irrc_interrupt(int irq, void * dev_id) {

	struct irrc_device *device_p;
	volatile int irq_stat;

	device_p = dev_id;

	// Read masked interrupt status
	irq_stat = IRRC_RD_REG_32(IRRC_IRQ_STAT(iobase));

	if (irq_stat == 0) { return IRQ_NONE; }		// false interrupt informs the kernel

	do { //loop until all irqs are served

		if (irq_stat && IRRC_IRQ_STAT_FRAME_END) {

			// Disable DMA transfer

			// Clear interrupt.
			IRRC_WR_FLD_32(IRRC_IRQ_CLR(iobase),IRRC_IRQ_CLEAR_FRAME_END, IRRC_BIT_SET);

			// Handle pending send commands in bottom-half interrupt routine.
			tasklet_schedule(&device_p->tasklet);
		}

		if (irq_stat && IRRC_IRQ_STAT_FIFO_UNDERFLOW) {

			// Clear interrupt.
			IRRC_WR_FLD_32(IRRC_IRQ_CLR(iobase),IRRC_IRQ_STAT_FIFO_UNDERFLOW, IRRC_BIT_SET);

			// Print message in bottom-half interrupt routine.
		}

		if (irq_stat & (IRRC_IRQ_STAT_FIFO_HALF_EMPTY | IRRC_IRQ_STAT_FIFO_FULL | IRRC_IRQ_STAT_FIFO_EMPTY)) {

			// Clear interrupt.
			IRRC_WR_FLD_32(IRRC_IRQ_CLR(iobase),
				IRRC_IRQ_STAT_FIFO_HALF_EMPTY |
				      IRRC_IRQ_STAT_FIFO_FULL |
                                      IRRC_IRQ_STAT_FIFO_EMPTY, IRRC_BIT_SET);

			// Print message in bottom-half interrupt routine.
		}

		irq_stat = IRRC_RD_REG_32(IRRC_IRQ_STAT(iobase));

	}while (irq_stat != 0);

	return IRQ_HANDLED;
}


/*
 * Register, Creates and Initializes the device
 */
static int irrc_init(void)
{
	signed int result = 0;
//	dev_t dev = 0;
//      status_t status = NO_CLEANUP;

	PDEBUG("init: enter: \'init\' \n");

	/*
	 * Register driver in system, by use of a fixed major number or from a dynamicaly set major numer.
	 */
/*
	if (major_dev_nr) {
		dev = MKDEV(major_dev_nr, MINOR_DEV_NR);
		result = register_chrdev_region(dev, NR_OF_DEVICE, ALIAS);
		PDEBUG("init:  Info, register region using static major number \n");
	}
	else {
		result = alloc_chrdev_region(&dev, MINOR_DEV_NR, NR_OF_DEVICE, ALIAS);
		if (down_interruptible(&init_mutex)) { return -ERESTARTSYS; }
		major_dev_nr = MAJOR(dev);
		minor_dev_nr = MINOR(dev);
		up(&init_mutex);
		PDEBUG("init:  Info, register region using dynamic major number\n");
	}
	if (result < 0) {
		PDEBUG("init:  Error: %d, register region failed!\n", result);
		status = INIT_ALLOC_REGISTER_REGION;
		goto fail;

	}
*/
	/*
	 * Create and reset device structure
	 */
//	if (down_interruptible(&init_mutex)) { return -ERESTARTSYS; }
	irrc_device_p = kmalloc(sizeof(struct irrc_device), GFP_KERNEL);	// to prevent memory leak the operation is protected within semaphores
	if (!irrc_device_p) {
//		up(&init_mutex);	// releas mutex
		result = -ENOMEM;
		PDEBUG("init:  creation of char device structure failed!\n");
//		status = INIT_CREATE_DEVICE;
//		goto fail;
		return result;
	}
//	up(&init_mutex);
	memset(irrc_device_p, 0, sizeof(struct irrc_device));

	tasklet_init(&irrc_device_p->tasklet, irrc_do_pend_cmd, (unsigned long) irrc_device_p);

	/*
	 * Initialize and adds the device.
	 */
/*
	if (down_interruptible(&init_mutex)) { return -ERESTARTSYS; }
	cdev_init(&irrc_device_p->cdev, &irrc_fops);		// returnes void
	irrc_device_p->cdev.owner = THIS_MODULE;
	irrc_device_p->cdev.ops = &irrc_fops;
	init_MUTEX(&irrc_device_p->mutex);
	result = cdev_add (&irrc_device_p->cdev, dev, 1);	// from now on the driver may be invoked by any user
	up(&init_mutex);

	if (result < 0)
	{
		PDEBUG("init: Error %d, adding failed!\n", result);
		status = INIT_ADD_DEVICE;
		goto fail;
	}
*/
/*
	if (down_interruptible(&irrc_device_p->mutex)) { return -ERESTARTSYS; }
	irrc_device_p->irrc_class_p = class_create(THIS_MODULE,"irrc");
	if (IS_ERR(irrc_device_p->irrc_class_p)) {
		up(&irrc_device_p->mutex);
		PDEBUG("init: Warning, create class failed!\n");
		status = INIT_CREATE_CLASS_DEVICE;
		goto fail;
	}
	(void)device_create(irrc_device_p->irrc_class_p, NULL, dev,0, "irrc");
	up(&irrc_device_p->mutex);
*/

	init_MUTEX(&irrc_device_p->mutex);

	irrc_device_p->misc_dev.minor	= MINOR_DEV_NR;
	irrc_device_p->misc_dev.name	= "irrc";
	irrc_device_p->misc_dev.fops	= &irrc_fops;
	/*
	 * Register device in /sysfs/ system and make it visible as /dev/irrc
	 */
	result = misc_register(&irrc_device_p->misc_dev);
	if (result < 0) {
		PDEBUG("init: Error %d, misc_register failed!\n", result);
//		status = INIT_MISC_DEVICE;
//		goto fail;
		kfree(irrc_device_p);
	}

	return 0;	// return success

	/*
	 * Cleanup part, takes care of memory deallocation and stuff.
	 */
//	fail:
//		result = irrc_cleanup(status);				// what if cleanup fails

	return result;
}

/*
 * Un-register, De-creates and Un-initializes the device
 */
static void irrc_exit(void)
{
	status_t status;
	int result;

	PDEBUG("exit: enter: \'exit\' \n");

	status = EXIT_BASIC;

	result = irrc_cleanup(status);

	if (result) { PDEBUG("exit: cleanup ended up in error\n"); }

}



module_init(irrc_init);
module_exit(irrc_exit);

// should be added in future
// MODULE_ALIAS("")
// MODULEE_DEVICE_TABLE("")
// MODULE_VERSIONS("")

MODULE_DESCRIPTION("ST-Ericsson InfraRed Remote Control IRRC driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrik Månsson");

