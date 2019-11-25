cmake_minimum_required(VERSION 3.13)
project(bootSdk)

macro(set_asm_props src)
	set_property(SOURCE "${src}" PROPERTY LANGUAGE C)
	set_property(SOURCE "${src}" PROPERTY COMPILE_DEFINITIONS __ASSEMBLY__)
endmacro()

function(get_last_ext fileName outVar)
	get_filename_component(ext "${fileName}" EXT)
	string(REGEX MATCH  "\.([a-zA-Z0-9]+)$" lastExt "${ext}")
	set(${outVar} "${lastExt}" PARENT_SCOPE)
endfunction()

function(get_name_wle fileName outVar)
	string(REGEX REPLACE "\.([a-zA-Z0-9]+)$" "" nameWle "${fileName}") 
	set(${outVar} "${nameWle}" PARENT_SCOPE)
endfunction()

function(add_binary)
	cmake_parse_arguments(BIN "" "TARGET;LDSCRIPT;LOADADDR;ENTRYPOINT" "SOURCES;EXTRA_CFLAGS;EXTRA_LDFLAGS" ${ARGN})
	
	get_filename_component(BIN_LDSCRIPT "${BIN_LDSCRIPT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

	get_last_ext("${BIN_LDSCRIPT}" ldsExt)
	if(ldsExt STREQUAL ".m4")
		get_filename_component(ldsName "${BIN_LDSCRIPT}" NAME)
		get_name_wle("${ldsName}" ldsBaseName)

		# construct target name
		file(RELATIVE_PATH ldsRel "${CMAKE_CURRENT_SOURCE_DIR}" "${BIN_LDSCRIPT}")
		get_filename_component(ldsDir "${BIN_LDSCRIPT}" DIRECTORY)
		string (REPLACE "/" "-" ldsTargetName "${ldsRel}")
		set(ldsTargetName "${ldsRel}")
		
		set(ldsOut "${CMAKE_CURRENT_BINARY_DIR}/${ldsBaseName}")		
		# command to preprocess .lds
		add_custom_command(
			OUTPUT "${ldsOut}"
			COMMAND sh -c \"m4 -D LOADADDR=${BIN_LOADADDR} -P "${BIN_LDSCRIPT}" > ${ldsOut}\"
			DEPENDS "${BIN_LDSCRIPT}"
		)
		# target to invoke the command
		add_custom_target(${ldsTargetName} DEPENDS ${ldsOut})

		# replace ldscript with preprocessed version
		message(STATUS ${ldsOut})
		
		set(BIN_LDSCRIPT "${ldsOut}")
		set(LDSCRIPT_TARGET "${ldsTargetName}")
	endif()

	# prepend directory basename to target
	get_filename_component(dirname "${CMAKE_CURRENT_LIST_DIR}" NAME)
	set(BIN_TARGET "${dirname}_${BIN_TARGET}")

	set(deps "")
	set(sources "")
	foreach(src ${BIN_SOURCES})
		# convert to absolute
		get_filename_component(src "${src}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

		## FIXME: use CMake 3.14 LAST_EXT/NAME_WLE (when CMake 3.14 is widely available)
		
		get_filename_component(ext "${src}" EXT)
		
		get_last_ext("${ext}" lastExt)

		# Skip non-m4 files
		if(NOT lastExt STREQUAL ".m4")
			if(lastExt STREQUAL ".S")
				set_asm_props("${src}")
			endif()
			list(APPEND sources "${src}")
			continue()
		endif()

		# get relative path
		file(RELATIVE_PATH relPath "${CMAKE_CURRENT_SOURCE_DIR}" "${src}")

		get_filename_component(relDir "${relPath}" DIRECTORY)
		get_name_wle("${relPath}" output)

		set(output "${CMAKE_CURRENT_BINARY_DIR}/${output}")
		if(relDir)
			file(MAKE_DIRECTORY "${relDir}")
			# append reldir
			set(output "${output}/${relDir}")
		endif()
		
		message(STATUS ${src})
		add_custom_command(
			OUTPUT "${output}"
			COMMAND sh -c \"m4 -P ${src} > ${output}\"
			DEPENDS "${src}"
		)

		# .S files must be passed to gcc just like C files
		get_filename_component(output_ext "${output}" EXT)
		if(output_ext STREQUAL ".S")
			set_asm_props("${output}")
		endif()

		get_filename_component(src_name "${src}" NAME)
		set(targetName ${dirname}_${src_name})
		add_custom_target(${targetName} DEPENDS ${output})

		list(APPEND deps "${targetName}")
		list(APPEND sources "${output}")
	endforeach()

	add_executable(${BIN_TARGET} ${sources})
	foreach(tdep ${deps})
		add_dependencies(${BIN_TARGET} ${tdep})
	endforeach()

	message(STATUS "${BIN_TARGET} ${sources}")

	set(cflags -fno-exceptions -nostartfiles -nostdlib -ffreestanding -Os -no-pie -fno-plt -s -Wl,--build-id=none)
	#set(cflags -fno-exceptions -nostartfiles -nostdlib -ffreestanding -fPIC -O0 -s -Wl,--build-id=none)
	set(ldflags ${cflags} -T"${BIN_LDSCRIPT}")
	
	string (REPLACE ";" " " cflags_str "${cflags}")
	string (REPLACE ";" " " ldflags_str "${ldflags}")
	
	if(BIN_EXTRA_CFLAGS)
		string (REPLACE ";" " " extra_cflags_str "${BIN_EXTRA_CFLAGS}")
		set(cflags_str "${cflags_str} ${extra_cflags_str}")
	endif()
	if(BIN_EXTRA_LDFLAGS)
		string (REPLACE ";" " " extra_ldflags_str "${BIN_EXTRA_LDFLAGS}")
		set(ldflags_str "${ldflags_str} ${extra_ldflags_str}")
	endif()

	set_target_properties(${BIN_TARGET} PROPERTIES COMPILE_FLAGS "${cflags_str}")
	set_target_properties(${BIN_TARGET} PROPERTIES LINK_FLAGS "${ldflags_str}")
	set_target_properties(${BIN_TARGET} PROPERTIES LINK_DEPENDS ${BIN_LDSCRIPT})
	target_include_directories(${BIN_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

	# if the user specified a .m4 based lds, add a dependency to its target
	if(LDSCRIPT_TARGET)
		add_dependencies(${BIN_TARGET} ${LDSCRIPT_TARGET})
	endif()

	# objcopy target: creates .bin file
	get_filename_component(outputBin "${BIN_TARGET}.bin" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	add_custom_command(
		OUTPUT "${outputBin}"
		COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${BIN_TARGET}> "${outputBin}"
		DEPENDS $<TARGET_FILE:${BIN_TARGET}>
	)
	add_custom_target(${BIN_TARGET}.bin ALL DEPENDS "${outputBin}")
	add_dependencies(${BIN_TARGET}.bin ${BIN_TARGET})

	# mkimage target: creates .uimg file
	get_filename_component(outputImage "${BIN_TARGET}.uimg" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	add_custom_command(
		OUTPUT "${outputImage}"
		COMMAND mkimage
				-A arm
				-T standalone
				-O u-boot
				-C none
				-n "${BIN_TARGET}"
				-a ${BIN_LOADADDR}
				-e ${BIN_ENTRYPOINT}
				-d ${outputBin} ${outputImage}
		DEPENDS ${outputBin}
	)
	
	add_custom_target(${BIN_TARGET}.uimg ALL DEPENDS "${outputImage}")
	add_dependencies(${BIN_TARGET}.uimg ${BIN_TARGET}.bin)
endfunction()

add_subdirectory(lxboot)
#add_subdirectory(uboot-mstar)
add_subdirectory(cfe)
