## App name
#
APPLICATION = CySkin-gt

## Target
#
BOARD ?= nucleo-l476rg
ifeq (native,$(BOARD))
CFLAGS += BOARD_NATIVE
endif

## This has to be the absolute path to the RIOT base directory:
#
RIOTBASE ?= $(CURDIR)/../../RIOT/

## Firmware version: <tag-name>-<last-commit-of-tag>-<date>
#
CFLAGS += -DIHB_FW_VER=\"$(shell git describe --tag --abbrev | cut -c 1-20)-$(shell date --iso=date)\"

## IHBNETSIM: Skin nodes for IHB. Set the network size, it should be almost to 248
#             (but there's a limit on the maxinum isotp payload which limit it to
#             ~157.)
#
CFLAGS += -DSK_N_S=12

## IHBCAN: Set the speed
# 0 = 1MiB
# 1 = 750KiB
# 2 = 500KiB
# 3 = 250KiB
CFLAGS += -DCAN_SPEED=0

## IHBCAN: Set the timeout for ISO TP transmissions
#
CFLAGS += -DISOTP_TIMEOUT_DEF=2

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

FEATURES_REQUIRED += periph_can
FEATURES_REQUIRED += periph_gpio_irq

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
