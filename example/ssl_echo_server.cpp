// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_ssl_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static constexpr char bind_addr[] = "[::]:3803";
static Easy_SSL_Server my_server;

static void
my_server_callback(const shptr<SSL_Socket>& socket, Abstract_Fiber& fiber,
                   Easy_Stream_Event event, linear_buffer& data, int code)
  {
    (void) fiber;

    switch(event)
      {
      case easy_stream_open:
        POSEIDON_LOG_WARN(("example SSL server accepted connection: $1"),
                          socket->remote_address());
        break;

      case easy_stream_data:
        POSEIDON_LOG_WARN(("example SSL server received data (eof = $1): $2"),
                          code, data);
        socket->ssl_send(data);
        data.clear();
        break;

      case easy_stream_close:
        POSEIDON_LOG_WARN(("example SSL server closed connection: (errno = $1) $2"),
                          code, data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  }

void
poseidon_module_main()
  {
    my_server.start(&bind_addr, my_server_callback);
    POSEIDON_LOG_WARN(("example SSL server started: $1"), bind_addr);
  }
