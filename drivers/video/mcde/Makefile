
mcde-objs			:= mcde_mod.o mcde_hw.o mcde_dss.o mcde_display.o mcde_bus.o mcde_fb.o
obj-$(CONFIG_FB_MCDE)		+= mcde.o

obj-$(CONFIG_MCDE_DISPLAY_GENERIC_DSI)	+= display-generic_dsi.o
obj-$(CONFIG_MCDE_DISPLAY_AB8500_DENC)	+= display-ab8500.o
obj-$(CONFIG_MCDE_DISPLAY_AV8100)	+= display-av8100.o

ifdef CONFIG_FB_MCDE_DEBUG
EXTRA_CFLAGS += -DDEBUG
endif
ifdef CONFIG_FB_MCDE_VDEBUG
EXTRA_CFLAGS += -DVERBOSE_DEBUG
endif
