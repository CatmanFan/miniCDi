cmake_minimum_required(VERSION 3.13...4.3)
project(miniCDi VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compilation config
# -------------------------------------------
add_executable(${PROJECT_NAME})
dkp_target_generate_symbol_list(${PROJECT_NAME})
target_compile_options(${PROJECT_NAME} PRIVATE -g -O2 -Wall)

option(MINICDI_ENABLE_DEBUG "Output debug information instead of VDSR." ON)
option(MINICDI_USE_MUSASHI "Use Musashi instead of Rocket68 for M68K core." ON)

if (MINICDI_ENABLE_DEBUG)
  MESSAGE(STATUS "MINICDI_ENABLE_DEBUG")
  target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_DEBUG)
endif()
if (MINICDI_USE_MUSASHI)
  MESSAGE(STATUS "MINICDI_USE_MUSASHI")
  target_compile_definitions(${PROJECT_NAME} PRIVATE MINICDI_MUSASHI)
endif()

set (APP_AUTHOR "Mr. Lechkar / CatmanFan")
set (APP_DESCRIPTION "I hope she made lotsa spaghetti!!!")

# Core source files
# -------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_sources(${PROJECT_NAME} PRIVATE
    src/cdi/Players.cpp
)

if (MINICDI_USE_MUSASHI)
target_include_directories(${PROJECT_NAME} PRIVATE src/cdi/Musashi)
target_sources(${PROJECT_NAME} PRIVATE
    src/cdi/Musashi/m68k.h
    src/cdi/Musashi/m68kconf.h
    src/cdi/Musashi/m68kcpu.c
    src/cdi/Musashi/m68kcpu.h
    src/cdi/Musashi/m68kdasm.c
    src/cdi/Musashi/m68kfpu.c
    src/cdi/Musashi/m68kmmu.h
    src/cdi/Musashi/m68kops.c
    src/cdi/Musashi/m68kops.h
	src/cdi/Musashi/softfloat/softfloat.c
	src/cdi/Musashi/softfloat/softfloat.h
	src/cdi/Musashi/softfloat/softfloat-macros
)
else()
target_sources(${PROJECT_NAME} PRIVATE
    src/cdi/rocket68/raw/disasm.c
    src/cdi/rocket68/raw/loader.c
    src/cdi/rocket68/raw/m68k.c
    src/cdi/rocket68/raw/ops_arith.c
    src/cdi/rocket68/raw/ops_bit.c
    src/cdi/rocket68/raw/ops_control.c
    src/cdi/rocket68/raw/ops_logic.c
    src/cdi/rocket68/raw/ops_move.c
)
endif()

# Console specific
# -------------------------------------------
if (NINTENDO_3DS)
	# Libraries
    target_link_libraries(${PROJECT_NAME} -lcitro2d -lcitro3d)
	add_compile_options(${PROJECT_NAME} -mfloat-abi=softfp)

    target_sources(${PROJECT_NAME} PRIVATE src/platforms/3ds/main.cpp)

    ctr_generate_smdh(${PROJECT_NAME}.smdh
    	NAME "${PROJECT_NAME} v${PROJECT_VERSION}"
    	DESCRIPTION "${APP_DESCRIPTION}"
    	AUTHOR "${APP_AUTHOR}"
    	ICON res/3ds/icon.png
    )

    ctr_create_3dsx(${PROJECT_NAME}
        SMDH ${PROJECT_NAME}.smdh
    	ROMFS res/3ds/romfs
    )
endif()

if (NINTENDO_WII OR NINTENDO_GAMECUBE)
	# Libraries
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} -laesnd -lfat)

    target_sources(${PROJECT_NAME} PRIVATE src/platforms/wii/main.cpp)

    ogc_create_dol(${PROJECT_NAME})
endif()

# if (NINTENDO_WIIU)
    # target_sources(${PROJECT_NAME} PRIVATE src/platforms/wiiu/main.cpp)

    # wut_create_rpx(${PROJECT_NAME})

	# wut_create_wuhb(${PROJECT_NAME}
		# NAME "${PROJECT_NAME} v${PROJECT_VERSION}"
    	# AUTHOR "${APP_AUTHOR}"

    	#ICON res/wiiu/iconTex.png
        #TVSPLASH res/wiiu/bootTvTex.png
        #DRCSPLASH res/wiiu/bootDrcTex.png

        # CONTENT res/wiiu/content
	# )
# endif()

# if (NINTENDO_SWITCH)
    # target_sources(${PROJECT_NAME} PRIVATE src/platforms/nx/main.cpp)

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
# endif()