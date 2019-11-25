#include "cfe.h"

// CFE definitions
extern void *defaultHelp;
extern int (*console_status)();
extern int (*console_get_char)(void *pbuf, int size);
extern void (*printf)(const char *, ...);
extern char *(*cmd_getarg)(void *argv, int argInd);
extern void (*cmd_addcmd)(
    const char *cmdName,
    void *pfnCmd,
    int unkZero,
    const char *cmdDescr,
    char *arg1Help,
    char *arg2Help
);

extern int decompressLZMA(unsigned char *in, unsigned insize, unsigned char *out, unsigned outsize);
extern void cfe_go(cfe_loadargs_t *la);
extern void memset (void * ptr, int value, long num);

extern unsigned long (*strtoul)(char *str);
extern int (*tftpLoad)(char *fileName, unsigned long loadAddr);

extern int (*runProgram)(
    const char *srcType,
    const char *srcParam,
    int unk3,
    const char *path
);

extern int (*autoRun)(
    const char *path
);

extern void (*printString)(char *);