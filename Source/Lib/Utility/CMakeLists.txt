# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# Utility Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/API/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/System/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/SRM/)


if(${COMPILE_AS_CPP} EQUAL 1) 

	file(GLOB SourceFiles
	    "*.c"
	)

	set_source_files_properties( 
		${SourceFiles} 
		PROPERTIES LANGUAGE CXX )
endif(${COMPILE_AS_CPP} EQUAL 1)

# Utility Source Files
add_library(Utility
    EbUtility.c
)



