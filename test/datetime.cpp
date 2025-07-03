// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/base/datetime.hpp"
using namespace ::poseidon;

struct test_datetime
  {
    ::time_t ts;
    char rfc1123[32];
    char git[32];
    char asctime[32];
    char rfc850[40];
  }
constexpr tests[] =
  {
    { 1467698107, "Tue, 05 Jul 2016 05:55:07 GMT", "2016-07-05 04:55:07 -0100", "Tue Jul  5 05:55:07 2016", "Tuesday, 05-Jul-16 05:55:07 GMT" },
    { 1467786876, "Wed, 06 Jul 2016 06:34:36 GMT", "2016-07-06 05:34:36 -0100", "Wed Jul  6 06:34:36 2016", "Wednesday, 06-Jul-16 06:34:36 GMT" },
    { 1467875645, "Thu, 07 Jul 2016 07:14:05 GMT", "2016-07-07 06:14:05 -0100", "Thu Jul  7 07:14:05 2016", "Thursday, 07-Jul-16 07:14:05 GMT" },
    { 1467964414, "Fri, 08 Jul 2016 07:53:34 GMT", "2016-07-08 06:53:34 -0100", "Fri Jul  8 07:53:34 2016", "Friday, 08-Jul-16 07:53:34 GMT" },
    { 1468053183, "Sat, 09 Jul 2016 08:33:03 GMT", "2016-07-09 07:33:03 -0100", "Sat Jul  9 08:33:03 2016", "Saturday, 09-Jul-16 08:33:03 GMT" },
    { 1468141952, "Sun, 10 Jul 2016 09:12:32 GMT", "2016-07-10 08:12:32 -0100", "Sun Jul 10 09:12:32 2016", "Sunday, 10-Jul-16 09:12:32 GMT" },
    { 1468230721, "Mon, 11 Jul 2016 09:52:01 GMT", "2016-07-11 08:52:01 -0100", "Mon Jul 11 09:52:01 2016", "Monday, 11-Jul-16 09:52:01 GMT" },
    { 1468319490, "Tue, 12 Jul 2016 10:31:30 GMT", "2016-07-12 09:31:30 -0100", "Tue Jul 12 10:31:30 2016", "Tuesday, 12-Jul-16 10:31:30 GMT" },
    { 1468408259, "Wed, 13 Jul 2016 11:10:59 GMT", "2016-07-13 10:10:59 -0100", "Wed Jul 13 11:10:59 2016", "Wednesday, 13-Jul-16 11:10:59 GMT" },
    { 1468497028, "Thu, 14 Jul 2016 11:50:28 GMT", "2016-07-14 10:50:28 -0100", "Thu Jul 14 11:50:28 2016", "Thursday, 14-Jul-16 11:50:28 GMT" },
    { 1468585797, "Fri, 15 Jul 2016 12:29:57 GMT", "2016-07-15 11:29:57 -0100", "Fri Jul 15 12:29:57 2016", "Friday, 15-Jul-16 12:29:57 GMT" },
    { 1468674566, "Sat, 16 Jul 2016 13:09:26 GMT", "2016-07-16 12:09:26 -0100", "Sat Jul 16 13:09:26 2016", "Saturday, 16-Jul-16 13:09:26 GMT" },
    { 1468763335, "Sun, 17 Jul 2016 13:48:55 GMT", "2016-07-17 12:48:55 -0100", "Sun Jul 17 13:48:55 2016", "Sunday, 17-Jul-16 13:48:55 GMT" },
    { 1468852104, "Mon, 18 Jul 2016 14:28:24 GMT", "2016-07-18 13:28:24 -0100", "Mon Jul 18 14:28:24 2016", "Monday, 18-Jul-16 14:28:24 GMT" },
    { 1468940873, "Tue, 19 Jul 2016 15:07:53 GMT", "2016-07-19 14:07:53 -0100", "Tue Jul 19 15:07:53 2016", "Tuesday, 19-Jul-16 15:07:53 GMT" },
    { 1469029642, "Wed, 20 Jul 2016 15:47:22 GMT", "2016-07-20 14:47:22 -0100", "Wed Jul 20 15:47:22 2016", "Wednesday, 20-Jul-16 15:47:22 GMT" },
    { 1469118411, "Thu, 21 Jul 2016 16:26:51 GMT", "2016-07-21 15:26:51 -0100", "Thu Jul 21 16:26:51 2016", "Thursday, 21-Jul-16 16:26:51 GMT" },
    { 1469207180, "Fri, 22 Jul 2016 17:06:20 GMT", "2016-07-22 16:06:20 -0100", "Fri Jul 22 17:06:20 2016", "Friday, 22-Jul-16 17:06:20 GMT" },
    { 1469295949, "Sat, 23 Jul 2016 17:45:49 GMT", "2016-07-23 16:45:49 -0100", "Sat Jul 23 17:45:49 2016", "Saturday, 23-Jul-16 17:45:49 GMT" },
    { 1469384718, "Sun, 24 Jul 2016 18:25:18 GMT", "2016-07-24 17:25:18 -0100", "Sun Jul 24 18:25:18 2016", "Sunday, 24-Jul-16 18:25:18 GMT" },
    { 1469473487, "Mon, 25 Jul 2016 19:04:47 GMT", "2016-07-25 18:04:47 -0100", "Mon Jul 25 19:04:47 2016", "Monday, 25-Jul-16 19:04:47 GMT" },
    { 1469562256, "Tue, 26 Jul 2016 19:44:16 GMT", "2016-07-26 18:44:16 -0100", "Tue Jul 26 19:44:16 2016", "Tuesday, 26-Jul-16 19:44:16 GMT" },
    { 1469651025, "Wed, 27 Jul 2016 20:23:45 GMT", "2016-07-27 19:23:45 -0100", "Wed Jul 27 20:23:45 2016", "Wednesday, 27-Jul-16 20:23:45 GMT" },
    { 1469739794, "Thu, 28 Jul 2016 21:03:14 GMT", "2016-07-28 20:03:14 -0100", "Thu Jul 28 21:03:14 2016", "Thursday, 28-Jul-16 21:03:14 GMT" },
    { 1469828563, "Fri, 29 Jul 2016 21:42:43 GMT", "2016-07-29 20:42:43 -0100", "Fri Jul 29 21:42:43 2016", "Friday, 29-Jul-16 21:42:43 GMT" },
    { 1469917332, "Sat, 30 Jul 2016 22:22:12 GMT", "2016-07-30 21:22:12 -0100", "Sat Jul 30 22:22:12 2016", "Saturday, 30-Jul-16 22:22:12 GMT" },
    { 1470006101, "Sun, 31 Jul 2016 23:01:41 GMT", "2016-07-31 22:01:41 -0100", "Sun Jul 31 23:01:41 2016", "Sunday, 31-Jul-16 23:01:41 GMT" },
    { 1470094870, "Mon, 01 Aug 2016 23:41:10 GMT", "2016-08-01 22:41:10 -0100", "Mon Aug  1 23:41:10 2016", "Monday, 01-Aug-16 23:41:10 GMT" },
    { 1470183639, "Wed, 03 Aug 2016 00:20:39 GMT", "2016-08-02 23:20:39 -0100", "Wed Aug  3 00:20:39 2016", "Wednesday, 03-Aug-16 00:20:39 GMT" },
    { 1470272408, "Thu, 04 Aug 2016 01:00:08 GMT", "2016-08-04 00:00:08 -0100", "Thu Aug  4 01:00:08 2016", "Thursday, 04-Aug-16 01:00:08 GMT" },
    { 1470361177, "Fri, 05 Aug 2016 01:39:37 GMT", "2016-08-05 00:39:37 -0100", "Fri Aug  5 01:39:37 2016", "Friday, 05-Aug-16 01:39:37 GMT" },
    { 1470449946, "Sat, 06 Aug 2016 02:19:06 GMT", "2016-08-06 01:19:06 -0100", "Sat Aug  6 02:19:06 2016", "Saturday, 06-Aug-16 02:19:06 GMT" },
    { 1470538715, "Sun, 07 Aug 2016 02:58:35 GMT", "2016-08-07 01:58:35 -0100", "Sun Aug  7 02:58:35 2016", "Sunday, 07-Aug-16 02:58:35 GMT" },
    { 1470627484, "Mon, 08 Aug 2016 03:38:04 GMT", "2016-08-08 02:38:04 -0100", "Mon Aug  8 03:38:04 2016", "Monday, 08-Aug-16 03:38:04 GMT" },
    { 1470716253, "Tue, 09 Aug 2016 04:17:33 GMT", "2016-08-09 03:17:33 -0100", "Tue Aug  9 04:17:33 2016", "Tuesday, 09-Aug-16 04:17:33 GMT" },
    { 1470805022, "Wed, 10 Aug 2016 04:57:02 GMT", "2016-08-10 03:57:02 -0100", "Wed Aug 10 04:57:02 2016", "Wednesday, 10-Aug-16 04:57:02 GMT" },
    { 1470893791, "Thu, 11 Aug 2016 05:36:31 GMT", "2016-08-11 04:36:31 -0100", "Thu Aug 11 05:36:31 2016", "Thursday, 11-Aug-16 05:36:31 GMT" },
    { 1470982560, "Fri, 12 Aug 2016 06:16:00 GMT", "2016-08-12 05:16:00 -0100", "Fri Aug 12 06:16:00 2016", "Friday, 12-Aug-16 06:16:00 GMT" },
    { 1471071329, "Sat, 13 Aug 2016 06:55:29 GMT", "2016-08-13 05:55:29 -0100", "Sat Aug 13 06:55:29 2016", "Saturday, 13-Aug-16 06:55:29 GMT" },
    { 1471160098, "Sun, 14 Aug 2016 07:34:58 GMT", "2016-08-14 06:34:58 -0100", "Sun Aug 14 07:34:58 2016", "Sunday, 14-Aug-16 07:34:58 GMT" },
    { 1471248867, "Mon, 15 Aug 2016 08:14:27 GMT", "2016-08-15 07:14:27 -0100", "Mon Aug 15 08:14:27 2016", "Monday, 15-Aug-16 08:14:27 GMT" },
    { 1471337636, "Tue, 16 Aug 2016 08:53:56 GMT", "2016-08-16 07:53:56 -0100", "Tue Aug 16 08:53:56 2016", "Tuesday, 16-Aug-16 08:53:56 GMT" },
    { 1471426405, "Wed, 17 Aug 2016 09:33:25 GMT", "2016-08-17 08:33:25 -0100", "Wed Aug 17 09:33:25 2016", "Wednesday, 17-Aug-16 09:33:25 GMT" },
    { 1471515174, "Thu, 18 Aug 2016 10:12:54 GMT", "2016-08-18 09:12:54 -0100", "Thu Aug 18 10:12:54 2016", "Thursday, 18-Aug-16 10:12:54 GMT" },
    { 1471603943, "Fri, 19 Aug 2016 10:52:23 GMT", "2016-08-19 09:52:23 -0100", "Fri Aug 19 10:52:23 2016", "Friday, 19-Aug-16 10:52:23 GMT" },
    { 1471692712, "Sat, 20 Aug 2016 11:31:52 GMT", "2016-08-20 10:31:52 -0100", "Sat Aug 20 11:31:52 2016", "Saturday, 20-Aug-16 11:31:52 GMT" },
    { 1471781481, "Sun, 21 Aug 2016 12:11:21 GMT", "2016-08-21 11:11:21 -0100", "Sun Aug 21 12:11:21 2016", "Sunday, 21-Aug-16 12:11:21 GMT" },
    { 1471870250, "Mon, 22 Aug 2016 12:50:50 GMT", "2016-08-22 11:50:50 -0100", "Mon Aug 22 12:50:50 2016", "Monday, 22-Aug-16 12:50:50 GMT" },
    { 1471959019, "Tue, 23 Aug 2016 13:30:19 GMT", "2016-08-23 12:30:19 -0100", "Tue Aug 23 13:30:19 2016", "Tuesday, 23-Aug-16 13:30:19 GMT" },
    { 1472047788, "Wed, 24 Aug 2016 14:09:48 GMT", "2016-08-24 13:09:48 -0100", "Wed Aug 24 14:09:48 2016", "Wednesday, 24-Aug-16 14:09:48 GMT" },
    { 1472136557, "Thu, 25 Aug 2016 14:49:17 GMT", "2016-08-25 13:49:17 -0100", "Thu Aug 25 14:49:17 2016", "Thursday, 25-Aug-16 14:49:17 GMT" },
    { 1472225326, "Fri, 26 Aug 2016 15:28:46 GMT", "2016-08-26 14:28:46 -0100", "Fri Aug 26 15:28:46 2016", "Friday, 26-Aug-16 15:28:46 GMT" },
    { 1472314095, "Sat, 27 Aug 2016 16:08:15 GMT", "2016-08-27 15:08:15 -0100", "Sat Aug 27 16:08:15 2016", "Saturday, 27-Aug-16 16:08:15 GMT" },
    { 1472402864, "Sun, 28 Aug 2016 16:47:44 GMT", "2016-08-28 15:47:44 -0100", "Sun Aug 28 16:47:44 2016", "Sunday, 28-Aug-16 16:47:44 GMT" },
    { 1472491633, "Mon, 29 Aug 2016 17:27:13 GMT", "2016-08-29 16:27:13 -0100", "Mon Aug 29 17:27:13 2016", "Monday, 29-Aug-16 17:27:13 GMT" },
    { 1472580402, "Tue, 30 Aug 2016 18:06:42 GMT", "2016-08-30 17:06:42 -0100", "Tue Aug 30 18:06:42 2016", "Tuesday, 30-Aug-16 18:06:42 GMT" },
    { 1472669171, "Wed, 31 Aug 2016 18:46:11 GMT", "2016-08-31 17:46:11 -0100", "Wed Aug 31 18:46:11 2016", "Wednesday, 31-Aug-16 18:46:11 GMT" },
    { 1472757940, "Thu, 01 Sep 2016 19:25:40 GMT", "2016-09-01 18:25:40 -0100", "Thu Sep  1 19:25:40 2016", "Thursday, 01-Sep-16 19:25:40 GMT" },
    { 1472846709, "Fri, 02 Sep 2016 20:05:09 GMT", "2016-09-02 19:05:09 -0100", "Fri Sep  2 20:05:09 2016", "Friday, 02-Sep-16 20:05:09 GMT" },
    { 1472935478, "Sat, 03 Sep 2016 20:44:38 GMT", "2016-09-03 19:44:38 -0100", "Sat Sep  3 20:44:38 2016", "Saturday, 03-Sep-16 20:44:38 GMT" },
    { 1473024247, "Sun, 04 Sep 2016 21:24:07 GMT", "2016-09-04 20:24:07 -0100", "Sun Sep  4 21:24:07 2016", "Sunday, 04-Sep-16 21:24:07 GMT" },
    { 1473113016, "Mon, 05 Sep 2016 22:03:36 GMT", "2016-09-05 21:03:36 -0100", "Mon Sep  5 22:03:36 2016", "Monday, 05-Sep-16 22:03:36 GMT" },
    { 1473201785, "Tue, 06 Sep 2016 22:43:05 GMT", "2016-09-06 21:43:05 -0100", "Tue Sep  6 22:43:05 2016", "Tuesday, 06-Sep-16 22:43:05 GMT" },
    { 1473290554, "Wed, 07 Sep 2016 23:22:34 GMT", "2016-09-07 22:22:34 -0100", "Wed Sep  7 23:22:34 2016", "Wednesday, 07-Sep-16 23:22:34 GMT" },
    { 1473379323, "Fri, 09 Sep 2016 00:02:03 GMT", "2016-09-08 23:02:03 -0100", "Fri Sep  9 00:02:03 2016", "Friday, 09-Sep-16 00:02:03 GMT" },
    { 1473468092, "Sat, 10 Sep 2016 00:41:32 GMT", "2016-09-09 23:41:32 -0100", "Sat Sep 10 00:41:32 2016", "Saturday, 10-Sep-16 00:41:32 GMT" },
    { 1473556861, "Sun, 11 Sep 2016 01:21:01 GMT", "2016-09-11 00:21:01 -0100", "Sun Sep 11 01:21:01 2016", "Sunday, 11-Sep-16 01:21:01 GMT" },
    { 1473645630, "Mon, 12 Sep 2016 02:00:30 GMT", "2016-09-12 01:00:30 -0100", "Mon Sep 12 02:00:30 2016", "Monday, 12-Sep-16 02:00:30 GMT" },
    { 1473734399, "Tue, 13 Sep 2016 02:39:59 GMT", "2016-09-13 01:39:59 -0100", "Tue Sep 13 02:39:59 2016", "Tuesday, 13-Sep-16 02:39:59 GMT" },
    { 1473823168, "Wed, 14 Sep 2016 03:19:28 GMT", "2016-09-14 02:19:28 -0100", "Wed Sep 14 03:19:28 2016", "Wednesday, 14-Sep-16 03:19:28 GMT" },
    { 1473911937, "Thu, 15 Sep 2016 03:58:57 GMT", "2016-09-15 02:58:57 -0100", "Thu Sep 15 03:58:57 2016", "Thursday, 15-Sep-16 03:58:57 GMT" },
    { 1474000706, "Fri, 16 Sep 2016 04:38:26 GMT", "2016-09-16 03:38:26 -0100", "Fri Sep 16 04:38:26 2016", "Friday, 16-Sep-16 04:38:26 GMT" },
    { 1474089475, "Sat, 17 Sep 2016 05:17:55 GMT", "2016-09-17 04:17:55 -0100", "Sat Sep 17 05:17:55 2016", "Saturday, 17-Sep-16 05:17:55 GMT" },
    { 1474178244, "Sun, 18 Sep 2016 05:57:24 GMT", "2016-09-18 04:57:24 -0100", "Sun Sep 18 05:57:24 2016", "Sunday, 18-Sep-16 05:57:24 GMT" },
    { 1474267013, "Mon, 19 Sep 2016 06:36:53 GMT", "2016-09-19 05:36:53 -0100", "Mon Sep 19 06:36:53 2016", "Monday, 19-Sep-16 06:36:53 GMT" },
    { 1474355782, "Tue, 20 Sep 2016 07:16:22 GMT", "2016-09-20 06:16:22 -0100", "Tue Sep 20 07:16:22 2016", "Tuesday, 20-Sep-16 07:16:22 GMT" },
    { 1474444551, "Wed, 21 Sep 2016 07:55:51 GMT", "2016-09-21 06:55:51 -0100", "Wed Sep 21 07:55:51 2016", "Wednesday, 21-Sep-16 07:55:51 GMT" },
    { 1474533320, "Thu, 22 Sep 2016 08:35:20 GMT", "2016-09-22 07:35:20 -0100", "Thu Sep 22 08:35:20 2016", "Thursday, 22-Sep-16 08:35:20 GMT" },
    { 1474622089, "Fri, 23 Sep 2016 09:14:49 GMT", "2016-09-23 08:14:49 -0100", "Fri Sep 23 09:14:49 2016", "Friday, 23-Sep-16 09:14:49 GMT" },
    { 1474710858, "Sat, 24 Sep 2016 09:54:18 GMT", "2016-09-24 08:54:18 -0100", "Sat Sep 24 09:54:18 2016", "Saturday, 24-Sep-16 09:54:18 GMT" },
    { 1474799627, "Sun, 25 Sep 2016 10:33:47 GMT", "2016-09-25 09:33:47 -0100", "Sun Sep 25 10:33:47 2016", "Sunday, 25-Sep-16 10:33:47 GMT" },
    { 1474888396, "Mon, 26 Sep 2016 11:13:16 GMT", "2016-09-26 10:13:16 -0100", "Mon Sep 26 11:13:16 2016", "Monday, 26-Sep-16 11:13:16 GMT" },
    { 1474977165, "Tue, 27 Sep 2016 11:52:45 GMT", "2016-09-27 10:52:45 -0100", "Tue Sep 27 11:52:45 2016", "Tuesday, 27-Sep-16 11:52:45 GMT" },
    { 1475065934, "Wed, 28 Sep 2016 12:32:14 GMT", "2016-09-28 11:32:14 -0100", "Wed Sep 28 12:32:14 2016", "Wednesday, 28-Sep-16 12:32:14 GMT" },
    { 1475154703, "Thu, 29 Sep 2016 13:11:43 GMT", "2016-09-29 12:11:43 -0100", "Thu Sep 29 13:11:43 2016", "Thursday, 29-Sep-16 13:11:43 GMT" },
    { 1475243472, "Fri, 30 Sep 2016 13:51:12 GMT", "2016-09-30 12:51:12 -0100", "Fri Sep 30 13:51:12 2016", "Friday, 30-Sep-16 13:51:12 GMT" },
    { 1475332241, "Sat, 01 Oct 2016 14:30:41 GMT", "2016-10-01 13:30:41 -0100", "Sat Oct  1 14:30:41 2016", "Saturday, 01-Oct-16 14:30:41 GMT" },
    { 1475421010, "Sun, 02 Oct 2016 15:10:10 GMT", "2016-10-02 14:10:10 -0100", "Sun Oct  2 15:10:10 2016", "Sunday, 02-Oct-16 15:10:10 GMT" },
    { 1475509779, "Mon, 03 Oct 2016 15:49:39 GMT", "2016-10-03 14:49:39 -0100", "Mon Oct  3 15:49:39 2016", "Monday, 03-Oct-16 15:49:39 GMT" },
    { 1475598548, "Tue, 04 Oct 2016 16:29:08 GMT", "2016-10-04 15:29:08 -0100", "Tue Oct  4 16:29:08 2016", "Tuesday, 04-Oct-16 16:29:08 GMT" },
    { 1475687317, "Wed, 05 Oct 2016 17:08:37 GMT", "2016-10-05 16:08:37 -0100", "Wed Oct  5 17:08:37 2016", "Wednesday, 05-Oct-16 17:08:37 GMT" },
    { 1475776086, "Thu, 06 Oct 2016 17:48:06 GMT", "2016-10-06 16:48:06 -0100", "Thu Oct  6 17:48:06 2016", "Thursday, 06-Oct-16 17:48:06 GMT" },
    { 1475864855, "Fri, 07 Oct 2016 18:27:35 GMT", "2016-10-07 17:27:35 -0100", "Fri Oct  7 18:27:35 2016", "Friday, 07-Oct-16 18:27:35 GMT" },
    { 1475953624, "Sat, 08 Oct 2016 19:07:04 GMT", "2016-10-08 18:07:04 -0100", "Sat Oct  8 19:07:04 2016", "Saturday, 08-Oct-16 19:07:04 GMT" },
    { 1476042393, "Sun, 09 Oct 2016 19:46:33 GMT", "2016-10-09 18:46:33 -0100", "Sun Oct  9 19:46:33 2016", "Sunday, 09-Oct-16 19:46:33 GMT" },
    { 1476131162, "Mon, 10 Oct 2016 20:26:02 GMT", "2016-10-10 19:26:02 -0100", "Mon Oct 10 20:26:02 2016", "Monday, 10-Oct-16 20:26:02 GMT" },
    { 1476219931, "Tue, 11 Oct 2016 21:05:31 GMT", "2016-10-11 20:05:31 -0100", "Tue Oct 11 21:05:31 2016", "Tuesday, 11-Oct-16 21:05:31 GMT" },
    { 1476308700, "Wed, 12 Oct 2016 21:45:00 GMT", "2016-10-12 20:45:00 -0100", "Wed Oct 12 21:45:00 2016", "Wednesday, 12-Oct-16 21:45:00 GMT" },
    { 1476397469, "Thu, 13 Oct 2016 22:24:29 GMT", "2016-10-13 21:24:29 -0100", "Thu Oct 13 22:24:29 2016", "Thursday, 13-Oct-16 22:24:29 GMT" },
    { 1476486238, "Fri, 14 Oct 2016 23:03:58 GMT", "2016-10-14 22:03:58 -0100", "Fri Oct 14 23:03:58 2016", "Friday, 14-Oct-16 23:03:58 GMT" },
  };

int
main()
  {
    ::setenv("TZ", "TEST+01", true);
    ::tzset();
    POSEIDON_TEST_CHECK(::timezone == +3600);
    POSEIDON_TEST_CHECK(::daylight == 0);

    DateTime dt;
    POSEIDON_TEST_CHECK(dt == system_time());
    char temp[64];

    for(const auto& r : tests) {
      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse_rfc1123_partial(r.rfc1123) == ::strlen(r.rfc1123));
      POSEIDON_TEST_CHECK(system_clock::to_time_t(dt.as_system_time()) == r.ts);

      dt.set_system_time({});
      POSEIDON_TEST_CHECK(dt.parse_git_partial(r.git) == ::strlen(r.git));
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
      POSEIDON_TEST_CHECK(dt.parse(r.git) == ::strlen(r.git));
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
      dt.print_git_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.git) == 0);

      ::memset(temp, '*', sizeof(temp));
      dt.print_rfc850_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.rfc850) == 0);

      ::memset(temp, '*', sizeof(temp));
      dt.print_asctime_partial(temp);
      POSEIDON_TEST_CHECK(::strcmp(temp, r.asctime) == 0);
    }
  }
