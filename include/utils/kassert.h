#pragma once

#include <lib/stdtypes.h>

/*
 * Api
 */
extern void kassert(const char * file, u_int line, bool expr);
extern void kassert_kernel_stack(const char * file, u_int line);
