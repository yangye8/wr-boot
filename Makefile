# Do not:
# # o  use make's built-in rules and variables
# #    (this increases performance and avoids hard-to-debug behaviour);
# # o  print "Entering directory ...";

#MAKEFLAGS += -rR --no-print-directory
MAKEFLAGS += -rR

# 操作符“+=”的作用是给变量（“+=”前面的MAKEFLAGS）追加值。
# # 如果变量（“+=”前面的MAKEFLAGS）之前没有定义过，那么，“+=”会自动变成“=”；
# # 如果前面有变量（“+=”前面的MAKEFLAGS）定义，那么“+=”会继承于前次操作的赋值符；
# # 如果前一次的是“:=”，那么“+=”会以“:=”作为其赋值符
# # 在执行make时的命令行选项参数被通过变量 “MAKEFLAGS”传递给子目录下的make程序。
# # 对于这个变量除非使用指示符“unexport”对它们进行声明，它们在整个make的执行过程中始终被自动的传递给所有的子make。
# # 还有个特殊变量SHELL与MAKEFLAGS一样，默认情况（没有用“unexport”声明）下在整个make的执行过程中被自动的传递给所有的子make。
# #
# # -r disable the built-in impilict rules.
# # -R disable the built-in variable setttings.

# “ifdef”是条件关键字。语法是ifdef ；; else ; endif
# # ifdef只检验一个变量是否被赋值，它并不会去推导这个变量，并不会把变量扩展到当前位置。
# # “ifeq”与“ifdef”类似。
# # “ifeq”语法是ifeq (;, ;)，功能是比较参数“arg1”和“arg2”的值是否相同。


# To put more focus on warnings, be less verbose as default
# # Use 'make V=1' to see the full commands

ifdef V
  ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE = $(V)
  endif
endif

# 函数origin并不操作变量的值，只是告诉你你的这个变量是哪里来的。
# # 语法是： $(origin ;)
# # origin函数的返回值有：
# # “undefined”从来没有定义过、
# “default”是一个默认的定义、
# “environment”是一个环境变量、
# # “file”这个变量被定义在Makefile中、
# “command line”这个变量是被命令行定义的、
# # “override”是被override指示符重新定义的、
# “automatic”是一个命令运行中的自动化变量

# 应用变量的语法是：$(变量名)。如KBUILD_VERBOSE = $(V)中的$(V)。

# ifndef与ifdef语法类似，但功能恰好相反。ifndef是判断变量是不是没有被赋值。

ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

# Beautify output
# # ---------------------------------------------------------------------------
# #
# # Normally, we echo the whole command before executing it. By making
# # that echo $($(quiet)$(cmd)), we now have the possibility to set
# # $(quiet) to choose other forms of output instead, e.g.
# #
# #         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
# #         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
# #
# # If $(quiet) is empty, the whole command will be printed.
# # If it is set to "quiet_", only the short version will be printed.
# # If it is set to "silent_", nothing will be printed at all, since
# # the variable $(silent_cmd_cc_o_c) doesn't exist.
# #
# # A simple variant is to prefix commands with $(Q) - that's useful
# # for commands that shall be hidden in non-verbose mode.
# #
# #>--$(Q)ln $@ :<
# #
# If KBUILD_VERBOSE equals 0 then the above command will be hidden.            
# # If KBUILD_VERBOSE equals 1 then the above command is displayed.

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(MAKEFLAGS)),)
  quiet=silent_
endif

export quiet Q KBUILD_VERBOSE

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
# See the file "Documentation/sparse.txt" for more details, including
# where to get the "sparse" utility.


ifdef C
  ifeq ("$(origin C)", "command line")
    KBUILD_CHECKSRC = $(C)
  endif
endif

ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

# Use make M=dir to specify directory of external module to build
# Old syntax make ... SUBDIRS=$PWD is still supported
# Setting the environment variable KBUILD_EXTMOD take precedence

ifdef M
  ifeq ("$(origin M)", "command line")
      KBUILD_EXTMOD := $(M)
   endif
endif

# 操作符“：=”与操作符“+=”的功能相同，只是
# 操作符“：=”后面的用来定义变量（KBUILD_EXTMOD）的变量M只能是前面定义好的，
# # 如果操作符“？=”前面的变量KBUILD_EXTMOD没有定义过，那么就将SUBDIRS赋给KBUILD_EXTMOD；
# # 如果定义过，则语句KBUILD_EXTMOD ?= $(SUBDIRS)什么也不做

ifdef SUBDIRS
  KBUILD_EXTMOD ?= $(SUBDIRS)
endif

# kbuild supports saving output files in a separate directory.
# To locate output files in a separate directory two syntaxes are supported.
# In both cases the working directory must be the root of the kernel src.
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the directory
# where the output files shall be placed.
# export KBUILD_OUTPUT=dir/to/store/output/files/
# make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.
# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
# 变量从没有被定义过则变量是空值 测试空值请使用ifeq($(foo),)

ifeq ($(KBUILD_SRC),)

# OK, Make called in directory where kernel src resides
# # Do we want to locate output files in a separate directory?

ifdef O
  ifeq ("$(origin O)", "command line")
    KBUILD_OUTPUT := $(O)
  endif
endif

# That's our default target when none is given on the command line
PHONY := _all
_all:

# 为变量PHONY追加_all
# # Makefile的规则：
# # 目标：依赖文件
# # 命令1
# # 命令2
# # ...
# #
# # 没有依赖文件的目标称为“伪目标”。伪目标并不是一个文件，只是一个标签。
# # 由于伪目标不是一个文件，所以make无法生成它的依赖关系和决定它是否要执行，
# # 只有在命令行中输入（即显示地指明）这个“目标”才能让其生效,此处为"make _all"。

ifneq ($(KBUILD_OUTPUT),)
# Invoke a second make in the output directory, passing relevant variables
# # check that the output directory actually exists
saved-output := $(KBUILD_OUTPUT)

KBUILD_OUTPUT := $(shell mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) \
  # >--->--->--->--->--->--->--->---&& /bin/pwd)

# 函数shell是make与外部环境的通讯工具，它用于命令的扩展。
# # shell函数起着调用shell命令（mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) && /bin/pwd）和返回命令输出结果的参数的作用。
# # Make仅仅处理返回结果，再返回结果替换调用点之前，make将每一个换行符或者一对回车/换行符处理为单个空格；
# # 如果返回结果最后是换行符（和回车符），make将把它们去掉。

$(if $(KBUILD_OUTPUT),, \
     $(error failed to create output directory "$(saved-output)"))

# 函数if对在函数上下文中扩展条件提供了支持（相对于GNU make makefile文件中的条件语句，例如ifeq指令。）
# # if函数的语法是：$(if ,) 或是 $(if ,,)。
# # 如果条件$(KBUILD_OUTPUT)为真（非空字符串），那么两个逗号之间的空字符（注意连续两个逗号的作用）将会是整个函数的返回值，
# # 如果$(KBUILD_OUTPUT)为假（空字符串），那么$(error output directory "$(saved-output)" does not exist）会是整个函数的返回值，
# # 此时如果没有被定义，那么，整个函数返回空字串。"")


# 函数error的语法是：$(error ;)
# # 函数error的功能是：产生一个致命的错误，output directory "$(saved-output)" does not exist是错误信息。
# # 注意，error函数不会在一被使用就会产生错误信息，所以如果你把其定义在某个变量中，并在后续的脚本中使用这个变量，那么也是可以的。

# 命令“$(if $(KBUILD_OUTPUT),, \”中最后的“\”的作用是：紧接在“\”下面的“哪一行”的命令是“\”所在行的命令的延续。
# # 如果要让前一个命令的参数等应用与下一个命令，那么这两个命令应该写在同一行，如果一行写不下两个命令，可以在第一行末尾添上符号"\"，然后在下一行接着写。

PHONY += $(MAKECMDGOALS)

# 将变量KBUILD_OUTPUT的值追加给变量saved-output，

$(filter-out _all,$(MAKECMDGOALS)) _all:
	$(if $(KBUILD_VERBOSE:1=),@)$(MAKE) -C $(KBUILD_OUTPUT) \
	-KBUILD_SRC=$(CURDIR) \
	KBUILD_EXTMOD="$(KBUILD_EXTMOD)" -f $(CURDIR)/Makefile $@


# 反过滤函数——filter-out，语法是：$(filter-out ;,;)
# # 函数filter-out的功能是：去掉$(MAKECMDGOALS）中符合规则_all的所有字符串后，剩下的作为返回值。
# # 函数filter-out调用与伪目标_all在同一行。
# # 伪目标_all下面的以tab开头的三行是命令，因为每行最后都有"\"，所以这三行命令应该是写在同一行的，即后面的命令要受到处于它之前的那些命令的影响"")

# $(if $(KBUILD_VERBOSE:1=),@) 含义是如果$(KBUILD_VERBOSE:1=) 不为空，则等于$@
# # 自动化变量"$@"表示规则中的目标文件集，在模式规则中，如果有多个目标，那么，"$@"就是匹配于目标中模式定义的集合。

# 宏变量$（MAKE）的值为make命令和参数（参数可省）。 

# 执行命令KBUILD_SRC=$(CURDIR)的结果是把变量CURDIR的值赋给变量KBUILD_SRC。
# # CURDIR这个变量是Makefile提供的,代表了make当前的工作路径。

# Leave processing to above invocation of make

skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
endif # ifeq ($(KBUILD_SRC),)

# 给变量skip-makefile追加值1.
# We process the rest of the Makefile if this is the final invocation of make

# 判断变量skip-makefile与空字符是否相同，即判断变量skip-makefile的值是否为空。

# If building an external module we do not care about the all: rule
# # but instead _all depend on modules
PHONY += all
 _all: all


# 为变量PHONY追加值all。

# CDPATH can have sideeffects; disable, since we do know where we want to cd to

export CDPATH=


# 调用if函数，根据变量KBUILD_SRC的值是否为空，决定将变量KBUILD_SRC或者变量CURDIR的值赋给变量srctree
# # 为变量TOPDIR追加变量srctree的值

srctree>>---:= $(if $(KBUILD_SRC),$(KBUILD_SRC),$(CURDIR))
objtree>>---:= $(CURDIR)
src>>---:= $(srctree)
obj>>---:= $(objtree)
TOPDIR   := $(srctree)
VPATH>-->---:= $(srctree)$(if $(KBUILD_EXTMOD),:$(KBUILD_EXTMOD))


# “VPATH”是Makefile文件中的特殊变量。
# # ，如果没有指明这个变量，make只会在当前的目录中去找寻依赖文件和目标文件。
# # 如果定义了这个变量，那么，make就会在当当前目录找不到的情况下，到所指定的目录中去找寻文件了。

export srctree objtree VPATH


# 为变量objtree、src、obj分别追加变量CURDIR、srctree、objtree的值
# # make使用“VPATH”变量来指定“依赖文件”的搜索路径。
# # 为变量VPATH追加变量VPATH的值
# # 关键词export用来声明变量，被声明的变量要被传递到下级Makefile中。
# # export srctree objtree VPATH TOPDIR声明了四个变量，这四个变量在make嵌套时都将被传递到下级Makefile。


HOSTARCH := $(shell uname -m | \
	sed -e s/i.86/i386/ \
	    -e s/sun4u/sparc64/ \
	    -e s/arm.*/arm/ \
	    -e s/sa110/arm/ \
	    -e s/powerpc/ppc/ \
	    -e s/macppc/ppc/)

HOSTOS := $(shell uname -s | tr A-Z a-z | \
	    sed -e 's/\(cygwin\).*/cygwin/')


# 为变量SUBARCH追加调用shell执行sed后的返回值。
# # sed 是一种在线编辑器，它一次处理一行内容。
# # Sed主要用来自动编辑一个或多个文件；简化对文件的反复操作；编写转换程序等。
#

export	HOSTARCH

# Deal with colliding definitions from tcsh etc.
VENDOR=

#########################################################################

TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
export	TOPDIR

# load ARCH, BOARD, and CPU configuration

CPU  = ppc
ARCH = powerpc

export  ARCH CPU BOARD VENDOR

# load other configuration

#include $(TOPDIR)/config.mk
include $(TOPDIR)/rules.vload

# set default to nothing for native builds
ifeq ($(HOSTARCH),$(ARCH))
CROSS_COMPILE ?=
endif

CROSS_COMPILE = powerpc-linux-gnu-

export	CROSS_COMPILE

# The "tools" are needed early, so put this first

SUBDIRS :=  boot/cpu/$(CPU) \
            boot/board/$(BOARD) \
            boot  loader \
            lib/libc  \
            lib/libfdt \
            src/drivers \
            src/drivers/mtd \
            src/common \
            fatfs/ff12a/src 

#########################################################################
# wrboot objects....order is important (i.e. start must be first)

OBJS  =	$(CURDIR)/boot/cpu/$(CPU)/start.o
OBJS += $(CURDIR)/boot/cpu/$(CPU)/util.o

LIBS := $(CURDIR)/boot/libboot.a
LIBS += $(CURDIR)/boot/board/$(BOARD)/libboard.a
LIBS += $(CURDIR)/src/drivers/libdrivers.a
LIBS += $(CURDIR)/src/drivers/mtd/libdmtd.a
LIBS += $(CURDIR)/loader/libloader.a
LIBS += $(CURDIR)/src/common/libcommon.a
LIBS += $(CURDIR)/fatfs/ff12a/src/fatfs.a
LIBS += $(CURDIR)/lib/libfdt/libfdt.a
LIBS += $(CURDIR)/lib/libc/libc.a

#########################################################################

all:	    wrboot.bin System.map wrboot.sym wrboot.symbol wrboot.dis wrboot.dump wrboot.nm

install:	all
		cp wrboot.bin /home/wind/simics/simics5/simics-5/simics-p2020rdb-images-5.0.3/targets/qoriq-p2/images/p2020rdb/vxworks6.9/bootrom2.bin

wrboot.bin:	wrboot
		$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

wrboot.sym:	wrboot
		$(OBJCOPY) --extract-symbol $< $@

wrboot.dis:	wrboot
		$(OBJDUMP) -d $< > $@

wrboot.dump:	wrboot
		$(OBJDUMP) -DSsx $< > $@		

wrboot.symbol: wrboot
		$(OBJDUMP) -treg $< > $@

wrboot.nm:		wrboot
		$(NM) -v -l $< > $@

wrboot2: $(SUBDIRS)
		$(MAKE) -C $< > $@

wrboot1:	depend subdirs $(OBJS) $(LIBS)
		$(LD) -X -r  $(OBJS) $(LIBS) -o tmp.o
		tclsh makeSymTbl.tcl ppc tmp.o symTbl.c
		$(CC) -c -fdollars-in-identifiers -mhard-float -mstrict-align -ansi -fno-zero-initialized-in-bss -O2  -Wall \
		-I$(TOPDIR)/h -w symTbl.c
		$(LD) $(LDFLAGS) tmp.o symTbl.o -Map wrboot.map -o wrboot

subdirs:
	$(info $(SUBDIRS))
	@echo "SUBDIRS := " $(SUBDIRS)
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir || exit 1 ; done

depend dep:
		$(info $(SUBDIRS))
		@echo "SUBDIRS := " $(SUBDIRS)
		@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir .depend ; done

wrboot: subdirs $(OBJS) $(LIBS)
	@echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@for obj in $(LIBS); do echo $$obj; done
	@echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
	@echo "\nStarting partitial link........."
	@echo "_______________________________________________________________________________"
	$(LD) -X -r  $(OBJS) $(LIBS) -o tmp.o
	@echo "\ngenerating symTbl.c file..."
	@echo "_______________________________________________________________________________"
	tclsh $(TOPDIR)/loader/makeSymTbl.tcl ppc tmp.o symTbl.c 
	$(CC) -c -fdollars-in-identifiers -mhard-float -mstrict-align -ansi -fno-zero-initialized-in-bss -O2 -Wall -I$(TOPDIR)/h -w symTbl.c
	@echo "\n Last Linker........"
	@echo "_______________________________________________________________________________"
	$(LD) $(LDFLAGS) tmp.o symTbl.o -Map wrboot.map -o wrboot
	@echo "\nElf -> Bin....."
	@echo "_______________________________________________________________________________"
	$(OBJCOPY) ${OBJCFLAGS} -O binary wrboot wrboot.bin
	cp wrboot.bin /home/wind/simics/simics5/simics-5/simics-p2020rdb-images-5.0.3/targets/qoriq-p2/images/p2020rdb/vxworks-7.0-up/bootrom.bin

tags:
		ctags -w `find $(SUBDIRS) include \
			\( -name CVS -prune \) -o \( -name '*.[ch]' -print \)`

etags:
		etags -a `find $(SUBDIRS) include \
			\( -name CVS -prune \) -o \( -name '*.[ch]' -print \)`

System.map:	wrboot
		@$(NM) $< | \
		grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | \
		sort > System.map

symTbl.c: wrboot
		tclsh makeSymTbl.tcl ppc $< > $@

#########################################################################

unconfig:
	rm -f include/config.h include/config.mk
#########################################################################

clean:
	find . -type f \
		\( -name 'core' -o -name '*.bak' -o -name '*~' \
		-o -name '*.o'  -o -name '*.a' -o -name '*.s' -o -name '*.dump' -o -name '*.sym'  -o -name '*.i' -o -name '*.d*' -o -name '*.symbol' \) -print \
		| xargs rm -f
	find . -type f \
		\( -name .depend -o -name '*.srec' -o -name '*.bin' \) \
		-print \
		| xargs rm -f
	rm -f $(OBJS) *.bak tags TAGS
	rm -fr *.*~
	rm -f common/shell.tab.c common/shell.tab.h common/lex.yy.c 
	rm -f wrboot wrboot.bin wrboot.elf wrboot.map System.map symTbl.c wrboot.nm
	rm -f tools/crc32.c tools/environment.c
	rm -f include/asm/arch include/asm

backup:
	F=`basename $(TOPDIR)` ; cd .. ; \
	tar --force-local -zcvf `date "+$$F-%Y-%m-%d-%T.tar.gz"` $$F

#help - The default goal
.PHONY: help
help:
	@make --print-data-base --question | awk '/^[^.%][-A-Za-z0-9_]*:/{print substr($$1,1,length($$1)-1)}' | sort | pr --omit-pagination --width=80 --columns=4
