## Copyright (C) 2024 by LH_Mouse <lh_mouse@126.com>
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

project('poseidon', 'cpp', version: '1.0.alpha-0')

#===========================================================
# List of files
#===========================================================
poseidon_include = [
  'poseidon/fwd.hpp',
  'poseidon/utils.hpp',
  'poseidon/third/openssl_fwd.hpp',
  'poseidon/third/zlib_fwd.hpp',
  'poseidon/third/asan_fwd.hpp',
  'poseidon/static/main_config.hpp',
  'poseidon/static/async_logger.hpp',
  'poseidon/static/timer_driver.hpp',
  'poseidon/static/fiber_scheduler.hpp',
  'poseidon/static/async_task_executor.hpp',
  'poseidon/static/network_driver.hpp',
  'poseidon/base/uuid.hpp',
  'poseidon/base/datetime.hpp',
  'poseidon/base/config_file.hpp',
  'poseidon/base/abstract_timer.hpp',
  'poseidon/base/abstract_async_task.hpp',
  'poseidon/base/abstract_deflator.hpp',
  'poseidon/base/abstract_inflator.hpp',
  'poseidon/fiber/abstract_future.hpp',
  'poseidon/fiber/dns_future.hpp',
  'poseidon/fiber/read_file_future.hpp',
  'poseidon/fiber/abstract_fiber.hpp',
  'poseidon/socket/enums.hpp',
  'poseidon/socket/socket_address.hpp',
  'poseidon/socket/abstract_socket.hpp',
  'poseidon/socket/udp_socket.hpp',
  'poseidon/socket/listen_socket.hpp',
  'poseidon/socket/tcp_socket.hpp',
  'poseidon/socket/ssl_socket.hpp',
  'poseidon/socket/http_server_session.hpp',
  'poseidon/socket/http_client_session.hpp',
  'poseidon/socket/https_server_session.hpp',
  'poseidon/socket/https_client_session.hpp',
  'poseidon/socket/ws_server_session.hpp',
  'poseidon/socket/ws_client_session.hpp',
  'poseidon/socket/wss_server_session.hpp',
  'poseidon/socket/wss_client_session.hpp',
  'poseidon/socket/async_connect.hpp',
  'poseidon/http/http_value.hpp',
  'poseidon/http/http_header_parser.hpp',
  'poseidon/http/http_query_parser.hpp',
  'poseidon/http/http_request_headers.hpp',
  'poseidon/http/http_request_parser.hpp',
  'poseidon/http/http_response_headers.hpp',
  'poseidon/http/http_response_parser.hpp',
  'poseidon/http/websocket_frame_header.hpp',
  'poseidon/http/websocket_frame_parser.hpp',
  'poseidon/http/websocket_deflator.hpp',
  'poseidon/easy/enums.hpp',
  'poseidon/easy/easy_timer.hpp',
  'poseidon/easy/easy_udp_server.hpp',
  'poseidon/easy/easy_udp_client.hpp',
  'poseidon/easy/easy_tcp_server.hpp',
  'poseidon/easy/easy_tcp_client.hpp',
  'poseidon/easy/easy_ssl_server.hpp',
  'poseidon/easy/easy_ssl_client.hpp',
  'poseidon/easy/easy_http_server.hpp',
  'poseidon/easy/easy_http_client.hpp',
  'poseidon/easy/easy_https_server.hpp',
  'poseidon/easy/easy_https_client.hpp',
  'poseidon/easy/easy_ws_server.hpp',
  'poseidon/easy/easy_ws_client.hpp',
  'poseidon/easy/easy_wss_server.hpp',
  'poseidon/easy/easy_wss_client.hpp',
]

poseidon_include_mysql = [
  'poseidon/third/mysql_fwd.hpp',
  'poseidon/mysql/enums.hpp',
  'poseidon/mysql/mysql_table_structure.hpp',
  'poseidon/mysql/mysql_value.hpp',
  'poseidon/mysql/mysql_connection.hpp',
  'poseidon/static/mysql_connector.hpp',
]

poseidon_include_mongodb = [
  'poseidon/third/mongodb_fwd.hpp',
  'poseidon/mongodb/enums.hpp',
  'poseidon/static/mongodb_connector.hpp',
]

poseidon_src = [
  'poseidon/fwd.cpp',
  'poseidon/utils.cpp',
  'poseidon/static/main_config.cpp',
  'poseidon/static/async_logger.cpp',
  'poseidon/static/timer_driver.cpp',
  'poseidon/static/fiber_scheduler.cpp',
  'poseidon/static/async_task_executor.cpp',
  'poseidon/static/network_driver.cpp',
  'poseidon/base/uuid.cpp',
  'poseidon/base/datetime.cpp',
  'poseidon/base/config_file.cpp',
  'poseidon/base/abstract_timer.cpp',
  'poseidon/base/abstract_async_task.cpp',
  'poseidon/base/abstract_deflator.cpp',
  'poseidon/base/abstract_inflator.cpp',
  'poseidon/fiber/abstract_future.cpp',
  'poseidon/fiber/dns_future.cpp',
  'poseidon/fiber/read_file_future.cpp',
  'poseidon/fiber/abstract_fiber.cpp',
  'poseidon/socket/socket_address.cpp',
  'poseidon/socket/abstract_socket.cpp',
  'poseidon/socket/udp_socket.cpp',
  'poseidon/socket/listen_socket.cpp',
  'poseidon/socket/tcp_socket.cpp',
  'poseidon/socket/ssl_socket.cpp',
  'poseidon/socket/http_server_session.cpp',
  'poseidon/socket/http_client_session.cpp',
  'poseidon/socket/https_server_session.cpp',
  'poseidon/socket/https_client_session.cpp',
  'poseidon/socket/ws_server_session.cpp',
  'poseidon/socket/ws_client_session.cpp',
  'poseidon/socket/wss_server_session.cpp',
  'poseidon/socket/wss_client_session.cpp',
  'poseidon/socket/async_connect.cpp',
  'poseidon/http/http_value.cpp',
  'poseidon/http/http_header_parser.cpp',
  'poseidon/http/http_query_parser.cpp',
  'poseidon/http/http_request_headers.cpp',
  'poseidon/http/http_request_parser.cpp',
  'poseidon/http/http_response_headers.cpp',
  'poseidon/http/http_response_parser.cpp',
  'poseidon/http/websocket_deflator.cpp',
  'poseidon/http/websocket_frame_header.cpp',
  'poseidon/http/websocket_frame_parser.cpp',
  'poseidon/easy/easy_timer.cpp',
  'poseidon/easy/easy_udp_server.cpp',
  'poseidon/easy/easy_udp_client.cpp',
  'poseidon/easy/easy_tcp_server.cpp',
  'poseidon/easy/easy_tcp_client.cpp',
  'poseidon/easy/easy_ssl_server.cpp',
  'poseidon/easy/easy_ssl_client.cpp',
  'poseidon/easy/easy_http_server.cpp',
  'poseidon/easy/easy_http_client.cpp',
  'poseidon/easy/easy_https_server.cpp',
  'poseidon/easy/easy_https_client.cpp',
  'poseidon/easy/easy_ws_server.cpp',
  'poseidon/easy/easy_ws_client.cpp',
  'poseidon/easy/easy_wss_server.cpp',
  'poseidon/easy/easy_wss_client.cpp',
]

poseidon_src_mysql = [
  'poseidon/mysql/mysql_table_structure.cpp',
  'poseidon/mysql/mysql_value.cpp',
  'poseidon/mysql/mysql_connection.cpp',
  'poseidon/static/mysql_connector.cpp',
]

poseidon_src_mongodb = [
  'poseidon/static/mongodb_connector.cpp',
]

etc_poseidon = [
  'etc/poseidon/main.conf',
  'etc/poseidon/ssl/test.crt',
  'etc/poseidon/ssl/test.key',
]

example_src = [
  'example/timer.cpp',
  'example/udp_echo_server.cpp',
  'example/tcp_echo_server.cpp',
  'example/ssl_echo_server.cpp',
  'example/http_server.cpp',
  'example/http_client.cpp',
  'example/https_server.cpp',
  'example/https_client.cpp',
  'example/ws_server.cpp',
  'example/ws_client.cpp',
  'example/wss_server.cpp',
  'example/wss_client.cpp',
]

test_src = [
  'test/utils.cpp',
  'test/socket_address.cpp',
  'test/uuid.cpp',
  'test/datetime.cpp',
  'test/http_value.cpp',
  'test/http_header_parser.cpp',
  'test/http_query_parser.cpp',
  'test/websocket_frame_header.cpp',
  'test/websocket_handshake.cpp',
]

test_src_mysql = [
  'test/mysql_table_structure.cpp',
  'test/mysql_value.cpp',
  'test/mysql_connection.cpp',
]

test_src_mongodb = [
]

#===========================================================
# Global configuration
#===========================================================
ver = configuration_data()
ver.set('abi_major', meson.project_version().split('.')[0])
ver.set('abi_minor', meson.project_version().split('.')[1])
ver.set_quoted('abi_string', meson.project_version())

cxx = meson.get_compiler('cpp')
cxx_is_i386 = cxx.compiles('int foo = __i386__;')
cxx_is_amd64 = cxx.compiles('int foo = __amd64__;')
cxx_asm_intel = cxx.compiles('int foo(int a) { return a + 1; }', args: [ '-masm=intel' ])

if cxx_asm_intel
  add_project_arguments('-masm=intel', language: [ 'c', 'cpp' ])
endif

if cxx_is_i386
  add_project_arguments('-msse2', '-mfpmath=sse', language: [ 'c', 'cpp' ])
endif

if get_option('enable-avx') and (cxx_is_i386 or cxx_is_amd64)
  add_project_arguments('-mavx', language: [ 'c', 'cpp' ])
endif

if get_option('enable-debug-checks')
  add_project_arguments('-D_GLIBCXX_DEBUG', '-D_LIBCPP_DEBUG', language: [ 'c', 'cpp' ])
endif

if get_option('enable-sanitizer') == 'address'
  add_project_arguments('-fsanitize=address,undefined', language: [ 'c', 'cpp' ])
endif

if get_option('enable-sanitizer') == 'thread'
  add_project_arguments('-fsanitize=thread', language: [ 'c', 'cpp' ])
endif

add_project_arguments(
    '-Wall', '-Wextra', '-Winvalid-pch',
    '-D_FILE_OFFSET_BITS=64', '-D_POSIX_C_SOURCE=200809', '-D_DEFAULT_SOURCE',
    '-D_DARWIN_C_SOURCE', '-U_FORTIFY_SOURCE', '-fno-stack-protector',
    '-fstrict-aliasing', '-fstrict-overflow', '-fmerge-all-constants',
    '-frounding-math', '-fno-math-errno', '-fno-trapping-math', '-fno-ident',
    '-fno-devirtualize-speculatively', '-fno-semantic-interposition',
    '-Werror=conversion', '-Werror=sign-compare', '-Werror=sign-conversion',
    '-Werror=write-strings', '-Werror=return-type', '-Werror=double-promotion',
    '-Wmissing-declarations', '-Wmissing-field-initializers', '-Wshadow',
    '-Wsuggest-attribute=noreturn', '-Wswitch-enum', '-Wunused-function',
    '-Wunused-label', '-Wunused-local-typedefs', '-Wunused-but-set-variable',
    '-Wunused-but-set-parameter',
    language: [ 'c', 'cpp' ])

add_project_arguments(
    '-std=c++17', '-fno-elide-constructors', '-fvisibility-inlines-hidden',
    '-fnothrow-opt', '-fno-enforce-eh-specs', '-Wzero-as-null-pointer-constant',
    '-Wno-redundant-move', '-Werror=non-virtual-dtor',
    language: [ 'cpp' ])

#===========================================================
# Dependencies
#===========================================================
dep_pthread = dependency('threads')
dep_asteria = dependency('asteria')

dep_libunwind = dependency('libunwind')
dep_dl = dependency('dl')
dep_openssl = dependency('openssl', version: '>= 3.0.0')
dep_zlib = dependency('zlib', version: '>= 1.2.9')
dep_libmagic = dependency('libmagic')
dep_http_parser = cxx.find_library('http_parser')

add_project_dependencies(dep_pthread, dep_asteria, language: 'cpp')

#===========================================================
# Optional dependencies
#===========================================================
dep_mysqlclient = disabler()
dep_mongoc = disabler()

if get_option('enable-mysql')
  dep_mysqlclient = dependency('mysqlclient')
  add_project_arguments('-DPOSEIDON_ENABLE_MYSQL', language: [ 'c', 'cpp' ])

  poseidon_include += poseidon_include_mysql
  poseidon_src += poseidon_src_mysql
  test_src += test_src_mysql
endif

if get_option('enable-mongodb')
  dep_mongoc = dependency('libmongoc-1.0')
  add_project_arguments('-DPOSEIDON_ENABLE_MONGODB', language: [ 'c', 'cpp' ])

  poseidon_include += poseidon_include_mongodb
  poseidon_src += poseidon_src_mongodb
  test_src += test_src_mongodb
endif

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
      dependencies: [ dep_libunwind, dep_openssl, dep_http_parser, dep_zlib,
                      dep_libmagic, dep_mysqlclient, dep_mongoc ],
      soversion: ver.get('abi_major'),
      version: '@0@.@1@.@2@'.format(ver.get('abi_major'), ver.get('abi_minor'), 0),
      install: true)

import('pkgconfig').generate(lib_poseidon,
      name: 'poseidon',
      url: 'https://github.com/lhmouse/poseidon',
      description: 'The Poseidon Server Framework is a modern framework for game '
                   + 'server development, with support for plain TCP, TCP over TLS, '
                   + 'UDP, HTTP, WebSocket (with permessage-deflate), MySQL, MongoDB, '
                   + 'and coroutines.')

#===========================================================
# Rules for 'bin/poseidon'
#===========================================================
executable('poseidon',
           cpp_pch: 'poseidon/xprecompiled.hpp',
           sources: [ 'poseidon/main.cpp' ],
           dependencies: [ dep_dl, dep_openssl, dep_mysqlclient, dep_mongoc ],
           link_with: lib_poseidon,
           install: true)

#===========================================================
# Rules for default configuration files
#===========================================================
install_data(etc_poseidon, install_dir: '.', preserve_path: true)

#===========================================================
# Rules for example libraries
#===========================================================
foreach src: example_src
  name = src.replace('.cpp', '').underscorify()
  shared_module('poseidon-' + name, src,
                link_with: lib_poseidon,
                install: true)
endforeach

#===========================================================
# Rules for tests
#===========================================================
foreach src: test_src
  name = src.replace('.cpp', '').underscorify()
  test = executable(name, src, link_with: lib_poseidon)
  test(name, test)
endforeach
