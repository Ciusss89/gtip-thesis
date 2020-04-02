## App name
#
APPLICATION = ihb

## Target
#
BOARD ?= nucleo-l476rg

## This has to be the absolute path to the RIOT base directory:
#
RIOTBASE ?= $(CURDIR)/../../RIOT/

## Chose the debug level: LOG_NONE, LOG_WARNING, LOG_ERROR, LOG_DEBUG, LOG_ALL
CFLAGS += -DLOG_LEVEL=LOG_NONE

#CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += conn_can
USEMODULE += auto_init_can
USEMODULE += can_pm
USEMODULE += random

FEATURES_REQUIRED += periph_can
FEATURES_REQUIRED += periph_gpio_irq

CFLAGS += -DGNRC_PKTBUF_SIZE=4096
CFLAGS += -DCAN_PKT_BUF_SIZE=64
CFLAGS += -DCAN_ROUTER_MAX_FILTER=32

## Custom modules
#

USEMODULE += uptime
DIRS += $(CURDIR)/uptime

USEMODULE += ihbcan
DIRS += $(CURDIR)/ihb-can

USEMODULE += ihbnetsim
DIRS += $(CURDIR)/ihb-netsim

CFLAGS += -I$(CURDIR)
include $(RIOTBASE)/Makefile.include
