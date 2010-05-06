# Make file for compiling and loadable module MCDE

obj- := mcde.o	# Dummy rule to force built-in.o to be made

obj-$(CONFIG_FB_MCDE) += mcde_hwaccess.o

obj-$(CONFIG_FB_MCDE) += dsi_hwaccess.o

obj-$(CONFIG_FB_MCDE) += dsi_link_utils.o

obj-$(CONFIG_FB_MCDE) += mcde_utils.o

obj-$(CONFIG_FB_MCDE) += mcde.o

clean-files := mcde_hwaccess.o dsi_hwaccess.o dsi_link_utils.o mcde_utils.o mcde.o modules.order built-in.o

