// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_client_session.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Client_Session::
HTTP_Client_Session()
  {
    this->m_resp_parser.emplace();
  }

HTTP_Client_Session::
~HTTP_Client_Session()
  {
  }

void
HTTP_Client_Session::
do_on_tcp_stream(linear_buffer& data, bool eof)
  {
    for(;;) {
      // Check whether the connection has switched to another protocol.
      if(this->m_upgrade_ack.load()) {
        this->m_resp_parser.reset();
        this->do_on_http_upgraded_stream(data, eof);
        return;
      }

      // If something has gone wrong, ignore further incoming data.
      if(this->m_resp_parser->error()) {
        data.clear();
        return;
      }

      if(!this->m_resp_parser->headers_complete()) {
        this->m_resp_parser->parse_headers_from_stream(data, eof);

        if(this->m_resp_parser->error()) {
          data.clear();
          this->quick_close();
          return;
        }

        if(!this->m_resp_parser->headers_complete())
          return;

        // Check headers.
        auto payload_type = this->do_on_http_response_headers(this->m_resp_parser->mut_headers());
        switch(payload_type)
          {
          case http_payload_normal:
            break;

          case http_payload_empty:
            this->m_resp_parser->set_no_payload();
            break;

          case http_payload_connect:
            this->m_resp_parser.reset();
            this->m_upgrade_ack.store(true);
            return this->do_on_http_upgraded_stream(data, eof);

          default:
            POSEIDON_THROW((
                "Invalid payload type `$3` returned from `do_http_parser_on_headers_complete()`",
                "[HTTP client session `$1` (class `$2`)]"),
                this, typeid(*this), payload_type);
          }
      }

      if(!this->m_resp_parser->payload_complete()) {
        this->m_resp_parser->parse_payload_from_stream(data, eof);

        if(this->m_resp_parser->error()) {
          data.clear();
          this->quick_close();
          return;
        }

        this->do_on_http_response_payload_stream(this->m_resp_parser->mut_payload());

        if(!this->m_resp_parser->payload_complete())
          return;

        // The message is complete now.
        uint32_t status = this->m_resp_parser->headers().status;

        this->do_on_http_response_finish(move(this->m_resp_parser->mut_headers()),
                                         move(this->m_resp_parser->mut_payload()),
                                         this->m_resp_parser->should_close_after_payload());

        // For WebSocket and HTTP 2.0, this indiciates the server has switched to
        // another protocol. CONNECT responses are handled differently after the
        // headers; see above.
        if(status == HTTP_STATUS_SWITCHING_PROTOCOLS)
          this->m_upgrade_ack.store(true);
      }

      this->m_resp_parser->next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

HTTP_Payload_Type
HTTP_Client_Session::
do_on_http_response_headers(HTTP_Response_Headers& resp)
  {
    POSEIDON_LOG_DEBUG((
        "HTTP client received response: $3 $4",
        "[HTTP client session `$1` (class `$2`)]"),
        this, typeid(*this), resp.status, resp.reason);

    // The default handler doesn't handle HEAD, CONNECT or Upgrade responses.
    return http_payload_normal;
  }

void
HTTP_Client_Session::
do_on_http_response_payload_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_http_response_finish()`,
    // but perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_resp_parser->max_content_length())
      POSEIDON_THROW((
          "HTTP response payload too large: `$3` > `$4`",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_resp_parser->max_content_length());
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
do_http_raw_request(const HTTP_Request_Headers& req, chars_view data)
  {
    // Compose the message and send it as a whole.
    tinyfmt_ln fmt;
    req.encode(fmt);
    fmt.putn(data.p, data.n);
    bool sent = this->tcp_send(fmt);

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTP_Client_Session::
http_request(HTTP_Request_Headers&& req, chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < req.headers.size();  hindex ++)
      if(ascii_ci_equal(req.headers.at(hindex).first, "Content-Length")
          || ascii_ci_equal(req.headers.at(hindex).first, "Transfer-Encoding"))
        req.headers.erase(hindex --);

    // By default, request messages do not have payload bodies. Hence the length
    // is only necessary if the payload is non-empty.
    if(data.n != 0)
      req.headers.emplace_back(&"Content-Length", (double)(int64_t) data.n);

    return this->do_http_raw_request(req, data);
  }

bool
HTTP_Client_Session::
http_chunked_request_start(HTTP_Request_Headers&& req)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < req.headers.size();  hindex ++)
      if(ascii_ci_equal(req.headers.at(hindex).first, "Transfer-Encoding"))
        req.headers.erase(hindex --);

    // Write a chunked header.
    req.headers.emplace_back(&"Transfer-Encoding", &"chunked");

    return this->do_http_raw_request(req, "");
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

    // Ignore empty chunks, which would have marked the end of the payload.
    if(data.n == 0)
      return this->socket_state() <= socket_established;

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
HTTP_Client_Session::
http_chunked_request_finish()
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->tcp_send("0\r\n\r\n");
  }

}  // namespace poseidon
