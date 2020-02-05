MODULE_big = system_stats
OBJS = \
        system_stats.o \
        system_stats_utils.o \
        disk_info.o \
        io_analysis.o \
        cpu_info.o \
        cpu_usage_info.o \
        os_info.o \
        memory_info.o \
        load_avg.o \
        process_info.o \
        network_info.o \
        cpu_memory_by_process.o
#PG_CPPFLAGS = -I$(libpq_srcdir)

EXTENSION = system_stats
DATA = system_stats--1.0.sql uninstall_system_stats.sql
PGFILEDESC = "system_stats - system statistics functions"

HEADERS = system_stats.h

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
