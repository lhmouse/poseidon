// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_PRECOMPILED_
#define POSEIDON_PRECOMPILED_

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

#include <iterator>
#include <memory>
#include <utility>
#include <exception>
#include <typeinfo>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <string>
#include <bitset>
#include <chrono>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

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
#ifdef __SSE2__
#include <x86intrin.h>
#include <immintrin.h>
#endif  // __SSE2__

#endif
