// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../socket/http_client_session.hpp"
#include "../../utils.hpp"
namespace poseidon {

HTTP_Client_Session::
HTTP_Client_Session(const cow_string& default_host)
  :
    m_default_host(default_host)
  {
  }

HTTP_Client_Session::
~HTTP_Client_Session()
  {
  }

void
HTTP_Client_Session::
do_on_tcp_stream(linear_buffer& data, bool eof)
  {
    if(this->m_upgrade_ack.load()) {
      this->do_on_http_upgraded_stream(data, eof);
      return;
    }

    for(;;) {
      // If something has gone wrong, ignore further incoming data.
      if(this->m_resp_parser.error()) {
        data.clear();
        return;
      }

      if(!this->m_resp_parser.headers_complete()) {
        HTTP_Method method = http_method_null;
        plain_mutex::unique_lock pipeline_lock(this->m_pipeline_mutex);
        this->m_pipeline.peekn(reinterpret_cast<char*>(&method), sizeof(method));
        pipeline_lock.unlock();
        this->m_resp_parser.parse_headers_from_stream(method, data, eof);

        if(this->m_resp_parser.error()) {
          data.clear();
          this->quick_shut_down();
          return;
        }

        if(!this->m_resp_parser.headers_complete())
          return;

        // Check headers.
        auto& resp = this->m_resp_parser.mut_headers();

        this->do_on_http_response_headers(method, resp);

        if((resp.status == 101) || !((resp.status >= 100) && (resp.status <= 199))) {
          // Pop a request and check whether the connection has switched to
          // another protocol.
          pipeline_lock.lock(this->m_pipeline_mutex);
          if(((method == http_method_CONNECT) && (resp.status >= 200) && (resp.status <= 299))
              || (resp.status == 101)) {  // switching protocols
            ::asteria::exchange(this->m_pipeline);
            pipeline_lock.unlock();

            this->m_upgrade_ack.store(true);
            this->m_resp_parser.deallocate();
            this->do_on_http_upgraded_stream(data, eof);
            return;
          }

          this->m_pipeline.discard(sizeof(method));
        }
      }

      if(!this->m_resp_parser.content_complete()) {
        this->m_resp_parser.parse_content_from_stream(data, eof);

        if(this->m_resp_parser.error()) {
          data.clear();
          this->quick_shut_down();
          return;
        }

        this->do_on_http_response_content_stream(this->m_resp_parser.mut_content());

        if(!this->m_resp_parser.content_complete())
          return;

        // The message is complete now.
        this->do_on_http_response_finish(move(this->m_resp_parser.mut_headers()),
                                         move(this->m_resp_parser.mut_content()));

        if(this->m_resp_parser.should_close_after_content())
          this->tcp_shut_down();
      }

      this->m_resp_parser.next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

void
HTTP_Client_Session::
do_on_http_response_headers(HTTP_Method method, HTTP_Response_Headers& resp)
  {
    POSEIDON_LOG_DEBUG((
        "HTTP client received response for method $3: $4 $5",
        "[HTTP client session `$1` (class `$2`)]"),
        this, typeid(*this), method, resp.status, resp.reason);
  }

void
HTTP_Client_Session::
do_on_http_response_content_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_http_response_finish()`,
    // but perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_resp_parser.max_content_length())
      POSEIDON_THROW((
          "HTTP response content too large: `$3` > `$4`",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_resp_parser.max_content_length());
  }

__attribute__((__noreturn__))
void
HTTP_Client_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_http_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTP client session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTP_Client_Session::
http_request(HTTP_Request_Headers& req, chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    bool has_host = false;

    // Erase `Transfer-Encoding:` and `Content-Length:` headers, if any. Check
    // whether `Host:` has been set.
    for(size_t index = 0;  index != req.headers.size();  index ++)
      if(req.headers.at(index).first == "Transfer-Encoding")
        req.headers.erase(index --);
      else if(req.headers.at(index).first == "Content-Length")
        req.headers.erase(index --);
      else if(!req.is_proxy && (req.headers.at(index).first == "Host"))
        has_host = true;

    // Ensure `Host:` is set, as per HTTP/1.1.
    if(!req.is_proxy && !has_host)
      req.headers.emplace_back(&"Host",
            this->m_default_host.empty() ? this->remote_address().to_string()
                                         : this->m_default_host);

    // For a request, lack of `Content-Length:` means there's no message body.
    // When a message body is given, do set it.
    if(data.n != 0)
      req.headers.emplace_back(&"Content-Length", static_cast<int64_t>(data.n));

    tinyfmt_ln fmt;
    req.encode(fmt);

    plain_mutex::unique_lock pipeline_lock(this->m_pipeline_mutex);
    this->m_pipeline.putn(reinterpret_cast<const char*>(&(req.method)), sizeof(req.method));
    bool sent = this->tcp_send(fmt);
    pipeline_lock.unlock();

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTP_Client_Session::
http_chunked_request_start(HTTP_Request_Headers& req)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    bool has_host = false;

    // Erase `Transfer-Encoding:` headers, if any. `Content-Length:` may be useful
    // so it's left intact, despite violation of the specification.
    for(size_t index = 0;  index != req.headers.size();  index ++)
      if(req.headers.at(index).first == "Transfer-Encoding")
        req.headers.erase(index --);
      else if(!req.is_proxy && (req.headers.at(index).first == "Host"))
        has_host = true;

    // Ensure `Host:` is set, as per HTTP/1.1.
    if(!req.is_proxy && !has_host)
      req.headers.emplace_back(&"Host",
            this->m_default_host.empty() ? this->remote_address().to_string()
                                         : this->m_default_host);

    // Write a chunked header.
    req.headers.emplace_back(&"Transfer-Encoding", &"chunked");

    tinyfmt_ln fmt;
    req.encode(fmt);

    plain_mutex::unique_lock pipeline_lock(this->m_pipeline_mutex);
    this->m_pipeline.putn(reinterpret_cast<const char*>(&(req.method)), sizeof(req.method));
    return this->tcp_send(fmt);
  }

bool
HTTP_Client_Session::
http_chunked_request_send(chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Ignore empty chunks, which would mark the end of the content.
    if(data.n == 0)
      return this->tcp_send("");

    // Compose a chunk and send it as a whole. The length of this chunk is
    // written as a hexadecimal integer without the `0x` prefix.
    tinyfmt_ln fmt;
    ::asteria::ascii_numput nump;
    nump.put_XU(data.n);
    fmt.putn(nump.data() + 2, nump.size() - 2);
    fmt << "\r\n";
    fmt.putn(data.p, data.n);
    fmt << "\r\n";

    return this->tcp_send(fmt);
  }

bool
HTTP_Client_Session::
http_chunked_request_finish(HTTP_Request_Headers& req)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    (void) req;

    // Send an empty chunk trailer.
    bool sent = this->tcp_send("0\r\n\r\n");

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

}  // namespace poseidon
