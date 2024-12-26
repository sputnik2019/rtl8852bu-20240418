ifeq ($(CONFIG_PLATFORM_AUTODETECT), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211 -DRTW_USE_CFG80211_STA_EVENT

# To ensure the driver is using the regulatory information from the os
EXTRA_CFLAGS += -DCONFIG_REGD_SRC_FROM_OS
# May be needed to ensure all channels are scanned
#EXTRA_CFLAGS += -DCONFIG_FORCE_SW_CHANNEL_PLAN

EXTRA_CFLAGS += -DCONFIG_RADIO_WORK
#EXTRA_CFLAGS += -DCONFIG_CONCURRENT_MODE

#SUBARCH := $(shell uname -m | sed -e s/i.86/i386/)
SUBARCH := $(shell uname -m | sed -e "s/i.86/i386/; s/aarch64/arm64/; s/armv.l/arm/; s/riscv.*/riscv/; s/ppc/powerpc/;")
ARCH ?= $(SUBARCH)

CROSS_COMPILE ?=
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
INSTALL_PREFIX :=
STAGINGMODDIR := /lib/modules/$(KVER)/kernel/drivers/staging

_PLATFORM_FILES += platform/platform_ops.o

endif
