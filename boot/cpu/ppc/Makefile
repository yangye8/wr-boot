#include $(TOPDIR)/rules.vload

sources    := start.S util.S
objects    := start.o util.o
deps       := start.d util.d

START	= start.o util.o
OBJS	= 

CROSS_COMPILE := powerpc-linux-gnu-

#########################################################################

#
## Include the make variables (CC, etc...)
#
#
AS>-= $(CROSS_COMPILE)as
CC := $(CROSS_COMPILE)gcc
AR>-= $(CROSS_COMPILE)ar
NM>-= $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

AFLAGS_DEBUG := -Wa,-gstabs
AFLAGS := $(AFLAGS_DEBUG) -D__ASSEMBLY__ $(CPPFLAGS)

all: start.o util.o

start.o: start.S
	$(CC) -c $(AFLAGS) $< -o $@

util.o: util.S
	$(CC) -c $(AFLAGS) $< -o $@

#自动产生依赖，用于描述.o文件和头文件的依赖关系，比如修改头文件但是不会重新编译.o，就是没有
#依赖关系，GCC支持通过查找C源文件中的"#include"关键字来自动推倒产生依赖关系的功能，
#"-M选项自动寻找源文件中包含的头文件，并生成文件的依赖关系"

#all: .depend $(START) $(LIB)
#$(LIB): $(OBJS)
#	$(AR) crv $@ $(OBJS)
#
#.depend: Makefile
#	$(CC) -M $(CFLAGS) start.S > $@
#sinclude .depend


# 过时?

%.d: %.S
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
sinclude $(deps)

.PHONY: clean
clean:
	$(RM) $(lib) $(objects) $(deps)*

#重建内嵌隐含规则，需要使用相同的目标和依赖模式，但是命令可以不同

#%.o: %.c
#	$(CC) $(CFLAGS) -D__DEBUG__ $< -o $@

#模式规则，只要目标文件是带有模式字符%的文件，模式规则的格式 %.o: %.c COMMAND
#预定义的编译ｃ文件到ｏ文件的隐含模式规则:

#%.o: %.c
#	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

# Show help.
# help:
# @echo 'Generic Makefile for C/C++ Programs (gcmakefile) version 0.5'
# @echo 'Copyright (C) 2007, 2008 whyglinux <whyglinux@hotmail.com>'
# @echo
# @echo 'Usage: make [TARGET]'
# @echo 'TARGETS:'
# @echo ' all (=make) compile and link.'
# @echo ' NODEP=yes make without generating dependencies.'
# @echo ' objs compile only (no linking).'
# @echo ' tags create tags for Emacs editor.'
# @echo ' ctags create ctags for VI editor.'
# @echo ' clean clean objects and the executable file.'
# @echo ' distclean clean objects, the executable and dependencies.'
# @echo ' show display variables (for debug use only).'
# @echo ' help print this message.'
# @echo
# @echo 'Report bugs to <whyglinux AT gmail DOT com>.'
#
# # Show variables (for debug use only.)
# show:
# @echo 'PROGRAM :' $(PROGRAM)
# @echo 'SRCDIRS :' $(SRCDIRS)
# @echo 'HEADERS :' $(HEADERS)
# @echo 'SOURCES :' $(SOURCES)
# @echo 'SRC_CXX :' $(SRC_CXX)
# @echo 'OBJS :' $(OBJS)
# @echo 'DEPS :' $(DEPS)
# @echo 'DEPEND :' $(DEPEND)
# @echo 'COMPILE.c :' $(COMPILE.c)
# @echo 'COMPILE.cxx :' $(COMPILE.cxx)
# @echo 'link.c :' $(LINK.c)
# @echo 'link.cxx :' $(LINK.cxx)''''''''''''''''''''''''''''''''''''''''''''''''''''
