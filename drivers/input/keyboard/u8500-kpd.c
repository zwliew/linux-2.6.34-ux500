/*----------------------------------------------------------------------------------*/
/* © copyright STMicroelectronics, 2007.                                            */
/* Copyright (C)ST-Ericsson, 2009 						    */
/*                                                                                  */
/* This program is free software; you can redistribute it and/or modify it under    */
/* the terms of the GNU General Public License as published by the Free	            */
/* Software Foundation; either version 2.1 of the License, or (at your option)      */
/* any later version.                                                               */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.   */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.             */
/*----------------------------------------------------------------------------------*/

/* Keypad driver Version */
#define KEYPAD_VER_X		4
#define KEYPAD_VER_Y		0
#define KEYPAD_VER_Z		0

#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <asm/bitops.h>
#include <mach/hardware.h>
//#include <mach/defs.h>
#include <mach/debug.h>
#include <mach/kpd.h>


#define KEYPAD_NAME		"DRIVER KEYPAD"

extern struct driver_debug_st DBG_ST;
/*function declarations h/w independent*/
irqreturn_t u8500_kp_intrhandler(/*int irq, */void *dev_id);
static void u8500_kp_wq_kscan(struct work_struct *work);

/*
 * Module parameter defination to pass mode of operation
 * 0 = to initialize driver in Interrupt mode (default mode)
 * 1 = to Intialize driver in polling mode of operation
 */
int kpmode = 0;
module_param(kpmode, int, 0);
MODULE_PARM_DESC(kpmode, "Keypad Operating mode (INT/POLL)=(0/1)");

#if 0
/*
 * u8500_kp_intrhandler - keypad interrupt handler
 *
 * checks for valid interrupt, disables interrupt to avoid any nested interrupt
 * starts work queue for further key processing with debouncing logic
 */
irqreturn_t u8500_kp_intrhandler(/*int irq, */void *dev_id)
{
	struct keypad_t *kp = (struct keypad_t *)dev_id;
	/*if (irq != kp->irq) return IRQ_NONE;*/
	if (!(test_bit(KPINTR_LKBIT, &kp->lockbits))) {
		stm_dbg2(DBG_ST.keypad,"Kp interrupt");
		____atomic_set_bit(KPINTR_LKBIT, &kp->lockbits);

		if(kp->board->irqdis_int)
		{	kp->board->irqdis_int(kp);
		}
		//schedule_delayed_work(&kp->kscan_work, kp->board->debounce_period);
		schedule_work(&kp->kscan_work);
	}
	return IRQ_HANDLED;
}
EXPORT_SYMBOL(u8500_kp_intrhandler);
#endif

/**
 * u8500_kp_wq_kscan - work queue for keypad scanning
 * @work:	pointer to keypad data
 * Executes at each scan tick, execute the key press/release function,
 * Generates key press/release event message for input subsystem for valid key
 * events, enables keypad interrupts (for int mode)
 */

static void u8500_kp_wq_kscan(struct work_struct *work)
{
	int key_cnt = 0;
	struct keypad_t *kp = container_of((struct delayed_work *)work, struct keypad_t, kscan_work);

        if (!kp->mode && kp->board->irqdis)
		kp->board->irqdis(kp);
	if (kp->board->autoscan_results) {
		key_cnt = kp->board->autoscan_results(kp);
	}
	else
		printk(KERN_ERR"key scan function not found");

	if (kp->mode){
		if (0 == key_cnt) {
			/*if no key is pressed and polling mode */
			schedule_delayed_work(&kp->kscan_work,
					      KEYPAD_SCAN_PERIOD);
		}
		else {
			/*if key is pressed and hold condition */
			printk("Work queue: pressed and hold\n");
			schedule_delayed_work(&kp->kscan_work, KEYPAD_RELEASE_PERIOD);
		}
	}
	else {
		/**
		 * if interrupt mode then just enable iterrupt and return
		*/
		clear_bit(KPINTR_LKBIT, &kp->lockbits);
		if (kp->board->irqen)
			kp->board->irqen(kp);
	}

}

/**
 * u8500_kp_init_keypad - keypad parameter initialization
 * @kp:		pointer to keypad data
 * Initializes Keybits to enable keyevents
 * Initializes Initial keypress status to default
 * Calls the keypad platform specific init function.
 */
int __init u8500_kp_init_keypad(struct keypad_t *kp)
{
	int row, column, err = 0;
	u8 *p_kcode = kp->board->kcode_tbl;



	if (kp->board->init) {
		err = kp->board->init(kp);
	}
	if (err)
		return (err);

	for (row = 0; row < MAX_KPROW; row++) {
		for (column = 0; column < MAX_KPCOL; column++) {
			/*set keybits for the keycodes in use */
			set_bit(*p_kcode, kp->inp_dev->keybit);
			/*set key status to default value */
			kp->key_state[row][column] = KEYPAD_STATE_DEFAULT;
			p_kcode++;
		}
	}
	return (err);
}

#ifdef CONFIG_PM
/**
 * u8500_kp_suspend - suspend keypad
 * @pdev:       platform data
 * @state:	power down level
 */
int u8500_kp_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct keypad_t *kp = platform_get_drvdata(pdev);
	if ( kpmode ) {
		printk("Enabling interrupt \n");

		kp->board->irqen(kp);
	}
	if ( !device_may_wakeup(&pdev->dev) )
	{	printk("Disabling interrupt\n");
		if (kp->board->irqdis)
			kp->board->irqdis(kp);
		if (kp->board->irqdis_int)
			kp->board->irqdis_int(kp);
	}
	return 0;
}

/**
 * u8500_kp_resume - resumes keypad
 * @pdev:       platform data
 */

int u8500_kp_resume(struct platform_device *pdev)
{	struct keypad_t *kp = platform_get_drvdata(pdev);
	if ( kpmode )
	{	printk("Disabling interrupt\n");
		if(kp->board->irqdis)
			kp->board->irqdis(kp);
		if (kp->board->irqdis_int)
			kp->board->irqdis_int(kp);
	}
	if ( !device_may_wakeup(&pdev->dev) )
	{	printk("Enabling interrupt\n");
		kp->board->irqen(kp);
	}
	return 0;
}

#else
#define u8500_kp_suspend NULL
#define u8500_kp_resume NULL
#endif				/* CONFIG_PM */

/**
 * u8500_kp_probe - keypad module probe function
 * @pdev:	driver platform data
 *
 * Allocates data memory, registers the module with input subsystem,
 * initializes keypad default condition, initializes keypad interrupt handler
 * for interrupt mode operation, initializes keypad work queues functions for
 * polling mode operation
 */
static int __init u8500_kp_probe(struct platform_device *pdev)
{
	struct keypad_t *kp;
	int err = 0;
	struct keypad_device *keypad_board;
	printk("\nkeypad probe called");

	if(!pdev)
		return -EINVAL;
	keypad_board = pdev->dev.platform_data;
	kp = kzalloc(sizeof(struct keypad_t), GFP_KERNEL);
	if (!kp) {
		err = -ENOMEM;
		goto err_kzalloc;
	}
	platform_set_drvdata(pdev, kp);
	kp = platform_get_drvdata(pdev);
	kp->irq = platform_get_irq(pdev, 0);
	if (!kp->irq) {
		printk(KERN_ERR "keypad irq not defined");
		err = -1;
		goto err_board;
	}
	printk("\nkp_probe 1");
	if (!keypad_board) {
		printk(KERN_ERR "keypad platform data not defined");
		err = -1;
		goto err_board;
	}
	printk("\nkp_probe 2");
	kp->board = keypad_board;
	kp->mode = kpmode;
	kp->inp_dev = input_allocate_device();
	if (!kp->inp_dev) {
		printk(KERN_ERR "Could not allocate memory for the device");
		err = -1;
		goto err_inp_devalloc;
	}
	printk("\nkp_probe 3");

	kp->inp_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REP);
	kp->inp_dev->name = pdev->name;
	kp->inp_dev->phys = "stkpd/input0";

	kp->inp_dev->id.product = KEYPAD_VER_X;
	kp->inp_dev->id.version = KEYPAD_VER_Y * 0x0ff + KEYPAD_VER_Z;
//	kp->inp_dev->private = kp;

	clear_bit(KPINTR_LKBIT, &kp->lockbits);

	err = u8500_kp_init_keypad(kp);
	if (err) {
		goto err_init_kpd;
	}
	printk("\nkp_probe 4");

	if (input_register_device(kp->inp_dev) < 0) {
		printk(KERN_ERR "Could not register input device");
		err = -1;
		goto err_inp_reg;
	} else {
		printk("Registered keypad module with input subsystem");
	}
	printk("\nkp_probe 5");

	/* Initialize keypad workques  */
	INIT_DELAYED_WORK(&kp->kscan_work, u8500_kp_wq_kscan);

	/* Initialize keypad interrupt handler  */
	if (!kp->mode) {	/* true if interrupt mode operation */
#if 0
		err = request_irq(kp->irq, u8500_kp_intrhandler,kp->board->irqtype, kp->inp_dev->name, kp);
		if (err) {
			stm_error("Could not allocate irq %d for keypad",
				   kp->irq);
			printk("ERROR = %d\n",err);
			goto err_req_irq;
		}
		kp->board->irqen(kp);
		kp->board->autoscan_en();
#endif
	} else {
		/* Schedule workqueue for polling mode operaion. */
		//kp->board->autoscan_en();
		schedule_delayed_work(&kp->kscan_work, KEYPAD_SCAN_PERIOD);
		printk(KERN_INFO "Keypad polling started");
	}

	printk(KERN_INFO "Module initialized Ver(%d.%d.%d)",
		 KEYPAD_VER_X, KEYPAD_VER_Y, KEYPAD_VER_Z);
	return 0;

      err_req_irq:
      err_inp_reg:
	/* unregistering device */
	input_unregister_device(kp->inp_dev);
      err_inp_devalloc:
      err_init_kpd:
	input_free_device(kp->inp_dev);
      err_board:
	kfree(kp);
      err_kzalloc:
	return err;
}

/**
 * u8500_kp_remove - keypad module remove function
 *
 * @pdev:	driver platform data
 * Disables Keypad interrupt if any, frees allocated keypad interrupt if any,
 * cancels keypad work queues if any, deallocate used GPIO pin, unregisters the
 * module, frees used memory
 */
static int u8500_kp_remove(struct platform_device *pdev)
{
	struct keypad_t *kp = platform_get_drvdata(pdev);

	/*Frees allocated keypad interrupt if any */
	if (kp->board->exit)
		kp->board->exit(kp);
#if 0
	if (!kp->mode)
		free_irq(kp->irq, kp);
#endif
	/* cancel and flush keypad work queues if any  */
	cancel_delayed_work(&kp->kscan_work);

	cancel_delayed_work_sync(&kp->kscan_work); /* block until work struct's callback terminates */

	input_unregister_device(kp->inp_dev);
	input_free_device(kp->inp_dev);
	kfree(kp);
	printk(KERN_INFO "Module removed....");
	return (0);
}

struct platform_driver u8500kpd_driver = {
	.probe = u8500_kp_probe,
	.remove = u8500_kp_remove,
	.driver = {
		   .name = "u8500-kp",
		   },
	.suspend = u8500_kp_suspend,
	.resume = u8500_kp_resume,
};

static int __devinit u8500_kp_init(void)
{
	return platform_driver_register(&u8500kpd_driver);
}

static void __exit u8500_kp_exit(void)
{
	platform_driver_unregister(&u8500kpd_driver);
}

module_init(u8500_kp_init);
module_exit(u8500_kp_exit);

MODULE_AUTHOR("ST Ericsson");
MODULE_DESCRIPTION("u8500 keyboard driver");
MODULE_LICENSE("GPL");
