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

int (*decompressLZMA)(unsigned char *in, unsigned insize, unsigned char *out, unsigned outsize);
void (*cfe_go)(cfe_loadargs_t *la);
void (*memset)(void * ptr, int value, long num);

unsigned long (*strtoul)(char *str);
int (*tftpLoad)(char *fileName, unsigned long loadAddr);

int (*runProgram)(
    const char *srcType,
    const char *srcParam,
    int unk3,
    const char *path
);

int (*autoRun)(
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
    autoRun = (void *)0xF1F7D8;
    printString = *(void **)0xF6D714;
    decompressLZMA = (void *)0xF462DC;
    memset = (void *)0xF3889C;
    cfe_go = (void *)0xF1E168;

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
    return runProgram("tftp", "eth0", 0x3, path);
}

int ui_autorun(void *argv){
    char *path = cmd_getarg(argv, 0);
    return autoRun(path);
}

int ui_bootc(void *argv){
    char *address = cmd_getarg(argv, 0);
    if (!address){
        printf("Invalid Arguments\n");
        printf("Usage: cboot <address>\n");
        return -1;
    }

    unsigned long addr = strtoul(address);
    return bootCompressedImage((uint32_t *)addr, 1);
}

// Compressed image head format in Big Endian:
// 1) Text Start address:    4 bytes
// 2) Program Entry point:   4 bytes
// 3) Compress image Length: 4 bytes
// 4) Compress data starts:  compressed data
int bootCompressedImage(uint32_t *puiCmpImage, int retry)
{
    unsigned char *pucSrc;
    unsigned char *pucDst;
    unsigned char *pucEntry;
    unsigned int dataLen;
    unsigned int uncomplen = 0;
    char          brcmMagic[] = {'B','R','C','M'};
    int ret = 0;
    cfe_loadargs_t la;

    memset((unsigned char *) &la, 0x00, sizeof(la));

    /* Boot compressed image that was downloaded to RAM. */
    pucDst = (unsigned char *) ((uintptr_t)*puiCmpImage);
    pucEntry = (unsigned char *)((uintptr_t)*(puiCmpImage + 1));
    dataLen = (unsigned int) *(puiCmpImage + 2);
    //new image format contains broadcom signature and uncompressed length.
    if ( (*(puiCmpImage + 3)) == (*(uint32_t*)brcmMagic))
    {
        uncomplen = (unsigned int) *(puiCmpImage + 4);
        pucSrc = (unsigned char *) (puiCmpImage + 5);
    }
    else
    {
    pucSrc = (unsigned char*) (puiCmpImage + 3);
    }

    printf("Code Address: 0x%08X, Entry Address: 0x%08x\n",
        (uintptr_t) pucDst, (uintptr_t)pucEntry);

    ret = decompressLZMA(pucSrc, dataLen, pucDst, 23*1024*1024);
    
    if (ret == 0) 
    {
        printf("Decompression %s image OK!\n",uncomplen ? "LZ4":"LZMA");
        la.la_entrypt = (long) pucEntry;
        la.la_flags = LA_DEFAULT_FLAGS;
        printf("Entry at 0x%p\n",la.la_entrypt);
        cfe_go(&la);  // never return...
    }
    else
        printf("Failed on decompression.  Corrupted image?\n");

    return ret;
}

