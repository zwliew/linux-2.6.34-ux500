ifeq ($(CONFIG_CAIF_DEBUG),1)
CAIF_DBG_FLAGS := -DDEBUG
endif

ccflags-y := $(CAIF_FLAGS) $(CAIF_DBG_FLAGS)

caif-objs := caif_dev.o caif_chnlif.o \
	generic/cfcnfg.o generic/cfmuxl.o generic/cfctrl.o  \
	generic/cffrml.o generic/cfveil.o generic/cflist.o  \
	generic/cfserl.o generic/cfdgml.o  \
	generic/cfrfml.o generic/cfvidl.o generic/cfutill.o \
	generic/cfsrvl.o generic/cfpkt_skbuff.o caif_config_util.o

clean-dirs:= .tmp_versions

clean-files:= \
	Module.symvers \
	modules.order \
	*.cmd \
	*~ \
	generic/*.o \
	generic/*~

obj-$(CONFIG_CAIF) += caif.o
obj-$(CONFIG_CAIF_NETDEV) += chnl_net.o
obj-$(CONFIG_CAIF_SOCK) += caif_socket.o

export-objs := caif.o

#official-kernel-patch-cut-here

# CAIF character device
obj-$(CONFIG_CAIF_CHARDEV) += chnl_chr.o caif_chr.o

