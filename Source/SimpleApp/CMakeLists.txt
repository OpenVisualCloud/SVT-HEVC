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
add_executable (SvtHevcEncSimpleApp 
    ../API/EbApi.h
	../API/EbErrorCodes.h
    EbSimpleAppContext.h
    ../API/EbTime.h
    EbSimpleAppContext.c
    EbSimpleAppMain.c
)

#********** SET COMPILE FLAGS************

if (UNIX)

	# Configure the RPATH to allow relative runtime linking	
	INSTALL(TARGETS SvtHevcEncSimpleApp RUNTIME DESTINATION ".:\$ORIGIN/../lib:\$ORIGIN/")

	# Link the Encoder App
	target_link_libraries (SvtHevcEncSimpleApp
		SvtHevcEnc
		pthread rt
		m)
endif()

if (MSVC OR MSYS OR MINGW OR WIN32)
	
	# Link the Encoder App
	target_link_libraries (SvtHevcEncSimpleApp
                           SvtHevcEnc)
endif()




