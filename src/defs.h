/*============================================================================
  
  defs.h

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

#ifdef __cplusplus
#define BEGIN_DECLS exetern "C" { 
#define END_DECLS }
#else
#define BEGIN_DECLS 
#define END_DECLS 
#endif

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef HIGH
#define HIGH 1
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef LOW 
#define LOW 0
#endif

#ifndef FALSE
#define FALSE 0
#endif


