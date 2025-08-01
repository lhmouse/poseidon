// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/geometry.hpp"
using namespace ::poseidon;

static
bool
approximate(float x, double y)
  {
    return ::lroundf(x * 100) == ::lround(y * 100);
  }

int
main()
  {
    POSEIDON_TEST_CHECK(point2().x == 0);
    POSEIDON_TEST_CHECK(point2().y == 0);
    POSEIDON_TEST_CHECK(point2(1, 2).x == 1);
    POSEIDON_TEST_CHECK(point2(1, 2).y == 2);
    POSEIDON_TEST_CHECK(point2(1, 2) == point2(1, 2));
    POSEIDON_TEST_CHECK(point2(-3, -4).x == -3);
    POSEIDON_TEST_CHECK(point2(-3, -4).y == -4);

    POSEIDON_TEST_CHECK(vector2().x == 0);
    POSEIDON_TEST_CHECK(vector2().y == 0);
    POSEIDON_TEST_CHECK(vector2(1, 2).x == 1);
    POSEIDON_TEST_CHECK(vector2(1, 2).y == 2);
    POSEIDON_TEST_CHECK(vector2(1, 2) == vector2(1, 2));
    POSEIDON_TEST_CHECK(vector2(-1, -2) == -vector2(1, 2));
    POSEIDON_TEST_CHECK(vector2(-3, -4).x == -3);
    POSEIDON_TEST_CHECK(vector2(-3, -4).y == -4);

    vector2 a(-3000,4000), b = a.unit();
    POSEIDON_TEST_CHECK(approximate(a.magnitude(), 5000));
    POSEIDON_TEST_CHECK(approximate(b.magnitude(), 1));
    POSEIDON_TEST_CHECK(approximate(b.x, -0.6));
    POSEIDON_TEST_CHECK(approximate(b.y, 0.8));

    vector2 c = a / 10;
    POSEIDON_TEST_CHECK(approximate(c.x, -300));
    POSEIDON_TEST_CHECK(approximate(c.y, 400));

    vector2 d = b * 7;
    POSEIDON_TEST_CHECK(approximate(d.x, -4.2));
    POSEIDON_TEST_CHECK(approximate(d.y, 5.6));

    vector2 e(1,-2), f(-3,5);
    POSEIDON_TEST_CHECK(e + f == vector2(-2,3));
    POSEIDON_TEST_CHECK(e - f == vector2(4,-7));
    POSEIDON_TEST_CHECK(e.dot(f) == -13);
    POSEIDON_TEST_CHECK(e.cross(f) == -1);

    vector2 g(5, 0);
    vector2 h = g;
    g = g.rotate(degrees(0));  // 0
    POSEIDON_TEST_CHECK(approximate(g.x, 5));
    POSEIDON_TEST_CHECK(approximate(g.y, 0));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(0));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(0));

    h = g;
    g = g.rotate(degrees(30));  // 30
    POSEIDON_TEST_CHECK(approximate(g.x, 4.330127));
    POSEIDON_TEST_CHECK(approximate(g.y, 2.5));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(30));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(330));

    h = g;
    g = g.rotate(degrees(15));  // 45
    POSEIDON_TEST_CHECK(approximate(g.x, 3.5355339));
    POSEIDON_TEST_CHECK(approximate(g.y, 3.5355339));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(15));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(345));

    h = g;
    g = g.rotate(degrees(45));  // 90
    POSEIDON_TEST_CHECK(approximate(g.x, 0));
    POSEIDON_TEST_CHECK(approximate(g.y, 5));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(45));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(315));

    h = g;
    g = g.rotate(degrees(30));  // 120
    POSEIDON_TEST_CHECK(approximate(g.x, -2.5));
    POSEIDON_TEST_CHECK(approximate(g.y, 4.330127));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(30));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(330));

    h = g;
    g = g.rotate(degrees(105));  // 225
    POSEIDON_TEST_CHECK(approximate(g.x, -3.5355339));
    POSEIDON_TEST_CHECK(approximate(g.y, -3.5355339));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(105));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(255));

    h = g;
    g = g.rotate(degrees(90));  // 315
    POSEIDON_TEST_CHECK(approximate(g.x, 3.5355339));
    POSEIDON_TEST_CHECK(approximate(g.y, -3.5355339));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(90));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(270));

    h = g;
    g = g.rotate(degrees(15));  // 330
    POSEIDON_TEST_CHECK(approximate(g.x, 4.330127));
    POSEIDON_TEST_CHECK(approximate(g.y, -2.5));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(15));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(345));

    h = g;
    g = g.rotate(degrees(30));  // 360
    POSEIDON_TEST_CHECK(approximate(g.x, 5));
    POSEIDON_TEST_CHECK(approximate(g.y, 0));
    POSEIDON_TEST_CHECK(angle(h, g) == degrees(30));
    POSEIDON_TEST_CHECK(angle(g, h) == degrees(330));

    POSEIDON_TEST_CHECK(angle(vector2(1, 0), vector2(0, 1)) == degrees(90));
    POSEIDON_TEST_CHECK(angle(vector2(3, 0), vector2(0, 3)) == degrees(90));

    for(int t = -1000;  t <= 1000; ++t) {
      int t0 = t % 360;
      POSEIDON_TEST_CHECK(degrees(t).reduce360().t == ((t0 < 0) ? (t0 + 360) : t0));
      POSEIDON_TEST_CHECK(degrees(t).reduce180().t == ((t0 < -179) ? (t0 + 360)
                                                       : (t0 > 180) ? (t0 - 360) : t0));
      POSEIDON_TEST_CHECK(degrees(t).abs().t == abs(t));
    }

    POSEIDON_TEST_CHECK(degrees(0).sin() == 0);
    POSEIDON_TEST_CHECK(degrees(0).cos() == 1);
    POSEIDON_TEST_CHECK(degrees(0).tan() == 0);
    POSEIDON_TEST_CHECK(vector2(1, 0).direction() == degrees(0));
    POSEIDON_TEST_CHECK(degrees(-360).sin() == 0);
    POSEIDON_TEST_CHECK(degrees(-360).cos() == 1);
    POSEIDON_TEST_CHECK(degrees(-360).tan() == 0);

    POSEIDON_TEST_CHECK(degrees(30).sin() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(30).cos() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(30).tan() == 0.577350269F);
    POSEIDON_TEST_CHECK(vector2(0.866025404F, 0.5F).direction() == degrees(30));
    POSEIDON_TEST_CHECK(degrees(-330).sin() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(-330).cos() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-330).tan() == 0.577350269F);

    POSEIDON_TEST_CHECK(degrees(45).sin() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(45).cos() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(45).tan() == 1);
    POSEIDON_TEST_CHECK(vector2(0.707106781F, 0.707106781F).direction() == degrees(45));
    POSEIDON_TEST_CHECK(degrees(-315).sin() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-315).cos() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-315).tan() == 1);

    POSEIDON_TEST_CHECK(degrees(60).sin() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(60).cos() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(60).tan() == 1.732050808F);
    POSEIDON_TEST_CHECK(vector2(0.5F, 0.866025404F).direction() == degrees(60));
    POSEIDON_TEST_CHECK(degrees(-300).sin() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-300).cos() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(-300).tan() == 1.732050808F);

    POSEIDON_TEST_CHECK(degrees(90).sin() == 1);
    POSEIDON_TEST_CHECK(degrees(90).cos() == 0);
    POSEIDON_TEST_CHECK(degrees(90).tan() > 1000000);  // infinity
    POSEIDON_TEST_CHECK(vector2(0, 1).direction() == degrees(90));
    POSEIDON_TEST_CHECK(degrees(-270).sin() == 1);
    POSEIDON_TEST_CHECK(degrees(-270).cos() == 0);
    POSEIDON_TEST_CHECK(degrees(-270).tan() > 1000000);  // infinity

    POSEIDON_TEST_CHECK(degrees(120).sin() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(120).cos() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(120).tan() == -1.732050808F);
    POSEIDON_TEST_CHECK(vector2(-0.5F, 0.866025404F).direction() == degrees(120));
    POSEIDON_TEST_CHECK(degrees(-240).sin() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-240).cos() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(-240).tan() == -1.732050808F);

    POSEIDON_TEST_CHECK(degrees(135).sin() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(135).cos() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(135).tan() == -1);
    POSEIDON_TEST_CHECK(vector2(-0.707106781F, 0.707106781F).direction() == degrees(135));
    POSEIDON_TEST_CHECK(degrees(-225).sin() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-225).cos() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-225).tan() == -1);

    POSEIDON_TEST_CHECK(degrees(150).sin() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(150).cos() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(150).tan() == -0.577350269F);
    POSEIDON_TEST_CHECK(vector2(-0.866025404F, 0.5F).direction() == degrees(150));
    POSEIDON_TEST_CHECK(degrees(-210).sin() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(-210).cos() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-210).tan() == -0.577350269F);

    POSEIDON_TEST_CHECK(degrees(180).sin() == 0);
    POSEIDON_TEST_CHECK(degrees(180).cos() == -1);
    POSEIDON_TEST_CHECK(degrees(180).tan() == 0);
    POSEIDON_TEST_CHECK(vector2(-1, 0).direction() == degrees(180));
    POSEIDON_TEST_CHECK(degrees(-180).sin() == 0);
    POSEIDON_TEST_CHECK(degrees(-180).cos() == -1);
    POSEIDON_TEST_CHECK(degrees(-180).tan() == 0);

    POSEIDON_TEST_CHECK(degrees(210).sin() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(210).cos() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(210).tan() == 0.577350269F);
    POSEIDON_TEST_CHECK(vector2(-0.866025404F, -0.5F).direction() == degrees(210));
    POSEIDON_TEST_CHECK(degrees(-150).sin() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(-150).cos() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-150).tan() == 0.577350269F);

    POSEIDON_TEST_CHECK(degrees(225).sin() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(225).cos() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(225).tan() == 1);
    POSEIDON_TEST_CHECK(vector2(-0.707106781F, -0.707106781F).direction() == degrees(225));
    POSEIDON_TEST_CHECK(degrees(-135).sin() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-135).cos() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-135).tan() == 1);

    POSEIDON_TEST_CHECK(degrees(240).sin() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(240).cos() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(240).tan() == 1.732050808F);
    POSEIDON_TEST_CHECK(vector2(-0.5F, -0.866025404F).direction() == degrees(240));
    POSEIDON_TEST_CHECK(degrees(-120).sin() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-120).cos() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(-120).tan() == 1.732050808F);

    POSEIDON_TEST_CHECK(degrees(270).sin() == -1);
    POSEIDON_TEST_CHECK(degrees(270).cos() == 0);
    POSEIDON_TEST_CHECK(degrees(270).tan() > 1000000);  // infinity
    POSEIDON_TEST_CHECK(vector2(0, -1).direction() == degrees(270));
    POSEIDON_TEST_CHECK(degrees(-90).sin() == -1);
    POSEIDON_TEST_CHECK(degrees(-90).cos() == 0);
    POSEIDON_TEST_CHECK(degrees(-90).tan() > 1000000);  // infinity

    POSEIDON_TEST_CHECK(degrees(300).sin() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(300).cos() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(300).tan() == -1.732050808F);
    POSEIDON_TEST_CHECK(vector2(0.5F, -0.866025404F).direction() == degrees(300));
    POSEIDON_TEST_CHECK(degrees(-60).sin() == -0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-60).cos() == 0.5F);
    POSEIDON_TEST_CHECK(degrees(-60).tan() == -1.732050808F);

    POSEIDON_TEST_CHECK(degrees(315).sin() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(315).cos() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(315).tan() == -1);
    POSEIDON_TEST_CHECK(vector2(0.707106781F, -0.707106781F).direction() == degrees(315));
    POSEIDON_TEST_CHECK(degrees(-45).sin() == -0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-45).cos() == 0.707106781F);
    POSEIDON_TEST_CHECK(degrees(-45).tan() == -1);

    POSEIDON_TEST_CHECK(degrees(330).sin() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(330).cos() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(330).tan() == -0.577350269F);
    POSEIDON_TEST_CHECK(vector2(0.866025404F, -0.5F).direction() == degrees(330));
    POSEIDON_TEST_CHECK(degrees(-30).sin() == -0.5F);
    POSEIDON_TEST_CHECK(degrees(-30).cos() == 0.866025404F);
    POSEIDON_TEST_CHECK(degrees(-30).tan() == -0.577350269F);
  }
