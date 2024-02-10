// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "https_server_session.hpp"
#include "../http/http_header_parser.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTPS_Server_Session::
HTTPS_Server_Session()
  {
    this->m_req_parser.emplace();
  }

HTTPS_Server_Session::
~HTTPS_Server_Session()
  {
  }

void
HTTPS_Server_Session::
do_on_ssl_stream(linear_buffer& data, bool eof)
  {
    for(;;) {
      // Check whether the connection has switched to another protocol.
      if(this->m_upgrade_ack.load()) {
        this->m_req_parser.reset();
        this->do_on_https_upgraded_stream(data, eof);
        return;
      }

      // If something has gone wrong, ignore further incoming data.
      if(this->m_req_parser->error()) {
        data.clear();
        return;
      }

      if(!this->m_req_parser->headers_complete()) {
        this->m_req_parser->parse_headers_from_stream(data, eof);

        if(this->m_req_parser->error()) {
          data.clear();
          this->do_on_https_request_error(this->m_req_parser->http_status_from_error());
          return;
        }

        if(!this->m_req_parser->headers_complete())
          return;

        // Check request headers.
        if(this->m_req_parser->mut_headers().is_proxy == false)
          this->m_req_parser->mut_headers().is_ssl = true;

        bool fin = this->m_req_parser->should_close_after_payload();
        auto payload_type = this->do_on_https_request_headers(this->m_req_parser->mut_headers(), fin);
        switch(payload_type)
          {
          case http_payload_normal:
          case http_payload_empty:
            break;

          case http_payload_connect:
            this->m_req_parser.reset();
            this->m_upgrade_ack.store(true);
            return this->do_on_https_upgraded_stream(data, eof);

          default:
            POSEIDON_THROW((
                "Invalid payload type `$3` returned from `do_http_parser_on_headers_complete()`",
                "[HTTPS server session `$1` (class `$2`)]"),
                this, typeid(*this), payload_type);
          }
      }

      if(!this->m_req_parser->payload_complete()) {
        this->m_req_parser->parse_payload_from_stream(data, eof);

        if(this->m_req_parser->error()) {
          data.clear();
          this->do_on_https_request_error(this->m_req_parser->http_status_from_error());
          return;
        }

        this->do_on_https_request_payload_stream(this->m_req_parser->mut_payload());

        if(!this->m_req_parser->payload_complete())
          return;

        // The message is complete now.
        this->do_on_https_request_finish(move(this->m_req_parser->mut_headers()),
                move(this->m_req_parser->mut_payload()),
                this->m_req_parser->should_close_after_payload());
      }

      this->m_req_parser->next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

char256
HTTPS_Server_Session::
do_on_ssl_alpn_request(vector<char256>&& protos)
  {
    for(const auto& proto : protos)
      if(::strcmp(proto.c_str(), "http/1.1") == 0)
        return proto;

    // Select no protocol.
    return "";
  }

HTTP_Payload_Type
HTTPS_Server_Session::
do_on_https_request_headers(HTTP_Request_Headers& req, bool /*close_after_payload*/)
  {
    if(req.is_proxy) {
      // Reject proxy requests.
      this->do_on_https_request_error(HTTP_STATUS_FORBIDDEN);
      return http_payload_normal;
    }

    POSEIDON_LOG_DEBUG((
        "HTTPS server received request: $3 $4",
        "[HTTPS server session `$1` (class `$2`)]"),
        this, typeid(*this), req.method, req.uri_path);

    // The default handler doesn't handle Upgrade requests.
    return http_payload_normal;
  }

void
HTTPS_Server_Session::
do_on_https_request_payload_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_https_request_finish()`,
    // but perform some security checks, so we won't be affected by compromized
    // 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_request_content_length = 1048576;

    auto conf_value = conf_file.query("network", "http", "max_request_content_length");
    if(conf_value.is_integer())
      max_request_content_length = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.http.max_request_content_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(max_request_content_length < 0)
      POSEIDON_THROW((
          "`network.http.max_request_content_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_request_content_length, conf_file.path());

    if(data.size() > (uint64_t) max_request_content_length)
      POSEIDON_THROW((
          "HTTP request payload too large: `$3` > `$4`",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_request_content_length);
  }

__attribute__((__noreturn__))
void
HTTPS_Server_Session::
do_on_https_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_http_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTPS server session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTPS_Server_Session::
do_https_raw_response(const HTTP_Response_Headers& resp, chars_view data)
  {
    // Compose the message and send it as a whole.
    tinyfmt_ln fmt;
    resp.encode(fmt);
    fmt.putn(data.p, data.n);
    bool sent = this->ssl_send(fmt);

    // For server sessions, a status of 101 indicates that the server will switch
    // to another protocol after this message. The client might have sent more
    // data before this, which would violate RFC 6455 anyway, so we don't care.
    if(resp.status == HTTP_STATUS_SWITCHING_PROTOCOLS)
      this->m_upgrade_ack.store(true);

    // If `Connection:` contains `close`, the connection should be closed.
    HTTP_Header_Parser hparser;
    for(const auto& hpair : resp.headers)
      if(ascii_ci_equal(hpair.first, "Connection")) {
        if(!hpair.second.is_string())
          continue;

        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), "close"))
            this->ssl_shut_down();
      }

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTPS_Server_Session::
https_response_headers_only(HTTP_Response_Headers&& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->do_https_raw_response(resp, "");
  }

bool
HTTPS_Server_Session::
https_response(HTTP_Response_Headers&& resp, chars_view data)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < resp.headers.size();  hindex ++)
      if(ascii_ci_equal(resp.headers.at(hindex).first, "Content-Length")
         || ascii_ci_equal(resp.headers.at(hindex).first, "Transfer-Encoding"))
        resp.headers.erase(hindex --);

    // Some responses are required to have no payload payload and require no
    // `Content-Length` header.
    if((resp.status <= 199) || is_any_of(resp.status, { HTTP_STATUS_NO_CONTENT, HTTP_STATUS_NOT_MODIFIED }))
      return this->do_https_raw_response(resp, "");

    // Otherwise, a `Content-Length` is required; otherwise the response would
    // be interpreted as terminating by closure ofthe connection.
    resp.headers.emplace_back(&"Content-Length", (double)(int64_t) data.n);

    return this->do_https_raw_response(resp, data);
  }

bool
HTTPS_Server_Session::
https_chunked_response_start(HTTP_Response_Headers&& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < resp.headers.size();  hindex ++)
      if(ascii_ci_equal(resp.headers.at(hindex).first, "Transfer-Encoding"))
        resp.headers.erase(hindex --);

    // Write a chunked header.
    resp.headers.emplace_back(&"Transfer-Encoding", &"chunked");

    return this->do_https_raw_response(resp, "");
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

    // Ignore empty chunks, which would mark the end of the payload.
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
    return this->ssl_send(fmt);
  }

bool
HTTPS_Server_Session::
https_chunked_response_finish()
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS server session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->ssl_send("0\r\n\r\n");
  }

}  // namespace poseidon
