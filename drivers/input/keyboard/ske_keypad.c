/*
 * Copyright (C) ST-Ericsson SA 2009
 * Author: Naveen Kumar G <naveen.gaddipati@stericsson.com> for ST-Ericsson
 * License terms:GNU General Public License (GPL) version 2
 *
 * This driver is used for SKE(Scroll Key Encoder) controller in
 * all Ux500 platform.
 */

#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/kpd.h>

#define SKE_CR 0x00
#define SKE_VAL0 0x04
#define SKE_VAL1 0x08
#define SKE_DBCR 0x0C
#define SKE_IMSC 0x10
#define SKE_RIS 0x14
#define SKE_MIS 0x18
#define SKE_ICR 0x1C
#define SKE_ASR0 0x20
#define SKE_ASR1 0x24
#define SKE_ASR2 0x28
#define SKE_ASR3 0x2C

#define SKE_KEYPAD_VER_MAJOR 1
#define SKE_KEYPAD_VER_MINOR 0
#define SKE_KEYPAD_VER_RELEASE 0
#define KP_ASDIS 0x00000000
#define KP_ASEN 0x00000004
#define KP_SOFTSCAN_INTR_EN 0x00000008
#define KP_SOFTSCAN_INTR_CLR 0x00000008
#define KP_HI_ALLCOLS 0x0000FF00
#define KP_LOW_ALLCOLS 0x00000000
#define KP_AS_INTR_EN 0x04

#define KP_MULT 0x40
#define KP_MULT_SCAN_ALL 0x38
#define KP_ASCANON 0x80
#define KP_DBCR_PERIOD 0x1000

#define CLR_KP_AUTOSCAN_INTR_MASK 0x00
#define CLR_ALL_INTR 0xF
#define KP_AUTOSCAN_INTR_CLR 0x04
#define KP_AUTOSCAN_INTR_RIS 0x04
#define DISABLE_AUTOSCAN 0x00000000
#define DEFAULT_ASCAN_VALUE 0x00000000

#define LOWER_BYTE 0xFF
#define UPPER_BYTE 0xFF00
#define SHIFT_8 8
#define REG_OFFSET 0x04

static void ske_kp_wq_kscan(struct work_struct *work);

/**
 * ske_kp_autoscan_en - enables the autoscan mode
 * @kp: keypad device data pointer
 * This function enables the autoscan mode
*/
static void ske_kp_autoscan_en(struct keypad_t *kp)
{
	int value;
	unsigned long flags;
	spin_lock_irqsave(&kp->cr_lock, flags);
	writel(KP_MULT, (kp->ske_regs + SKE_CR));
	value = readl(kp->ske_regs + SKE_CR);
	value = (value | KP_ASEN | KP_LOW_ALLCOLS | KP_MULT_SCAN_ALL);
	writel(value, (kp->ske_regs + SKE_CR));
	writel(KP_DBCR_PERIOD, (kp->ske_regs + SKE_DBCR));
	writel(CLR_ALL_INTR, (kp->ske_regs + SKE_ICR));
	spin_unlock_irqrestore(&kp->cr_lock, flags);
}

/**
 * ske_kp_autoscan_disable - Disables the autoscan mode
 * @kp:  keypad device data pointer
 * This function disables the autoscan mode
*/
static void ske_kp_autoscan_disable(struct keypad_t *kp)
{
	unsigned long ske_cr_reg_value = KP_ASCANON;
	unsigned long flags;
	spin_lock_irqsave(&kp->cr_lock, flags);
	while (ske_cr_reg_value & KP_ASCANON)
		ske_cr_reg_value = readl(kp->ske_regs + SKE_CR);
	writel(DISABLE_AUTOSCAN, (kp->ske_regs + SKE_CR));
	spin_unlock_irqrestore(&kp->cr_lock, flags);
}

/**
 * ske_kp_autoscan_intr_enable - Enables the autoscan mode
 * @kp:  keypad device data pointer
 * This function enables the interrupt in autoscan mode
*/
static void ske_kp_autoscan_intr_enable(struct keypad_t *kp)
{
	unsigned long flags;
	spin_lock_irqsave(&kp->cr_lock, flags);
	writel(KP_AS_INTR_EN, (kp->ske_regs + SKE_IMSC));
	kp->board->int_status = KP_INT_ENABLED;
	spin_unlock_irqrestore(&kp->cr_lock, flags);
}

/**
 * ske_kp_autoscan_intr_disable - Disables the autoscan mode
 * @kp:  keypad device data pointer
 * This function disables the interrupt in autoscan mode
*/
static void ske_kp_autoscan_intr_disable(struct keypad_t *kp)
{
	unsigned long flags;
	spin_lock_irqsave(&kp->cr_lock, flags);
	writel(CLR_KP_AUTOSCAN_INTR_MASK, (kp->ske_regs + SKE_IMSC));
	writel(KP_AUTOSCAN_INTR_CLR, (kp->ske_regs + SKE_ICR));
	kp->board->int_status = KP_INT_DISABLED;
	spin_unlock_irqrestore(&kp->cr_lock, flags);
}

/**
 * ske_kp_autoscan_check - check the autoscan mode
 * @kp: keypad device data pointer
 * This function used to check the autoscan mode
*/
static bool ske_kp_autoscan_check(struct keypad_t *kp)
{
	unsigned long ske_cr_reg_value = 0;
	unsigned long flags = 0;
	bool ret;
	spin_lock_irqsave(&kp->cr_lock, flags);
	ske_cr_reg_value = readl(kp->ske_regs + SKE_CR);
	spin_unlock_irqrestore(&kp->cr_lock, flags);
	ret = (ske_cr_reg_value & KP_ASCANON);
	return ret;
}
/**
 * ske_kp_key_pressed - report key pressed
 * @kp:  keypad device data pointer
 * @row:  row number
 * @col:  col number
 * @scancode:  scane code for the key
 * This function is used to report key pressed
*/
static void ske_kp_key_pressed(struct keypad_t *kp, int row, int col,
				u8 *scancode)
{
	if (kp->key_state[row][col] == KEYPAD_STATE_DEFAULT) {
		kp->key_cnt++;
		input_report_key(kp->inp_dev, *scancode, true);
		kp->key_state[row][col] = KEYPAD_STATE_PRESSACK;
	}
}

/**
 * ske_kp_key_released - report key released
 * @kp:  keypad device data pointer
 * @row:  row number
 * @col:  col number
 * @scancode:  scane code for the key
 * This function is used to report key released
*/
static void ske_kp_key_released(struct keypad_t *kp, int row, int col,
				u8 *scancode)
{
	int value;
	if (kp->key_state[row][col] == KEYPAD_STATE_PRESSACK) {
		value = KP_AUTOSCAN_INTR_RIS;
		while (value && KP_AUTOSCAN_INTR_RIS)
			value = readl(kp->ske_regs + SKE_RIS);
		kp->key_cnt--;
		input_report_key(kp->inp_dev, *scancode, false);
		kp->key_state[row][col] = KEYPAD_STATE_DEFAULT;
	}
}

/**
 * ske_kp_autoscan_results - check the results of autoscan
 * @kp:  keypad device data pointer
 * This function is used to check the results of autoscan
*/
static void ske_kp_autoscan_results(struct keypad_t *kp)
{
	unsigned int col_val[MAX_KPCOL];
	int col = 0, row = 0, mask;
	unsigned long add = 0;
	u8 *p_kcode;
	int key_pressed;
	unsigned long flags;
	spin_lock_irqsave(&kp->cr_lock, flags);
	while (col < MAX_KPCOL) {
		col_val[col] =
			readl(kp->ske_regs + SKE_ASR0 + add) & LOWER_BYTE;
		col_val[col + 1] =
			(readl(kp->ske_regs + SKE_ASR0 + add) & UPPER_BYTE)
				>> SHIFT_8;
		col += 2;
		add = add + REG_OFFSET;
	}
	spin_unlock_irqrestore(&kp->cr_lock, flags);
	for (col = 0; col < MAX_KPCOL; col++) {
		if (col_val[col] != 0) {
			p_kcode = kp->board->kcode_tbl + col;
			for (row = 0; row < MAX_KPROW; row++) {
				mask = (1 << row);
				key_pressed = 0;
				key_pressed = (col_val[col] & mask);
				if (key_pressed != 0) {
					ske_kp_key_pressed(kp, row, col,
								p_kcode);
					ske_kp_key_released(kp, row, col,
								p_kcode);
				}
				p_kcode += MAX_KPROW;
				mask = 0;
			}
		}
	}
	input_sync(kp->inp_dev);
}
/**
 * ske_kp_key_irqen- enables keypad interrupt
 * @kp:  keypad device data pointer
 *
 * enables keypad interrupt
 */
static int ske_kp_key_irqen(struct keypad_t *kp)
{
	if (kp->board->int_status != KP_INT_ENABLED)
		ske_kp_autoscan_intr_enable(kp);
	return 0;
}

/**
 * ske_kp_key_irqdis- disables keypad interrupt
 * @kp:  keypad device data pointer
 *
 * disables keypad interrupt
 */
static int ske_kp_key_irqdis(struct keypad_t *kp)
{
	if (kp->board->int_status != KP_INT_DISABLED)
		ske_kp_autoscan_intr_disable(kp);
	return 0;
}
/**
 * ske_kp_intrhandler - keypad interrupt handler
 * @irq: irq number for keypad
 * @dev_id: pointer to keypad device id
 *
 * checks for valid interrupt, disables interrupt to avoid any nested interrupt
 * starts work queue for further key processing with debouncing logic
 */
static irqreturn_t ske_kp_intrhandler(int irq, void *dev_id)
{
	struct keypad_t *kp = (struct keypad_t *)dev_id;
	if (!(test_bit(KPINTR_LKBIT, &kp->lockbits))) {
		set_bit(KPINTR_LKBIT, &kp->lockbits);
		ske_kp_key_irqdis(kp);
		schedule_delayed_work(&kp->kscan_work,
				kp->board->debounce_period);
	}
	return IRQ_HANDLED;
}

/**
 * ske_kp_wq_kscan - work queue for keypad scanning
 * @work: pointer to keypad data
 *
 * Executes at each scan tick, execute the key press/release function,
 * Generates key press/release event message for input subsystem for valid key
 * events, enables keypad interrupts (for int mode)
 */

static void ske_kp_wq_kscan(struct work_struct *work)
{
	bool ret = true;
	struct keypad_t *kp =
		container_of((struct delayed_work *)work,
				struct keypad_t, kscan_work);

	while (ret) {
		udelay(100);
		ret = ske_kp_autoscan_check(kp);
	}
	kp->key_cnt = 0;
	ske_kp_autoscan_results(kp);
	clear_bit(KPINTR_LKBIT, &kp->lockbits);
	ske_kp_key_irqen(kp);
}

/**
 * ske_kp_init_keypad - keypad parameter initialization
 * @kp:  keypad device data pointer
 *
 * Initializes Keybits to enable keyevents
 * Initializes Initial keypress status to default
 * Calls the keypad platform specific init function.
 */
int __init ske_kp_init_keypad(struct keypad_t *kp)
{
	int row, column, err;
	u8 *p_kcode = kp->board->kcode_tbl;
	if (kp->board->init) {
		err = kp->board->init(kp);
		if (err)
			return err;
	}
	for (row = 0; row < MAX_KPROW; row++) {
		for (column = 0; column < MAX_KPCOL; column++) {
			set_bit(*p_kcode, kp->inp_dev->keybit);
			kp->key_state[row][column] =
					KEYPAD_STATE_DEFAULT;
			p_kcode++;
		}
	}
	return err;
}

#ifdef CONFIG_PM
/**
 * ske_kp_suspend - suspend keypad
 * @pdev: platform data
 * @state: power down level
 * This function is used to suspend the keypad
 */
static int ske_kp_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct keypad_t *kp = platform_get_drvdata(pdev);
	dev_dbg(&pdev->dev, "suspend start\n");
	if (!device_may_wakeup(&pdev->dev))
		ske_kp_key_irqdis(kp);
	dev_dbg(&pdev->dev, "suspend done\n");
	return 0;
}

/**
 * ske_kp_resume - resumes keypad
 * @pdev: platform data
 * This function is used to resume the keypad
 */

static int ske_kp_resume(struct platform_device *pdev)
{
	struct keypad_t *kp = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "resume start\n");
	if (!device_may_wakeup(&pdev->dev))
		ske_kp_key_irqen(kp);
	dev_dbg(&pdev->dev, "resume done\n");
	return 0;
}

#endif

/**
 * ske_kp_probe - keypad module probe function
 * @pdev: driver platform data
 *
 * Allocates data memory, registers the module with input subsystem,
 * initializes keypad default condition, initializes keypad interrupt handler
 * for interrupt mode operation.
 */
static int __init ske_kp_probe(struct platform_device *pdev)
{
	struct keypad_t *kp;
	struct resource *res = NULL;
	int err = 0;
	struct keypad_device *keypad_board;

	if (!pdev)
		return -EINVAL;
	dev_dbg(&pdev->dev, "probe start\n");

	keypad_board = pdev->dev.platform_data;
	kp = kzalloc(sizeof(struct keypad_t), GFP_KERNEL);
	if (!kp) {
		err = -ENOMEM;
		goto err_kzalloc;
	}
	platform_set_drvdata(pdev, kp);
	kp = platform_get_drvdata(pdev);
	if (!keypad_board) {
		dev_err(&pdev->dev, "platform data not defined\n");
		err = -1;
		goto err_board;
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "memory resources not defined\n");
		err = -EINVAL;
		goto err_board;
	} else {
		kp->ske_regs = ioremap(res->start, resource_size(res));
		if (kp->ske_regs == NULL) {
			dev_err(&pdev->dev, "alloc reg space failed\n");
			err = -ENOMEM;
			goto err_board;
		}
	}
	kp->irq = platform_get_irq(pdev, 0);
	if (!kp->irq) {
		dev_err(&pdev->dev, "irq not defined\n");
		err = -EINVAL;
		goto err_board;
	}
	kp->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(kp->clk)) {
		dev_err(&pdev->dev, "clk error\n");
		err = PTR_ERR(kp->clk);
		goto err_board;
	} else {
		clk_enable(kp->clk);
	}
	kp->board = keypad_board;
	kp->inp_dev = input_allocate_device();
	if (!kp->inp_dev) {
		dev_err(&pdev->dev, "alloc memory failed\n");
		err = -EINVAL;
		goto err_inp_devalloc;
	}
	clear_bit(KPINTR_LKBIT, &kp->lockbits);
	spin_lock_init(&kp->cr_lock);
	err = ske_kp_init_keypad(kp);
	if (err) {
		dev_err(&pdev->dev, "init hardware failed\n");
		goto err_init_kpd;
	}
	INIT_DELAYED_WORK(&kp->kscan_work, ske_kp_wq_kscan);
	ske_kp_autoscan_en(kp);
	err = request_irq(kp->irq, ske_kp_intrhandler,
				kp->board->irqtype, "ske", kp);
	if (err) {
		dev_err(&pdev->dev, "allocate irq %d failed\n", kp->irq);
		goto err_req_irq;
	}
	ske_kp_key_irqen(kp);

	set_bit(EV_KEY, kp->inp_dev->evbit);
	set_bit(EV_REP, kp->inp_dev->evbit);
	kp->inp_dev->id.bustype = BUS_HOST;
	kp->inp_dev->name = "ske-kp";
	kp->inp_dev->phys = "ske-kp/input0";
	kp->inp_dev->id.product = SKE_KEYPAD_VER_MAJOR;
	kp->inp_dev->id.version = SKE_KEYPAD_VER_MINOR * 0x0ff + SKE_KEYPAD_VER_RELEASE;
	kp->inp_dev->dev.parent = &pdev->dev;
	kp->inp_dev->keycode = kp->board->kcode_tbl;
	kp->inp_dev->keycodesize = sizeof(unsigned char);
	kp->inp_dev->keycodemax = MAX_KEYS;
	clear_bit(0, kp->inp_dev->keybit);
	err = input_register_device(kp->inp_dev);
	if (err) {
		dev_err(&pdev->dev, "could not register input device\n");
		err = -EINVAL;
		goto err_inp_reg;
	}
	dev_dbg(&pdev->dev, "Module initialized Ver(%d.%d.%d)\n",
			SKE_KEYPAD_VER_MAJOR, SKE_KEYPAD_VER_MINOR, SKE_KEYPAD_VER_RELEASE);
	return 0;

err_req_irq:
	free_irq(kp->irq, kp);
err_inp_reg:
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
 * ske_kp_remove - keypad module remove function
 * @pdev:	driver platform data
 *
 * Disables Keypad interrupt if any, frees allocated keypad interrupt if any,
 * cancels keypad work queues if any, deallocate used GPIO pin, unregisters the
 * module, frees used memory
 */
static int ske_kp_remove(struct platform_device *pdev)
{
	struct keypad_t *kp = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "keypad remove start\n");
	ske_kp_autoscan_disable(kp);
	if (kp->board->exit)
		kp->board->exit(kp);
	free_irq(kp->irq, kp);
	cancel_delayed_work(&kp->kscan_work);
	cancel_delayed_work_sync(&kp->kscan_work);
	input_unregister_device(kp->inp_dev);
	input_free_device(kp->inp_dev);
	kp->clk = 0;
	kfree(kp);
	dev_dbg(&pdev->dev, "keypad removed\n");
	return 0;
}

struct platform_driver ske_driver = {
	.probe = ske_kp_probe,
	.remove = ske_kp_remove,
	.driver = {
		.name = "ske-kp",
		},
#ifdef CONFIG_PM
	.suspend = ske_kp_suspend,
	.resume = ske_kp_resume,
#endif
};

static int __devinit ske_kp_init(void)
{
	return platform_driver_register(&ske_driver);
}

static void __exit ske_kp_exit(void)
{
	platform_driver_unregister(&ske_driver);
}

module_init(ske_kp_init);
module_exit(ske_kp_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("NAVEEN KUMAR G");
MODULE_DESCRIPTION("SKE keyboard driver");
