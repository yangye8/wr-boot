/*
 * vivi/include/command.h
 *
 */
#ifndef _COMMAND_H_
#define _COMMAND_H_

long paramoldvalue;
int argc;

enum ParseState {
    PS_WHITESPACE,
    PS_TOKEN,
    PS_STRING,
    PS_ESCAPE
};

enum ParseState stackedState;

//struct user_command_t;

typedef struct user_command {
    const char *name;
    void (*cmdfunc)(int argc, const char **);
    struct user_command *next_cmd;
    const char *helpstr;
} user_command_t;

typedef struct user_subcommand {
    const char *name;
    void (*cmdfunc)(int argc, const char **);
    const char *helpstr;
} user_subcommand_t;

/* General interfaces */
extern void add_command(user_command_t *cmd);
void execcmd(int, const char **);
void exec_string(char *);
void execsubcmd(user_subcommand_t *, int, const char **);
void print_usage(char *strhead, user_subcommand_t *);
void invalid_cmd(const char *cmd_name, user_subcommand_t *cmds);
int init_builtin_cmds(void);

/* Porocessor memory map */
#define ROM_BASE0    0x00000000      /* base address of rom bank 0 */
#define ROM_BASE1    0x08000000      /* base address of rom bank 1 */
#define DRAM_BASE0   0x30000000      /* base address of dram bank 0 */
#define DRAM_BASE1   0x38000000>-/* base address of dram bank 1 */

#define DRAM_BASE    DRAM_BASE0
#define DRAM_SIZE    SZ_64M

/* ROM */
#define VIVI_ROM_BASE       0x00000000
#define VIVI_PRIV_ROM_BASE  0x01FC0000

#define MTD_PART_SIZE       SZ_16K
#define MTD_PART_OFFSET     0x00000000
#define PARAMETER_TLB_SIZE  SZ_16K
#define PARAMETER_TLB_OFFSET 0x00004000
#define LINUX_CMD_SIZE      SZ_16K
#define LINUX_CMD_OFFSET    0x00008000
#define VIVI_PRIV_SIZE      (MTD_PART_SIZE + PARAMETER_TLB_SIZE + LINUX_CMD_SIZE)


/* RAM */
#define VIVI_RAM_SIZE    SZ_1M
#define VIVI_RAM_BASE    (DRAM_BASE + DRAM_SIZE - VIVI_RAM_SIZE)
#define HEAP_SIZE    SZ_1M
#define HEAP_BASE    (VIVI_RAM_BASE - HEAP_SIZE)
#define MMU_TABLE_SIZE    SZ_16K
#define MMU_TABLE_BASE    (HEAP_BASE - MMU_TABLE_SIZE)
#define VIVI_PRIV_RAM_BASE (MMU_TABLE_BASE - VIVI_PRIV_SIZE)
#define STACK_SIZE    SZ_32K
#define STACK_BASE    (VIVI_PRIV_RAM_BASE - STACK_SIZE)
#define RAM_SIZE    (STACK_BASE - DRAM_BASE)
#define RAM_BASE    DRAM_BASE


#endif /* _COMMAND_H_ */
