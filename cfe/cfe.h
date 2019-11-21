#ifndef _CFE_H
#define _CFE_H

typedef unsigned long uintptr_t;

extern void main();

#define TRACE() \
    do { \
        register unsigned long r11 asm("r11"); \
        register unsigned long lr asm("lr"); \
        register unsigned long sp asm("sp"); \
        ((void(*)(char *, ...))0xF37D00)("%s: %x,%x,%x\n", __func__, r11, lr, sp); \
    } while(0)


// Helper
void enableIrqFiq();
void enableMMU();

void cfe_init_funcs();

// Own UI functions
int ui_tftpget(void *argv);
int ui_goex(void *argv);

#endif