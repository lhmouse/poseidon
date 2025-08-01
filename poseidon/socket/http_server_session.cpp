// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_server_session.hpp"
#include "../http/http_header_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Server_Session::
HTTP_Server_Session()
  {
  }

HTTP_Server_Session::
~HTTP_Server_Session()
  {
  }

void
HTTP_Server_Session::
do_on_tcp_stream(linear_buffer& data, bool eof)
  {
    if(this->m_upgrade_ack.load()) {
      this->do_on_http_upgraded_stream(data, eof);
      return;
    }

    for(;;) {
      // Check whether the connection has switched to another protocol.
      if(this->m_upgrade_ack.load()) {
        this->m_req_parser.deallocate();
        this->do_on_http_upgraded_stream(data, eof);
        return;
      }

      // If something has gone wrong, ignore further incoming data.
      if(this->m_req_parser.error()) {
        data.clear();
        return;
      }

      if(!this->m_req_parser.headers_complete()) {
        this->m_req_parser.parse_headers_from_stream(data, eof);

        if(this->m_req_parser.error()) {
          data.clear();
          this->do_on_http_request_error(this->m_req_parser.headers().method == http_HEAD,
                                         this->m_req_parser.http_status_from_error());
          return;
        }

        if(!this->m_req_parser.headers_complete())
          return;

        // Check headers.
        auto& headers = this->m_req_parser.mut_headers();

        if(headers.is_proxy == false)
          headers.is_ssl = false;

        if(headers.raw_host.empty()) {
          data.clear();
          this->do_on_http_request_error(this->m_req_parser.headers().method == http_HEAD,
                                         http_status_bad_request);
          return;
        }

        bool fin = this->m_req_parser.should_close_after_payload();
        auto payload_type = this->do_on_http_request_headers(headers, fin);
        switch(payload_type)
          {
          case http_payload_normal:
          case http_payload_empty:
            break;

          case http_payload_connect:
            this->m_upgrade_ack.store(true);
            this->m_req_parser.deallocate();
            this->do_on_http_upgraded_stream(data, eof);
            return;

          default:
            POSEIDON_THROW((
                "Invalid payload type `$3` returned from `do_on_http_request_headers()`",
                "[HTTP server session `$1` (class `$2`)]"),
                this, typeid(*this), payload_type);
          }
      }

      if(!this->m_req_parser.payload_complete()) {
        this->m_req_parser.parse_payload_from_stream(data, eof);

        if(this->m_req_parser.error()) {
          data.clear();
          this->do_on_http_request_error(this->m_req_parser.headers().method == http_HEAD,
                                         this->m_req_parser.http_status_from_error());
          return;
        }

        this->do_on_http_request_payload_stream(this->m_req_parser.mut_payload());

        if(!this->m_req_parser.payload_complete())
          return;

        // The message is complete now.
        this->do_on_http_request_finish(move(this->m_req_parser.mut_headers()),
                                        move(this->m_req_parser.mut_payload()),
                                        this->m_req_parser.should_close_after_payload());
      }

      this->m_req_parser.next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

HTTP_Payload_Type
HTTP_Server_Session::
do_on_http_request_headers(HTTP_C_Headers& req, bool /*eot*/)
  {
    if(req.is_proxy) {
      // Reject proxy requests.
      this->do_on_http_request_error(false, http_status_forbidden);
      return http_payload_normal;
    }

    POSEIDON_LOG_DEBUG((
        "HTTP server received request: $3 $4",
        "[HTTP server session `$1` (class `$2`)]"),
        this, typeid(*this), req.method_str, req.raw_path);

    // The default handler doesn't handle Upgrade requests.
    return http_payload_normal;
  }

void
HTTP_Server_Session::
do_on_http_request_payload_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_http_request_finish()`,
    // but perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_req_parser.max_content_length())
      POSEIDON_THROW((
          "HTTP request payload too large: `$3` > `$4`",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_req_parser.max_content_length());
  }

__attribute__((__noreturn__))
void
HTTP_Server_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_http_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTP server session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTP_Server_Session::
do_http_raw_response(const HTTP_S_Headers& resp, chars_view data)
  {
    // Compose the message and send it as a whole.
    tinyfmt_ln fmt;
    resp.encode(fmt);
    fmt.putn(data.p, data.n);
    bool sent = this->tcp_send(fmt);

    // For server sessions, a status of 101 indicates that the server will switch
    // to another protocol after this message. The client might have sent more
    // data before this, which would violate RFC 6455 anyway, so we don't care.
    if(resp.status == 101)
      this->m_upgrade_ack.store(true);

    // If `Connection:` contains `close`, the connection should be closed.
    HTTP_Header_Parser hparser;
    for(const auto& hr : resp.headers)
      if(hr.first == "Connection") {
        hparser.reload(hr.second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "close")
            this->tcp_shut_down();
      }

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTP_Server_Session::
http_response_headers_only(HTTP_S_Headers&& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->do_http_raw_response(resp, "");
  }

bool
HTTP_Server_Session::
http_response(bool method_was_head, HTTP_S_Headers&& resp, chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Some responses are required to have no payload payload and require no
    // `Content-Length` header.
    if((resp.status <= 199) || (resp.status == 204) || (resp.status == 304))
      return this->do_http_raw_response(resp, "");

    // Otherwise, a `Content-Length` is required; otherwise the response would
    // be interpreted as terminating by closure ofthe connection.
    resp.headers.emplace_back(&"Content-Length", static_cast<int64_t>(data.n));

    return this->do_http_raw_response(resp, method_was_head ? "" : data);
  }

bool
HTTP_Server_Session::
http_chunked_response_start(HTTP_S_Headers&& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Write a chunked header.
    resp.headers.emplace_back(&"Transfer-Encoding", &"chunked");

    return this->do_http_raw_response(resp, "");
  }

bool
HTTP_Server_Session::
http_chunked_response_send(chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Ignore empty chunks, which would mark the end of the payload.
    if(data.n == 0)
      return this->tcp_send("");

    // Compose a chunk and send it as a whole. The length of this chunk is
    // written as a hexadecimal integer without the `0x` prefix.
    tinyfmt_ln fmt;
    ::rocket::ascii_numput nump;
    nump.put_XU(data.n);
    fmt.putn(nump.data() + 2, nump.size() - 2);
    fmt << "\r\n";
    fmt.putn(data.p, data.n);
    fmt << "\r\n";
    return this->tcp_send(fmt);
  }

bool
HTTP_Server_Session::
http_chunked_response_finish()
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->tcp_send("0\r\n\r\n");
  }

bool
HTTP_Server_Session::
http_shut_down(HTTP_Status status)
  noexcept
  {
    if(this->m_upgrade_ack.load())
      return this->quick_shut_down();

    bool succ = false;
    try {
      // Compose a default page.
      HTTP_S_Headers resp;
      resp.status = status;
      resp.reason = ::rocket::sref(::http_status_str(static_cast<::http_status>(status)));
      resp.headers.emplace_back(&"Content-Type", &"text/html");
      resp.headers.emplace_back(&"Connection", &"close");

      static constexpr char default_page[] =
          "<html>"
          "<head><title>$1 $2</title></head>"
          "<body><h1>$1 $2</h1></body>"
          "</html>";

      tinyfmt_ln fmt;
      format(fmt, default_page, status, resp.reason);

      succ = this->do_http_raw_response(resp, fmt.get_buffer());
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send default page: $3",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this), stdex);
    }
    succ |= this->tcp_shut_down();
    return succ;
  }

bool
HTTP_Server_Session::
http_shut_down(int status)
  noexcept
  {
    HTTP_Status real_status = http_status_bad_request;
    if((status >= 200) && (status <= 599))
      real_status = static_cast<HTTP_Status>(real_status);
    return this->http_shut_down(real_status);
  }

}  // namespace poseidon
