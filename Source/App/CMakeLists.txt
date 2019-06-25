#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# APP Directory CMakeLists.txt

# Include Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

# App Source Files
add_executable(SvtHevcEncApp
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
    EbAppInputy4m.c)

#********** SET COMPILE FLAGS************

# Link the Encoder App
target_link_libraries(SvtHevcEncApp
    SvtHevcEnc)

if(UNIX)
	target_link_libraries(SvtHevcEncApp
        pthread)
    if(NOT APPLE)
        target_link_libraries(SvtHevcEncApp
            rt)
    endif()
endif()

install(TARGETS SvtHevcEncApp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
