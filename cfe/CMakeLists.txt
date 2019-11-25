add_binary(
	TARGET bootstrap
	LDSCRIPT cfe.lds.m4
	LOADADDR 0x4000000
	ENTRYPOINT 0x4000000
	SOURCES
		entry.S
		cfe.c
		bootstrap.c
	EXTRA_LDFLAGS
		-e start
)

## Smx must go to work, so he won't do this.
add_binary(
	TARGET stage2
	LDSCRIPT cfe_stage2.lds.m4
	LOADADDR 0x4010000
	ENTRYPOINT 0x4010000
	SOURCES
		entry.S
		cfe.c
		stage2.c
	EXTRA_LDFLAGS
		-e start
)

add_custom_target(deploy
	COMMAND echo [+] Deploying cfe_boostrap.bin
	COMMAND php ${CMAKE_CURRENT_LIST_DIR}/deploy.php ${CMAKE_CURRENT_BINARY_DIR}/cfe_bootstrap.bin 0x4000000
	COMMAND echo [+] Executing cfe_boostrap.bin...
	COMMAND echo go 0x4000000 > /dev/ttyUSB0
	COMMAND echo [+] Copying cfe_stage2.bin to TFTP directory...
	COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cfe_stage2.bin /srv/tftp/cfe_stage2.bin
	COMMAND echo [+] Executing tftpget cfe_stage2.bin on target...
	COMMAND echo tftpget 192.168.1.100:cfe_stage2.bin 0x4010000 > /dev/ttyUSB0
	COMMAND echo [+] Executing cfe_stage2.bin...
	COMMAND echo go 0x4010000 > /dev/ttyUSB0
	DEPENDS cfe_bootstrap.bin cfe_stage2.bin
)

add_custom_target(reboot
	COMMAND stty -F /dev/ttyUSB0 raw ispeed 115200 ospeed 115200 ixoff cs8
	COMMAND toggle_router_power
	COMMAND sleep 2
	COMMAND echo -ne \\r\\n > /dev/ttyUSB0
	COMMAND sleep 2
	# make sure we build before doing an useless reboot
	DEPENDS cfe_bootstrap.bin cfe_stage2.bin
)

add_custom_target(patch
	COMMAND php ${CMAKE_CURRENT_LIST_DIR}/patch.php
)

add_custom_target(view
	COMMAND screen /dev/ttyUSB0 115200,ixoff,cs8
)

# reboot, then patch
add_dependencies(patch reboot)
# patch, then deploy
add_dependencies(deploy patch)
# deploy, then view
add_dependencies(view deploy)
