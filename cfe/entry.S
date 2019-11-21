#define PROPER_LR 0x00F224AC

.global start
.section .entry

start:
ldr lr, =PROPER_LR
push {LR}
BL main
POP {PC}