/*----------------------------------------------------------------------*/
/* FatFs Module Sample Program / Renesas RX62N        (C)ChaN, 2016     */
/*----------------------------------------------------------------------*/
/* Ev.Board: FRK-RX62N from CQ Publishing                               */
/* Console: N81 115200bps (port is defined in scif.c)                   */
/* MMC/SDC: SPI mode (port is defined in mmc_rspi.c)                    */
/*----------------------------------------------------------------------*/


#include <machine.h>
#include "iodefine.h"
#include "vect.h"
#include "integer.h"
#include "scif.h"
#include "xprintf.h"
#include "diskio.h"
#include "ff.h"
#include "sound.h"

#define	F_PCLK	96000000UL



/*---------------------------------------------------------*/
/* Work Area                                               */
/*---------------------------------------------------------*/

FATFS FatFs[_VOLUMES];	/* File system object */
FIL File[2];			/* File objects */
DIR Dir;				/* Directory object */

DWORD AccSize;			/* File counter (fs command) */
WORD AccFiles, AccDirs;

FILINFO Finfo;			/* File properties (fs/fl command) */

char Line[256];				/* Console input buffer */
BYTE Buff[16384];			/* Disk I/O working buffer */

volatile UINT Timer;		/* Performance timer (1kHz) */




/*---------------------------------------------------------*/
/* 1000Hz interval timer (CMT0)                            */
/*---------------------------------------------------------*/

void Excep_CMTU0_CMT0(void)		/* ISR: vect.h is required */
{
	static WORD b;


	/* Increment performance timer */
	Timer++;

	/* Blink LED1(P15) on the board */
	b++;
	PORT1.DR.BIT.B5 = ((b & 0x40) && (b & 0x680) == 0) ? 0 : 1;

	/* Drive MMC/SD control module (mmc_rspi.c) */
	disk_timerproc();
}


void delay_ms (					/* Delay in unit of msec */
	UINT ms
)
{
	for (Timer = 0; Timer < ms; ) ;
}



/*---------------------------------------------------------*/
/* Initialize clock system and start timer service         */
/*---------------------------------------------------------*/


static
void clock_init (void)
{
	PORT1.DDR.BIT.B5 = 1;				/* P15:LED ON */

	SYSTEM.SCKCR.LONG = 0x00010000;		/* Select system clock: ICLK=8x(96MHz), BCLK=4x(48MHz), PCLK=8x(96MHz) */

	/* Initialize CMT0 (1kHz IVT) */
	MSTP_CMT0 = 0;						/* Enable CMT0/1 module */
	CMT0.CMCR.WORD = 0x0040;			/* CMIE=1, CKS=0(PCLK/8) */
	CMT0.CMCOR = F_PCLK / 8 / 1000 - 1;	/* Select clock divider */
	CMT.CMSTR0.BIT.STR0 = 1;			/* Start CMT0 */
	IPR(CMT0, CMI0) = 8;				/* Interrupt priority = 8 */
	IEN(CMT0, CMI0) = 1;				/* Enable CMT0 interrupt */
}



/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

#if !_FS_NORTC
DWORD get_fattime (void)
{
	/* No RTC feature provided. Return a fixed value 2014/11/9 0:00:00 */
	return	  ((DWORD)(2014 - 1980) << 25)	/* Y */
			| ((DWORD)11 << 21)				/* M */
			| ((DWORD)9 << 16)				/* D */
			| ((DWORD)0 << 11)				/* H */
			| ((DWORD)0 << 5)				/* M */
			| ((DWORD)0 >> 1);				/* S */
}
#endif



/*--------------------------------------------------------------------------*/
/* Monitor                                                                  */
/*--------------------------------------------------------------------------*/


int xstrlen (const char *str)
{
	int n = 0;

	while (*str++) n++;
	return n;
}


char *xstrcpy (char* dst, const char* src)
{
	char c, *d = dst;

	do {
		c = *src++;
		*d++ = c;
	} while (c);

	return dst;
}


void *xmemset (void *p, int c, int sz)
{
	char *pf = (char*)p;

	while (sz--) *pf++ = (char)c;
	return p;
}


char *xstrchr (const char *str, int c)
{
	while (*str) {
		if (*str == (char)c) return (char*)str;
		str++;
	}
	return 0;
}


static
FRESULT scan_files (	/* Scan directory in recursive */
	char* path			/* Pointer to the path name working buffer */
)
{
	DIR dirs;
	FRESULT res;
	int i;


	res = f_opendir(&dirs, path);	/* Open the directory */
	if (res == FR_OK) {
		while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {	/* Get an entry from the dir */
			if (Finfo.fattrib & AM_DIR) {	/* It is a directory */
				AccDirs++;
				i = xstrlen(path);
				path[i] = '/'; xstrcpy(path+i+1, Finfo.fname);	/* Scan the directory */
				res = scan_files(path);
				path[i] = '\0';
				if (res != FR_OK) break;
			} else {						/* It is a file  */
			/*	xprintf("%s/%s\n", path, fn); */
				AccFiles++;
				AccSize += Finfo.fsize;				/* Accumulate the file size in unit of byte */
			}
		}
	}

	return res;
}



static
void put_rc (		/* Put FatFs result code with defined symbol */
	FRESULT rc
)
{
	const char *str =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
		"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0" "INVALID_NAME\0";
	FRESULT i;

	for (i = FR_OK; i != rc && *str; i++) {
		while (*str++) ;
	}
	xprintf("rc=%u FR_%s\n", (UINT)rc, str);
}


static
const char HelpMsg[] =
	"[Disk contorls]\n"
	" di <pd#> - Initialize disk\n"
	" dd [<pd#> <lba>] - Dump a secrtor\n"
	" ds <pd#> - Show disk status\n"
	"[Buffer controls]\n"
	" bd <ofs> - Dump working buffer\n"
	" be <ofs> [<data>] ... - Edit working buffer\n"
	" br <pd#> <lba> [<count>] - Read disk into working buffer\n"
	" bw <pd#> <lba> [<count>] - Write working buffer into disk\n"
	" bf <val> - Fill working buffer\n"
	"[File system controls]\n"
	" fi <ld#> [<mount>] - Force initialized the volume\n"
	" fs [<path>] - Show volume status\n"
	" fl [<path>] - Show a directory\n"
	" fL <dir> <pat> - Find directory\n"
	" fo <mode> <file> - Open a file\n"
	" fc - Close the file\n"
	" fe <ofs> - Move fp in normal seek\n"
	" fd <len> - Read and dump the file\n"
	" fr <len> - Read the file\n"
	" fw <len> <val> - Write to the file\n"
	" fn <org.name> <new.name> - Rename an object\n"
	" fu <name> - Unlink an object\n"
	" fv - Truncate the file at current fp\n"
	" fk <name> - Create a directory\n"
	" fa <atrr> <mask> <object name> - Change attribute of an object\n"
	" ft <year> <month> <day> <hour> <min> <sec> <name> - Change timestamp of an object\n"
	" fx <src.file> <dst.file> - Copy a file\n"
	" fg <path> - Change current directory\n"
	" fq - Show current directory\n"
	" fb <name> - Set volume label\n"
	" fm <ld#> <type> <csize> - Create file system\n"
	" fz [<len>] - Change/Show R/W length for fr/fw/fx command\n"
	"[Misc commands]\n"
	" p <wavfile> - Play RIFF-WAVE file\n"
	" md[b|h|w] <addr> [<count>] - Dump memory\n"
	" mf <addr> <value> <count> - Fill memory\n"
	" me[b|h|w] <addr> [<value> ...] - Edit memory\n"
	"\n";



/*--------------------------------------------------------------------------*/
/* Main                                                                     */
/*--------------------------------------------------------------------------*/

int main (void)
{
	char *ptr, *ptr2;
	long p1, p2, p3;
	BYTE dr, b1, drv = 0;
	FRESULT res;
	UINT s1, s2, cnt, blen = sizeof Buff;
	DWORD ofs = 0, sect = 0;
	FATFS *fs;
	static const char *ft[] = {"", "FAT12", "FAT16", "FAT32", "exFAT"};


	clock_init();			/* Initialize clock and timer */
	scif_init(38400);		/* Initialize serial port */

	xdev_in(scif_getc);		/* Join scif.c and monitor.c */
	xdev_out(scif_putc);

	delay_ms(10);
	xputs("\nFatFs module test monitor for FRK-RN62N evaluation board\n");
	xputs(_USE_LFN ? "LFN Enabled" : "LFN Disabled");
	xprintf(", Code page: %u/ANSI\n", _CODE_PAGE);


	for (;;) {
		xputc('>');
		xgets(Line, sizeof Line);

		ptr = Line;
		switch (*ptr++) {
		case '?' :		/* ? - Show usage */
			xputs(HelpMsg);
			break;

		case 'm' :	/* Memory dump/fill/edit */
			switch (*ptr++) {
			case 'd' :	/* md[b|h|w] <address> [<count>] - Dump memory */
				switch (*ptr++) {
				case 'w': p3 = DW_LONG; break;
				case 'h': p3 = DW_SHORT; break;
				default: p3 = DW_CHAR;
				}
				if (!xatoi(&ptr, &p1)) break;
				if (!xatoi(&ptr, &p2)) p2 = 128 / p3;
				for (ptr = (char*)p1; p2 >= 16 / p3; ptr += 16, p2 -= 16 / p3)
					put_dump(ptr, (DWORD)ptr, 16 / p3, p3);
				if (p2) put_dump((BYTE*)ptr, (UINT)ptr, p2, p3);
				break;
			case 'f' :	/* mf <address> <value> <count> - Fill memory */
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
				while (p3--) {
					*(BYTE*)p1 = (BYTE)p2;
					p1++;
				}
				break;
			case 'e' :	/* me[b|h|w] <address> [<value> ...] - Edit memory */
				switch (*ptr++) {	/* Get data width */
				case 'w': p3 = DW_LONG; break;
				case 'h': p3 = DW_SHORT; break;
				default: p3 = DW_CHAR;
				}
				if (!xatoi(&ptr, &p1)) break;	/* Get start address */
				if (xatoi(&ptr, &p2)) {	/* 2nd parameter is given (direct mode) */
					do {
						switch (p3) {
						case DW_LONG: *(DWORD*)p1 = (DWORD)p2; break;
						case DW_SHORT: *(WORD*)p1 = (WORD)p2; break;
						default: *(BYTE*)p1 = (BYTE)p2;
						}
						p1 += p3;
					} while (xatoi(&ptr, &p2));	/* Get next value */
					break;
				}
				for (;;) {				/* 2nd parameter is not given (interactive mode) */
					switch (p3) {
					case DW_LONG: xprintf("%08X 0x%08X-", p1, *(DWORD*)p1); break;
					case DW_SHORT: xprintf("%08X 0x%04X-", p1, *(WORD*)p1); break;
					default: xprintf("%08X 0x%02X-", p1, *(BYTE*)p1);
					}
					ptr = Line; xgets(ptr, sizeof Line);
					if (*ptr == '.') break;
					if ((BYTE)*ptr >= ' ') {
						if (!xatoi(&ptr, &p2)) continue;
						switch (p3) {
						case DW_LONG: *(DWORD*)p1 = (DWORD)p2; break;
						case DW_SHORT: *(WORD*)p1 = (WORD)p2; break;
						default: *(BYTE*)p1 = (BYTE)p2;
						}
					}
					p1 += p3;
				}
				break;
			}
			break;

		case 'd' :	/* Disk I/O layer controls */
			switch (*ptr++) {
			case 'd' :	/* dd [<pd#> <sect>] - Dump secrtor */
				if (!xatoi(&ptr, &p1)) {
					p1 = drv; p2 = sect;
				} else {
					if (!xatoi(&ptr, &p2)) break;
				}
				drv = (BYTE)p1; sect = p2;
				dr = disk_read(drv, Buff, sect, 1);
				if (dr) { xprintf("rc=%d\n", (WORD)dr); break; }
				xprintf("PD#:%u LBA:%lu\n", drv, sect++);
				for (ptr=(char*)Buff, ofs = 0; ofs < 0x200; ptr += 16, ofs += 16)
					put_dump((BYTE*)ptr, ofs, 16, DW_CHAR);
				break;

			case 'i' :	/* di <pd#> - Initialize disk */
				if (!xatoi(&ptr, &p1)) break;
				xprintf("rc=%d\n", (WORD)disk_initialize((BYTE)p1));
				break;

			case 's' :	/* ds <pd#> - Show disk status */
				if (!xatoi(&ptr, &p1)) break;
				if (disk_ioctl((BYTE)p1, GET_SECTOR_COUNT, &p2) == RES_OK)
					{ xprintf("Drive size: %lu sectors\n", p2); }
				if (disk_ioctl((BYTE)p1, GET_BLOCK_SIZE, &p2) == RES_OK)
					{ xprintf("Block size: %lu sectors\n", p2); }
				if (disk_ioctl((BYTE)p1, MMC_GET_TYPE, &b1) == RES_OK)
					{ xprintf("Media type: %u\n", b1); }
				if (disk_ioctl((BYTE)p1, MMC_GET_CSD, Buff) == RES_OK)
					{ xputs("CSD:\n"); put_dump(Buff, 0, 16, DW_CHAR); }
				if (disk_ioctl((BYTE)p1, MMC_GET_CID, Buff) == RES_OK)
					{ xputs("CID:\n"); put_dump(Buff, 0, 16, DW_CHAR); }
				if (disk_ioctl((BYTE)p1, MMC_GET_OCR, Buff) == RES_OK)
					{ xputs("OCR:\n"); put_dump(Buff, 0, 4, DW_CHAR); }
				if (disk_ioctl((BYTE)p1, MMC_GET_SDSTAT, Buff) == RES_OK) {
					xputs("SD Status:\n");
					for (s1 = 0; s1 < 64; s1 += 16) put_dump(Buff+s1, s1, 16, DW_CHAR);
				}
				break;

			}
			break;

		case 'b' :	/* Buffer controls */
			switch (*ptr++) {
			case 'd' :	/* bd <ofs> - Dump R/W buffer */
				if (!xatoi(&ptr, &p1)) break;
				for (ptr=(char*)&Buff[p1], ofs = p1, cnt = 32; cnt; cnt--, ptr+=16, ofs+=16)
					put_dump((BYTE*)ptr, ofs, 16, DW_CHAR);
				break;

			case 'e' :	/* be <ofs> [<data>] ... - Edit R/W buffer */
				if (!xatoi(&ptr, &p1)) break;
				if (xatoi(&ptr, &p2)) {
					do {
						Buff[p1++] = (BYTE)p2;
					} while (xatoi(&ptr, &p2));
					break;
				}
				for (;;) {
					xprintf("%04X %02X-", (WORD)(p1), (WORD)Buff[p1]);
					xgets(Line, sizeof Line);
					ptr = Line;
					if (*ptr == '.') break;
					if (*ptr < ' ') { p1++; continue; }
					if (xatoi(&ptr, &p2))
						Buff[p1++] = (BYTE)p2;
					else
						xputs("???\n");
				}
				break;

			case 'r' :	/* br <pd#> <lba> [<num>] - Read disk into R/W buffer */
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
				if (!xatoi(&ptr, &p3)) p3 = 1;
				xprintf("rc=%u\n", (WORD)disk_read((BYTE)p1, Buff, p2, p3));
				break;

			case 'w' :	/* bw <pd#> <lba> [<num>] - Write R/W buffer into disk */
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
				if (!xatoi(&ptr, &p3)) p3 = 1;
				xprintf("rc=%u\n", (WORD)disk_write((BYTE)p1, Buff, p2, p3));
				break;

			case 'f' :	/* bf <val> - Fill working buffer */
				if (!xatoi(&ptr, &p1)) break;
				memset(Buff, (BYTE)p1, sizeof Buff);
				break;

			}
			break;

		case 'f' :	/* FatFS API controls */
			switch (*ptr++) {

			case 'i' :	/* fi <ld#> [<mount>] - Force initialized the logical drive */
				if (!xatoi(&ptr, &p1) || p1 > 9 || p1 < 0) break;
				if (!xatoi(&ptr, &p2)) p2 = 0;
				xsprintf(ptr, "%u:", (BYTE)p1);
				put_rc(f_mount(&FatFs[p1], ptr, (BYTE)p2));
				break;

			case 's' :	/* fs [<path>] - Show volume status */
				while (*ptr == ' ') ptr++;
				res = f_getfree(ptr, (DWORD*)&p1, &fs);
				if (res) { put_rc(res); break; }
				xprintf("FAT type = %s\nBytes/Cluster = %lu\nNumber of FATs = %u\n"
						"Root DIR entries = %u\nSectors/FAT = %lu\nNumber of clusters = %lu\n"
						"Volume start (lba) = %lu\nFAT start (lba) = %lu\nDIR start (lba,clustor) = %lu\nData start (lba) = %lu\n\n",
						ft[fs->fs_type], (DWORD)fs->csize * 512, fs->n_fats,
						fs->n_rootdir, fs->fsize, (DWORD)fs->n_fatent - 2,
						fs->volbase, fs->fatbase, fs->dirbase, fs->database);
#if _USE_LABEL
				res = f_getlabel(ptr, (char*)Buff, (DWORD*)&p2);
				if (res) { put_rc(res); break; }
				xprintf(Buff[0] ? "Volume name is %s\n" : "No volume label\n", (char*)Buff);
				xprintf("Volume S/N is %04X-%04X\n", (DWORD)p2 >> 16, (DWORD)p2 & 0xFFFF);
#endif
				AccSize = AccFiles = AccDirs = 0;
				xprintf("...");
				res = scan_files(ptr);
				if (res) { put_rc(res); break; }
				xprintf("\r%u files, %lu bytes.\n%u folders.\n"
						"%lu KiB total disk space.\n%lu KB available.\n",
						AccFiles, AccSize, AccDirs,
						(fs->n_fatent - 2) * (fs->csize / 2), (DWORD)p1 * (fs->csize / 2)
				);
				break;

			case 'l' :	/* fl [<path>] - Directory listing */
				while (*ptr == ' ') ptr++;
				res = f_opendir(&Dir, ptr);
				if (res) { put_rc(res); break; }
				p1 = s1 = s2 = 0;
				for(;;) {
					res = f_readdir(&Dir, &Finfo);
					if ((res != FR_OK) || !Finfo.fname[0]) break;
					if (Finfo.fattrib & AM_DIR) {
						s2++;
					} else {
						s1++; p1 += Finfo.fsize;
					}
					xprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n",
							(Finfo.fattrib & AM_DIR) ? 'D' : '-',
							(Finfo.fattrib & AM_RDO) ? 'R' : '-',
							(Finfo.fattrib & AM_HID) ? 'H' : '-',
							(Finfo.fattrib & AM_SYS) ? 'S' : '-',
							(Finfo.fattrib & AM_ARC) ? 'A' : '-',
							(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
							(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
							Finfo.fsize, Finfo.fname);
				}
				xprintf("%4u File(s),%10lu KiB total\n%4u Dir(s)", s1, p1, s2);
				res = f_getfree(ptr, (DWORD*)&p1, &fs);
				if (res == FR_OK)
					xprintf(", %10lu KiB free\n", p1 * fs->csize / 2);
				else
					put_rc(res);
				break;
#if _USE_FIND
			case 'L' :	/* fL <path> <pattern> - Directory search */
				while (*ptr == ' ') ptr++;
				ptr2 = ptr;
				while (*ptr != ' ') ptr++;
				*ptr++ = 0;
				res = f_findfirst(&Dir, &Finfo, ptr2, ptr);
				while (res == FR_OK && Finfo.fname[0]) {
#if _USE_LFN
					xprintf("%-12s  %s\n", Finfo.altname, Finfo.fname);
#else
					xprintf("%s\n", Finfo.fname);
#endif
					res = f_findnext(&Dir, &Finfo);
				}
				if (res) put_rc(res);
				f_closedir(&Dir);
				break;
#endif
			case 'o' :	/* fo <mode> <file> - Open a file */
				if (!xatoi(&ptr, &p1)) break;
				while (*ptr == ' ') ptr++;
				put_rc(f_open(&File[0], ptr, (BYTE)p1));
				break;

			case 'c' :	/* fc - Close a file */
				put_rc(f_close(&File[0]));
				break;

			case 'e' :	/* fe - Seek file pointer */
				if (!xatoi(&ptr, &p1)) break;
				res = f_lseek(&File[0], p1);
				put_rc(res);
				if (res == FR_OK)
					xprintf("fptr=%lu(0x%lX)\n", File[0].fptr, File[0].fptr);
				break;

			case 'd' :	/* fd <len> - read and dump file from current fp */
				if (!xatoi(&ptr, &p1)) break;
				ofs = File[0].fptr;
				while (p1) {
					if ((UINT)p1 >= 16) { cnt = 16; p1 -= 16; }
					else 				{ cnt = p1; p1 = 0; }
					res = f_read(&File[0], Buff, cnt, &cnt);
					if (res != FR_OK) { put_rc(res); break; }
					if (!cnt) break;
					put_dump(Buff, ofs, cnt, DW_CHAR);
					ofs += 16;
				}
				break;

			case 'r' :	/* fr <len> - read file */
				if (!xatoi(&ptr, &p1)) break;
				p2 = 0;
				Timer = 0;
				while (p1) {
					if ((UINT)p1 >= blen) {
						cnt = blen; p1 -= blen;
					} else {
						cnt = p1; p1 = 0;
					}
					res = f_read(&File[0], Buff, cnt, &s2);
					if (res != FR_OK) { put_rc(res); break; }
					p2 += s2;
					if (cnt != s2) break;
				}
				xprintf("%lu bytes read with %lu kB/sec.\n", p2, Timer ? (p2 / Timer) : 0);
				break;

			case 'w' :	/* fw <len> <val> - write file */
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
				memset(Buff, (BYTE)p2, blen);
				p2 = 0;
				Timer = 0;
				while (p1) {
					if ((UINT)p1 >= blen) {
						cnt = blen; p1 -= blen;
					} else {
						cnt = p1; p1 = 0;
					}
					res = f_write(&File[0], Buff, cnt, &s2);
					if (res != FR_OK) { put_rc(res); break; }
					p2 += s2;
					if (cnt != s2) break;
				}
				xprintf("%lu bytes written with %lu kB/sec.\n", p2, Timer ? (p2 / Timer) : 0);
				break;

			case 'n' :	/* fn <org.name> <new.name> - Change name of an object */
				while (*ptr == ' ') ptr++;
				ptr2 = xstrchr(ptr, ' ');
				if (!ptr2) break;
				*ptr2++ = 0;
				while (*ptr2 == ' ') ptr2++;
				put_rc(f_rename(ptr, ptr2));
				break;

			case 'u' :	/* fu <name> - Unlink an object */
				while (*ptr == ' ') ptr++;
				put_rc(f_unlink(ptr));
				break;

			case 'v' :	/* fv - Truncate file */
				put_rc(f_truncate(&File[0]));
				break;

			case 'k' :	/* fk <name> - Create a directory */
				while (*ptr == ' ') ptr++;
				put_rc(f_mkdir(ptr));
				break;

			case 'a' :	/* fa <atrr> <mask> <name> - Change attribute of an object */
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
				while (*ptr == ' ') ptr++;
				put_rc(f_chmod(ptr, p1, p2));
				break;

			case 't' :	/* ft <year> <month> <day> <hour> <min> <sec> <name> - Change timestamp of an object */
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
				Finfo.fdate = ((p1 - 1980) << 9) | ((p2 & 15) << 5) | (p3 & 31);
				if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
				Finfo.ftime = ((p1 & 31) << 11) | ((p2 & 63) << 5) | ((p3 >> 1) & 31);
				put_rc(f_utime(ptr, &Finfo));
				break;

			case 'x' : /* fx <src.name> <dst.name> - Copy a file */
				while (*ptr == ' ') ptr++;
				ptr2 = xstrchr(ptr, ' ');
				if (!ptr2) break;
				*ptr2++ = 0;
				while (*ptr2 == ' ') ptr2++;
				xprintf("Opening \"%s\"", ptr);
				res = f_open(&File[0], ptr, FA_OPEN_EXISTING | FA_READ);
				xputc('\n');
				if (res) {
					put_rc(res);
					break;
				}
				xprintf("Creating \"%s\"", ptr2);
				res = f_open(&File[1], ptr2, FA_CREATE_ALWAYS | FA_WRITE);
				xputc('\n');
				if (res) {
					put_rc(res);
					f_close(&File[0]);
					break;
				}
				xprintf("Copying file...");
				Timer = 0;
				p1 = 0;
				for (;;) {
					res = f_read(&File[0], Buff, blen, &s1);
					if (res || s1 == 0) break;   /* error or eof */
					res = f_write(&File[1], Buff, s1, &s2);
					p1 += s2;
					if (res || s2 < s1) break;   /* error or disk full */
				}
				xprintf("\n%lu bytes copied with %lu kB/sec.\n", p1, p1 / Timer);
				f_close(&File[0]);
				f_close(&File[1]);
				break;
#if _FS_RPATH
			case 'g' :	/* fg <path> - Change current directory */
				while (*ptr == ' ') ptr++;
				put_rc(f_chdir(ptr));
				break;
#if _FS_RPATH >= 2
			case 'q' :	/* fq - Show current dir path */
				res = f_getcwd(Line, sizeof Line);
				if (res)
					put_rc(res);
				else
					xprintf("%s\n", Line);
				break;
#endif
#endif
#if _USE_LABEL
			case 'b' :	/* fb <name> - Set volume label */
				while (*ptr == ' ') ptr++;
				put_rc(f_setlabel(ptr));
				break;
#endif	/* _USE_LABEL */
#if _USE_MKFS
			case 'm' :	/* fm <ld#> <rule> <csize> - Create file system */
				if (!xatoi(&ptr, &p1) || (UINT)p1 > 9 || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
				xprintf("The volume will be formatted. Are you sure? (Y/n)=");
				xgets(Line, sizeof Line);
				if (Line[0] == 'Y') {
					xsprintf(Line, "%u:", (UINT)p1);
					put_rc(f_mkfs(Line, (BYTE)p2, (DWORD)p3, Buff, sizeof Buff));
				}
				break;
#endif	/* _USE_MKFS */
			case 'z' :	/* fz [<size>] - Change/Show R/W length for fr/fw/fx command */
				if (xatoi(&ptr, &p1) && p1 >= 1 && p1 <= (long)sizeof Buff)
					blen = p1;
				xprintf("blen=%u\n", blen);
				break;
			}
			break;
#ifdef SOUND_DEFINED
		case 'p' :	/* p <wavfile> - Play RIFF-WAV file */
			while (*ptr == ' ') ptr++;
			res = f_open(&File[0], ptr, FA_READ);
			if (res) {
				put_rc(res);
			} else {
				load_wav(&File[0], "WAV Player", Buff, sizeof Buff);
				f_close(&File[0]);
			}
			break;
#endif
		}
	}
}


