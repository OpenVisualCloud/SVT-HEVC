/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbBuild_h
#define EbBuild_h

#ifdef _MSC_VER

// Windows Compiler
#pragma warning( disable : 4127 )
#pragma warning( disable : 4201 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 4456 )  
#pragma warning( disable : 4457 )
#pragma warning( disable : 4459 )
#pragma warning( disable : 4334 ) 

#elif __INTEL_COMPILER

#else
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif 
#ifdef __cplusplus
#define EB_EXTERN extern "C"
#else
#define EB_EXTERN
#endif // __cplusplus


#endif // _MSC_VER


