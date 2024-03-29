#
# Makefile for the linux kernel, U8500 machine.
#
#

ifeq ($(CONFIG_CRYPTO_DEV_UX500_HASH), m)
	CFLAGS_devices.o += -DCONFIG_CRYPTO_DEV_UX500_HASH
	CFLAGS_board-mop500.o += -DCONFIG_CRYPTO_DEV_UX500_HASH
endif

obj-y					:= clock.o timer.o timer-rtt.o gpio.o \
						localtimer.o
obj-$(CONFIG_REGULATOR)			+= regulator.o  mop500-regulators.o
obj-$(CONFIG_ARCH_U8500)		+= devices.o cpu.o
obj-$(CONFIG_STM_DMA)			+= dma_40.o   #part1 no. of dma is 0x40
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o
obj-$(CONFIG_MACH_U8500_MOP)		+= board-mop500.o mop500-sdi.o
obj-$(CONFIG_UX500_SOC_DB5500)		+= db5500-devices.o cpu-db5500.o
obj-$(CONFIG_UX500_SOC_DB8500)		+= db8500-devices.o hsi.o mcde.o \
					   cpu-db8500.o prcmu-fw.o timer-db8500-prcm.o
obj-$(CONFIG_FB_MCDE)			+= board-mop500-mcde.o
obj-$(CONFIG_MACH_U8500_SIMULATOR)	+= board-mop500.o mop500-sdi.o
obj-$(CONFIG_MACH_U5500_SIMULATOR)	+= board-u5500.o
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o localtimer.o
obj-$(CONFIG_U8500_CPUIDLE) 		+= cpuidle.o
obj-$(CONFIG_U8500_CPUFREQ) 		+= cpufreq.o
obj-$(CONFIG_U8500_PM)			+= pm.o savecontext.o
obj-$(CONFIG_SENSORS1P_MOP)            	+= sensors1p.o
obj-$(CONFIG_USB)            		+= musb_db8500.o
obj-$(CONFIG_U5500_MLOADER_HELPER)      += mloader_helper.o
obj-$(CONFIG_U5500_MODEM_IRQ)		+= modem_irq.o
obj-$(CONFIG_U5500_MBOX)		+= mbox.o
obj-$(CONFIG_U5500_PWM)			+= pwm.o

ifeq ($(CONFIG_MFD_STE_CONN), m)
obj-y					+= ste_conn_devices.o
else
obj-$(CONFIG_MFD_STE_CONN)		+= ste_conn_devices.o
endif

ifndef CONFIG_MACH_U5500_SIMULATOR
	obj-$(CONFIG_TEE_SUPPORT)	+= tee_ux500.o
endif
