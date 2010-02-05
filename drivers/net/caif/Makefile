ifeq ($(CONFIG_CAIF_DEBUG),1)
CAIF_DBG_FLAGS := -DDEBUG
endif

KBUILD_EXTRA_SYMBOLS=net/caif/Module.symvers

ccflags-y := $(CAIF_FLAGS) $(CAIF_DBG_FLAGS)
clean-dirs:= .tmp_versions
clean-files:= Module.symvers modules.order *.cmd *~ \

# Serial interface
obj-$(CONFIG_CAIF_TTY) += caif_serial.o


#official-kernel-patch-cut-here
# Loopback
obj-$(CONFIG_CAIF_LOOPBACK) += caif_loop.o

# Legacy Drivers, remove when migration ready
#obj-$(CONFIG_CAIF_TTY) += phyif_ser.o
#obj-$(CONFIG_CAIF_LOOPBACK) += phyif_loop.o


# SPI master physical interfaces module
obj-$(CONFIG_CAIF_PHYIF_MSPI) += phyif_mspi.o

# SPI slave physical interfaces module
obj-$(CONFIG_CAIF_PHYIF_SSPI) += phyif_sspi.o

# Shared memory
obj-$(CONFIG_CAIF_SHM) += phyif_shm.o

# Mailbox generic
obj-$(CONFIG_CAIF_MBXIF) += shm_mbxifc.o

# Simulated SPI master devices
obj-$(CONFIG_CAIF_SIM_MSPI) += spi_mspi.o

# Simulated SPI slave devices
obj-$(CONFIG_CAIF_SIM_SSPI) += spi_sspi.o

# Simulated mailbox
obj-$(CONFIG_CAIF_SMBX) += shm_smbx.o




