// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "uuid.hpp"
#include "../utils.hpp"
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
    static constexpr UUID st = POSEIDON_UUID(00000000,0000,0000,0000,000000000000);
    return st;
  }

const UUID&
UUID::
max() noexcept
  {
    static constexpr UUID st = POSEIDON_UUID(ffffffff,ffff,ffff,ffff,ffffffffffff);
    return st;
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

    static atomic_relaxed<uint64_t> s_serial;
    high += s_serial.xadd(1) << 16;

    // Then, set the `M` field, which is always `4` (UUID version 4).
    high |= 0x4000;

    // Then, set the `yyy` field, which is the PID of the current process,
    // truncated to 12 bits.
    high |= static_cast<uint32_t>(::getpid()) & 0x0FFF;

    // Finally, generate the `Nzzz-zzzzzzzzzzzz` part with libuuid.
    UUID st;
    ::uuid_generate_random(st.m_uuid);
    high = ROCKET_HTOBE64(high);
    ::memcpy(st.m_uuid, &high, 8);
    return st;
  }

size_t
UUID::
parse_partial(const char* str) noexcept
  {
    int r = ::uuid_parse_range(str, str + 36, this->m_uuid);
    ROCKET_ASSERT((r == 0) || (r == -1));
    return static_cast<uint32_t>(r & 36) ^ 36;
  }

size_t
UUID::
parse(chars_view str) noexcept
  {
    if(str.n != 36)
      return 0;

    int r = ::uuid_parse_range(str.p, str.p + 36, this->m_uuid);
    ROCKET_ASSERT((r == 0) || (r == -1));
    return static_cast<uint32_t>(r & 36) ^ 36;
  }

size_t
UUID::
print_partial(char* str) const noexcept
  {
    ::uuid_unparse_lower(this->m_uuid, str);
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
