## Set up project
AC_INIT([Poseidon], [0.1], [lh_mouse@126.com], [poseidon], [https://github.com/lhmouse/poseidon])
AC_CONFIG_SRCDIR([poseidon/src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG([C++])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
LT_INIT([disable-static])

## Check for required libraries
AC_CHECK_LIB([dl], [dlopen], [], [AC_MSG_ERROR(dl library not found)])
AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR(pthread library not found)])
AC_CHECK_LIB([asteria], [_ZN7asteria14Global_ContextD2Ev], [], [AC_MSG_ERROR(Asteria not found)])
AC_CHECK_LIB([ssl], [TLS_method], [], [AC_MSG_ERROR(OpenSSL 1.1 not found)])
AC_CHECK_LIB([crypto], [ERR_error_string], [], [AC_MSG_ERROR(OpenSSL not found)])
AC_CHECK_LIB([z], [deflate], [], [AC_MSG_ERROR(zlib not found)])

## Set up optional features
AC_ARG_ENABLE([debug-checks], AS_HELP_STRING([--enable-debug-checks], [enable assertions]))
AM_CONDITIONAL([enable_debug_checks], [test "${enable_debug_checks}" == "yes"])
AM_COND_IF([enable_debug_checks], [
  AC_DEFINE([_GLIBCXX_DEBUG], [1], [Define to 1 to enable debug checks of libstdc++.])
  AC_DEFINE([_LIBCPP_DEBUG], [1], [Define to 1 to enable debug checks of libc++.])
  AC_DEFINE([_DEBUG], [1], [Define to 1 to enable debug checks of MSVC standard library.])
])

AC_ARG_ENABLE([sanitizer], AS_HELP_STRING([--enable-sanitizer=address|thread], [enable sanitizer (address sanitizer and thread sanitizer cannot be enabled at the same time)]))
AM_CONDITIONAL([enable_address_sanitizer], [test "${enable_sanitizer}" == "address"])
AM_COND_IF([enable_address_sanitizer], [
  AC_CHECK_LIB([asan], [__asan_report_error], [], [AC_MSG_ERROR([address sanitizer not found])])
  AC_DEFINE([POSEIDON_ENABLE_ADDRESS_SANITIZER], [1], [Define to 1 to enable address sanitizer.])
])
AM_CONDITIONAL([enable_thread_sanitizer], [test "${enable_sanitizer}" == "thread"])
AM_COND_IF([enable_thread_sanitizer], [
  AC_CHECK_LIB([tsan], [__tsan_on_report], [], [AC_MSG_ERROR([thread sanitizer not found])])
  AC_DEFINE([POSEIDON_ENABLE_THREAD_SANITIZER], [1], [Define to 1 to enable thread sanitizer.])
])

AC_ARG_ENABLE([mysql], AS_HELP_STRING([--disable-mysql], [disable MySQL support]))
AM_CONDITIONAL([enable_mysql], [test "${enable_mysql}" != "no"])
AM_COND_IF([enable_mysql], [
  AC_CHECK_LIB([mysqlclient], [mysql_real_connect], [], [AC_MSG_ERROR([libmysqlclient not found (pass `--disable-mysql` to disable MySQL)])])
  AC_DEFINE([POSEIDON_ENABLE_MYSQL], [1], [Define to 1 to build the MySQL daemon.])
])

AC_ARG_ENABLE([mongodb], AS_HELP_STRING([--disable-mongodb], [disable MongoDB support]))
AM_CONDITIONAL([enable_mongodb], [test "${enable_mongodb}" != "no"])
AM_COND_IF([enable_mongodb], [
  AC_CHECK_LIB([bson-1.0], [bson_new], [], [AC_MSG_ERROR([libbson-1.0 not found (pass `--disable-mongodb` to disable MongoDB)])])
  AC_CHECK_LIB([mongoc-1.0], [mongoc_uri_new_for_host_port], [], [AC_MSG_ERROR([libmongoc-1.0 not found (pass `--disable-mongodb` to disable MongoDB)])])
  AC_DEFINE([POSEIDON_ENABLE_MONGODB], [1], [Define to 1 to build the MongoDB daemon.])
])

AC_ARG_ENABLE([magic], AS_HELP_STRING([--disable-magic], [disable magic number recognition]))
AM_CONDITIONAL([enable_magic], [test "${enable_magic}" != "no"])
AM_COND_IF([enable_magic], [
  AC_CHECK_LIB([magic], [magic_open], [], [AC_MSG_ERROR([libmagic not found (pass `--disable-magic` to disable libmagic)])])
  AC_DEFINE([POSEIDON_ENABLE_MAGIC], [1], [Define to 1 to build the Magic daemon.])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
