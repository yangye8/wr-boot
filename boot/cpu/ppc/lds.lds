/* link.DOTBOOTROM - gnu linker command file for VxWorks: DOTBOOT ROM */

ENTRY(_start)
SECTIONS
{
    
    .text      :
  {
    
    wrs_kernel_text_start = ABSOLUTE(.); _wrs_kernel_text_start = ABSOLUTE(.);
  _VX_START_TEXT = .;
    *(.text)
    wrs_kernel_text_fast = ABSOLUTE(.); _wrs_kernel_text_fast = ABSOLUTE(.);
    *(.text.fast)
    wrs_kernel_text_init = ABSOLUTE(.); _wrs_kernel_text_init = ABSOLUTE(.);
    *(.text.init)
    *(.text.*) *(.stub) *(.gnu.warning) *(.gnu.linkonce.t*)
    KEEP(*(.init)) KEEP(*(.fini))
    *(.glue_7t) *(.glue_7) *(.rdata)			/* for ARM */
    *(.mips16.fn.*) *(.mips16.call.*) *(.reginfo)	/* for MIPS */
    *(.rodata) *(.rodata.*) *(.gnu.linkonce.r*) *(.rodata1)
    . = ALIGN(16);
    . = ALIGN (8);
    *(.eh_frame_hdr)
    . = ALIGN (8);
    __EH_FRAME_BEGIN__ = .;
    KEEP (*(.eh_frame))
    LONG (0)
    . = ALIGN (8);
    *(.gcc_except_table .gcc_except_table.*)
  }
  .wrs_build_vars  : {
    *(.wrs_build_vars)
    . = ALIGN(16);
  }
  .sdata2      : {
    _SDA2_BASE_ = . + 0x7ff0 ;
    *(.sdata2) *(.sbss2)				/* for PPC */
    . = ALIGN(16);
  }
  . = ALIGN(16);
  wrs_kernel_text_end = ABSOLUTE(.); _wrs_kernel_text_end = ABSOLUTE(.);
  etext = .; _etext = .;

  .data    : AT(etext)
  {
    
    wrs_kernel_data_start = .; _wrs_kernel_data_start = .;
    _VX_START_DATA = .;
    *(.data)
    wrs_kernel_data_fast = .; _wrs_kernel_data_fast = .;
    *(.data.fast)
    wrs_kernel_data_init = .; _wrs_kernel_data_init = .;
    *(.data.init)
    *(.data.*) *(.gnu.linkonce.d*) SORT(CONSTRUCTORS) *(.data1)
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /* got*, dynamic, sdata*, lit[48], and sbss should follow _gp */
    *(.got.plt) *(.got) *(.dynamic)
    *(.got2)
    *(.lit8) *(.lit4)
    . = ALIGN(16);
  }
  wrs_kernel_tls_data_start = .; _wrs_kernel_tls_data_start = .;
  .tls_data     : {
    *(.tls_data)
  }
  wrs_kernel_tls_data_size = . - wrs_kernel_tls_data_start; _wrs_kernel_tls_data_size = . - _wrs_kernel_tls_data_start;
  wrs_kernel_tls_data_align = 16; _wrs_kernel_tls_data_align = 16;
  . = ALIGN(8);
  wrs_kernel_tls_vars_start = .; _wrs_kernel_tls_vars_start = .;
  .tls_vars     : {
    *(.tls_vars)
  }
  wrs_kernel_tls_vars_size = SIZEOF(.tls_vars); _wrs_kernel_tls_vars_size = SIZEOF(.tls_vars);
  . = ALIGN(16);
  _gp = ALIGN(16) + 0x7ff0;		/* set gp for MIPS startup code */
  .sdata     :
  {
    _SDA_BASE_ = . + 0x7ff0 ;
    *(.sdata) *(.sdata.*) *(.gnu.linkonce.s.*)
    . = ALIGN(16);
  }
  . = ALIGN(16);
  edata = .; _edata = .;
  wrs_kernel_data_end = .; _wrs_kernel_data_end = .;
  .sbss      :
  {
    wrs_kernel_bss_start = .; _wrs_kernel_bss_start = .;
    *(.sbss) *(.sbss.*) *(.scommon) 
    . = ALIGN(16);
  }
  .bss       :
  {
    *(.dynbss) *(.bss) *(.bss.*) *(COMMON) *(.gnu.linkonce.b*)
    . = ALIGN(16);
  }
  . = ALIGN(16);
  end = .; _end = .;
  wrs_kernel_bss_end = .; _wrs_kernel_bss_end = .;

  .boot (wrs_kernel_text_start + wrs_kernel_rom_size - 0x800) : AT(wrs_kernel_text_start + wrs_kernel_rom_size - 0x800)
  {
  *(.boot)
  }
  .reset (wrs_kernel_text_start + wrs_kernel_rom_size - 4) : AT(wrs_kernel_text_start + wrs_kernel_rom_size - 4)
  {
  *(.reset)
  }

  /DISCARD/ :
  {
  *(.note)				/* seen on Pentium ELF */
  *(.comment)				/* seen on PPC & SH ELF */
  *(.pdr)				/* seen on MIPS ELF */
  }
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}