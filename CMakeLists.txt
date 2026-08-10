cmake_minimum_required(VERSION 3.27...4.3)
project(miniCDi VERSION 0.11 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set (APP_AUTHOR "Mr. Lechkar aka CatmanFan")

# Debug options
option(MINICDI_DISABLE_VERBOSE "Completely disable verbose logging of any kind (ignored for Windows/Mac build)." ON)
option(MINICDI_ENABLE_VERBOSE_DISPLAY "Use verbose debug as output instead of VDSR." OFF)
option(MINICDI_ENABLE_VERBOSE_DISPLAY_CPU "Print verbose CPU information (PC, SR, FC, D*, A*)." OFF)
option(MINICDI_ENABLE_VERBOSE_OS9 "Print OS-9 syscalls to verbose." OFF)
option(MINICDI_ENABLE_VERBOSE_MODULE "Prefix verbose with OS-9 modules and addresses." OFF)
option(MINICDI_ENABLE_FORCE_LOGFILE "Always use the verbose log to text file option." OFF)

# Emulation options
option(MINICDI_DISABLE_THROTTLING "Disable throttling on Windows and macOS." OFF)
option(MINICDI_DISABLE_DISC_IFSTREAM "Use C library FILE* instead of std::ifstream for disc sectors." ON)
option(MINICDI_ENABLE_BENCHMARK "Log duration of frame execution." OFF)
option(MINICDI_ENABLE_NATIVERES "Render VDSR in native resolution instead of full-screen or adjusted mode (Wii/Wii U only)." OFF)
option(MINICDI_ENABLE_DEADNVRAM "Simulate a dead Timekeeper, just for funsies (breaks player shell if no NVRAM settings data exists)." OFF)
option(MINICDI_ENABLE_CDINDICATOR "Show an on-screen indicator for when the CD is being read." OFF)

# Compilation config
# -------------------------------------------
add_executable(${PROJECT_NAME})
if (WIN32)
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie -g -pg")
	# target_compile_options(${PROJECT_NAME} PRIVATE -D_WIN32 -O3 -fno-math-errno -Wall)
	target_compile_options(${PROJECT_NAME} PRIVATE -O3 -fomit-frame-pointer -fno-math-errno -Wall)
elseif (APPLE)
	# ...
	target_compile_options(${PROJECT_NAME} PRIVATE -O3 -fomit-frame-pointer -fno-math-errno -Wall)
else()
	dkp_target_generate_symbol_list(${PROJECT_NAME})
	target_compile_options(${PROJECT_NAME} PRIVATE -O3 -fomit-frame-pointer -fno-math-errno -Wall)
endif()

if (MINICDI_DISABLE_VERBOSE AND NOT WIN32 AND NOT APPLE)
	MESSAGE(STATUS "MINICDI_DISABLE_VERBOSE")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_NOLOG)
endif()
if (MINICDI_ENABLE_VERBOSE_DISPLAY AND NOT MINICDI_DISABLE_VERBOSE)
	MESSAGE(STATUS "MINICDI_ENABLE_VERBOSE_DISPLAY")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_DEBUG)
	if (MINICDI_ENABLE_VERBOSE_DISPLAY_CPU)
		MESSAGE(STATUS "MINICDI_ENABLE_VERBOSE_DISPLAY_CPU")
		target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_DEBUG_CPU)
	endif()
endif()
if (MINICDI_ENABLE_VERBOSE_OS9)
	MESSAGE(STATUS "MINICDI_ENABLE_VERBOSE_OS9")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_DEBUG_OS9)
endif()
if (MINICDI_ENABLE_VERBOSE_MODULE)
	MESSAGE(STATUS "MINICDI_ENABLE_VERBOSE_MODULE")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_DEBUG_MODULE)
endif()
if (MINICDI_ENABLE_FORCE_LOGFILE)
	MESSAGE(STATUS "MINICDI_ENABLE_FORCE_LOGFILE")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_FORCE_LOGFILE)
endif()

if (MINICDI_DISABLE_THROTTLING)
	MESSAGE(STATUS "MINICDI_DISABLE_THROTTLING")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_NO_THROTTLING)
endif()
if (MINICDI_DISABLE_DISC_IFSTREAM)
	MESSAGE(STATUS "MINICDI_DISABLE_DISC_IFSTREAM")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_NO_DISC_IFSTREAM)
endif()
if (MINICDI_ENABLE_BENCHMARK)
	MESSAGE(STATUS "MINICDI_ENABLE_BENCHMARK")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_BENCHMARKING)
endif()
if (MINICDI_ENABLE_NATIVERES)
	MESSAGE(STATUS "MINICDI_ENABLE_NATIVERES")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_NATIVERES)
endif()
if (MINICDI_ENABLE_DEADNVRAM)
	MESSAGE(STATUS "MINICDI_ENABLE_DEADNVRAM")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_DEADNVRAM)
endif()
if (MINICDI_ENABLE_CDINDICATOR)
	MESSAGE(STATUS "MINICDI_ENABLE_CDINDICATOR")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_CDINDICATOR)
endif()

# Core source files
# -------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE src src/cdi/m68k)
target_sources(${PROJECT_NAME} PRIVATE
	# Hardware
	src/cdi/chips/IMS66490_CDIC.cpp
	src/cdi/chips/MCD212.cpp
	src/cdi/chips/SCC68070.cpp

	# Musashi core for M68K
	src/cdi/m68k/m68k.h
	src/cdi/m68k/m68kconf.h
	src/cdi/m68k/m68kcpu.c
	src/cdi/m68k/m68kcpu.h
	src/cdi/m68k/m68kdasm.c
	src/cdi/m68k/m68kops.c
	src/cdi/m68k/m68kops.h

	# OS9
	src/cdi/os9/OS9.cpp

	# General
	src/cdi/AdpcmDecoder.cpp
	src/cdi/CDiDisc.cpp
	src/cdi/common.cpp
	src/cdi/PointingDevice.cpp
)

# Platform build specific
# -------------------------------------------

if (WIN32 OR APPLE)
# -------------------------------------------
	# Libraries
	# ************
	find_package(SDL2 REQUIRED)
	# find_package(wxWidgets REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	# target_link_libraries(${PROJECT_NAME} PRIVATE wxWidgets::wxWidgets ${SDL2_LIBRARIES})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_AUDIO=1) # SDL2

	if (WIN32)
		target_sources(${PROJECT_NAME} PRIVATE res/win/windows.rc)
	endif ()

	# Source files/directories
	# ************
	target_include_directories(${PROJECT_NAME} PRIVATE
		# src/frontends/wx
		src/platforms/win
	)

	target_sources(${PROJECT_NAME} PRIVATE
		# src/frontends/wx/main.cpp
		# src/frontends/wx/miniCDiFrame.cpp
		# src/frontends/wx/miniCDiWindow.cpp
		src/platforms/win/main.cpp
	)
# -------------------------------------------
endif()

if (NINTENDO_3DS)
# -------------------------------------------
	# Libraries
	# ************
	target_link_libraries(${PROJECT_NAME} PRIVATE -lcitro2d -lcitro3d)

	# Source files/directories
	# ************
	target_include_directories(${PROJECT_NAME} PRIVATE
		# src/platforms/common/imgui
		src/platforms/3ds
	)

	# ctr_add_shader_library(vshader src/platforms/common/imgui/vshader.v.pica)
	# dkp_add_embedded_binary_library(vshader_data vshader)
	# target_link_libraries(${PROJECT_NAME} PRIVATE vshader_data)

	target_sources(${PROJECT_NAME} PRIVATE
		# src/platforms/common/imgui/imgui.cpp
		# src/platforms/common/imgui/imgui_demo.cpp
		# src/platforms/common/imgui/imgui_draw.cpp
		# src/platforms/common/imgui/imgui_tables.cpp
		# src/platforms/common/imgui/imgui_widgets.cpp
		# src/platforms/common/imgui/backends/imgui_citro3d.cpp
		# src/platforms/common/imgui/backends/imgui_ctru.cpp
		src/platforms/3ds/main.cpp
	)

	# Build package
	# ************
	ctr_generate_smdh(${PROJECT_NAME}.smdh
		NAME "${PROJECT_NAME} v${PROJECT_VERSION}"
		DESCRIPTION "I hope she made lotsa spaghetti!!!"
		AUTHOR "${APP_AUTHOR}"
		ICON res/3ds/icon.png
	)

	ctr_create_3dsx(${PROJECT_NAME}
		SMDH ${PROJECT_NAME}.smdh
	)
# -------------------------------------------
endif()


if (NINTENDO_WII OR NINTENDO_GAMECUBE)
# -------------------------------------------
	# Libraries
	# ************
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} -laesnd -lfat)
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_AUDIO=1) # SDL2

	# Source files/directories
	# ************
	target_include_directories(${PROJECT_NAME} PRIVATE src/platforms/ogc)
	target_sources(${PROJECT_NAME} PRIVATE
		src/platforms/ogc/main.cpp
	)

	# Build package
	# ************
	ogc_create_dol(${PROJECT_NAME})

	if (NINTENDO_WII)
		string(TIMESTAMP PROJECT_BUILDDATE "%Y%m%d000000")
		configure_file(res/wii/meta.xml.in res/wii/meta.xml @ONLY)
	endif()
# -------------------------------------------
endif()


if (NINTENDO_WIIU)
# -------------------------------------------
	# Libraries
	# ************
	find_package(SDL2 REQUIRED)
	find_package(SDL2_ttf REQUIRED)
    find_package(Freetype REQUIRED)
	include_directories(${DEVKITPRO}/portlibs/wiiu/include ${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} -lSDL2_ttf -lfreetype -lpng -lz -lm -lharfbuzz -lbrotlidec -lbrotlicommon -lbz2 -lwut)
	target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_AUDIO=1) # SDL2

	# Source files/directories
	# ************
	target_include_directories(${PROJECT_NAME} PRIVATE
		src/platforms/common/imgui
		src/platforms/wiiu
	)
	target_sources(${PROJECT_NAME} PRIVATE
		# Dear ImGUI library
		src/platforms/common/imgui/imgui.cpp
		src/platforms/common/imgui/imgui_demo.cpp
		src/platforms/common/imgui/imgui_draw.cpp
		src/platforms/common/imgui/imgui_tables.cpp
		src/platforms/common/imgui/imgui_widgets.cpp
		src/platforms/common/imgui/backends/imgui_impl_sdl2.cpp
		src/platforms/common/imgui/backends/imgui_impl_sdlrenderer2.cpp

		src/frontends/imgui/main.cpp
	)

	# Build package
	# ************
	wut_create_rpx(${PROJECT_NAME})
	wut_create_wuhb(${PROJECT_NAME}
		NAME "${PROJECT_NAME} v${PROJECT_VERSION}"
		CONTENT res/wiiu/content
		AUTHOR "${APP_AUTHOR}"

		ICON res/wiiu/iconTex.png
		TVSPLASH res/wiiu/bootTvTex.png
		DRCSPLASH res/wiiu/bootDrcTex.png
	)
# -------------------------------------------
endif()


# if (NINTENDO_SWITCH)
# -------------------------------------------
	## Source files/directories
	## ************
	# target_sources(${PROJECT_NAME} PRIVATE
		# src/platforms/nx/main.cpp
	# )

	## Build package
	## ************
	# nx_generate_nacp(${PROJECT_NAME}.nacp
		# NAME "${PROJECT_NAME}"
		# AUTHOR "${APP_AUTHOR}"
		# VERSION ${PROJECT_VERSION}
	# )

	# nx_create_nro(${PROJECT_NAME}
		# NACP ${PROJECT_NAME}.nacp
		# ICON res/nx/icon.png
		# ROMFS res/3ds/romfs
		# OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.nro
	# )
# -------------------------------------------
# endif()