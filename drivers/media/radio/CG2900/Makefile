#
# Makefile for the STE CG2900 Radio Driver
#

radio_CG2900-objs := radio-CG2900.o stefmapi.o fmdriver.o platformosapi.o

obj-$(CONFIG_RADIO_CG2900) += radio_CG2900.o

EXTRA_CFLAGS += -Iinclude/linux -Iinclude/media -Idrivers/media/radio/CG2900 -Idrivers/mfd/ste_conn -Iinclude/linux/mfd
