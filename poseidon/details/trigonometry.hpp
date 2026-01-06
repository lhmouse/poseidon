// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_DETAILS_TRIGONOMETRY_
#define POSEIDON_DETAILS_TRIGONOMETRY_

#include "../fwd.hpp"
namespace poseidon {

struct trigonometric
  {
    float sin;
    float cos;
    float tan;
    float reserved;
  };

const trigonometric&
do_trig_degrees(int theta)
  noexcept __attribute__((__const__, __leaf__));

int
do_arctan_degrees(float y, float x)
  noexcept __attribute__((__const__, __leaf__));

}  // namespace poseidon
#endif
