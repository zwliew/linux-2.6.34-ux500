# Makefile for the Linux sound card driver
#

obj-$(CONFIG_U8500_ACODEC) += u8500mod_acodec.o
obj-$(CONFIG_SOUND) += soundcore.o
obj-$(CONFIG_SOUND_PRIME) += sound_firmware.o
obj-$(CONFIG_SOUND_PRIME) += oss/
obj-$(CONFIG_DMASOUND) += oss/
obj-$(CONFIG_SND) += core/ i2c/ drivers/ isa/ pci/ ppc/ arm/ sh/ synth/ usb/ \
	sparc/ spi/ parisc/ pcmcia/ mips/ soc/ atmel/
obj-$(CONFIG_SND_AOA) += aoa/

# This one must be compilable even if sound is configured out
obj-$(CONFIG_AC97_BUS) += ac97_bus.o

ifeq ($(CONFIG_SND),y)
  obj-y += last.o
endif

ifeq ($(CONFIG_U8500_AB8500_ACODEC),y)
u8500mod_acodec-objs := u8500_acodec_ab8500.o ab8500_codec.o
endif
soundcore-objs  := sound_core.o
