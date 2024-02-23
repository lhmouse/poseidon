// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "uuid.hpp"
#include "../utils.hpp"
#define DEFINE_UUID_REP_(x, y)  UUID_DEFINE(x,y,y,y,y,y,y,y,y,y,y,y,y,y,y,y,y)
namespace poseidon {

UUID::
UUID(chars_view str)
  {
    if(this->parse(str) != str.n)
      POSEIDON_THROW(("Could not parse UUID string `$1`"), str);
  }

const UUID&
UUID::
min() noexcept
  {
    DEFINE_UUID_REP_(uuid_bytes, 0x00);
    return reinterpret_cast<const UUID&>(uuid_bytes);
  }

const UUID&
UUID::
max() noexcept
  {
    DEFINE_UUID_REP_(uuid_bytes, 0xFF);
    return reinterpret_cast<const UUID&>(uuid_bytes);
  }

UUID
UUID::
random() noexcept
  {
    // First, generate a random UUID. This creates the `Nzzz-zzzzzzzzzzzz` part
    // so we need not fill it again.
    ::uuid_t uuid_bytes;
    ::uuid_generate_random(uuid_bytes);

    // Write the `xxxxxxxx-xxxx-Myyy` part. This is a 64-bit integer, composed
    // in little-endian order.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    static atomic<uint64_t> seq;

    uint64_t qword = (uint64_t) ts.tv_sec * 30518U + (uint32_t) ts.tv_nsec / 32768U;
    qword += seq.xadd(1);
    qword <<= 16;
    qword |= 0x4000U;
    qword += (uint32_t) ::getpid() & 0x0FFFU;

    qword = ROCKET_HTOBE64(qword);
    ::memcpy(uuid_bytes, &qword, 8);

    static_assert(::std::is_trivially_copyable<UUID>::value);
    return reinterpret_cast<const UUID&>(uuid_bytes);
  }

size_t
UUID::
parse_partial(const char* str) noexcept
  {
    char temp[40];
    size_t len = ::strnlen(str, 36);
    ::memcpy(temp, str, len);
    temp[len] = 0;

    if(::uuid_parse(temp, this->m_uuid) != 0)
      return 0;

    return len;
  }

size_t
UUID::
parse(chars_view str) noexcept
  {
    // A string with an erroneous length will not be accepted, so we just need
    // to check for possibilities by `str.n`.
    if(str.n >= 36)
      if(size_t aclen = this->parse_partial(str.p))
        return aclen;

    return 0;
  }

size_t
UUID::
print_partial(char* str) const noexcept
  {
    ::uuid_unparse_upper(this->m_uuid, str);
    return 36;
  }

tinyfmt&
UUID::
print_to(tinyfmt& fmt) const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return fmt.putn(str, len);
  }

cow_string
UUID::
print_to_string() const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
