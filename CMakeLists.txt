cmake_minimum_required(VERSION 3.15)

project(
	CSnake
	VERSION 1.0
	DESCRIPTION "Classic Snake Game written in C"
	HOMEPAGE_URL "E-Mail: galaxyking0419@gmail.com"
	LANGUAGES C
)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (UNIX OR MINGW)
	set(COMMON_FLAGS "-std=gnu18 -march=x86-64 -mtune=generic -flto -fuse-linker-plugin -pipe -Wno-unused-function -Wno-unused-parameter -Wno-attributes")
	set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS} -O3 -s")
	set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS} -Wall -Wextra -g -O0")
elseif(WIN32)
	# Use Folder for default projects
	add_compile_options($<$<CONFIG:Release>:/MT>)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set(COMMON_FLAGS "/std:c++17 /utf-8")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${COMMON_FLAGS} /GL")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COMMON_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib" CACHE STRING "" FORCE)
endif()

add_executable(
	snake
	src/common-def.h
	src/queue.h
	src/queue.c
	src/tui.h
	src/tui.c
	src/snake.h
	src/snake.c
)

if (UNIX OR MINGW)
	target_link_libraries(snake pthread)
endif()
