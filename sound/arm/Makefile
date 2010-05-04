#
# Makefile for ALSA
#

obj-$(CONFIG_SND_ARMAACI)	+= snd-aaci.o
snd-aaci-objs			:= aaci.o

obj-$(CONFIG_SND_PXA2XX_PCM)	+= snd-pxa2xx-pcm.o
snd-pxa2xx-pcm-objs		:= pxa2xx-pcm.o

obj-$(CONFIG_SND_PXA2XX_AC97)	+= snd-pxa2xx-ac97.o
snd-pxa2xx-ac97-objs		:= pxa2xx-ac97.o
obj-$(CONFIG_SND_U8500_ALSA_AB8500)  += u8500mod_alsa.o
ifneq ($(CONFIG_SND_U8500_ALSA_AB8500),n)
u8500mod_alsa-objs		:= u8500_alsa_ab8500.o devdma.o u8500_alsa_hdmi.o
endif
