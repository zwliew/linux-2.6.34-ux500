#
# Makefile for the linux kernel, U8500 machine.
#
#
ifeq ($(wildcard $(TOPDIR)/.config), $(TOPDIR)/.config)
include $(TOPDIR)/.config
endif

# Object file lists.

PLATFORM_NAME = $(shell echo $(CONFIG_U8500_TARGET))

obj-y					:= clock.o timer.o prcmu-fw.o timer-rtt.o
obj-$(CONFIG_REGULATOR)			+= regulator.o
obj-$(CONFIG_ARCH_U8500)		+= devices.o mcde.o hsi.o
obj-$(CONFIG_STM_DMA)			+= dma_40.o   #part1 no. of dma is 0x40
obj-$(CONFIG_HOTPLUG_CPU)           += hotplug.o
obj-$(CONFIG_MACH_U8500_MOP)		+= mop500_devices.o
obj-$(CONFIG_MACH_U8500_SIMULATOR)	+= sim500_devices.o
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o localtimer.o
obj-$(CONFIG_U8500_CPUIDLE) += cpuidle.o
obj-$(CONFIG_U8500_CPUFREQ) += cpufreq.o
obj-$(CONFIG_PM)            += pm.o

ifeq ($(CONFIG_MFD_STE_CONN), m)
obj-y					+= ste_conn_devices.o
else
obj-$(CONFIG_MFD_STE_CONN)		+= ste_conn_devices.o
endif

PDEV_HEADER = arch/arm/mach-u8500/include/mach/board_devices.h

$(TOPDIR)/$(PDEV_HEADER):
	$(Q)echo "Generating SYMLINK $(PDEV_HEADER)->$(PLATFORM_NAME)_devices.h"
	$(Q)rm -rf $@
	$(Q)ln -fsn $(PLATFORM_NAME)_devices.h $@

machprepare: $(TOPDIR)/$(PDEV_HEADER)

machclean:
	$(Q)rm -rf *mod.o *.mod.c *.o *.ko $(TOPDIR)/$(PDEV_HEADER)

machmrproper:
	$(Q)rm -rf $(TOPDIR)/$(PDEV_HEADER)
