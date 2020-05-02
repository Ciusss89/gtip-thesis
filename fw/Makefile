## App name
#
APPLICATION = IHB.GTip

## Target
#
BOARD ?= nucleo-l476rg

## This has to be the absolute path to the RIOT base directory:
#
RIOTBASE ?= $(CURDIR)/../../RIOT/

## Firmware version: <tag-name>-<last-commit-of-tag>-<date>
#
CFLAGS += -DIHB_FW_VER=\"$(shell git describe --tags)-$(shell date --iso=date)\"

## IHBNETSIM: Skin nodes for IHB. Set the network size, max size 256.
#
CFLAGS += -DSK_N_S=16

## IHBNETSIM: Tactile sensors per Skin node, max size 12.
#
CFLAGS += -DSK_T_S=12

## IHBCAN: Set the timeout for ISO TP transmissions
#
CFLAGS += -DISOTP_TIMEOUT_DEF=2

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += auto_init_can
USEMODULE += can_isotp
USEMODULE += conn_can
USEMODULE += can_pm
USEMODULE += xtimer
USEMODULE += random

FEATURES_REQUIRED += periph_can
FEATURES_REQUIRED += periph_gpio_irq

## Custom modules
#

USEMODULE += uptime
DIRS += $(CURDIR)/uptime

USEMODULE += ihbcan
DIRS += $(CURDIR)/ihb-can

USEMODULE += ihbnetsim
DIRS += $(CURDIR)/ihb-netsim

USEMODULE += ihbtools
DIRS += $(CURDIR)/ihb-tools

CFLAGS += -I$(CURDIR)
include $(RIOTBASE)/Makefile.include
