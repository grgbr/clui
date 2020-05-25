config-in           := Config.in
config-h            := clui/config.h

solibs             := libclui.so
libclui.so-objs     = clui.o
libclui.so-cflags  := $(EXTRA_CFLAGS) -Wall -Wextra -D_GNU_SOURCE -DPIC -fpic
libclui.so-ldflags  = $(EXTRA_LDFLAGS) -shared -fpic -Wl,-soname,libclui.so
libclui.so-pkgconf  = $(call kconf_enabled,CLUI_ASSERT,libutils)

HEADERDIR          := $(CURDIR)/include
headers             = clui/clui.h

define libclui_pkgconf_tmpl
prefix=$(PREFIX)
exec_prefix=$${prefix}
libdir=$${exec_prefix}/lib
includedir=$${prefix}/include

Name: libclui
Description: Command Line User Interface library
Version: %%PKG_VERSION%%
Requires: $(call kconf_enabled,CLUI_ASSERT,libutils)
Cflags: -I$${includedir}
Libs: -L$${libdir} -lclui
endef

pkgconfigs         := libclui.pc
libclui.pc-tmpl    := libclui_pkgconf_tmpl
