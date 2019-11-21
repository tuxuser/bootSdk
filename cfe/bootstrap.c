#include "cfe.h"
#include "cfe_globals.h"

#define ENTRY_FUNC  __attribute__((__section__(".entry")))

void main(){
    cfe_init_funcs();
    printf("[+] CFE bootstrap started...\n");
    cmd_addcmd("tftpget", &ui_tftpget, 0, "tftpget <file> <addr>", defaultHelp, defaultHelp);
    printf("[+] CFE bootstrap done!\n");
}