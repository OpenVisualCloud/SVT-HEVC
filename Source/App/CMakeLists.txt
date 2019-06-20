# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# APP Directory CMakeLists.txt
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

# Include Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

# App Source Files
add_executable (SvtHevcEncApp
    EbAppConfig.c
    EbAppContext.c
    EbAppFifo.c
    EbAppMain.c
    EbAppProcessCmd.c
    EbTime.c
    ../API/EbApi.h
    EbAppConfig.h
    EbAppContext.h
    ../API/EbErrorCodes.h
    ../API/EbTime.h
    EbAppInputy4m.c
)

#********** SET COMPILE FLAGS************

if (UNIX)

	# Link the Encoder App
	target_link_libraries (SvtHevcEncApp
							SvtHevcEnc
							pthread
							rt)
endif()

if (MSVC OR MSYS OR MINGW OR WIN32)
	
	# Link the Encoder App
	target_link_libraries (SvtHevcEncApp
                           SvtHevcEnc)
endif()

if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR bin)
endif()

install(TARGETS SvtHevcEncApp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})