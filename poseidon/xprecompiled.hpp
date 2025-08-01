// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_XPRECOMPILED_
#define POSEIDON_XPRECOMPILED_

// Prevent use of standard streams.
#define _IOS_BASE_H  1
#define _STREAM_ITERATOR_H  1
#define _STREAMBUF_ITERATOR_H  1
#define _GLIBCXX_ISTREAM  1
#define _GLIBCXX_OSTREAM  1
#define _GLIBCXX_IOSTREAM  1

#include "version.h"

#include <rocket/cow_string.hpp>
#include <rocket/cow_vector.hpp>
#include <rocket/cow_hashmap.hpp>
#include <rocket/static_vector.hpp>
#include <rocket/prehashed_string.hpp>
#include <rocket/unique_handle.hpp>
#include <rocket/unique_posix_file.hpp>
#include <rocket/unique_posix_dir.hpp>
#include <rocket/unique_posix_fd.hpp>
#include <rocket/variant.hpp>
#include <rocket/optional.hpp>
#include <rocket/array.hpp>
#include <rocket/reference_wrapper.hpp>
#include <rocket/tinyfmt.hpp>
#include <rocket/tinyfmt_str.hpp>
#include <rocket/tinyfmt_file.hpp>
#include <rocket/ascii_numget.hpp>
#include <rocket/ascii_numput.hpp>
#include <rocket/atomic.hpp>
#include <rocket/mutex.hpp>
#include <rocket/recursive_mutex.hpp>
#include <rocket/condition_variable.hpp>
#include <rocket/shared_function.hpp>
#include <rocket/static_char_buffer.hpp>
#include <asteria/value.hpp>
#include <asteria/utils.hpp>
#include <taxon.hpp>

#include <iterator>
#include <memory>
#include <utility>
#include <exception>
#include <typeinfo>
#include <type_traits>
#include <chrono>
#include <array>
#include <string>
#include <vector>

#include <cstdio>
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
#include <cxxabi.h>
#include <x86intrin.h>
#include <nmmintrin.h>
#include <immintrin.h>

#endif
