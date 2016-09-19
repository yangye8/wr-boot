include $(TOPDIR)/rules.vload

lib        := fatfs.a
target     :=
OBJS	= 

all: $(lib)

$(lib): $(objects)
	$(AR) crv $@ $^

#自动产生依赖，用于描述.o文件和头文件的依赖关系，比如修改头文件但是不会重新编译.o，就是没有
#依赖关系，GCC支持通过查找C源文件中的"#include"关键字来自动推倒产生依赖关系的功能，
#"-M选项自动寻找源文件中包含的头文件，并生成文件的依赖关系"

#.depend: $(sources)
#	@$(CC) -M $(sources) > $@
#
#sinclude .depend

# 过时?

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f  $@.$$$$
sinclude $(deps)


# 到这里基本的意义弄明白了，但是让我不解的是%d: %c这个依赖的规则怎么能被执行到的？按照我的理解Makefile在执行时首先检查终极目标main是否存在，如果不存在则建立（根据main的依赖规则），如果存在在需要查看
#     main的依赖文件是否存在并且是最新的，这我的例子里就是要看test.o dList.o memory.o debug.o是否存在且最新。这样追下去是否没有%d: %c什么事啊， .d文件也应该不存在或者说是空的。尽管我们include了.d文件，但是没有依赖规则去执行它啊。后来仔细阅读了
#	     Makefile文件的重建才明白了。
#		     Makefile如果由其它文件重建（这里我的Makefile include了所有.d文件，.d也可以看成是一个Makefile），Makefile在读入所有其他makefile文件（.d）之后，首先将所读取的每个makefile（.d）作为一个目标，寻找更新它们的规则。同样
#			     如果此目标不存在则根据依赖规则重新创建。在例子里其实.d文件开始并不存在，所以当Makefile在include这些.d文件时首先看.d存在不，不存在就要去寻找.d的依赖文件和规则。这里就找到了%d: %c从而创建出真正的.d文件。其实这里的关键点就是对于
#				     include了理解，它是把include的文件首先当成一个目标，然后要去寻找其依赖文件和规则的，而不是我事先想象的简单的把其他文件的内容包含过来。
#

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
