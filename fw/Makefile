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

## IHBCAN: Set the tx port for ISO TP transmissions
#
CFLAGS += -DISOTP_TX_PORT=0x700

## IHBCAN: Set the rx port ISO TP transmissions
#
CFLAGS += -DISOTP_RX_PORT=0x708

## IHBCAN: Set the IHBTOOL broadcast frame id
#
CFLAGS += -DIHBTOLL_FRAME_ID=0x100

## IHBCAN: Force the IHB node to use a custom CAN ID
#
#CFLAGS += -DIHB_FORCE_CAN_ID=0x22

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += auto_init_can
USEMODULE += can_isotp
USEMODULE += conn_can

USEMODULE += xtimer
USEMODULE += random

## Do not force FEATURES check for the native board
ifneq ($(BOARD), native)
  FEATURES_REQUIRED += periph_can
  FEATURES_REQUIRED += periph_gpio_irq
endif

## Custom modules
#
USEMODULE += ihbcan
DIRS += $(CURDIR)/ihb-can

USEMODULE += ihbnetsim
DIRS += $(CURDIR)/ihb-netsim

USEMODULE += ihbtools
DIRS += $(CURDIR)/ihb-tools

CFLAGS += -I$(CURDIR)
include $(RIOTBASE)/Makefile.include
