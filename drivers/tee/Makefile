#
# Copyright (C) ST-Ericsson SA 2010
# Author: Martin Hovang (martin.xm.hovang@stericsson.com)
# License terms: GNU General Public License (GPL) version 2
#

EXTRA_CFLAGS = -O0

ifdef CONFIG_MACH_U5500_SIMULATOR
	obj-$(CONFIG_TEE_SUPPORT) += tee_service_svp.o
endif

obj-$(CONFIG_TEE_SUPPORT) += tee_service.o
obj-$(CONFIG_TEE_SUPPORT) += tee_driver.o
obj-$(CONFIG_TEE_SUPPORT) += ta/
