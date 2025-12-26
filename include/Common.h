#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)

#include <iostream>
#include <cassert>

#ifndef ASSERT
    #define ASSERT(x) assert(x)
#endif

#else

#define ASSERT(x)                   do {} while(0)

#endif