// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "uuid.hpp"
#include "../utils.hpp"
#define do_define_uuid_rep_(x, y)  UUID_DEFINE(x,y,y,y,y,y,y,y,y,y,y,y,y,y,y,y,y)
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
    do_define_uuid_rep_(uuid_bytes, 0x00);
    return reinterpret_cast<const UUID&>(uuid_bytes);
  }

const UUID&
UUID::
max() noexcept
  {
    do_define_uuid_rep_(uuid_bytes, 0xFF);
    return reinterpret_cast<const UUID&>(uuid_bytes);
  }

UUID
UUID::
random() noexcept
  {
    // The UUID shall look like `xxxxxxxx-xxxx-Myyy-Nzzz-zzzzzzzzzzzz`.
    // First, generate the `xxxxxxxx-xxxx` field. This is the number of 1/30518
    // seconds since 2001-03-01T00:00:00Z, plus a serial number to keep it
    // monotonic.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t high = static_cast<uint64_t>(ts.tv_sec - 983404800) * 30518 << 16;
    high += static_cast<uint32_t>(ts.tv_nsec) / 32768 << 16;

    static ::std::atomic_uint64_t s_serial = { 0 };
    high += s_serial.fetch_add(1, ::std::memory_order_relaxed) << 16;

    // Then, set the `M` field, which is always `4` (UUID version 4).
    high |= 0x4000;

    // Then, set the `yyy` field, which is the PID of the current process,
    // truncated to 12 bits.
    high |= static_cast<uint32_t>(::getpid()) & 0x0FFF;

    // Finally, generate the `Nzzz-zzzzzzzzzzzz` part with libuuid, convert our
    // higher part to big-endian order and overwrite it.
    ::uuid_t uuid_bytes;
    ::uuid_generate_random(uuid_bytes);
    reinterpret_cast<uint64_t&>(uuid_bytes) = ROCKET_HTOBE64(high);
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
