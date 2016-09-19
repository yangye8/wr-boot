/*
 * vivi/lib/load-file.c: Load a file via serial or usb.
 *
 * Copyright (C) 2002 MIZI Research, Inc.
 *
 * Author : Janghoon Lyu <nandy@mizi.com>
 * Date   : $Date: 2004/02/04 06:22:25 $
 *
 * $Revision: 1.1.1.1 $
 *
 * History
 *
 * 2002-06-27: Janghoon Lyu <nandy@mizi.com>
 *    - Initial code
 *
 * 2002-07-29: Janghoon Lyu <nandy@mizi.com>
 *
 * TODO:
 */

#include "command.h"
#include "priv_data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wrboot.h>
#include <types.h>

static int modem_is(const char *mt)
{
    if (strncmp("x", mt, 1) == 0) {
        return X_MODEM;
    } else if (strncmp("y", mt, 1) == 0) {
        return Y_MODEM;
    } else if (strncmp("z", mt, 1) == 0) {
        return Z_MODEM;
    } else {
        return UNKNOWN_MODEM;
    }
}

static u32 download_file(char *buf, size_t size, int modem)
{
    switch (modem) {
        case X_MODEM:
            printf("Ready for downloading using xmodem...\n");
            printf("Waiting...\n");
//            return xmodem_receive(buf, size);
            break;
        case Y_MODEM:
            printf("Not support ymodem yet. Using xmodem\n");
            printf("Ready for downling using xmodem\n");
            printf("Waiting...\n");
//            return xmodem_receive(buf, size);
            break;
        case Z_MODEM:
            printf("Not support zmodem yet. using xmodem\n");
            printf("Ready for downling using xmodem\n");
            printf("Wating...\n");
//            return xmodem_receive(buf, size);
            break;
        default:
            printf("Not support this modem\n");
            break;
    }
    return 0;
}

/*
 * Sub-commands
 */
user_subcommand_t load_cmds[];

/*
 * vivi> load ram <addr> <modem>
 * vivi> load ram <modem> 
 *
 */
static void command_load_ram(int argc, const char **argv)
{
    int ret;
    char *buf = (char *)RAM_BASE;
    size_t size;
    u32 retlen;
    int modem = 0;

    switch (argc) {
    case 2: /* load file to RAM_BASE */
      buf = (char *)RAM_BASE;
      size = 0;
      if ((modem = modem_is(argv[1])) == UNKNOWN_MODEM)
        goto error_parse_arg;
      break;
    case 3:
      buf = (char *)strtoul(argv[1], NULL, 0);
      if (ret)
        goto error_parse_arg;
      size = 0;
      if ((modem = modem_is(argv[2])) == UNKNOWN_MODEM)
        goto error_parse_arg;
      break;
    case 4:
      buf = (char *)strtoul(argv[1], NULL, 0);
      if (ret)
        goto error_parse_arg;
      size = (size_t)strtoul(argv[2], NULL, 0);
      if (ret)
        goto error_parse_arg;
      if ((modem = modem_is(argv[3])) == UNKNOWN_MODEM)
        goto error_parse_arg;
      break;
    default:
      printf("invalid 'load ram' command: too few or many arguments\n");
      return;
    }

    retlen = download_file(buf, size, modem);
    if (retlen == 0)
      goto error_download;
    printf("downloaed file at 0x%08lx, size = %d bytes\n", buf, retlen);
    return;

error_parse_arg:
    printf("Can't parsing argumets\n");
error_download:
    printf("Failed downloading file\n");
    return;
}

static void command_load_flash(int argc, const char **argv)
{
    int ret;
    u32 to; 
    char *buf = (char *)RAM_BASE;
    size_t size;
    u32 retlen;
    int modem;
    mtd_partition_t *dst_part;
    int flag;

    if (argc != 4 && argc != 3) {
        printf("invalid 'load flash' command: too few or many arguments\n");
    }

    if (argc == 3) {
        dst_part = get_mtd_partition(argv[1]);
        if (dst_part == NULL) {
            //printf("Could not found \"%s\" partition\n", argv[1]);
            return;
        }
        to = dst_part->offset;
        size = dst_part->size;
        flag = dst_part->flag;
        modem = modem_is(argv[2]);

    } else {
        to = strtoul(argv[1], NULL, 0);
        if (ret) goto error_parse_arg;
        size = (size_t)strtoul(argv[2], NULL, 0);
        if (ret) goto error_parse_arg;
        modem = modem_is(argv[3]);
        flag = 0;
    }

    retlen = download_file(buf, size, modem);

    /* hacked by nandy. delay for serial output */
    { int i = 0x10000; while (i > 0) i--; }

    if (retlen == 0) {
        printf("Failed downloading file\n");
        return;
    }
    printf("Downloaded file at 0x%08lx, size = %d bytes\n", buf, retlen);

    if (retlen > size) {
        printf("An image size is too large to write flash.\n\t\t\t"
            "wanted = 0x%08lx, loaded = 0x%08lx\n", size, retlen);
        return;
    }

    ret = write_to_flash(to, retlen, buf, flag);

    return;

error_parse_arg:
    printf("Can't parsing argumets\n");
    return;
}

static void command_help(int argc, const char **argv)
{
    print_usage("load", load_cmds);
}

user_subcommand_t load_cmds[] = {
{
    "help",
    command_help,
    "help \t\t\t-- x = xmodem, ..."
}, {
    "flash",
    command_load_flash,
    "flash <mtd_part_name|addr length> <x|y|z> \t-- Load a file to Flash" 
}, {
    "ram",
    command_load_ram,
    "ram [addr [length]] <x|y|z> \t\t-- Load a file to RAM " 
}, {
    NULL,
    NULL,
    NULL
}    
};


/* Main command */
void command_load(int argc, const char **argv)
{
    if (argc == 1) {
        printf("invaild 'load' command: too few arguments\n");
        command_help(0, NULL);
        return;
    }
    execsubcmd(load_cmds, argc-1, argv+1);
}

user_command_t load_cmd = {
    "load",
    command_load,
    NULL,
    "load [{cmds}] \t\t\t-- Load a file to RAM/Flash"
};
