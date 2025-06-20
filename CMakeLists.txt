cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# enforce GBA compilation
set(DKP_GBA_PLATFORM_LIBRARY "libtonc")
set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/cmake/GBA.cmake")

project(Entombed VERSION 1.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}.")

# exe
add_executable(entombed
	ext/fade.c
	ext/fade.h
	ext/itoa.cpp
	ext/itoa.h
	include/enemies.h
	include/gameDefines.h
	include/globalvars.h
	include/guns.h
	include/highscore_entry.h
	include/layer2tileLUT.h
	include/level.h
	include/level_data.h
	include/level_selector.h
	include/main.h
	include/map.h
	include/moving_blocks.h
	include/moving_platforms.h
	include/objPropertiesLUT.h
	include/player.h
	include/projectiles.h
	include/sequences.h
	include/sfx.h
	include/text.h
	include/title.h
	src/enemies.cpp
	src/guns.cpp
	src/highscore_entry.cpp
	src/layer2tileLUT.c
	src/level.cpp
	src/level_data.c
	src/level_selector.cpp
	src/main.cpp
	src/map.cpp
	src/moving_blocks.cpp
	src/moving_platforms.cpp
	src/objPropertiesLUT.c
	src/player.cpp
	src/projectiles.cpp
	src/sequences.cpp
	src/sfx.cpp
	src/text.c
	src/title.cpp)
target_include_directories(entombed PRIVATE ext include)
target_compile_options(entombed PRIVATE
	-Wall -Wextra
	$<$<CONFIG:Release,RelWithDebInfo>:-mlong-calls -fomit-frame-pointer -ffast-math>
	$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
)

# gfx
# TODO: maybe use -gzl to save some rom space?
grit_add_binary_target(entombed_font gfx/entombed_font.bmp       # Font (8x8) - Has own palette
	NO_MAP OUTPUT_PAL "pal_font" OPTIONS -pn16)
grit_add_binary_target(tilemap_walls gfx/tilemap_walls.bmp       # Walls (32x16), Background palette - Black is transparent
	NO_MAP OUTPUT_PAL "pal_bg" OPTIONS -pn32 -Mw4 -Mh2)
grit_add_binary_target(spr_player gfx/spr_player.bmp             # Player (32x32), Sprite palette - Black is transparent
	NO_MAP OUTPUT_PAL "pal_oam" OPTIONS -pn16 -Mw4 -Mh4)
grit_add_binary_target(tilemap_fixtures gfx/tilemap_fixtures.bmp # Fixtures (32x16)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh2)
grit_add_binary_target(spr_title gfx/spr_title.bmp               # Title screen (32x32)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh4)
grit_add_binary_target(spr_levelselect gfx/spr_levelselect.bmp   # Level selector (32x32)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh4)
grit_add_binary_target(spr_projectiles gfx/spr_projectiles.bmp   # Projectiles (8x8)
	NO_MAP NO_PALETTE)
grit_add_binary_target(spr_enemies gfx/spr_enemies.bmp           # Enemies (32x32)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh4)
grit_add_binary_target(spr_platform gfx/spr_platform.bmp         # Moving platform (32x16)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh2)
grit_add_binary_target(spr_urn gfx/spr_urn.bmp                   # Breaking urn (32x32)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh4)
grit_add_binary_target(spr_gameover gfx/spr_gameover.bmp         # Game over (32x16)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh2)
grit_add_binary_target(spr_coinscores gfx/spr_coinscores.bmp     # Coin scores (32x8)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh1)
grit_add_binary_target(spr_digits gfx/spr_digits.bmp             # Hourglass digit (16x16)
	NO_MAP NO_PALETTE OPTIONS -Mw2 -Mh2)
grit_add_binary_target(spr_blocks gfx/spr_blocks.bmp             # Moving blocks (32x16)
	NO_MAP NO_PALETTE OPTIONS -Mw4 -Mh2)
dkp_add_embedded_binary_library(entombed-graphics
	entombed_font tilemap_walls spr_player tilemap_fixtures
	spr_title spr_levelselect spr_projectiles spr_enemies spr_platform
	spr_urn spr_gameover spr_coinscores spr_digits spr_blocks)

# sfx
mm_add_soundbank_target(soundbank
	HEADER "soundbank.h"
	INPUTS
		sound/ankh.wav
		sound/arrow.wav
		sound/arrow_hit.wav
		sound/bow.wav
		sound/bullet.wav
		sound/change.wav
		sound/checkpoint.wav
		sound/climb.wav
		sound/coin.wav
		sound/door.wav
		sound/enemy_die.wav
		sound/exit.wav
		sound/jump.wav
		sound/jump-old1.wav
		sound/key.wav
		sound/player_die.wav
		sound/teleport.wav
		sound/title.wav
		sound/urn_break.wav
		sound/walk.wav)
target_include_directories(entombed PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
dkp_add_embedded_binary_library(entombed-sounds soundbank)

# libs
add_library(maxmod STATIC IMPORTED)
set_target_properties(maxmod PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "$ENV{DEVKITPRO}/libgba/include"
	IMPORTED_LOCATION "$ENV{DEVKITPRO}/libgba/lib/libmm.a")
target_link_libraries(entombed PRIVATE
	entombed-graphics entombed-sounds maxmod)

# debug
dkp_target_generate_symbol_list(entombed)

# rom
gba_create_rom(entombed
	TITLE "Entombed"
	GAMECODE "TOMB"
	MAKERCODE "SE")

# installation (used for packaging)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/entombed.gba
	DESTINATION . COMPONENT rom)
install(TARGETS entombed RUNTIME DESTINATION . COMPONENT debug)
install(FILES
			${CMAKE_CURRENT_BINARY_DIR}/entombed.map
			${CMAKE_CURRENT_BINARY_DIR}/entombed.lst
	DESTINATION . COMPONENT debug)

# packaging
set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR ";")
set(CPACK_PACKAGE_DIRECTORY "packages")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_FILE_NAME "entombed-gba-${PROJECT_VERSION}")
set(CPACK_ARCHIVE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_ARCHIVE_ROM_FILE_NAME "${CPACK_ARCHIVE_FILE_NAME}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)
