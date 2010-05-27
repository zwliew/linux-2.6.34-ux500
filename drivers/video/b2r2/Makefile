# Make file for compiling and loadable module B2R2

# TODO: This should be moved to Kconfig
CONFIG_B2R2_DEBUG := y

# Fix for hardware bug requiring rectangles to be a multiple of 16 high
EXTRA_CFLAGS += -DB2R2_ROTATION_HEIGHT_BUGFIX

# Enable fallback to generic
EXTRA_CFLAGS += -DB2R2_GEN_OPT_MIX

obj-$(CONFIG_FB_B2R2) += b2r2.o

b2r2-objs = b2r2_blt_main.o b2r2_core.o b2r2_mem_alloc.o b2r2_generic.o b2r2_node_gen.o b2r2_node_split.o b2r2_profiler_socket.o b2r2_timing.o

ifdef CONFIG_B2R2_DEBUG
EXTRA_CFLAGS += -DCONFIG_B2R2_DEBUG -DDEBUG
b2r2-objs += b2r2_debug.o
endif

ifeq ($(CONFIG_FB_B2R2),m)
obj-y += b2r2_kernel_if.o
endif

clean-files := b2r2.o b2r2_blt_main.o b2r2_core.o b2r2_mem_alloc.o b2r2_generic.o b2r2_node_gen.o b2r2_node_split.o b2r2_profiler_socket.o b2r2_timing.o b2r2_debug.o modules.order built-in.o
