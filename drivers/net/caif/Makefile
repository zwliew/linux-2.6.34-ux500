ifeq ($(CONFIG_CAIF_DEBUG),1)
CAIF_DBG_FLAGS := -DDEBUG
endif

KBUILD_EXTRA_SYMBOLS=net/caif/Module.symvers

ccflags-y := $(CAIF_FLAGS) $(CAIF_DBG_FLAGS)
clean-dirs:= .tmp_versions
clean-files:= Module.symvers modules.order *.cmd *~ \

# Serial interface
obj-$(CONFIG_CAIF_TTY) += caif_serial.o
#deprecated-functionality-below
# Loopback
obj-$(CONFIG_CAIF_LOOPBACK) += caif_loop.o

# Legacy Drivers, remove when migration ready
#obj-$(CONFIG_CAIF_TTY) += phyif_ser.o
#obj-$(CONFIG_CAIF_LOOPBACK) += phyif_loop.o

cfspi_master-objs := caif_spi.o caif_spi_master.o
cfspi_slave-objs := caif_spi.o caif_spi_slave.o

# SPI master physical interfaces module
obj-$(CONFIG_CAIF_SPI_MASTER) += cfspi_master.o

# SPI slave physical interfaces module
obj-$(CONFIG_CAIF_SPI_SLAVE) += cfspi_slave.o

# Shared memory
obj-$(CONFIG_CAIF_SHM) += phyif_shm.o

# Mailbox generic
obj-$(CONFIG_CAIF_MBXIF) += shm_mbxifc.o

# Simulated SPI loop devices
obj-$(CONFIG_CAIF_SPI_LOOP) += spi_loop_master.o spi_loop_slave.o

# Simulated mailbox
obj-$(CONFIG_CAIF_SMBX) += shm_smbx.o
