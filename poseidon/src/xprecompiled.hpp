// This file is part of Asteria.
// Copyright (C) 2024-2026 LH_Mouse. All wrongs reserved.

#define _IOS_BASE_H  1
#define _STREAM_ITERATOR_H  1
#define _STREAMBUF_ITERATOR_H  1
#define _GLIBCXX_ISTREAM  1
#define _GLIBCXX_OSTREAM  1
#define _GLIBCXX_IOSTREAM  1
#define _GLIBCXX_CHRONO_IO_H  1
#define _GLIBCXX_SSTREAM  1
#include <iosfwd>

#include <iterator>
#include <utility>
#include <exception>
#include <typeinfo>
#include <type_traits>

#include <climits>
#include <cmath>
#include <cfenv>
#include <cfloat>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <wchar.h>

#include <x86intrin.h>
#include <emmintrin.h>
#include <immintrin.h>
