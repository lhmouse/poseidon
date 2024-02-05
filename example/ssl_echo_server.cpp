// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.hpp"
#include "../poseidon/easy/easy_ssl_server.hpp"
#include "../poseidon/easy/enums.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_SSL_Server my_server;

void
event_callback(shptrR<SSL_Socket> socket, Abstract_Fiber& /*fiber*/,
               Easy_Stream_Event event, linear_buffer& data, int code)
  {
    switch(event) {
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

int
start_server()
  {
    my_server.start(sref("[::]:3803"));
    POSEIDON_LOG_WARN(("example SSL server started: bind = $1"),
                      my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_SSL_Server my_server(event_callback);
int dummy = start_server();

}  // namespace
