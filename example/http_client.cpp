// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_http_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static Easy_HTTP_Client my_client(
  // callback
  *[](shptrR<HTTP_Client_Session> session, Abstract_Fiber& fiber, Easy_HTTP_Event event,
      HTTP_Response_Headers&& resp, linear_buffer&& data)
  {
    (void) fiber;

    switch(event)
      {
      case easy_http_open:
        POSEIDON_LOG_ERROR(("example HTTP client connected to server: $1"),
                           session->remote_address());
        break;

      case easy_http_message:
        {
          POSEIDON_LOG_ERROR(("example HTTP client received response: $1 $2"),
                             resp.status, resp.reason);
          for(const auto& pair : resp.headers)
            POSEIDON_LOG_ERROR(("  $1 --> $2"), pair.first, pair.second);

          POSEIDON_LOG_ERROR(("    payload ($1 bytes):\n$2"), data.size(), data);
        }
        break;

      case easy_http_close:
        POSEIDON_LOG_ERROR(("example HTTP client shutdown: $1"), data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  });

static Easy_Timer my_timer(
  // callback
  *[](shptrR<Abstract_Timer> timer, Abstract_Fiber& fiber, steady_time now)
  {
    (void) timer;
    (void) fiber;
    (void) now;

    static uint32_t state;
    static shptr<HTTP_Client_Session> my_client_session;

    if(my_client_session == nullptr)
      state = 0;
    else
      state ++;

    switch(state)
      {
      case 0:
        {
          cow_string addr = &"www.example.org";
          my_client_session = my_client.connect(addr);
          POSEIDON_LOG_ERROR(("example HTTP client connecting: addr = $1"), addr);
        }
        break;

      case 1:
        {
          HTTP_Request_Headers req;
          req.method = http_GET;
          req.uri_path = &"/";
          req.headers.emplace_back(&"Connection", &"keep-alive");
          my_client_session->http_request(move(req), "");
          POSEIDON_LOG_ERROR(("example HTTP client: $1 $2"), req.method, req.uri_path);
        }
        break;

      case 2:
        {
          HTTP_Request_Headers req;
          req.method = http_POST;
          req.uri_path = &"/";
          my_client_session->http_request(move(req), "testdata");
          POSEIDON_LOG_ERROR(("example HTTP client: $1 $2"), req.method, req.uri_path);
        }
        break;

      case 3:
        {
          HTTP_Request_Headers req;
          req.method = http_DELETE;
          req.uri_path = &"/";
          my_client_session->http_request(move(req), "");
          POSEIDON_LOG_ERROR(("example HTTP client: $1 $2"), req.method, req.uri_path);
        }
        break;

      default:
        POSEIDON_LOG_ERROR(("example HTTP client shutting down"));
        my_client_session->quick_close();
        my_client_session.reset();
      }
  });

void
poseidon_module_main()
  {
    my_timer.start(1s, 2s);
  }
