#include "cfe.h"
#include "cfe_globals.h"

#define ENTRY_FUNC  __attribute__((__section__(".entry")))

void ENTRY_FUNC main(){
	cfe_init_funcs();
    printf("[+] CFE Stage 2 started...\n");
    cmd_addcmd("goex", &ui_goex, 0, "goex <tftp_path>", defaultHelp, defaultHelp);
    printf("[+] CFE Stage 2 done!\n");
}