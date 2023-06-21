// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "char256.hpp"
#include "../utils.hpp"
namespace poseidon {

static_assert(char256().c_str()[0] == 0);
static_assert(char256("abc").c_str()[0] == 'a');
static_assert(char256("abc").c_str()[1] == 'b');
static_assert(char256("abc").c_str()[2] == 'c');
static_assert(char256("abc").c_str()[3] == 0);

}  // namespace poseidon
