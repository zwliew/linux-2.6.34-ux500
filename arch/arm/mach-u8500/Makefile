#
# Makefile for the linux kernel, U8500 machine.
#
#

obj-y					:= clock.o timer.o timer-rtt.o
obj-$(CONFIG_REGULATOR)			+= regulator.o
obj-$(CONFIG_ARCH_U8500)		+= devices.o
obj-$(CONFIG_STM_DMA)			+= dma_40.o   #part1 no. of dma is 0x40
obj-$(CONFIG_HOTPLUG_CPU)           += hotplug.o
obj-$(CONFIG_MACH_U8500_MOP)		+= mop500_devices.o mop500-sdi.o
obj-$(CONFIG_UX500_SOC_DB5500)		+= db5500-devices.o cpu-db5500.o
obj-$(CONFIG_UX500_SOC_DB8500)		+= db8500-devices.o hsi.o mcde.o \
					   cpu-db8500.o prcmu-fw.o
obj-$(CONFIG_MACH_U8500_SIMULATOR)	+= mop500_devices.o mop500-sdi.o
obj-$(CONFIG_MACH_U5500_SIMULATOR)	+= board-u5500.o
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o localtimer.o
obj-$(CONFIG_U8500_CPUIDLE) += cpuidle.o
obj-$(CONFIG_U8500_CPUFREQ) += cpufreq.o
obj-$(CONFIG_U8500_PM)			+= pm.o

ifeq ($(CONFIG_MFD_STE_CONN), m)
obj-y					+= ste_conn_devices.o
else
obj-$(CONFIG_MFD_STE_CONN)		+= ste_conn_devices.o
endif
