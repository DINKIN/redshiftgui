/**\file		common.h
 * \author		Mao Yu
 * \brief		Common definitions.
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef _WIN32
# define WINVER  0x0500
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "logger.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

// Logging macros
#if 0
#define LOG(LVL,...) log_log(LVL,__FILE__,__FUNCTION__ ,__LINE__,__VA_ARGS__)
#else
#define LOG(LVL,...) log_log(LVL,NULL,NULL,0,__VA_ARGS__)
#endif
#define LOGERR		1
#define LOGWARN		2
#define LOGINFO		3
#define LOGVERBOSE	4

// Function return codes
#define RET_FUN_FAILED	0
#define RET_FUN_SUCCESS	1

// Configuration file strings
#define LONGEST_PATH 2048
#ifndef _WIN32
 #define PATH_SEP '/'
 #define RSG_RCFILE ".redshiftgrc"
#else
 #define PATH_SEP '\\'
 #define RSG_RCFILE "redshiftg.ini"
#endif

// Math macros
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

#endif//__COMMON_H__
