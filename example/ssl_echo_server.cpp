// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_ssl_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static Easy_SSL_Server my_server(
  // callback
  *[](shptrR<SSL_Socket> socket, Abstract_Fiber& fiber, Easy_Stream_Event event,
      linear_buffer& data, int code)
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
  });

void
poseidon_module_main()
  {
    static constexpr char bind_addr[] = "[::]:3803";
    my_server.start(&bind_addr);
    POSEIDON_LOG_WARN(("example SSL server started: $1"), bind_addr);
  }
