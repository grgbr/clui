################################################################################
# SPDX-License-Identifier: LGPL-3.0-only
#
# This file is part of Clui.
# Copyright (C) 2020-2024 Grégor Boirie <gregor.boirie@free.fr>
################################################################################

config-in          := Config.in
config-h           := $(PACKAGE)/config.h

HEADERDIR          := $(CURDIR)/include
headers             = clui/clui.h
headers            += $(call kconf_enabled,CLUI_SHELL,clui/shell.h)
headers            += $(call kconf_enabled,CLUI_TABLE,clui/table.h)

ifneq ($(filter y,$(CONFIG_CLUI_ASSERT) $(CONFIG_CLUI_SHELL)),)
pkgconf-libutils   := libutils
endif # !($(filter y,$(CONFIG_CLUI_ASSERT)$(CONFIG_CLUI_SHELL)),)

solibs             := libclui.so
libclui.so-objs     = clui.o
libclui.so-objs    += $(call kconf_enabled,CLUI_SHELL,shell.o)
libclui.so-objs    += $(call kconf_enabled,CLUI_TABLE,table.o)
libclui.so-cflags  := $(EXTRA_CFLAGS) -Wall -Wextra -D_GNU_SOURCE -DPIC -fpic
libclui.so-ldflags := $(EXTRA_LDFLAGS) -shared -fpic -Wl,-soname,libclui.so
libclui.so-pkgconf  = $(pkgconf-libutils) \
                      $(call kconf_enabled,CLUI_SHELL,readline) \
                      $(call kconf_enabled,CLUI_TABLE,smartcols)

define libclui_pkgconf_tmpl
prefix=$(PREFIX)
exec_prefix=$${prefix}
libdir=$${exec_prefix}/lib
includedir=$${prefix}/include

Name: libclui
Description: Command Line User Interface library
Version: $(VERSION)
Requires: $(pkgconf-libutils) \
          $(call kconf_enabled,CLUI_SHELL,readline) \
          $(call kconf_enabled,CLUI_TABLE,smartcols)
Cflags: -I$${includedir}
Libs: -L$${libdir} -lclui
endef

pkgconfigs         := libclui.pc
libclui.pc-tmpl    := libclui_pkgconf_tmpl

################################################################################
# Source code tags generation
################################################################################

tagfiles := $(shell find $(CURDIR) -type f)
