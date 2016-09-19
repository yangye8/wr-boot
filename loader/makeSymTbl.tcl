# makeSymTbl.tcl - built-in system symbol table creation utility
#
# SYNOPSIS:
# .tS
# makeSymTbl.tcl <cpu type> <objMod>
# .tE
#
# DESCRIPTION
# This tool creates the C code for a symbol table structure containing the
# names, addresses and types of all meaningful global symbols in the specified
# object module; the generated code is written to standard output.
#
# This tool is used only when creating a standalone system (`vxWorks.st')
# or for generating automated tests (BSP VTS).
# Development systems do not need this since the system symbol table is
# constructed by reading and interpreting the symbol table contained in the
# loadable system symbol table module (`vxWorks.sym'), either from the local
# boot disk (floppy, ata/ide, scsi, etc.) or from a host system over the
# network.
#
# The built-in symbol table is an array of type SYMBOL accessible through
# the global variable `standTbl'.  The array contains `standTblSize' elements.
# These are the only external declarations in the generated code. When the
# standalone VxWorks image is built from a BSP the symbols are inserted in
# the system symbol table  by usrRoot() in usrConfig.c. When the standalone
# VxWorks image is built from a project, the symbols are inserted by the
# usrStandaloneInit() routine in the comps/src/usrStandalone.c configlette.
#
# For an example, see the file $WIND_BASE/target/config/<bspName>/symTbl.c,
# which is generated by this tool for `vxWorks.st' in the same directory.
#
# FILES
# .TS
# tab(|);
# lf3 l.
# symLib.h   |  symbol table header file
# symbol.h   |  VxWorks symbol definitions header file
# .TE
#
# SEE ALSO:
#
# NOROUTINES


# namespace globals

namespace eval makeSymTbl {
    set nmCmd		"powerpc-linux-gnu-nm"
    set nmFlags		"-g"
    set symPrefixToIgnore ""
    set symPrefixToAdd	""
    set cpuType		""
    set fdOut           ""
    set symDecl() ""
    set symDefT() ""
    set symDefD() ""
}

##############################################################################
#
# standTblCreate - create an array of symbol for an object module
#
# This procedure creates an array of symbols (C code) based on a file symbol
# table.
#
# The input of this procedure is a module. The output of this
# procedure is stored in a file named symTbl.c.
#
# SYNOPSIS:
#   standTblCreate { objMod }
#
# PARAMETERS:
#   module name
#
# RETURNS: number of symbols in the module's symbol table
#
# ERRORS: N/A
#

proc makeSymTbl::standTblCreate {modName} {
    variable symPrefixToIgnore
    variable symPrefixToAdd
    variable cpuType
    variable nmCmd
    variable nmFlags
    variable symDecl
    variable symDefT
    variable symDefD
    variable fdOut

    set symbolInfo ""
    set symList {}
    set nsyms 0

    # get all the global/local symbols from the object module

    set symbolInfo [eval exec $nmCmd $nmFlags $modName]

    # replace consecutive whitespaces with single whitespace

    regsub -all { +} $symbolInfo " " symbolInfo

    set symbolInfoListRaw [split $symbolInfo \n]
    set symbolTblEntryList {}

    # Reject the symbols that are either meaningless for VxWorks or local
    # symbols (then can not be linked). These symbols are represented by the
    # following letters in nm's output:
    #
    # I: indirect reference to another symbol (extension of the a.out format
    #    specific to GNU)
    # N: debugging symbol (no point in having them in the system's symbol
    #    table).
    # U: undefined symbols (hence not part of the system's symbol table).
    # w: local weak symbol, not filtered by the -g option to nm. WIND00126330.
    # -: stabs symbol in an a.out object file (no point in having them in the
    #    system's symbol table).
    # ?: symbol of unknown type.
    #
    # all lower case letters: local symbols (they cannot be used to link
    # symTbl.c with the VxWorks library). Filtered by the -g option of nm.
    #
    # See the manpage of GNU nm for a complete explanation of the letters
    # representing the symbol types.

    # Define architecture specific symbol exclusion rules

    set archSpecificTest ""

    # Parse the symbol list to create the symbol table

    foreach symbolEntry $symbolInfoListRaw {
	# Store output fields
	if {![regexp {^(.*) (.*) (.*)} $symbolEntry dummy addr type name]} {
	    continue
	}

	# Remove unwanted symbols. This also comprises symbols names beginning
	# with a dot : they would trigger a parse error in a C file.
	# Note : locals were already filtered out by the -g option of nm.
        # Note 2: local weak symbols, 'w', are *not* filtered w/ either
        # -g or --extern-only, so we remove them here.
        if {[string match {[INUw\?\-]} $type] 
	    || [string first "." $name] != -1} {
	    continue
	}

	# Do architecture specific tests

	eval $archSpecificTest

        # Convert the filtered nm output to an IMPORT list of symbols;
	#
	# Symbols could be of following types:
	#
	# A: the symbol's value is absolute, and will not be changed by further
	#    linking.
	# B: the symbol is in the uninitialized data section (known as BSS).
	# C: the symbol is common. Common symbols are uninitialized data.
	# D: the symbol is in the initialized data section.
	# G: symbol from an initialized data section for small objects.
	# R: the symbol is in a read only data section.
	# S: symbol from an uninitialized data section for small objects.
	# T: the symbol is in the text (code) section.
	# V: the symbol is a weak object.
	# W: the symbol is a weak symbol that has not been specifically tagged
	#    as a weak object symbol.
	#
	# Some symbols need special handling via the symDecl, symDefT and
	# symDefD assertions. See below for more information.

	regsub $symPrefixToIgnore $name "" cName
            
	switch -glob $type {
	    [A] {
		lappend symbolTblEntryList "\{\{NULL\}, \"$cName\",\
					    (char*) 0x$addr, 0, 0,\
					    SYM_GLOBAL | SYM_ABS\},"
	    }
	    [VDRG] { 
		if [info exists symDecl($cName)] {
			puts $fdOut "[subst $symDecl($cName)]"
		} else {
		    puts $fdOut "IMPORT int ${cName};"
		}

		if [info exists symDefD($cName)] {
		    lappend symbolTblEntryList "[subst $symDefD($cName)]"
		} else {
		    lappend symbolTblEntryList "\{\{NULL\}, \"$name\",\
						(char*) \&$cName, 0, 0, \
						SYM_GLOBAL | SYM_DATA\},"
		}
	    }
	    [BCS] { 
		puts $fdOut "IMPORT int ${cName};"
		lappend symbolTblEntryList "\{\{NULL\}, \"$name\",\
					    (char*) \&$cName, 0, 0, \
					    SYM_GLOBAL | SYM_BSS\},"
	    }
	    [TW] {
		if [info exists symDecl($cName)] {
		    puts $fdOut "[subst $symDecl($cName)]"
		} else {
		    puts $fdOut "IMPORT int ${cName} ();"
		}

		if [info exists symDefT($cName)] {
		    lappend symbolTblEntryList "[subst $symDefT($cName)]"
		} else {
		    lappend symbolTblEntryList "\{\{NULL\}, \"$name\",\
						(char*) $cName, 0, 0, \
						SYM_GLOBAL | SYM_TEXT\},"
		}
	    }
	    default {
		puts "Warning: makeSymTbl.tcl - invalid symbol information \
			($symbolEntry)"
	    }
	}
    }

    set nsyms [llength $symbolTblEntryList]

    # convert nm output to symbol entries in array

    puts $fdOut ""
    puts $fdOut "SYMBOL ${symPrefixToAdd}standTbl \[$nsyms\] ="
    puts $fdOut "    {"

    foreach symbolTblEntry $symbolTblEntryList {
        puts $fdOut "        $symbolTblEntry"
    }

    puts $fdOut "    };"
    puts $fdOut ""

    return $nsyms
}

##############################################################################
#
# main - entry point of utility
#

# check for correct number of args

if {$argc < 3} {
    puts stderr "Usage: makeSymTbl.tcl <cpu type> <objMod> <file>"
    exit 1
}

set outFile [lindex $argv 2]

# set appropriate version of the nm command for the processor

if [catch {set makeSymTbl::cpuType [lindex $argv 0]}] {
    set makeSymTbl::cpuType ""
}

#set makeSymTbl::nmCmd "nm$makeSymTbl::cpuType"

# parse the arguments

set modName [lindex $argv 1]

# Create the module's standalone symbol table

set makeSymTbl::fdOut [open $outFile w+]

# The following (C code) goes into the wrapper file symTbl.c

puts $makeSymTbl::fdOut "/* symTbl.c - standalone symbol tables wrapper */"
puts $makeSymTbl::fdOut ""
puts $makeSymTbl::fdOut "/* CREATED BY $argv0"
puts $makeSymTbl::fdOut " *  WITH ARGS $argv"
puts $makeSymTbl::fdOut " *         ON [clock format [clock seconds]]"
puts $makeSymTbl::fdOut " */"
puts $makeSymTbl::fdOut ""
puts $makeSymTbl::fdOut "#include \<wrboot.h\>"
puts $makeSymTbl::fdOut "#include \<symbol.h\>"
puts $makeSymTbl::fdOut ""

# The following are for symbols that require special handling. These include :
# 1. Some symbols that are declared in sllLib.h, which is included through
#    symbol.h. They must be declared correctly or they cause multiple 
#    definition complaints.
# Since a symbol can have only one type at a time, symDecl is sufficient for
# both DATA and TEXT types. However, as a symbol can have different, manually
# specified, definitions for TEXT and DATA, we need symDefT and symDefD.
# Otherwise, the wrong definition would be picked up.

# DATA symbols that require special DECLARATION handling

set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllCreate) "IMPORT SL_LIST *\$cName \(\);"
set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllGet) "IMPORT SL_NODE *\$cName \(\);"
set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllPrevious) "IMPORT SL_NODE *\$cName \(\);"
set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllEach) "IMPORT SL_NODE *\$cName \(\);"
set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllPutAtHead) "IMPORT void \$cName ();"
set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllPutAtTail) "IMPORT void \$cName ();"
set makeSymTbl::symDecl(${makeSymTbl::symPrefixToAdd}sllRemove) "IMPORT void \$cName ();"

# DATA symbols that require special DEFINITION handling
# (add yours here if needed with the symDefD assertion)

# Create the symbol table

set nsyms [makeSymTbl::standTblCreate $modName]

puts $makeSymTbl::fdOut ""
puts $makeSymTbl::fdOut "const unsigned int ${makeSymTbl::symPrefixToAdd}standTblSize = ${nsyms};"

close $makeSymTbl::fdOut