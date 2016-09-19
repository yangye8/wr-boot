#
# Freescale P2020RDB board
#

/*
 * Memory map
 *
 * 0x0000_0000 0x7fff_ffff>-DDR>>---Up to 2GB cacheable
 * 0x8000_0000 0xdfff_ffff>-PCI Express Mem>1.5G non-cacheable(PCIe * 3)
 * 0xec00_0000 0xefff_ffff>-NOR flash>--Up to 64M non-cacheable>CS0/1
 * 0xf8f8_0000 0xf8ff_ffff>-L2 SRAM>>---Up to 512K cacheable
 *   (early boot only)
 * 0xff80_0000 0xff80_7fff>-NAND flash>-32K non-cacheable>--CS1/0
 * 0xff98_0000 0xff98_ffff>-PMC>>---64K non-cacheable>--CS2
 * 0xffa0_0000 0xffaf_ffff>-CPLD>--->---1M non-cacheable>---CS3
 * 0xffb0_0000 0xffbf_ffff>-VSC7385 switch  1M non-cacheable>---CS2
 * 0xffc0_0000 0xffc3_ffff>-PCI IO range>---256k non-cacheable
 * 0xffd0_0000 0xffd0_3fff>-L1 for stack>---16K cacheable
 * 0xffe0_0000 0xffef_ffff>-CCSR>--->---1M non-cacheable
 */

/*
 * Local Bus Definitions
 */
#if (defined(CONFIG_P1020MBG) || defined(CONFIG_P1020RDB_PD))
#define CONFIG_SYS_MAX_FLASH_SECT>--512>/* 64M */
#define CONFIG_SYS_FLASH_BASE>-->---0xec000000
#elif defined(CONFIG_P1020UTM)
#define CONFIG_SYS_MAX_FLASH_SECT>--256>/* 32M */
#define CONFIG_SYS_FLASH_BASE>-->---0xee000000
#else
#define CONFIG_SYS_MAX_FLASH_SECT>--128>/* 16M */
#define CONFIG_SYS_FLASH_BASE>-->---0xef000000
#endif

#define CONFIG_FLASH_BR_PRELIM (BR_PHYS_ADDR(CONFIG_SYS_FLASH_BASE_PHYS) \
>---| BR_PS_16 | BR_V)

#define CONFIG_FLASH_OR_PRELIM>-0xfc000ff7

#define CONFIG_SYS_FLASH_BANKS_LIST>{CONFIG_SYS_FLASH_BASE_PHYS}
#define CONFIG_SYS_FLASH_QUIET_TEST
#define CONFIG_FLASH_SHOW_PROGRESS>-45>-/* count down from 45/5: 9..1 */

#define CONFIG_SYS_MAX_FLASH_BANKS>-1>--/* number of banks */

#undef CONFIG_SYS_FLASH_CHECKSUM
#define CONFIG_SYS_FLASH_ERASE_TOUT>60000>--/* Flash Erase Timeout (ms) */
#define CONFIG_SYS_FLASH_WRITE_TOUT>500>/* Flash Write Timeout (ms) */

#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_EMPTY_INFO
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE
