#include "cfe.h"

void *defaultHelp;
int (*console_status)();
int (*console_get_char)(void *pbuf, int size);
void (*printf)(const char *, ...);
char *(*cmd_getarg)(void *argv, int argInd);
void (*cmd_addcmd)(
    const char *cmdName,
    void *pfnCmd,
    int unkZero,
    const char *cmdDescr,
    char *arg1Help,
    char *arg2Help
);

unsigned long (*strtoul)(char *str);
int (*tftpLoad)(char *fileName, unsigned long loadAddr);

int (*runProgram)(
    const char *srcType,
    const char *srcParam,
    int unk3,
    const char *path
);

void (*printString)(char *);

void enableIrqFiq(){
	asm volatile(
		"mrs %%r0, CPSR\r\n"
		"mov %%r1, #0xC0\r\n"
		"and %%r0, %%r0, %%r1\r\n"
		"msr CPSR_cf, %%r0\r\n"
		::: 
	);
}

void enableMMU(){
    asm volatile(
        "mrc p15, 0, %%r0, c1, c0, 0\r\n"
        "orr %%r0, #0x1\r\n"
        "mcr p15, 0, %%r0, c1, c0, 0\r\n"
        :::
    );
}

void cfe_init_funcs()
{
    printf = (void *)0xF37D00;
    defaultHelp = (void *)0xF4B53C;   
    console_status = (void *)0xF3A30C;
    console_get_char = (void *)0xF3A1F8;
    cmd_getarg = (void *)0xf3b2f4;
    cmd_addcmd = (void *)0xF3B98C;
    strtoul = (void *)0xf38b70;
    tftpLoad = (void *)0x00F1D8F0;
    runProgram = (void *)0xF1E1F4;
    printString = *(void **)0xF6D714;
};

int ui_tftpget(void *argv){
    char *path = cmd_getarg(argv, 0);
    char *addr = cmd_getarg(argv, 1);
    if(!path || !addr){
        printf("invalid arguments\n");
        return -1;
    }

    unsigned long loadAddr = strtoul(addr);
    return tftpLoad(path, loadAddr);
}

int ui_goex(void *argv){
    char *path = cmd_getarg(argv, 0);
    return runProgram("tftp", "eth0", 3, path);
}