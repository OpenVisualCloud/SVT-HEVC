# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# Source (ROOT) Directory CMakeLists.txt
cmake_minimum_required (VERSION 2.6)   
cmake_policy(SET CMP0045 OLD)
############################################
# MACROS
############################################

# This will add arguments not found in ${parameter} to the end.  It
# does not attempt to remove duplicate arguments already existing in
# ${parameter}.

MACRO(FORCE_ADD_FLAGS parameter)
  # Create a separated list of the arguments to loop over
  SET(p_list ${${parameter}})
  SEPARATE_ARGUMENTS(p_list)
  # Make a copy of the current arguments in ${parameter}
  SET(new_parameter ${${parameter}})
  # Now loop over each required argument and see if it is in our
  # current list of arguments.
  FOREACH(required_arg ${ARGN})
    # This helps when we get arguments to the function that are
    # grouped as a string:
    #
    # ["-O3 -g"]  instead of [-O3 -g]
    SET(TMP ${required_arg}) #elsewise the Seperate command doesn't work)
    SEPARATE_ARGUMENTS(TMP)
    FOREACH(option ${TMP})
      # Look for the required argument in our list of existing arguments
      SET(found FALSE)
      FOREACH(p_arg ${p_list})
	IF (${p_arg} STREQUAL ${option})
	  SET(found TRUE)
	ENDIF (${p_arg} STREQUAL ${option})
      ENDFOREACH(p_arg)
      IF(NOT found)
	# The required argument wasn't found, so we need to add it in.
	SET(new_parameter "${new_parameter} ${option}")
      ENDIF(NOT found)
    ENDFOREACH(option ${TMP})
  ENDFOREACH(required_arg ${ARGN})
  SET(${parameter} ${new_parameter} CACHE STRING "" FORCE)
ENDMACRO(FORCE_ADD_FLAGS)

############################################
# BUILD
############################################



if(${COMPILE_AS_CPP} EQUAL 1) 
	#message(Compile as CPP)
	project (%CMAKE_PROJECT_NAME CXX NONE)
else(${COMPILE_AS_CPP} EQUAL 1)
	#message(Compile as C)
	project (%CMAKE_PROJECT_NAME C NONE)
endif(${COMPILE_AS_CPP} EQUAL 1)



#*** SETUP C FLAGS***

SET(CMAKE_EXE_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")
SET(CMAKE_SHARED_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")
SET(CMAKE_CXX_FLAGS "-fPIE -fPIC -fstack-protector -flto -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Wall -Wextra -msse4.1 -fvisibility=hidden ")
SET(CMAKE_C_FLAGS "-fPIE -fPIC -fstack-protector -flto -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Wall -Wextra -msse4.1 -fvisibility=hidden")
      



#*** SETUP ASM FLAGS***

# Linux

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
	SET(CMAKE_ASM_NASM_FLAGS_INIT "-f elf64 -D UNIX64 -g dwarf2")
else()
	SET(CMAKE_ASM_NASM_FLAGS_INIT "-f elf64 -D UNIX64")
endif()


# Enable NASM (same syntax as YASM)
enable_language(ASM_NASM)

# Add Subdirectories
add_subdirectory (Lib)
add_subdirectory (App)
add_subdirectory (SimpleApp)








