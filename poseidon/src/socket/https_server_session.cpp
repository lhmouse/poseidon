// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../socket/https_server_session.hpp"
#include "../../http/http_header_parser.hpp"
#include "../../utils.hpp"
namespace poseidon {

HTTPS_Server_Session::
HTTPS_Server_Session()
  {
  }

HTTPS_Server_Session::
~HTTPS_Server_Session()
  {
  }

void
HTTPS_Server_Session::
do_on_ssl_stream(linear_buffer& data, bool eof)
  {
    if(this->m_upgrade_ack.load()) {
      this->do_on_https_upgraded_stream(data, eof);
      return;
    }

    for(;;) {
      // Check whether the connection has switched to another protocol.
      if(this->m_upgrade_ack.load()) {
        this->m_req_parser.deallocate();
        this->do_on_https_upgraded_stream(data, eof);
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
          this->do_on_https_request_error(this->m_req_parser.headers().method,
                                          this->m_req_parser.http_status_from_error());
          return;
        }

        if(!this->m_req_parser.headers_complete())
          return;

        // Check headers.
        auto& headers = this->m_req_parser.mut_headers();

        if(headers.is_proxy == false)
          headers.is_ssl = true;

        if(headers.raw_host.empty()) {
          data.clear();
          this->do_on_https_request_error(this->m_req_parser.headers().method,
                                          http_status_bad_request);
          return;
        }

        bool fin = this->m_req_parser.should_close_after_content();
        this->do_on_https_request_headers(headers, fin);
      }

      if(!this->m_req_parser.content_complete()) {
        this->m_req_parser.parse_content_from_stream(data, eof);

        if(this->m_req_parser.error()) {
          data.clear();
          this->do_on_https_request_error(this->m_req_parser.headers().method,
                                          this->m_req_parser.http_status_from_error());
          return;
        }

        this->do_on_https_request_content_stream(this->m_req_parser.mut_content());

        if(!this->m_req_parser.content_complete())
          return;

        // The message is complete now.
        this->do_on_https_request_finish(move(this->m_req_parser.mut_headers()),
                                         move(this->m_req_parser.mut_content()),
                                         this->m_req_parser.should_close_after_content());
      }

      this->m_req_parser.next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

void
HTTPS_Server_Session::
do_on_https_request_headers(HTTP_Request_Headers& req, bool /*eot*/)
  {
    if(req.is_proxy) {
      // Reject proxy requests.
      this->do_on_https_request_error(req.method, http_status_forbidden);
      return;
    }

    POSEIDON_LOG_DEBUG((
        "HTTPS server received request: $3 $4",
        "[HTTPS server session `$1` (class `$2`)]"),
        this, typeid(*this), req.method, req.raw_path);
  }

void
HTTPS_Server_Session::
do_on_https_request_content_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_https_request_finish()`,
    // but perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_req_parser.max_content_length())
      POSEIDON_THROW((
          "HTTP request content too large: `$3` > `$4`",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_req_parser.max_content_length());
  }

__attribute__((__noreturn__))
void
HTTPS_Server_Session::
do_on_https_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_https_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTPS server session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTPS_Server_Session::
https_response(HTTP_Method method, HTTP_Response_Headers& resp, chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    tinyfmt_ln fmt;
    HTTP_Header_Parser hparser;
    bool connection_close = false;

    // Check for `Connection: close`. Erase `Transfer-Encoding:` and
    // `Content-Length:` headers, if any.
    for(size_t index = 0;  index != resp.headers.size();  index ++)
      if(resp.headers.at(index).first == "Transfer-Encoding")
        resp.headers.erase(index --);
      else if(resp.headers.at(index).first == "Content-Length")
        resp.headers.erase(index --);
      else if(resp.headers.at(index).first == "Connection") {
        hparser.reload(resp.headers.at(index).second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "close")
            connection_close = true;
      }

    // For 2xx responses to CONNECT, and for 1xx and 204 responses, there must be
    // neither `Content-Length:` nor a response body. For 304 responses and
    // responses to HEAD requests, there must be no response body, but
    // `Content-Length:` may exist.
    if(((method == http_method_CONNECT) && (resp.status >= 200) && (resp.status <= 299))
        || ((resp.status >= 100) && (resp.status <= 199))
        || (resp.status == 204))
      resp.encode(fmt);
    else {
      resp.headers.emplace_back(&"Content-Length", static_cast<int64_t>(data.n));
      resp.encode(fmt);
      if(!((resp.status == 304) || (method == http_method_HEAD)))
        fmt.putn(data.p, data.n);
    }

    bool sent = this->ssl_send(fmt);
    if(connection_close)
      this->ssl_shut_down();

    // Check whether the connection should switch to another protocol.
    if(((method == http_method_CONNECT) && (resp.status >= 200) && (resp.status <= 299))
        || (resp.status == 101))
      this->m_upgrade_ack.store(true);

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTPS_Server_Session::
https_chunked_response_start(HTTP_Method method, HTTP_Response_Headers& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    if(((method == http_method_CONNECT) && (resp.status >= 200) && (resp.status <= 299))
        || ((resp.status >= 100) && (resp.status <= 199))
        || (resp.status == 204)
        || (resp.status == 304)
        || (method == http_method_HEAD))
      POSEIDON_THROW((
          "Message body not allowed for this response; call `https_response()` instead",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    tinyfmt_ln fmt;

    // Erase `Transfer-Encoding:` headers, if any. `Content-Length:` may be useful
    // so it's left intact, despite violation of the specification.
    for(size_t index = 0;  index != resp.headers.size();  index ++)
      if(resp.headers.at(index).first == "Transfer-Encoding")
        resp.headers.erase(index --);

    // Write a chunked header.
    resp.headers.emplace_back(&"Transfer-Encoding", &"chunked");
    resp.encode(fmt);
    return this->ssl_send(fmt);
  }

bool
HTTPS_Server_Session::
https_chunked_response_send(chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    tinyfmt_ln fmt;
    ::asteria::ascii_numput nump;

    // Ignore empty chunks, which would mark the end of the content.
    if(data.n == 0)
      return this->ssl_send("");

    // Compose a chunk and send it as a whole. The length of this chunk is
    // written as a hexadecimal integer without the `0x` prefix.
    nump.put_XU(data.n);
    fmt.putn(nump.data() + 2, nump.size() - 2);
    fmt << "\r\n";
    fmt.putn(data.p, data.n);
    fmt << "\r\n";
    return this->ssl_send(fmt);
  }

bool
HTTPS_Server_Session::
https_chunked_response_finish(HTTP_Response_Headers& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    HTTP_Header_Parser hparser;
    bool connection_close = false;

    // Check for `Connection: close`.
    for(size_t index = 0;  index != resp.headers.size();  index ++)
      if(resp.headers.at(index).first == "Connection") {
        hparser.reload(resp.headers.at(index).second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "close")
            connection_close = true;
      }

    // Send an empty chunk trailer.
    bool sent = this->ssl_send("0\r\n\r\n");
    if(connection_close)
      this->ssl_shut_down();

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTPS_Server_Session::
https_shut_down(HTTP_Status status) noexcept
  {
    if(this->m_upgrade_ack.load())
      return this->quick_shut_down();

    bool succ = false;
    try {
      // Compose a default page.
      HTTP_Response_Headers resp;
      resp.status = status;
      resp.reason = HTTP_Response_Headers::default_reason(status);
      resp.headers.emplace_back(&"Content-Type", &"text/html");
      resp.headers.emplace_back(&"Connection", &"close");

      static constexpr char default_page[] =
          "<html>"
          "<head><title>$1 $2</title></head>"
          "<body><h1>$1 $2</h1></body>"
          "</html>";

      tinyfmt_ln fmt;
      format(fmt, default_page, status, resp.reason);

      succ = this->https_response(http_method_GET, resp, fmt.get_buffer());
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send default page: $3",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this), stdex);
    }
    succ |= this->ssl_shut_down();
    return succ;
  }

bool
HTTPS_Server_Session::
https_shut_down(int status) noexcept
  {
    HTTP_Status real_status = http_status_bad_request;
    if((status >= 200) && (status <= 599))
      real_status = static_cast<HTTP_Status>(status);
    return this->https_shut_down(real_status);
  }

}  // namespace poseidon
