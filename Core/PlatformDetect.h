/**
* @file PlatformDetect.h
* @brief The Specification for Platform Detection
* @authors: Frank Reiser
* @date Created on Jan 23, 2021
*/

#ifndef PLATFORMDETECT_H_
#define PLATFORMDETECT_H_

// We are primarily targeting a modern (>4.8.5) GNU C++. Windows under VS2019 for C++ ports exist
#ifdef __GNUC__

#if !defined( __cplusplus ) || ( __cplusplus < 201103L )
#error "This file requires a C++11 compilation!"
#endif

#if ( __GNUC__ < 4 )
#define UNSUPPORTED_GCC_VERSION
#endif
#if ( __GNUC__ == 4 ) && ( __GNUC_MINOR__ < 8 )
#define UNSUPPORTED_GCC_VERSION
#endif
#if ( __GNUC__ == 4 ) && ( __GNUC_MINOR__ == 8 ) && ( __GNUC_PATCHLEVEL__ < 5 )
#define UNSUPPORTED_GCC_VERSION
#endif


#ifdef UNSUPPORTED_GCC_VERSION
#error "This file requires GCC Version 4.8.5 or higher in order to compile."
#else
#define REISER_RT_GCC
#endif

// Put other platform detections here using #elif's such as Windows.
// I forget what the macros are (_WIN32 _WIN64, single/double underscore??)

// Unknown compiler
#else
// Punting for now and not calling this an error. Let the user determine what to do.
//#error "Unsupported Compiler"
#endif


#endif /* PLATFORMDETECT_H_ */
