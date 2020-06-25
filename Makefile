MODULE_big = system_stats

#Detect OS type and accordingly include the source files for compilation
UNAME := $(shell uname)

OS_PLATFORM := $(shell echo $(UNAME) | tr '[:upper:]' '[:lower:]')

$(info "Platform is: $(OS_PLATFORM)")

# FreeBSD is not supported so do not build it
ifneq (,$(findstring freebsd, $(OS_PLATFORM)))
    $(error FreeBSD is not supported by this extension)
endif

# Solaris is not supported so do not build it
ifneq (,$(findstring solaris, $(OS_PLATFORM)))
    $(error Solaris is not supported by this extension)
endif
ifneq (,$(findstring sparc, $(OS_PLATFORM)))
    $(error Solaris is not supported by this extension)
endif

# HP-UX is not supported so do not build it
ifneq (,$(findstring hp-ux, $(OS_PLATFORM)))
    $(error HP-UX is not supported by this extension)
endif

ifeq ($(UNAME), Linux)
OBJS = \
        system_stats.o \
        linux/system_stats_utils.o \
        linux/disk_info.o \
        linux/io_analysis.o \
        linux/cpu_info.o \
        linux/cpu_usage_info.o \
        linux/os_info.o \
        linux/memory_info.o \
        linux/load_avg.o \
        linux/process_info.o \
        linux/network_info.o \
        linux/cpu_memory_by_process.o

HEADERS = system_stats.h

endif

ifeq ($(UNAME), Darwin)
OBJS = \
        system_stats.o \
        darwin/system_stats_utils.o \
        darwin/disk_info.o \
        darwin/io_analysis.o \
        darwin/cpu_info.o \
        darwin/cpu_usage_info.o \
        darwin/os_info.o \
        darwin/memory_info.o \
        darwin/load_avg.o \
        darwin/process_info.o \
        darwin/network_info.o \
        darwin/cpu_memory_by_process.o

HEADERS = system_stats.h

PG_LDFLAGS= -framework IOKit -framework CoreFoundation
endif

EXTENSION = system_stats
DATA = system_stats--1.0.sql uninstall_system_stats.sql
PGFILEDESC = "system_stats - system statistics functions"


REGRESS = system_stats

ifndef USE_PGXS
top_builddir = ../..
makefile_global = $(top_builddir)/src/Makefile.global
ifeq "$(wildcard $(makefile_global))" ""
USE_PGXS = 1    # use pgxs if not in contrib directory
endif
endif

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/$(MODULE_big)
include $(makefile_global)
include $(top_srcdir)/contrib/contrib-global.mk
endif
