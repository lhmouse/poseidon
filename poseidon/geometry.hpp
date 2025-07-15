// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_GEOMETRY_
#define POSEIDON_GEOMETRY_

#include "fwd.hpp"
#include "details/trigonometry.hpp"
#include <math.h>
#include <xmmintrin.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic error "-Wdouble-promotion"
namespace poseidon {

struct degrees
  {
    int t;

    constexpr
    degrees()
      noexcept : t(0)  { }

    explicit constexpr
    degrees(int n1)
      noexcept : t(n1)  { }

    constexpr
    bool
    operator==(degrees rhs)
      const noexcept { return t == rhs.t;  }

    constexpr
    bool
    operator!=(degrees rhs)
      const noexcept { return t != rhs.t;  }

    constexpr
    float
    sin()
      const noexcept
      {
        if(ROCKET_CONSTANT_P(::sinf(t * 0.0174532925F)))
          return ::sinf(t * 0.0174532925F);
        else
          return noadl::do_trig_degrees(t).sin;
      }

    constexpr
    float
    cos()
      const noexcept
      {
        if(ROCKET_CONSTANT_P(::cosf(t * 0.0174532925F)))
          return ::cosf(t * 0.0174532925F);
        else
          return noadl::do_trig_degrees(t).cos;
      }

    constexpr
    float
    tan()
      const noexcept
      {
        if(ROCKET_CONSTANT_P(::tanf(t * 0.0174532925F)))
          return ::tanf(t * 0.0174532925F);
        else
          return noadl::do_trig_degrees(t).tan;
      }
  };

inline
void
swap(degrees& lhs, degrees& rhs)
  noexcept
  {
    ::std::swap(lhs.t, rhs.t);
  }

struct vector2
  {
    float x, y;

    constexpr
    vector2()
      noexcept : x(0), y(0)  { }

    constexpr
    vector2(float x1, float y1)
      noexcept : x(x1), y(y1)  { }

    constexpr
    vector2(float rho, degrees theta)
      noexcept : x(rho * theta.cos()), y(rho * theta.sin())  { }

    constexpr
    bool
    operator==(vector2 rhs)
      const noexcept { return (x == rhs.x) && (y == rhs.y);  }

    constexpr
    bool
    operator!=(vector2 rhs)
      const noexcept { return (x != rhs.x) || (y != rhs.y);  }

    constexpr
    float
    dot(vector2 rhs)
      const noexcept { return x * rhs.x + y * rhs.y;  }

    constexpr
    float
    cross(vector2 rhs)
      const noexcept { return x * rhs.y - y * rhs.x;  }

    constexpr
    float
    magnitude()
      const noexcept
      {
        if(ROCKET_CONSTANT_P(::hypotf(x, y)))
          return ::hypotf(x, y);
        else {
          __m128 ps = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(this)));
          __m128 r2 = _mm_hadd_ps(_mm_mul_ps(ps, ps), _mm_setzero_ps());
          return _mm_cvtss_f32(_mm_sqrt_ss(r2));
        }
      }

    constexpr
    degrees
    direction()
      const noexcept
      {
        if(ROCKET_CONSTANT_P(::atan2f(y, x) * 57.2957795F))
          return degrees(::atan2f(y, x) * 57.2957795F);
        else
          return degrees(noadl::do_arctan_degrees(y, x));
      }

    constexpr
    vector2
    unit()
      const noexcept
      {
        if((y == 0) || (y != y))
          return vector2(signbit(x) ? -1 : 1, 0);
        else if((x == 0) || (x != x))
          return vector2(0, signbit(y) ? -1 : 1);

        if(ROCKET_CONSTANT_P(::hypotf(x, y)))
          return vector2(x / ::hypotf(x, y), y / ::hypotf(x, y));
        else {
          __m128 ps = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(this)));
          __m128 r2 = _mm_hadd_ps(_mm_mul_ps(ps, ps), _mm_setzero_ps());
          ps = _mm_mul_ps(ps, _mm_rsqrt_ps(_mm_unpacklo_ps(r2, r2)));
          alignas(16) vector2 rv[2];
          _mm_store_ps(reinterpret_cast<float*>(rv), ps);
          return rv[0];
        }
      }

    constexpr
    vector2
    rotate(degrees theta)
      const noexcept
      {
        float cos_th = theta.cos();
        float sin_th = theta.sin();
        return vector2(x * cos_th - y * sin_th, x * sin_th + y * cos_th);
      }
  };

inline
void
swap(vector2& lhs, vector2& rhs)
  noexcept
  {
    ::std::swap(lhs.x, rhs.x);
    ::std::swap(lhs.y, rhs.y);
  }

struct point2
  {
    float x, y;

    constexpr
    point2()
      noexcept : x(0), y(0)  { }

    constexpr
    point2(float x1, float y1)
      noexcept : x(x1), y(y1)  { }

    constexpr
    bool
    operator==(point2 rhs)
      const noexcept { return (x == rhs.x) && (y == rhs.y);  }

    constexpr
    bool
    operator!=(point2 rhs)
      const noexcept { return (x != rhs.x) || (y != rhs.y);  }
  };

inline
void
swap(point2& lhs, point2& rhs)
  noexcept
  {
    ::std::swap(lhs.x, rhs.x);
    ::std::swap(lhs.y, rhs.y);
  }

constexpr
float
sin(degrees d)
  noexcept { return d.sin();  }

constexpr
float
cos(degrees d)
  noexcept { return d.cos();  }

constexpr
float
tan(degrees d)
  noexcept { return d.tan();  }

constexpr
float
dot(vector2 lhs, vector2 rhs)
  noexcept { return lhs.dot(rhs);  }

constexpr
float
cross(vector2 lhs, vector2 rhs)
  noexcept { return lhs.cross(rhs);  }

constexpr
float
abs(vector2 v)
  noexcept { return v.magnitude();  }

constexpr
float
magnitude(vector2 v)
  noexcept { return v.magnitude();  }

constexpr
degrees
direction(vector2 v)
  noexcept { return v.direction();  }

constexpr
vector2
unit(vector2 v)
  noexcept { return v.unit();  }

constexpr
vector2
rotate(vector2 v, degrees theta)
  noexcept { return v.rotate(theta);  }

constexpr
degrees
operator+(degrees rhs)
  noexcept { return rhs;  }

constexpr
vector2
operator+(vector2 rhs)
  noexcept { return rhs;  }

constexpr
degrees
operator-(degrees rhs)
  noexcept { return degrees(- rhs.t);  }

constexpr
vector2
operator-(vector2 rhs)
  noexcept { return vector2(- rhs.x, - rhs.y);  }

constexpr
degrees
operator+(degrees lhs, degrees rhs)
  noexcept { return degrees(lhs.t + rhs.t);  }

constexpr
vector2
operator+(vector2 lhs, vector2 rhs)
  noexcept { return vector2(lhs.x + rhs.x, lhs.y + rhs.y);  }

constexpr
point2
operator+(vector2 lhs, point2 rhs)
  noexcept { return point2(lhs.x + rhs.x, lhs.y + rhs.y);  }

constexpr
point2
operator+(point2 lhs, vector2 rhs)
  noexcept { return point2(lhs.x + rhs.x, lhs.y + rhs.y);  }

constexpr
degrees
operator-(degrees lhs, degrees rhs)
  noexcept { return degrees(lhs.t - rhs.t);  }

constexpr
vector2
operator-(vector2 lhs, vector2 rhs)
  noexcept { return vector2(lhs.x - rhs.x, lhs.y - rhs.y);  }

constexpr
point2
operator-(point2 lhs, vector2 rhs)
  noexcept { return point2(lhs.x - rhs.x, lhs.y - rhs.y);  }

constexpr
vector2
operator-(point2 lhs, point2 rhs)
  noexcept { return vector2(lhs.x - rhs.x, lhs.y - rhs.y);  }

constexpr
degrees
operator*(degrees lhs, int rhs)
  noexcept { return degrees(lhs.t * rhs);  }

constexpr
degrees
operator*(int lhs, degrees rhs)
  noexcept { return degrees(lhs * rhs.t);  }

constexpr
degrees
operator*(degrees lhs, float rhs)
  noexcept { return degrees(::lrintf(lhs.t * rhs));  }

constexpr
degrees
operator*(float lhs, degrees rhs)
  noexcept { return degrees(::lrintf(lhs * rhs.t));  }

constexpr
vector2
operator*(vector2 lhs, int rhs)
  noexcept { return vector2(lhs.x * rhs, lhs.y * rhs);  }

constexpr
vector2
operator*(int lhs, vector2 rhs)
  noexcept { return vector2(lhs * rhs.x, lhs * rhs.y);  }

constexpr
vector2
operator*(vector2 lhs, float rhs)
  noexcept { return vector2(lhs.x * rhs, lhs.y * rhs);  }

constexpr
vector2
operator*(float lhs, vector2 rhs)
  noexcept { return vector2(lhs * rhs.x, lhs * rhs.y);  }

constexpr
degrees
operator/(degrees lhs, int rhs)
  noexcept { return degrees(lhs.t / rhs);  }

constexpr
degrees
operator/(degrees lhs, float rhs)
  noexcept { return degrees(lhs.t / rhs);  }

constexpr
vector2
operator/(vector2 lhs, int rhs)
  noexcept { return vector2(lhs.x / rhs, lhs.y / rhs);  }

constexpr
vector2
operator/(vector2 lhs, float rhs)
  noexcept { return vector2(lhs.x / rhs, lhs.y / rhs);  }

constexpr
degrees&
operator+=(degrees& lhs, degrees rhs)
  noexcept { return lhs = lhs + rhs;  }

constexpr
vector2&
operator+=(vector2& lhs, vector2 rhs)
  noexcept { return lhs = lhs + rhs;  }

constexpr
point2&
operator+=(point2& lhs, vector2 rhs)
  noexcept { return lhs = lhs + rhs;  }

constexpr
degrees&
operator-=(degrees& lhs, degrees rhs)
  noexcept { return lhs = lhs - rhs;  }

constexpr
vector2&
operator-=(vector2& lhs, vector2 rhs)
  noexcept { return lhs = lhs - rhs;  }

constexpr
point2&
operator-=(point2& lhs, vector2 rhs)
  noexcept { return lhs = lhs - rhs;  }

constexpr
degrees&
operator*=(degrees& lhs, int rhs)
  noexcept { return lhs = lhs * rhs;  }

constexpr
degrees&
operator*=(degrees& lhs, float rhs)
  noexcept { return lhs = lhs * rhs;  }

constexpr
vector2&
operator*=(vector2& lhs, int rhs)
  noexcept { return lhs = lhs * rhs;  }

constexpr
vector2&
operator*=(vector2& lhs, float rhs)
  noexcept { return lhs = lhs * rhs;  }

constexpr
degrees&
operator/=(degrees& lhs, int rhs)
  noexcept { return lhs = lhs / rhs;  }

constexpr
degrees&
operator/=(degrees& lhs, float rhs)
  noexcept { return lhs = lhs / rhs;  }

constexpr
vector2&
operator/=(vector2& lhs, int rhs)
  noexcept { return lhs = lhs / rhs;  }

constexpr
vector2&
operator/=(vector2& lhs, float rhs)
  noexcept { return lhs = lhs / rhs;  }

}  // namespace poseidon
#pragma GCC diagnostic pop
#endif
