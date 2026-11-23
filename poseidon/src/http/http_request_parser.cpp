// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_request_parser.hpp"
#include "../../base/config_file.hpp"
#include "../../static/main_config.hpp"
#include "../../utils.hpp"
namespace poseidon {

POSEIDON_VISIBILITY_HIDDEN
const ::llhttp_settings_t
HTTP_Request_Parser::s_llhttp_settings =
#define this   static_cast<HTTP_Request_Parser*>(ps->data)
  {
    // on_message_begin
    nullptr,

    // on_protocol
    nullptr,

    // on_url
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_payload.putn(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_status
    nullptr,

    // on_method
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_payload.putn(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_version
    nullptr,

    // on_header_field
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_hreq >= hreq_headers_done)  // chunk trailer
          return 0;

        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_payload.putn(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_header_value
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_hreq >= hreq_headers_done)  // chunk trailer
          return 0;

        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_payload.putn(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_chunk_extension_name
    nullptr,

    // on_chunk_extension_value
    nullptr,

    // on_headers_complete
    +[](::llhttp_t* ps) -> int
      {
        if(this->m_hreq >= hreq_headers_done)  // chunk trailer
          return 0;

        if(!this->m_headers.is_proxy) {
          // Get the `Host:` header. Multiple `Host:` headers are not allowed.
          uint32_t count = 0;
          for(const auto& hr : this->m_headers.headers)
            if(hr.first == "Host") {
              count ++;
              if(count == 1)
                this->m_headers.raw_host = hr.second.as_string();
              else
                break;
            }

          if(count != 1)
            return -1;  // fail
        }

        this->m_hreq = hreq_headers_done;
        this->m_close_after_payload = ::llhttp_should_keep_alive(ps) == 0;
        this->m_payload.clear();
        return HPE_PAUSED;
      },

    // on_body
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(is_none_of(ps->method, { HTTP_POST, HTTP_PUT, HTTP_PATCH }))
          return 0;  // ignore

        if(this->m_payload.size() + len > this->m_max_content_length)
          POSEIDON_THROW((
              "HTTP content length limit exceeded: `$1` > `$2`"),
              this->m_payload.size() + len, this->m_max_content_length);

        this->m_payload.putn(str, len);
        return 0;
      },

    // on_message_complete
    +[](::llhttp_t* ps) -> int
      {
        this->m_hreq = hreq_payload_done;
        return HPE_PAUSED;
      },

    // on_protocol_complete
    nullptr,

    // on_url_complete
    +[](::llhttp_t* ps) -> int
      {
        cow_string url;
        url.append(this->m_payload.data(), this->m_payload.size());

        Network_Reference ref;
        size_t pos = url.find("://");
        if(pos != url.npos) {
          // An explicit scheme indicates a proxy request.
          this->m_headers.is_proxy = true;

          if((pos == 4) && ::asteria::ascii_ci_equal(url.data(), 4, "http", 4)) {
            this->m_headers.port = 80;
            this->m_headers.is_ssl = false;
            url.erase(0, 7);
          }
          else if((pos == 5) && ::asteria::ascii_ci_equal(url.data(), 5, "https", 5)) {
            this->m_headers.port = 443;
            this->m_headers.is_ssl = true;
            url.erase(0, 8);
          }
          else
            return -1;  // fail

          pos = url.find_of("@/");
          if((pos != url.npos) && (url[pos] == '@')) {
            this->m_headers.raw_userinfo.assign(url, 0, pos);
            url.erase(0, pos + 1);
          }
        }
        else {
          // The path must be absolute.
          if(url[0] != '/')
            return -1;  // fail

          url.insert(0, "a");  // placeholder
        }

        if(parse_network_reference(ref, url) != url.size())
          return -1;  // fail

        if(this->m_headers.is_proxy)
          this->m_headers.raw_host.assign(ref.host.p, ref.host.n);

        if(ref.port.n != 0)
          this->m_headers.port = ref.port_num;

        if(ref.path.n != 0)
          this->m_headers.raw_path.assign(ref.path.p, ref.path.n);
        else
          this->m_headers.raw_path = &"/";

        if(ref.query.n != 0)
          this->m_headers.raw_query.assign(ref.query.p, ref.query.n);

        this->m_hreq = hreq_headers_done;
        this->m_close_after_payload = ::llhttp_should_keep_alive(ps) == 0;
        this->m_payload.clear();
        return 0;
      },

    // on_status_complete
    nullptr,

    // on_method_complete
    +[](::llhttp_t* ps) -> int
      {
        if(is_none_of(ps->method, { HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT,
                                    HTTP_DELETE, HTTP_CONNECT, HTTP_OPTIONS,
                                    HTTP_TRACE, HTTP_PATCH }))
          return -1;  // error

        this->m_payload.putn("\0\0\0\0\0\0\0", 8);
        ::memcpy(this->m_headers.method_str, this->m_payload.data(), 8);
        this->m_payload.clear();
        return 0;
      },

    // on_version_complete
    nullptr,

    // on_header_field_complete
    +[](::llhttp_t* ps) -> int
      {
        if(this->m_hreq >= hreq_headers_done)  // chunk trailer
          return 0;

        cow_string name(this->m_payload.data(), this->m_payload.size());
        this->m_headers.headers.emplace_back(move(name), &"");
        this->m_header_size += 2;
        this->m_payload.clear();
        return 0;
      },

    // on_header_value_complete
    +[](::llhttp_t* ps) -> int
      {
        if(this->m_hreq >= hreq_headers_done)  // chunk trailer
          return 0;

        cow_string value(this->m_payload.data(), this->m_payload.size());
        this->m_headers.headers.mut_back().second = move(value);
        this->m_payload.clear();
        return 0;
      },

    // on_chunk_extension_name_complete
    nullptr,

    // on_chunk_extension_value_complete
    nullptr,

    // on_chunk_header
    nullptr,

    // on_chunk_complete
    nullptr,

    // on_reset
    nullptr,
  };
#undef this

HTTP_Request_Parser::
HTTP_Request_Parser()
  {
    ::llhttp_init(&(this->m_llhttp_parser), HTTP_REQUEST, &(s_llhttp_settings));
    this->m_llhttp_parser.data = this;

    auto conf_file = main_config.copy();
    this->m_default_compression_level = static_cast<int>(conf_file.get_integer_opt(
        &"network.http.default_compression_level", 0, 9).value_or(6));
    this->m_max_header_length = static_cast<uint32_t>(conf_file.get_integer_opt(
        &"network.http.max_header_length", 256, 16777216).value_or(262144));
    this->m_max_content_length = static_cast<uint32_t>(conf_file.get_integer_opt(
        &"network.http.max_request_content_length", 256, 16777216).value_or(1048576));
  }

HTTP_Request_Parser::
~HTTP_Request_Parser()
  {
  }

HTTP_Status
HTTP_Request_Parser::
http_status_from_error()
  const noexcept
  {
    switch(this->m_llhttp_parser.error)
      {
      case HPE_OK:
      case HPE_PAUSED:
        return http_status_ok;

      case HPE_INVALID_VERSION:
        return http_status_http_version_not_supported;

      case HPE_INVALID_METHOD:
        return http_status_method_not_allowed;

      case HPE_INVALID_TRANSFER_ENCODING:
        return http_status_length_required;

      default:
        return http_status_bad_request;
      }
  }

void
HTTP_Request_Parser::
clear()
  noexcept
  {
    ::llhttp_init(&(this->m_llhttp_parser), HTTP_REQUEST, &(s_llhttp_settings));
    this->m_llhttp_parser.data = this;

    this->m_headers.clear();
    this->m_payload.clear();
    this->m_header_size = 0;
    this->m_hreq = hreq_new;
    this->m_close_after_payload = false;
  }

void
HTTP_Request_Parser::
deallocate()
  noexcept
  {
    ::asteria::exchange(this->m_headers.raw_host);
    ::asteria::exchange(this->m_headers.raw_userinfo);
    ::asteria::exchange(this->m_headers.raw_path);
    ::asteria::exchange(this->m_headers.raw_query);
    ::asteria::exchange(this->m_headers.headers);
    ::asteria::exchange(this->m_payload);
  }

void
HTTP_Request_Parser::
parse_headers_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hreq >= hreq_headers_done)
      return;

    this->m_llhttp_parser.error_pos = nullptr;
    ::llhttp_errno_t err = ::llhttp_execute(&(this->m_llhttp_parser), data.data(), data.size());

    // If `err` is `HPE_PAUSED`, we should consume parsed bytes; otherwise it
    // means either all bytes have been consumed or there is an error, and we
    // discard all input data.
    if(err == HPE_PAUSED)
      data.discard(static_cast<size_t>(this->m_llhttp_parser.error_pos - data.data()));
    else
      data.clear();

    if((err == HPE_OK) && eof) {
      // `llhttp_finish()` doesn't set `m_llhttp_parser.error` so it has to be
      // copied explicitly.
      err = ::llhttp_finish(&(this->m_llhttp_parser));
      if(err != HPE_OK)
        this->m_llhttp_parser.error = err;
    }

    // Normalize errors that are returned by our calbacks.
    if(this->m_llhttp_parser.error == HPE_CB_METHOD_COMPLETE)
      this->m_llhttp_parser.error = HPE_INVALID_METHOD;
    else if(this->m_llhttp_parser.error == HPE_CB_URL_COMPLETE)
      this->m_llhttp_parser.error = HPE_INVALID_URL;

    // `HPE_PAUSED` means the end of a message has been reached, which should
    // not passed to the caller.
    if(this->m_llhttp_parser.error == HPE_PAUSED)
      this->m_llhttp_parser.error = HPE_OK;
  }

void
HTTP_Request_Parser::
parse_payload_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hreq >= hreq_payload_done)
      return;

    if(this->m_hreq != hreq_headers_done)
      POSEIDON_THROW((
          "HTTP request header not parsed yet (state `$1` not valid)"),
          this->m_hreq);

    this->m_llhttp_parser.error_pos = nullptr;
    ::llhttp_errno_t err = ::llhttp_execute(&(this->m_llhttp_parser), data.data(), data.size());

    // If `err` is `HPE_PAUSED`, we should consume parsed bytes; otherwise it
    // means either all bytes have been consumed or there is an error, and we
    // discard all input data.
    if(err == HPE_PAUSED)
      data.discard(static_cast<size_t>(this->m_llhttp_parser.error_pos - data.data()));
    else
      data.clear();

    if((err == HPE_OK) && eof) {
      // `llhttp_finish()` doesn't set `m_llhttp_parser.error` so it has to be
      // copied explicitly.
      err = ::llhttp_finish(&(this->m_llhttp_parser));
      if(err != HPE_OK)
        this->m_llhttp_parser.error = err;
    }

    // `HPE_PAUSED` means the end of a message has been reached, which should
    // not passed to the caller.
    if(this->m_llhttp_parser.error == HPE_PAUSED)
      this->m_llhttp_parser.error = HPE_OK;
  }

}  // namespace poseidon
