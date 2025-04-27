// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/base/datetime.hpp"
using namespace ::poseidon;

struct test_datetime
  {
    ::time_t ts;
    char rfc1123[32];
    char iso8601[32];
    char asctime[32];
    char rfc850[40];
  }
constexpr tests[] =
  {
    { 1467698107, "Tue, 05 Jul 2016 05:55:07 GMT", "2016-07-05T05:55:07Z", "Tue Jul  5 05:55:07 2016", "Tuesday, 05-Jul-16 05:55:07 GMT" },
    { 1467786876, "Wed, 06 Jul 2016 06:34:36 GMT", "2016-07-06T06:34:36Z", "Wed Jul  6 06:34:36 2016", "Wednesday, 06-Jul-16 06:34:36 GMT" },
    { 1467875645, "Thu, 07 Jul 2016 07:14:05 GMT", "2016-07-07T07:14:05Z", "Thu Jul  7 07:14:05 2016", "Thursday, 07-Jul-16 07:14:05 GMT" },
    { 1467964414, "Fri, 08 Jul 2016 07:53:34 GMT", "2016-07-08T07:53:34Z", "Fri Jul  8 07:53:34 2016", "Friday, 08-Jul-16 07:53:34 GMT" },
    { 1468053183, "Sat, 09 Jul 2016 08:33:03 GMT", "2016-07-09T08:33:03Z", "Sat Jul  9 08:33:03 2016", "Saturday, 09-Jul-16 08:33:03 GMT" },
    { 1468141952, "Sun, 10 Jul 2016 09:12:32 GMT", "2016-07-10T09:12:32Z", "Sun Jul 10 09:12:32 2016", "Sunday, 10-Jul-16 09:12:32 GMT" },
    { 1468230721, "Mon, 11 Jul 2016 09:52:01 GMT", "2016-07-11T09:52:01Z", "Mon Jul 11 09:52:01 2016", "Monday, 11-Jul-16 09:52:01 GMT" },
    { 1468319490, "Tue, 12 Jul 2016 10:31:30 GMT", "2016-07-12T10:31:30Z", "Tue Jul 12 10:31:30 2016", "Tuesday, 12-Jul-16 10:31:30 GMT" },
    { 1468408259, "Wed, 13 Jul 2016 11:10:59 GMT", "2016-07-13T11:10:59Z", "Wed Jul 13 11:10:59 2016", "Wednesday, 13-Jul-16 11:10:59 GMT" },
    { 1468497028, "Thu, 14 Jul 2016 11:50:28 GMT", "2016-07-14T11:50:28Z", "Thu Jul 14 11:50:28 2016", "Thursday, 14-Jul-16 11:50:28 GMT" },
    { 1468585797, "Fri, 15 Jul 2016 12:29:57 GMT", "2016-07-15T12:29:57Z", "Fri Jul 15 12:29:57 2016", "Friday, 15-Jul-16 12:29:57 GMT" },
    { 1468674566, "Sat, 16 Jul 2016 13:09:26 GMT", "2016-07-16T13:09:26Z", "Sat Jul 16 13:09:26 2016", "Saturday, 16-Jul-16 13:09:26 GMT" },
    { 1468763335, "Sun, 17 Jul 2016 13:48:55 GMT", "2016-07-17T13:48:55Z", "Sun Jul 17 13:48:55 2016", "Sunday, 17-Jul-16 13:48:55 GMT" },
    { 1468852104, "Mon, 18 Jul 2016 14:28:24 GMT", "2016-07-18T14:28:24Z", "Mon Jul 18 14:28:24 2016", "Monday, 18-Jul-16 14:28:24 GMT" },
    { 1468940873, "Tue, 19 Jul 2016 15:07:53 GMT", "2016-07-19T15:07:53Z", "Tue Jul 19 15:07:53 2016", "Tuesday, 19-Jul-16 15:07:53 GMT" },
    { 1469029642, "Wed, 20 Jul 2016 15:47:22 GMT", "2016-07-20T15:47:22Z", "Wed Jul 20 15:47:22 2016", "Wednesday, 20-Jul-16 15:47:22 GMT" },
    { 1469118411, "Thu, 21 Jul 2016 16:26:51 GMT", "2016-07-21T16:26:51Z", "Thu Jul 21 16:26:51 2016", "Thursday, 21-Jul-16 16:26:51 GMT" },
    { 1469207180, "Fri, 22 Jul 2016 17:06:20 GMT", "2016-07-22T17:06:20Z", "Fri Jul 22 17:06:20 2016", "Friday, 22-Jul-16 17:06:20 GMT" },
    { 1469295949, "Sat, 23 Jul 2016 17:45:49 GMT", "2016-07-23T17:45:49Z", "Sat Jul 23 17:45:49 2016", "Saturday, 23-Jul-16 17:45:49 GMT" },
    { 1469384718, "Sun, 24 Jul 2016 18:25:18 GMT", "2016-07-24T18:25:18Z", "Sun Jul 24 18:25:18 2016", "Sunday, 24-Jul-16 18:25:18 GMT" },
    { 1469473487, "Mon, 25 Jul 2016 19:04:47 GMT", "2016-07-25T19:04:47Z", "Mon Jul 25 19:04:47 2016", "Monday, 25-Jul-16 19:04:47 GMT" },
    { 1469562256, "Tue, 26 Jul 2016 19:44:16 GMT", "2016-07-26T19:44:16Z", "Tue Jul 26 19:44:16 2016", "Tuesday, 26-Jul-16 19:44:16 GMT" },
    { 1469651025, "Wed, 27 Jul 2016 20:23:45 GMT", "2016-07-27T20:23:45Z", "Wed Jul 27 20:23:45 2016", "Wednesday, 27-Jul-16 20:23:45 GMT" },
    { 1469739794, "Thu, 28 Jul 2016 21:03:14 GMT", "2016-07-28T21:03:14Z", "Thu Jul 28 21:03:14 2016", "Thursday, 28-Jul-16 21:03:14 GMT" },
    { 1469828563, "Fri, 29 Jul 2016 21:42:43 GMT", "2016-07-29T21:42:43Z", "Fri Jul 29 21:42:43 2016", "Friday, 29-Jul-16 21:42:43 GMT" },
    { 1469917332, "Sat, 30 Jul 2016 22:22:12 GMT", "2016-07-30T22:22:12Z", "Sat Jul 30 22:22:12 2016", "Saturday, 30-Jul-16 22:22:12 GMT" },
    { 1470006101, "Sun, 31 Jul 2016 23:01:41 GMT", "2016-07-31T23:01:41Z", "Sun Jul 31 23:01:41 2016", "Sunday, 31-Jul-16 23:01:41 GMT" },
    { 1470094870, "Mon, 01 Aug 2016 23:41:10 GMT", "2016-08-01T23:41:10Z", "Mon Aug  1 23:41:10 2016", "Monday, 01-Aug-16 23:41:10 GMT" },
    { 1470183639, "Wed, 03 Aug 2016 00:20:39 GMT", "2016-08-03T00:20:39Z", "Wed Aug  3 00:20:39 2016", "Wednesday, 03-Aug-16 00:20:39 GMT" },
    { 1470272408, "Thu, 04 Aug 2016 01:00:08 GMT", "2016-08-04T01:00:08Z", "Thu Aug  4 01:00:08 2016", "Thursday, 04-Aug-16 01:00:08 GMT" },
    { 1470361177, "Fri, 05 Aug 2016 01:39:37 GMT", "2016-08-05T01:39:37Z", "Fri Aug  5 01:39:37 2016", "Friday, 05-Aug-16 01:39:37 GMT" },
    { 1470449946, "Sat, 06 Aug 2016 02:19:06 GMT", "2016-08-06T02:19:06Z", "Sat Aug  6 02:19:06 2016", "Saturday, 06-Aug-16 02:19:06 GMT" },
    { 1470538715, "Sun, 07 Aug 2016 02:58:35 GMT", "2016-08-07T02:58:35Z", "Sun Aug  7 02:58:35 2016", "Sunday, 07-Aug-16 02:58:35 GMT" },
    { 1470627484, "Mon, 08 Aug 2016 03:38:04 GMT", "2016-08-08T03:38:04Z", "Mon Aug  8 03:38:04 2016", "Monday, 08-Aug-16 03:38:04 GMT" },
    { 1470716253, "Tue, 09 Aug 2016 04:17:33 GMT", "2016-08-09T04:17:33Z", "Tue Aug  9 04:17:33 2016", "Tuesday, 09-Aug-16 04:17:33 GMT" },
    { 1470805022, "Wed, 10 Aug 2016 04:57:02 GMT", "2016-08-10T04:57:02Z", "Wed Aug 10 04:57:02 2016", "Wednesday, 10-Aug-16 04:57:02 GMT" },
    { 1470893791, "Thu, 11 Aug 2016 05:36:31 GMT", "2016-08-11T05:36:31Z", "Thu Aug 11 05:36:31 2016", "Thursday, 11-Aug-16 05:36:31 GMT" },
    { 1470982560, "Fri, 12 Aug 2016 06:16:00 GMT", "2016-08-12T06:16:00Z", "Fri Aug 12 06:16:00 2016", "Friday, 12-Aug-16 06:16:00 GMT" },
    { 1471071329, "Sat, 13 Aug 2016 06:55:29 GMT", "2016-08-13T06:55:29Z", "Sat Aug 13 06:55:29 2016", "Saturday, 13-Aug-16 06:55:29 GMT" },
    { 1471160098, "Sun, 14 Aug 2016 07:34:58 GMT", "2016-08-14T07:34:58Z", "Sun Aug 14 07:34:58 2016", "Sunday, 14-Aug-16 07:34:58 GMT" },
    { 1471248867, "Mon, 15 Aug 2016 08:14:27 GMT", "2016-08-15T08:14:27Z", "Mon Aug 15 08:14:27 2016", "Monday, 15-Aug-16 08:14:27 GMT" },
    { 1471337636, "Tue, 16 Aug 2016 08:53:56 GMT", "2016-08-16T08:53:56Z", "Tue Aug 16 08:53:56 2016", "Tuesday, 16-Aug-16 08:53:56 GMT" },
    { 1471426405, "Wed, 17 Aug 2016 09:33:25 GMT", "2016-08-17T09:33:25Z", "Wed Aug 17 09:33:25 2016", "Wednesday, 17-Aug-16 09:33:25 GMT" },
    { 1471515174, "Thu, 18 Aug 2016 10:12:54 GMT", "2016-08-18T10:12:54Z", "Thu Aug 18 10:12:54 2016", "Thursday, 18-Aug-16 10:12:54 GMT" },
    { 1471603943, "Fri, 19 Aug 2016 10:52:23 GMT", "2016-08-19T10:52:23Z", "Fri Aug 19 10:52:23 2016", "Friday, 19-Aug-16 10:52:23 GMT" },
    { 1471692712, "Sat, 20 Aug 2016 11:31:52 GMT", "2016-08-20T11:31:52Z", "Sat Aug 20 11:31:52 2016", "Saturday, 20-Aug-16 11:31:52 GMT" },
    { 1471781481, "Sun, 21 Aug 2016 12:11:21 GMT", "2016-08-21T12:11:21Z", "Sun Aug 21 12:11:21 2016", "Sunday, 21-Aug-16 12:11:21 GMT" },
    { 1471870250, "Mon, 22 Aug 2016 12:50:50 GMT", "2016-08-22T12:50:50Z", "Mon Aug 22 12:50:50 2016", "Monday, 22-Aug-16 12:50:50 GMT" },
    { 1471959019, "Tue, 23 Aug 2016 13:30:19 GMT", "2016-08-23T13:30:19Z", "Tue Aug 23 13:30:19 2016", "Tuesday, 23-Aug-16 13:30:19 GMT" },
    { 1472047788, "Wed, 24 Aug 2016 14:09:48 GMT", "2016-08-24T14:09:48Z", "Wed Aug 24 14:09:48 2016", "Wednesday, 24-Aug-16 14:09:48 GMT" },
    { 1472136557, "Thu, 25 Aug 2016 14:49:17 GMT", "2016-08-25T14:49:17Z", "Thu Aug 25 14:49:17 2016", "Thursday, 25-Aug-16 14:49:17 GMT" },
    { 1472225326, "Fri, 26 Aug 2016 15:28:46 GMT", "2016-08-26T15:28:46Z", "Fri Aug 26 15:28:46 2016", "Friday, 26-Aug-16 15:28:46 GMT" },
    { 1472314095, "Sat, 27 Aug 2016 16:08:15 GMT", "2016-08-27T16:08:15Z", "Sat Aug 27 16:08:15 2016", "Saturday, 27-Aug-16 16:08:15 GMT" },
    { 1472402864, "Sun, 28 Aug 2016 16:47:44 GMT", "2016-08-28T16:47:44Z", "Sun Aug 28 16:47:44 2016", "Sunday, 28-Aug-16 16:47:44 GMT" },
    { 1472491633, "Mon, 29 Aug 2016 17:27:13 GMT", "2016-08-29T17:27:13Z", "Mon Aug 29 17:27:13 2016", "Monday, 29-Aug-16 17:27:13 GMT" },
    { 1472580402, "Tue, 30 Aug 2016 18:06:42 GMT", "2016-08-30T18:06:42Z", "Tue Aug 30 18:06:42 2016", "Tuesday, 30-Aug-16 18:06:42 GMT" },
    { 1472669171, "Wed, 31 Aug 2016 18:46:11 GMT", "2016-08-31T18:46:11Z", "Wed Aug 31 18:46:11 2016", "Wednesday, 31-Aug-16 18:46:11 GMT" },
    { 1472757940, "Thu, 01 Sep 2016 19:25:40 GMT", "2016-09-01T19:25:40Z", "Thu Sep  1 19:25:40 2016", "Thursday, 01-Sep-16 19:25:40 GMT" },
    { 1472846709, "Fri, 02 Sep 2016 20:05:09 GMT", "2016-09-02T20:05:09Z", "Fri Sep  2 20:05:09 2016", "Friday, 02-Sep-16 20:05:09 GMT" },
    { 1472935478, "Sat, 03 Sep 2016 20:44:38 GMT", "2016-09-03T20:44:38Z", "Sat Sep  3 20:44:38 2016", "Saturday, 03-Sep-16 20:44:38 GMT" },
    { 1473024247, "Sun, 04 Sep 2016 21:24:07 GMT", "2016-09-04T21:24:07Z", "Sun Sep  4 21:24:07 2016", "Sunday, 04-Sep-16 21:24:07 GMT" },
    { 1473113016, "Mon, 05 Sep 2016 22:03:36 GMT", "2016-09-05T22:03:36Z", "Mon Sep  5 22:03:36 2016", "Monday, 05-Sep-16 22:03:36 GMT" },
    { 1473201785, "Tue, 06 Sep 2016 22:43:05 GMT", "2016-09-06T22:43:05Z", "Tue Sep  6 22:43:05 2016", "Tuesday, 06-Sep-16 22:43:05 GMT" },
    { 1473290554, "Wed, 07 Sep 2016 23:22:34 GMT", "2016-09-07T23:22:34Z", "Wed Sep  7 23:22:34 2016", "Wednesday, 07-Sep-16 23:22:34 GMT" },
    { 1473379323, "Fri, 09 Sep 2016 00:02:03 GMT", "2016-09-09T00:02:03Z", "Fri Sep  9 00:02:03 2016", "Friday, 09-Sep-16 00:02:03 GMT" },
    { 1473468092, "Sat, 10 Sep 2016 00:41:32 GMT", "2016-09-10T00:41:32Z", "Sat Sep 10 00:41:32 2016", "Saturday, 10-Sep-16 00:41:32 GMT" },
    { 1473556861, "Sun, 11 Sep 2016 01:21:01 GMT", "2016-09-11T01:21:01Z", "Sun Sep 11 01:21:01 2016", "Sunday, 11-Sep-16 01:21:01 GMT" },
    { 1473645630, "Mon, 12 Sep 2016 02:00:30 GMT", "2016-09-12T02:00:30Z", "Mon Sep 12 02:00:30 2016", "Monday, 12-Sep-16 02:00:30 GMT" },
    { 1473734399, "Tue, 13 Sep 2016 02:39:59 GMT", "2016-09-13T02:39:59Z", "Tue Sep 13 02:39:59 2016", "Tuesday, 13-Sep-16 02:39:59 GMT" },
    { 1473823168, "Wed, 14 Sep 2016 03:19:28 GMT", "2016-09-14T03:19:28Z", "Wed Sep 14 03:19:28 2016", "Wednesday, 14-Sep-16 03:19:28 GMT" },
    { 1473911937, "Thu, 15 Sep 2016 03:58:57 GMT", "2016-09-15T03:58:57Z", "Thu Sep 15 03:58:57 2016", "Thursday, 15-Sep-16 03:58:57 GMT" },
    { 1474000706, "Fri, 16 Sep 2016 04:38:26 GMT", "2016-09-16T04:38:26Z", "Fri Sep 16 04:38:26 2016", "Friday, 16-Sep-16 04:38:26 GMT" },
    { 1474089475, "Sat, 17 Sep 2016 05:17:55 GMT", "2016-09-17T05:17:55Z", "Sat Sep 17 05:17:55 2016", "Saturday, 17-Sep-16 05:17:55 GMT" },
    { 1474178244, "Sun, 18 Sep 2016 05:57:24 GMT", "2016-09-18T05:57:24Z", "Sun Sep 18 05:57:24 2016", "Sunday, 18-Sep-16 05:57:24 GMT" },
    { 1474267013, "Mon, 19 Sep 2016 06:36:53 GMT", "2016-09-19T06:36:53Z", "Mon Sep 19 06:36:53 2016", "Monday, 19-Sep-16 06:36:53 GMT" },
    { 1474355782, "Tue, 20 Sep 2016 07:16:22 GMT", "2016-09-20T07:16:22Z", "Tue Sep 20 07:16:22 2016", "Tuesday, 20-Sep-16 07:16:22 GMT" },
    { 1474444551, "Wed, 21 Sep 2016 07:55:51 GMT", "2016-09-21T07:55:51Z", "Wed Sep 21 07:55:51 2016", "Wednesday, 21-Sep-16 07:55:51 GMT" },
    { 1474533320, "Thu, 22 Sep 2016 08:35:20 GMT", "2016-09-22T08:35:20Z", "Thu Sep 22 08:35:20 2016", "Thursday, 22-Sep-16 08:35:20 GMT" },
    { 1474622089, "Fri, 23 Sep 2016 09:14:49 GMT", "2016-09-23T09:14:49Z", "Fri Sep 23 09:14:49 2016", "Friday, 23-Sep-16 09:14:49 GMT" },
    { 1474710858, "Sat, 24 Sep 2016 09:54:18 GMT", "2016-09-24T09:54:18Z", "Sat Sep 24 09:54:18 2016", "Saturday, 24-Sep-16 09:54:18 GMT" },
    { 1474799627, "Sun, 25 Sep 2016 10:33:47 GMT", "2016-09-25T10:33:47Z", "Sun Sep 25 10:33:47 2016", "Sunday, 25-Sep-16 10:33:47 GMT" },
    { 1474888396, "Mon, 26 Sep 2016 11:13:16 GMT", "2016-09-26T11:13:16Z", "Mon Sep 26 11:13:16 2016", "Monday, 26-Sep-16 11:13:16 GMT" },
    { 1474977165, "Tue, 27 Sep 2016 11:52:45 GMT", "2016-09-27T11:52:45Z", "Tue Sep 27 11:52:45 2016", "Tuesday, 27-Sep-16 11:52:45 GMT" },
    { 1475065934, "Wed, 28 Sep 2016 12:32:14 GMT", "2016-09-28T12:32:14Z", "Wed Sep 28 12:32:14 2016", "Wednesday, 28-Sep-16 12:32:14 GMT" },
    { 1475154703, "Thu, 29 Sep 2016 13:11:43 GMT", "2016-09-29T13:11:43Z", "Thu Sep 29 13:11:43 2016", "Thursday, 29-Sep-16 13:11:43 GMT" },
    { 1475243472, "Fri, 30 Sep 2016 13:51:12 GMT", "2016-09-30T13:51:12Z", "Fri Sep 30 13:51:12 2016", "Friday, 30-Sep-16 13:51:12 GMT" },
    { 1475332241, "Sat, 01 Oct 2016 14:30:41 GMT", "2016-10-01T14:30:41Z", "Sat Oct  1 14:30:41 2016", "Saturday, 01-Oct-16 14:30:41 GMT" },
    { 1475421010, "Sun, 02 Oct 2016 15:10:10 GMT", "2016-10-02T15:10:10Z", "Sun Oct  2 15:10:10 2016", "Sunday, 02-Oct-16 15:10:10 GMT" },
    { 1475509779, "Mon, 03 Oct 2016 15:49:39 GMT", "2016-10-03T15:49:39Z", "Mon Oct  3 15:49:39 2016", "Monday, 03-Oct-16 15:49:39 GMT" },
    { 1475598548, "Tue, 04 Oct 2016 16:29:08 GMT", "2016-10-04T16:29:08Z", "Tue Oct  4 16:29:08 2016", "Tuesday, 04-Oct-16 16:29:08 GMT" },
    { 1475687317, "Wed, 05 Oct 2016 17:08:37 GMT", "2016-10-05T17:08:37Z", "Wed Oct  5 17:08:37 2016", "Wednesday, 05-Oct-16 17:08:37 GMT" },
    { 1475776086, "Thu, 06 Oct 2016 17:48:06 GMT", "2016-10-06T17:48:06Z", "Thu Oct  6 17:48:06 2016", "Thursday, 06-Oct-16 17:48:06 GMT" },
    { 1475864855, "Fri, 07 Oct 2016 18:27:35 GMT", "2016-10-07T18:27:35Z", "Fri Oct  7 18:27:35 2016", "Friday, 07-Oct-16 18:27:35 GMT" },
    { 1475953624, "Sat, 08 Oct 2016 19:07:04 GMT", "2016-10-08T19:07:04Z", "Sat Oct  8 19:07:04 2016", "Saturday, 08-Oct-16 19:07:04 GMT" },
    { 1476042393, "Sun, 09 Oct 2016 19:46:33 GMT", "2016-10-09T19:46:33Z", "Sun Oct  9 19:46:33 2016", "Sunday, 09-Oct-16 19:46:33 GMT" },
    { 1476131162, "Mon, 10 Oct 2016 20:26:02 GMT", "2016-10-10T20:26:02Z", "Mon Oct 10 20:26:02 2016", "Monday, 10-Oct-16 20:26:02 GMT" },
    { 1476219931, "Tue, 11 Oct 2016 21:05:31 GMT", "2016-10-11T21:05:31Z", "Tue Oct 11 21:05:31 2016", "Tuesday, 11-Oct-16 21:05:31 GMT" },
    { 1476308700, "Wed, 12 Oct 2016 21:45:00 GMT", "2016-10-12T21:45:00Z", "Wed Oct 12 21:45:00 2016", "Wednesday, 12-Oct-16 21:45:00 GMT" },
    { 1476397469, "Thu, 13 Oct 2016 22:24:29 GMT", "2016-10-13T22:24:29Z", "Thu Oct 13 22:24:29 2016", "Thursday, 13-Oct-16 22:24:29 GMT" },
    { 1476486238, "Fri, 14 Oct 2016 23:03:58 GMT", "2016-10-14T23:03:58Z", "Fri Oct 14 23:03:58 2016", "Friday, 14-Oct-16 23:03:58 GMT" },
  };

int
main()
  {
    DateTime dt;
    POSEIDON_TEST_CHECK(dt == system_time());
    char temp[64];

    for(const auto& r : tests) {
      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse_rfc1123_partial(r.rfc1123) == ::strlen(r.rfc1123));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse_iso8601_partial(r.iso8601) == ::strlen(r.iso8601));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse_rfc850_partial(r.rfc850) == ::strlen(r.rfc850));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse_asctime_partial(r.asctime) == ::strlen(r.asctime));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse(r.rfc1123) == ::strlen(r.rfc1123));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse(r.iso8601) == ::strlen(r.iso8601));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse(r.rfc850) == ::strlen(r.rfc850));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse(r.asctime) == ::strlen(r.asctime));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);
    }

    for(const auto& r : tests) {
      dt.set_system_time(system_clock::from_time_t(r.ts));

      ::memset(temp, '*', sizeof(temp));
      dt.print_rfc1123_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.rfc1123) == 0);

      ::memset(temp, '*', sizeof(temp));
      dt.print_iso8601_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.iso8601) == 0);

      ::memset(temp, '*', sizeof(temp));
      dt.print_rfc850_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.rfc850) == 0);

      ::memset(temp, '*', sizeof(temp));
      dt.print_asctime_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.asctime) == 0);
    }
  }
