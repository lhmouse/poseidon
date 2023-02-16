// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_datetime.hpp"
using namespace ::poseidon;

struct test_datetime
  {
    ::time_t ts;
    char rfc1123[32];
    char asctime[32];
    char rfc850[40];
  }
constexpr tests[] =
  {
    { 1467627924, "Mon, 04 Jul 2016 10:25:24 GMT", "Mon Jul  4 10:25:24 2016", "Monday, 04-Jul-16 10:25:24 GMT" },
    { 1467716689, "Tue, 05 Jul 2016 11:04:49 GMT", "Tue Jul  5 11:04:49 2016", "Tuesday, 05-Jul-16 11:04:49 GMT" },
    { 1467805454, "Wed, 06 Jul 2016 11:44:14 GMT", "Wed Jul  6 11:44:14 2016", "Wednesday, 06-Jul-16 11:44:14 GMT" },
    { 1467894219, "Thu, 07 Jul 2016 12:23:39 GMT", "Thu Jul  7 12:23:39 2016", "Thursday, 07-Jul-16 12:23:39 GMT" },
    { 1467982984, "Fri, 08 Jul 2016 13:03:04 GMT", "Fri Jul  8 13:03:04 2016", "Friday, 08-Jul-16 13:03:04 GMT" },
    { 1468071749, "Sat, 09 Jul 2016 13:42:29 GMT", "Sat Jul  9 13:42:29 2016", "Saturday, 09-Jul-16 13:42:29 GMT" },
    { 1468160514, "Sun, 10 Jul 2016 14:21:54 GMT", "Sun Jul 10 14:21:54 2016", "Sunday, 10-Jul-16 14:21:54 GMT" },
    { 1468249279, "Mon, 11 Jul 2016 15:01:19 GMT", "Mon Jul 11 15:01:19 2016", "Monday, 11-Jul-16 15:01:19 GMT" },
    { 1468338044, "Tue, 12 Jul 2016 15:40:44 GMT", "Tue Jul 12 15:40:44 2016", "Tuesday, 12-Jul-16 15:40:44 GMT" },
    { 1468426809, "Wed, 13 Jul 2016 16:20:09 GMT", "Wed Jul 13 16:20:09 2016", "Wednesday, 13-Jul-16 16:20:09 GMT" },
    { 1468515574, "Thu, 14 Jul 2016 16:59:34 GMT", "Thu Jul 14 16:59:34 2016", "Thursday, 14-Jul-16 16:59:34 GMT" },
    { 1468604339, "Fri, 15 Jul 2016 17:38:59 GMT", "Fri Jul 15 17:38:59 2016", "Friday, 15-Jul-16 17:38:59 GMT" },
    { 1468693104, "Sat, 16 Jul 2016 18:18:24 GMT", "Sat Jul 16 18:18:24 2016", "Saturday, 16-Jul-16 18:18:24 GMT" },
    { 1468781869, "Sun, 17 Jul 2016 18:57:49 GMT", "Sun Jul 17 18:57:49 2016", "Sunday, 17-Jul-16 18:57:49 GMT" },
    { 1468870634, "Mon, 18 Jul 2016 19:37:14 GMT", "Mon Jul 18 19:37:14 2016", "Monday, 18-Jul-16 19:37:14 GMT" },
    { 1468959399, "Tue, 19 Jul 2016 20:16:39 GMT", "Tue Jul 19 20:16:39 2016", "Tuesday, 19-Jul-16 20:16:39 GMT" },
    { 1469048164, "Wed, 20 Jul 2016 20:56:04 GMT", "Wed Jul 20 20:56:04 2016", "Wednesday, 20-Jul-16 20:56:04 GMT" },
    { 1469136929, "Thu, 21 Jul 2016 21:35:29 GMT", "Thu Jul 21 21:35:29 2016", "Thursday, 21-Jul-16 21:35:29 GMT" },
    { 1469225694, "Fri, 22 Jul 2016 22:14:54 GMT", "Fri Jul 22 22:14:54 2016", "Friday, 22-Jul-16 22:14:54 GMT" },
    { 1469314459, "Sat, 23 Jul 2016 22:54:19 GMT", "Sat Jul 23 22:54:19 2016", "Saturday, 23-Jul-16 22:54:19 GMT" },
    { 1469403224, "Sun, 24 Jul 2016 23:33:44 GMT", "Sun Jul 24 23:33:44 2016", "Sunday, 24-Jul-16 23:33:44 GMT" },
    { 1469491989, "Tue, 26 Jul 2016 00:13:09 GMT", "Tue Jul 26 00:13:09 2016", "Tuesday, 26-Jul-16 00:13:09 GMT" },
    { 1469580754, "Wed, 27 Jul 2016 00:52:34 GMT", "Wed Jul 27 00:52:34 2016", "Wednesday, 27-Jul-16 00:52:34 GMT" },
    { 1469669519, "Thu, 28 Jul 2016 01:31:59 GMT", "Thu Jul 28 01:31:59 2016", "Thursday, 28-Jul-16 01:31:59 GMT" },
    { 1469758284, "Fri, 29 Jul 2016 02:11:24 GMT", "Fri Jul 29 02:11:24 2016", "Friday, 29-Jul-16 02:11:24 GMT" },
    { 1469847049, "Sat, 30 Jul 2016 02:50:49 GMT", "Sat Jul 30 02:50:49 2016", "Saturday, 30-Jul-16 02:50:49 GMT" },
    { 1469935814, "Sun, 31 Jul 2016 03:30:14 GMT", "Sun Jul 31 03:30:14 2016", "Sunday, 31-Jul-16 03:30:14 GMT" },
    { 1470024579, "Mon, 01 Aug 2016 04:09:39 GMT", "Mon Aug  1 04:09:39 2016", "Monday, 01-Aug-16 04:09:39 GMT" },
    { 1470113344, "Tue, 02 Aug 2016 04:49:04 GMT", "Tue Aug  2 04:49:04 2016", "Tuesday, 02-Aug-16 04:49:04 GMT" },
    { 1470202109, "Wed, 03 Aug 2016 05:28:29 GMT", "Wed Aug  3 05:28:29 2016", "Wednesday, 03-Aug-16 05:28:29 GMT" },
    { 1470290874, "Thu, 04 Aug 2016 06:07:54 GMT", "Thu Aug  4 06:07:54 2016", "Thursday, 04-Aug-16 06:07:54 GMT" },
    { 1470379639, "Fri, 05 Aug 2016 06:47:19 GMT", "Fri Aug  5 06:47:19 2016", "Friday, 05-Aug-16 06:47:19 GMT" },
    { 1470468404, "Sat, 06 Aug 2016 07:26:44 GMT", "Sat Aug  6 07:26:44 2016", "Saturday, 06-Aug-16 07:26:44 GMT" },
    { 1470557169, "Sun, 07 Aug 2016 08:06:09 GMT", "Sun Aug  7 08:06:09 2016", "Sunday, 07-Aug-16 08:06:09 GMT" },
    { 1470645934, "Mon, 08 Aug 2016 08:45:34 GMT", "Mon Aug  8 08:45:34 2016", "Monday, 08-Aug-16 08:45:34 GMT" },
    { 1470734699, "Tue, 09 Aug 2016 09:24:59 GMT", "Tue Aug  9 09:24:59 2016", "Tuesday, 09-Aug-16 09:24:59 GMT" },
    { 1470823464, "Wed, 10 Aug 2016 10:04:24 GMT", "Wed Aug 10 10:04:24 2016", "Wednesday, 10-Aug-16 10:04:24 GMT" },
    { 1470912229, "Thu, 11 Aug 2016 10:43:49 GMT", "Thu Aug 11 10:43:49 2016", "Thursday, 11-Aug-16 10:43:49 GMT" },
    { 1471000994, "Fri, 12 Aug 2016 11:23:14 GMT", "Fri Aug 12 11:23:14 2016", "Friday, 12-Aug-16 11:23:14 GMT" },
    { 1471089759, "Sat, 13 Aug 2016 12:02:39 GMT", "Sat Aug 13 12:02:39 2016", "Saturday, 13-Aug-16 12:02:39 GMT" },
    { 1471178524, "Sun, 14 Aug 2016 12:42:04 GMT", "Sun Aug 14 12:42:04 2016", "Sunday, 14-Aug-16 12:42:04 GMT" },
    { 1471267289, "Mon, 15 Aug 2016 13:21:29 GMT", "Mon Aug 15 13:21:29 2016", "Monday, 15-Aug-16 13:21:29 GMT" },
    { 1471356054, "Tue, 16 Aug 2016 14:00:54 GMT", "Tue Aug 16 14:00:54 2016", "Tuesday, 16-Aug-16 14:00:54 GMT" },
    { 1471444819, "Wed, 17 Aug 2016 14:40:19 GMT", "Wed Aug 17 14:40:19 2016", "Wednesday, 17-Aug-16 14:40:19 GMT" },
    { 1471533584, "Thu, 18 Aug 2016 15:19:44 GMT", "Thu Aug 18 15:19:44 2016", "Thursday, 18-Aug-16 15:19:44 GMT" },
    { 1471622349, "Fri, 19 Aug 2016 15:59:09 GMT", "Fri Aug 19 15:59:09 2016", "Friday, 19-Aug-16 15:59:09 GMT" },
    { 1471711114, "Sat, 20 Aug 2016 16:38:34 GMT", "Sat Aug 20 16:38:34 2016", "Saturday, 20-Aug-16 16:38:34 GMT" },
    { 1471799879, "Sun, 21 Aug 2016 17:17:59 GMT", "Sun Aug 21 17:17:59 2016", "Sunday, 21-Aug-16 17:17:59 GMT" },
    { 1471888644, "Mon, 22 Aug 2016 17:57:24 GMT", "Mon Aug 22 17:57:24 2016", "Monday, 22-Aug-16 17:57:24 GMT" },
    { 1471977409, "Tue, 23 Aug 2016 18:36:49 GMT", "Tue Aug 23 18:36:49 2016", "Tuesday, 23-Aug-16 18:36:49 GMT" },
    { 1472066174, "Wed, 24 Aug 2016 19:16:14 GMT", "Wed Aug 24 19:16:14 2016", "Wednesday, 24-Aug-16 19:16:14 GMT" },
    { 1472154939, "Thu, 25 Aug 2016 19:55:39 GMT", "Thu Aug 25 19:55:39 2016", "Thursday, 25-Aug-16 19:55:39 GMT" },
    { 1472243704, "Fri, 26 Aug 2016 20:35:04 GMT", "Fri Aug 26 20:35:04 2016", "Friday, 26-Aug-16 20:35:04 GMT" },
    { 1472332469, "Sat, 27 Aug 2016 21:14:29 GMT", "Sat Aug 27 21:14:29 2016", "Saturday, 27-Aug-16 21:14:29 GMT" },
    { 1472421234, "Sun, 28 Aug 2016 21:53:54 GMT", "Sun Aug 28 21:53:54 2016", "Sunday, 28-Aug-16 21:53:54 GMT" },
    { 1472509999, "Mon, 29 Aug 2016 22:33:19 GMT", "Mon Aug 29 22:33:19 2016", "Monday, 29-Aug-16 22:33:19 GMT" },
    { 1472598764, "Tue, 30 Aug 2016 23:12:44 GMT", "Tue Aug 30 23:12:44 2016", "Tuesday, 30-Aug-16 23:12:44 GMT" },
    { 1472687529, "Wed, 31 Aug 2016 23:52:09 GMT", "Wed Aug 31 23:52:09 2016", "Wednesday, 31-Aug-16 23:52:09 GMT" },
    { 1472776294, "Fri, 02 Sep 2016 00:31:34 GMT", "Fri Sep  2 00:31:34 2016", "Friday, 02-Sep-16 00:31:34 GMT" },
    { 1472865059, "Sat, 03 Sep 2016 01:10:59 GMT", "Sat Sep  3 01:10:59 2016", "Saturday, 03-Sep-16 01:10:59 GMT" },
    { 1472953824, "Sun, 04 Sep 2016 01:50:24 GMT", "Sun Sep  4 01:50:24 2016", "Sunday, 04-Sep-16 01:50:24 GMT" },
    { 1473042589, "Mon, 05 Sep 2016 02:29:49 GMT", "Mon Sep  5 02:29:49 2016", "Monday, 05-Sep-16 02:29:49 GMT" },
    { 1473131354, "Tue, 06 Sep 2016 03:09:14 GMT", "Tue Sep  6 03:09:14 2016", "Tuesday, 06-Sep-16 03:09:14 GMT" },
    { 1473220119, "Wed, 07 Sep 2016 03:48:39 GMT", "Wed Sep  7 03:48:39 2016", "Wednesday, 07-Sep-16 03:48:39 GMT" },
    { 1473308884, "Thu, 08 Sep 2016 04:28:04 GMT", "Thu Sep  8 04:28:04 2016", "Thursday, 08-Sep-16 04:28:04 GMT" },
    { 1473397649, "Fri, 09 Sep 2016 05:07:29 GMT", "Fri Sep  9 05:07:29 2016", "Friday, 09-Sep-16 05:07:29 GMT" },
    { 1473486414, "Sat, 10 Sep 2016 05:46:54 GMT", "Sat Sep 10 05:46:54 2016", "Saturday, 10-Sep-16 05:46:54 GMT" },
    { 1473575179, "Sun, 11 Sep 2016 06:26:19 GMT", "Sun Sep 11 06:26:19 2016", "Sunday, 11-Sep-16 06:26:19 GMT" },
    { 1473663944, "Mon, 12 Sep 2016 07:05:44 GMT", "Mon Sep 12 07:05:44 2016", "Monday, 12-Sep-16 07:05:44 GMT" },
    { 1473752709, "Tue, 13 Sep 2016 07:45:09 GMT", "Tue Sep 13 07:45:09 2016", "Tuesday, 13-Sep-16 07:45:09 GMT" },
    { 1473841474, "Wed, 14 Sep 2016 08:24:34 GMT", "Wed Sep 14 08:24:34 2016", "Wednesday, 14-Sep-16 08:24:34 GMT" },
    { 1473930239, "Thu, 15 Sep 2016 09:03:59 GMT", "Thu Sep 15 09:03:59 2016", "Thursday, 15-Sep-16 09:03:59 GMT" },
    { 1474019004, "Fri, 16 Sep 2016 09:43:24 GMT", "Fri Sep 16 09:43:24 2016", "Friday, 16-Sep-16 09:43:24 GMT" },
    { 1474107769, "Sat, 17 Sep 2016 10:22:49 GMT", "Sat Sep 17 10:22:49 2016", "Saturday, 17-Sep-16 10:22:49 GMT" },
    { 1474196534, "Sun, 18 Sep 2016 11:02:14 GMT", "Sun Sep 18 11:02:14 2016", "Sunday, 18-Sep-16 11:02:14 GMT" },
    { 1474285299, "Mon, 19 Sep 2016 11:41:39 GMT", "Mon Sep 19 11:41:39 2016", "Monday, 19-Sep-16 11:41:39 GMT" },
    { 1474374064, "Tue, 20 Sep 2016 12:21:04 GMT", "Tue Sep 20 12:21:04 2016", "Tuesday, 20-Sep-16 12:21:04 GMT" },
    { 1474462829, "Wed, 21 Sep 2016 13:00:29 GMT", "Wed Sep 21 13:00:29 2016", "Wednesday, 21-Sep-16 13:00:29 GMT" },
    { 1474551594, "Thu, 22 Sep 2016 13:39:54 GMT", "Thu Sep 22 13:39:54 2016", "Thursday, 22-Sep-16 13:39:54 GMT" },
    { 1474640359, "Fri, 23 Sep 2016 14:19:19 GMT", "Fri Sep 23 14:19:19 2016", "Friday, 23-Sep-16 14:19:19 GMT" },
    { 1474729124, "Sat, 24 Sep 2016 14:58:44 GMT", "Sat Sep 24 14:58:44 2016", "Saturday, 24-Sep-16 14:58:44 GMT" },
    { 1474817889, "Sun, 25 Sep 2016 15:38:09 GMT", "Sun Sep 25 15:38:09 2016", "Sunday, 25-Sep-16 15:38:09 GMT" },
    { 1474906654, "Mon, 26 Sep 2016 16:17:34 GMT", "Mon Sep 26 16:17:34 2016", "Monday, 26-Sep-16 16:17:34 GMT" },
    { 1474995419, "Tue, 27 Sep 2016 16:56:59 GMT", "Tue Sep 27 16:56:59 2016", "Tuesday, 27-Sep-16 16:56:59 GMT" },
    { 1475084184, "Wed, 28 Sep 2016 17:36:24 GMT", "Wed Sep 28 17:36:24 2016", "Wednesday, 28-Sep-16 17:36:24 GMT" },
    { 1475172949, "Thu, 29 Sep 2016 18:15:49 GMT", "Thu Sep 29 18:15:49 2016", "Thursday, 29-Sep-16 18:15:49 GMT" },
    { 1475261714, "Fri, 30 Sep 2016 18:55:14 GMT", "Fri Sep 30 18:55:14 2016", "Friday, 30-Sep-16 18:55:14 GMT" },
    { 1475350479, "Sat, 01 Oct 2016 19:34:39 GMT", "Sat Oct  1 19:34:39 2016", "Saturday, 01-Oct-16 19:34:39 GMT" },
    { 1475439244, "Sun, 02 Oct 2016 20:14:04 GMT", "Sun Oct  2 20:14:04 2016", "Sunday, 02-Oct-16 20:14:04 GMT" },
    { 1475528009, "Mon, 03 Oct 2016 20:53:29 GMT", "Mon Oct  3 20:53:29 2016", "Monday, 03-Oct-16 20:53:29 GMT" },
    { 1475616774, "Tue, 04 Oct 2016 21:32:54 GMT", "Tue Oct  4 21:32:54 2016", "Tuesday, 04-Oct-16 21:32:54 GMT" },
    { 1475705539, "Wed, 05 Oct 2016 22:12:19 GMT", "Wed Oct  5 22:12:19 2016", "Wednesday, 05-Oct-16 22:12:19 GMT" },
    { 1475794304, "Thu, 06 Oct 2016 22:51:44 GMT", "Thu Oct  6 22:51:44 2016", "Thursday, 06-Oct-16 22:51:44 GMT" },
    { 1475883069, "Fri, 07 Oct 2016 23:31:09 GMT", "Fri Oct  7 23:31:09 2016", "Friday, 07-Oct-16 23:31:09 GMT" },
    { 1475971834, "Sun, 09 Oct 2016 00:10:34 GMT", "Sun Oct  9 00:10:34 2016", "Sunday, 09-Oct-16 00:10:34 GMT" },
    { 1476060599, "Mon, 10 Oct 2016 00:49:59 GMT", "Mon Oct 10 00:49:59 2016", "Monday, 10-Oct-16 00:49:59 GMT" },
    { 1476149364, "Tue, 11 Oct 2016 01:29:24 GMT", "Tue Oct 11 01:29:24 2016", "Tuesday, 11-Oct-16 01:29:24 GMT" },
    { 1476238129, "Wed, 12 Oct 2016 02:08:49 GMT", "Wed Oct 12 02:08:49 2016", "Wednesday, 12-Oct-16 02:08:49 GMT" },
    { 1476326894, "Thu, 13 Oct 2016 02:48:14 GMT", "Thu Oct 13 02:48:14 2016", "Thursday, 13-Oct-16 02:48:14 GMT" },
    { 1476415659, "Fri, 14 Oct 2016 03:27:39 GMT", "Fri Oct 14 03:27:39 2016", "Friday, 14-Oct-16 03:27:39 GMT" },
  };

int
main()
  {
    HTTP_DateTime dt;
    POSEIDON_TEST_CHECK(dt == http_datetime_min);

    for(const auto& r : tests) {
      dt.set_seconds((seconds) 0);
      POSEIDON_TEST_CHECK(dt.parse_rfc1123_partial(r.rfc1123) == ::strlen(r.rfc1123));
      POSEIDON_TEST_CHECK(dt.as_seconds() == (seconds) r.ts);

      dt.set_seconds((seconds) 0);
      POSEIDON_TEST_CHECK(dt.parse_rfc850_partial(r.rfc850) == ::strlen(r.rfc850));
      POSEIDON_TEST_CHECK(dt.as_seconds() == (seconds) r.ts);

      dt.set_seconds((seconds) 0);
      POSEIDON_TEST_CHECK(dt.parse_asctime_partial(r.asctime) == ::strlen(r.asctime));
      POSEIDON_TEST_CHECK(dt.as_seconds() == (seconds) r.ts);

      dt.set_seconds((seconds) 0);
      size_t tlen = 123;
      POSEIDON_TEST_CHECK(dt.parse(r.rfc1123, &tlen) == true);
      POSEIDON_TEST_CHECK(tlen == ::strlen(r.rfc1123));
      POSEIDON_TEST_CHECK(dt.as_seconds() == (seconds) r.ts);

      dt.set_seconds((seconds) 0);
      tlen = 123;
      POSEIDON_TEST_CHECK(dt.parse(r.rfc850, &tlen) == true);
      POSEIDON_TEST_CHECK(tlen == ::strlen(r.rfc850));
      POSEIDON_TEST_CHECK(dt.as_seconds() == (seconds) r.ts);

      dt.set_seconds((seconds) 0);
      tlen = 123;
      POSEIDON_TEST_CHECK(dt.parse(r.asctime, &tlen) == true);
      POSEIDON_TEST_CHECK(tlen == ::strlen(r.asctime));
      POSEIDON_TEST_CHECK(dt.as_seconds() == (seconds) r.ts);
    }

    for(const auto& r : tests) {
      dt.set_seconds((seconds) r.ts);
      POSEIDON_TEST_CHECK(dt.print_to_string() == r.rfc1123);

      char temp[64];
      dt.print_rfc1123_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.rfc1123) == 0);

      dt.print_rfc850_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.rfc850) == 0);

      dt.print_asctime_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.asctime) == 0);
    }
  }
