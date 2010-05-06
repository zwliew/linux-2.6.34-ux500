#
# Makefile for ST-Ericsson connectivity multifunction miscellaneous devices
#
#EXTRA_CFLAGS += $(KBUILD_GCOV_FLAGS)

obj-$(CONFIG_MFD_STE_CONN)       += ste_conn.o
ste_conn-objs                    := ste_conn_ccd.o ste_conn_cpd.o ste_conn_char_devices.o
export-objs                      := ste_conn_cpd.o

obj-$(CONFIG_MFD_STE_CONN_BLUEZ) += ste_conn_hci_driver.o

obj-$(CONFIG_MFD_STE_CG29XX_AUDIO) += ste_cg29xx_audio.o

obj-$(CONFIG_MFD_STE_CONN_CG2900) += ste_cg2900.o
obj-$(CONFIG_MFD_STE_CONN_STLC2690) += ste_stlc2690.o
