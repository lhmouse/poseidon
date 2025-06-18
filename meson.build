## Copyright (C) 2024-2025 by LH_Mouse <lh_mouse@126.com>
##
## Permission to use, copy, modify, and/or distribute this
## software for any purpose with or without fee is hereby granted.
##
## THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
## WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
## THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
## CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
## LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
## NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
## CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

project('poseidon',
        'cpp',
        version: '0.3.alpha',
        license: 'BSD-3-Clause',
        default_options: [ 'cpp_std=c++17', 'warning_level=2' ],
        meson_version: '>= 1.3.0')

#===========================================================
# List of files
#===========================================================
poseidon_include = [
  'poseidon/fwd.hpp', 'poseidon/utils.hpp', 'poseidon/third/openssl_fwd.hpp',
  'poseidon/third/zlib_fwd.hpp', 'poseidon/static/main_config.hpp',
  'poseidon/static/logger.hpp', 'poseidon/static/timer_driver.hpp',
  'poseidon/static/fiber_scheduler.hpp', 'poseidon/static/task_executor.hpp',
  'poseidon/static/network_driver.hpp', 'poseidon/base/uuid.hpp',
  'poseidon/base/datetime.hpp', 'poseidon/base/config_file.hpp',
  'poseidon/base/abstract_timer.hpp', 'poseidon/base/abstract_task.hpp',
  'poseidon/base/abstract_deflator.hpp', 'poseidon/base/abstract_inflator.hpp',
  'poseidon/fiber/abstract_fiber.hpp', 'poseidon/fiber/abstract_future.hpp',
  'poseidon/fiber/dns_query_future.hpp', 'poseidon/fiber/read_file_future.hpp',
  'poseidon/socket/enums.hpp', 'poseidon/socket/ipv6_address.hpp',
  'poseidon/socket/abstract_socket.hpp', 'poseidon/socket/udp_socket.hpp',
  'poseidon/socket/tcp_acceptor.hpp', 'poseidon/socket/tcp_socket.hpp',
  'poseidon/socket/ssl_socket.hpp', 'poseidon/socket/http_server_session.hpp',
  'poseidon/socket/http_client_session.hpp', 'poseidon/socket/https_server_session.hpp',
  'poseidon/socket/https_client_session.hpp', 'poseidon/socket/ws_server_session.hpp',
  'poseidon/socket/ws_client_session.hpp', 'poseidon/socket/wss_server_session.hpp',
  'poseidon/socket/wss_client_session.hpp', 'poseidon/socket/dns_connect_task.hpp',
  'poseidon/http/enums.hpp', 'poseidon/http/http_value.hpp',
  'poseidon/http/http_field_name.hpp', 'poseidon/http/http_header_parser.hpp',
  'poseidon/http/http_query_parser.hpp', 'poseidon/http/http_request_headers.hpp',
  'poseidon/http/http_request_parser.hpp', 'poseidon/http/http_response_headers.hpp',
  'poseidon/http/http_response_parser.hpp', 'poseidon/http/websocket_frame_header.hpp',
  'poseidon/http/websocket_frame_parser.hpp', 'poseidon/http/websocket_deflator.hpp',
  'poseidon/easy/enums.hpp', 'poseidon/easy/easy_timer.hpp',
  'poseidon/easy/easy_udp_server.hpp', 'poseidon/easy/easy_udp_client.hpp',
  'poseidon/easy/easy_tcp_server.hpp', 'poseidon/easy/easy_http_server.hpp',
  'poseidon/easy/easy_hws_server.hpp', 'poseidon/easy/easy_ws_server.hpp',
  'poseidon/easy/easy_ssl_server.hpp', 'poseidon/easy/easy_https_server.hpp',
  'poseidon/easy/easy_hwss_server.hpp', 'poseidon/easy/easy_wss_server.hpp',
  'poseidon/easy/easy_tcp_client.hpp', 'poseidon/easy/easy_http_client.hpp',
  'poseidon/easy/easy_ws_client.hpp', 'poseidon/easy/easy_ssl_client.hpp',
  'poseidon/easy/easy_https_client.hpp', 'poseidon/easy/easy_wss_client.hpp',
  'poseidon/third/mysql_fwd.hpp', 'poseidon/static/mysql_connector.hpp',
  'poseidon/mysql/enums.hpp', 'poseidon/mysql/mysql_table_structure.hpp',
  'poseidon/mysql/mysql_value.hpp', 'poseidon/mysql/mysql_table_column.hpp',
  'poseidon/mysql/mysql_table_index.hpp', 'poseidon/mysql/mysql_connection.hpp',
  'poseidon/fiber/mysql_query_future.hpp', 'poseidon/fiber/mysql_check_table_future.hpp',
  'poseidon/third/mongo_fwd.hpp', 'poseidon/static/mongo_connector.hpp',
  'poseidon/mongo/enums.hpp', 'poseidon/mongo/mongo_value.hpp',
  'poseidon/mongo/mongo_connection.hpp', 'poseidon/fiber/mongo_query_future.hpp',
  'poseidon/third/redis_fwd.hpp', 'poseidon/static/redis_connector.hpp',
  'poseidon/redis/enums.hpp', 'poseidon/redis/redis_value.hpp',
  'poseidon/redis/redis_connection.hpp', 'poseidon/fiber/redis_query_future.hpp',
  'poseidon/fiber/redis_scan_and_get_future.hpp' ]

poseidon_src = [
  'poseidon/fwd.cpp', 'poseidon/utils.cpp', 'poseidon/static/main_config.cpp',
  'poseidon/static/logger.cpp', 'poseidon/static/timer_driver.cpp',
  'poseidon/static/fiber_scheduler.cpp', 'poseidon/static/task_executor.cpp',
  'poseidon/static/network_driver.cpp', 'poseidon/base/uuid.cpp',
  'poseidon/base/datetime.cpp', 'poseidon/base/config_file.cpp',
  'poseidon/base/abstract_timer.cpp', 'poseidon/base/abstract_task.cpp',
  'poseidon/base/abstract_deflator.cpp', 'poseidon/base/abstract_inflator.cpp',
  'poseidon/fiber/abstract_fiber.cpp', 'poseidon/fiber/abstract_future.cpp',
  'poseidon/fiber/dns_query_future.cpp', 'poseidon/fiber/read_file_future.cpp',
  'poseidon/socket/ipv6_address.cpp', 'poseidon/socket/abstract_socket.cpp',
  'poseidon/socket/udp_socket.cpp', 'poseidon/socket/tcp_acceptor.cpp',
  'poseidon/socket/tcp_socket.cpp', 'poseidon/socket/ssl_socket.cpp',
  'poseidon/socket/http_server_session.cpp', 'poseidon/socket/http_client_session.cpp',
  'poseidon/socket/https_server_session.cpp', 'poseidon/socket/https_client_session.cpp',
  'poseidon/socket/ws_server_session.cpp', 'poseidon/socket/ws_client_session.cpp',
  'poseidon/socket/wss_server_session.cpp', 'poseidon/socket/wss_client_session.cpp',
  'poseidon/socket/dns_connect_task.cpp', 'poseidon/http/http_value.cpp',
  'poseidon/http/http_field_name.cpp', 'poseidon/http/http_header_parser.cpp',
  'poseidon/http/http_query_parser.cpp', 'poseidon/http/http_request_headers.cpp',
  'poseidon/http/http_request_parser.cpp', 'poseidon/http/http_response_headers.cpp',
  'poseidon/http/http_response_parser.cpp', 'poseidon/http/websocket_deflator.cpp',
  'poseidon/http/websocket_frame_header.cpp', 'poseidon/http/websocket_frame_parser.cpp',
  'poseidon/easy/easy_timer.cpp', 'poseidon/easy/easy_udp_server.cpp',
  'poseidon/easy/easy_udp_client.cpp', 'poseidon/easy/easy_tcp_server.cpp',
  'poseidon/easy/easy_http_server.cpp', 'poseidon/easy/easy_hws_server.cpp',
  'poseidon/easy/easy_ws_server.cpp', 'poseidon/easy/easy_ssl_server.cpp',
  'poseidon/easy/easy_https_server.cpp', 'poseidon/easy/easy_hwss_server.cpp',
  'poseidon/easy/easy_wss_server.cpp', 'poseidon/easy/easy_tcp_client.cpp',
  'poseidon/easy/easy_http_client.cpp', 'poseidon/easy/easy_ws_client.cpp',
  'poseidon/easy/easy_ssl_client.cpp', 'poseidon/easy/easy_https_client.cpp',
  'poseidon/easy/easy_wss_client.cpp',
  'poseidon/static/mysql_connector.cpp', 'poseidon/mysql/mysql_table_structure.cpp',
  'poseidon/mysql/mysql_value.cpp', 'poseidon/mysql/mysql_table_column.cpp',
  'poseidon/mysql/mysql_table_index.cpp', 'poseidon/mysql/mysql_connection.cpp',
  'poseidon/fiber/mysql_query_future.cpp', 'poseidon/fiber/mysql_check_table_future.cpp',
  'poseidon/static/mongo_connector.cpp', 'poseidon/mongo/mongo_value.cpp',
  'poseidon/mongo/mongo_connection.cpp', 'poseidon/fiber/mongo_query_future.cpp',
  'poseidon/static/redis_connector.cpp', 'poseidon/redis/redis_value.cpp',
  'poseidon/redis/redis_connection.cpp', 'poseidon/fiber/redis_query_future.cpp',
  'poseidon/fiber/redis_scan_and_get_future.cpp' ]

poseidon_etc = [
  'etc/poseidon/main.conf', 'etc/poseidon/ssl/test.crt', 'etc/poseidon/ssl/test.key' ]

example_src = [
  'example/udp_echo_server.cpp', 'example/tcp_echo_server.cpp',
  'example/ssl_echo_server.cpp',
  'example/hws_server.cpp', 'example/http_client.cpp', 'example/ws_client.cpp',
  'example/hwss_server.cpp', 'example/https_client.cpp', 'example/wss_client.cpp' ]

test_src = [
  'test/utils.cpp', 'test/ipv6_address.cpp', 'test/uuid.cpp', 'test/datetime.cpp',
  'test/http_value.cpp', 'test/http_field_name.cpp', 'test/http_header_parser.cpp',
  'test/http_query_parser.cpp', 'test/websocket_frame_header.cpp', 'test/mysql_value.cpp',
  'test/websocket_handshake.cpp', 'test/mysql_connection.cpp', 'test/mongo_value.cpp',
  'test/mongo_connection.cpp', 'test/redis_value.cpp', 'test/redis_connection.cpp' ]

#===========================================================
# Global configuration
#===========================================================
ver = configuration_data()
ver.set('abi_major', meson.project_version().split('.')[0])
ver.set('abi_minor', meson.project_version().split('.')[1])
ver.set_quoted('abi_string', meson.project_version())

cxx = meson.get_compiler('cpp')
cxx_is_i386 = cxx.has_define('__i386__')
cxx_is_amd64 = cxx.has_define('__amd64__')

add_project_arguments(
    '-Winvalid-pch', '-D_FILE_OFFSET_BITS=64', '-D_POSIX_C_SOURCE=200809',
    '-D_DEFAULT_SOURCE', '-D_DARWIN_C_SOURCE',
    cxx.get_supported_arguments(
      '-fno-elide-constructors', '-fvisibility-inlines-hidden', '-fstrict-aliasing',
      '-fno-stack-protector', '-fstrict-overflow', '-fmerge-all-constants',
      '-frounding-math', '-fno-math-errno', '-fno-ident', '-fno-trapping-math',
      '-fno-devirtualize-speculatively', '-fno-semantic-interposition', '-fno-plt',
      '-fno-omit-frame-pointer', '-momit-leaf-frame-pointer', '-masm=intel',
      '-mno-outline-atomics', '-Wno-option-ignored', '-Wno-unknown-warning-option',
      '-Werror=conversion', '-Werror=sign-compare', '-Werror=sign-conversion',
      '-Werror=write-strings', '-Werror=return-type', '-Werror=double-promotion',
      '-Werror=non-virtual-dtor', '-Wzero-as-null-pointer-constant',
      '-Wmissing-declarations', '-Wmissing-field-initializers', '-Wshadow',
      '-Wsuggest-attribute=noreturn', '-Wswitch-enum', '-Wunused-function',
      '-Wunused-label', '-Wunused-local-typedefs', '-Wunused-but-set-variable',
      '-Wunused-but-set-parameter', '-Wno-array-bounds', '-Wno-stringop-overflow',
      '-Wno-redundant-move', '-Wno-unqualified-std-cast-call', '-Wno-string-plus-int',
      '-Wno-bitwise-instead-of-logical', '-Wno-potentially-evaluated-expression',
      '-Wno-unused-private-field', '-Wno-missing-braces'),
    language: 'cpp')

if get_option('enable-avx') and (cxx_is_i386 or cxx_is_amd64)
  add_project_arguments('-mavx', language: 'cpp')
endif

if get_option('enable-debug-checks')
  add_project_arguments('-D_GLIBCXX_DEBUG', '-D_LIBCPP_DEBUG', language: 'cpp')
endif

#===========================================================
# Dependencies
#===========================================================
dep_pthread = dependency('threads')
dep_asteria = dependency('asteria')
dep_taxon = dependency('taxon')
dep_libunwind = dependency('libunwind')
dep_openssl = dependency('openssl', version: '>= 3.0.0')
dep_zlib = dependency('zlib', version: '>= 1.2.9')
dep_http_parser = dependency('http-parser')

dep_mysqlclient = dependency('mysqlclient')
dep_bson = dependency('libbson-1.0')
dep_mongoc = dependency('libmongoc-1.0')
dep_hiredis = dependency('hiredis')

#===========================================================
# Rules for headers
#===========================================================
configure_file(input: 'poseidon/version.h.in',
               configuration: ver,
               output: 'version.h',
               install: false)

install_headers(poseidon_include, preserve_path: true)
install_headers(meson.current_build_dir() / 'version.h', subdir: 'poseidon')

#===========================================================
# Rules for 'lib/libposeidon.so'
#===========================================================
lib_poseidon = shared_library('poseidon',
      cpp_pch: 'poseidon/xprecompiled.hpp',
      sources: [ poseidon_src ],
      dependencies: [ dep_pthread, dep_asteria, dep_taxon, dep_libunwind,
                      dep_openssl, dep_zlib, dep_http_parser,
                      dep_mysqlclient, dep_bson, dep_mongoc, dep_hiredis ],
      soversion: ver.get('abi_major'),
      version: '.'.join([ ver.get('abi_major'), ver.get('abi_minor'), '0' ]),
      install: true)

import('pkgconfig').generate(lib_poseidon,
      name: 'poseidon',
      url: 'https://github.com/lhmouse/poseidon',
      description: 'The Poseidon Server Framework is a modern framework for '
                   + 'game server development, with support for plain TCP, '
                   + 'TCP over SSL/TLS, UDP, HTTP, WebSocket, MySQL, MongoDB, '
                   + 'Redis, and coroutines.')

#===========================================================
# Rules for 'bin/poseidon'
#===========================================================
executable('poseidon', 'poseidon/main.cpp',
      cpp_pch: 'poseidon/xprecompiled.hpp',
      dependencies: [ dep_pthread, dep_asteria, dep_taxon ],
      link_with: lib_poseidon,
      install: true)

#===========================================================
# Rules for default configuration files
#===========================================================
install_data(poseidon_etc, install_dir: '.', preserve_path: true)

#===========================================================
# Rules for example libraries
#===========================================================
foreach src : example_src
  shared_module('poseidon-' + src.replace('.cpp', '').underscorify(), src,
                link_with: lib_poseidon,
                install: true)
endforeach

#===========================================================
# Rules for tests
#===========================================================
foreach src : test_src
  basename = src.replace('test/', '')
  test_deps = [ dep_pthread, dep_asteria, dep_taxon ]

  if basename.startswith('mysql_')
    test_deps += [ dep_mysqlclient ]
  endif

  if basename.startswith('mongo_')
    test_deps += [ dep_bson, dep_mongoc ]
  endif

  if basename.startswith('redis_')
    test_deps += [ dep_hiredis ]
  endif

  test_exe = executable(src.underscorify(), src,
      dependencies: test_deps,
      link_with: lib_poseidon,
      build_by_default: false,
      install: false)

  test('..' / src, test_exe)
endforeach
