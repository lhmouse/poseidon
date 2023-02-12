// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_deflator.hpp"
#include "../base/deflator.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Final_Deflator final : Deflator
  {
    ::std::string m_out;

    explicit
    Final_Deflator(zlib_Format format, int level)
      : Deflator(format, level)
      { }

    virtual
    pair<char*, size_t>
    do_on_deflate_get_output_buffer() override
      {
        size_t old_size = this->m_out.size();
        this->m_out.append(1024, '*');
        return ::std::make_pair(&(this->m_out[0]) + old_size, this->m_out.size() - old_size);
      }

    virtual
    void
    do_on_deflate_truncate_output_buffer(size_t nbackup) override
      {
        this->m_out.erase(this->m_out.size() - nbackup);
      }
  };

}  // namespace

Easy_Deflator::
~Easy_Deflator()
  {
  }

void
Easy_Deflator::
start(zlib_Format format, int level)
  {
    this->m_defl = ::std::make_shared<Final_Deflator>(format, level);
  }

void
Easy_Deflator::
reset() noexcept
  {
    this->m_defl = nullptr;
  }

const char*
Easy_Deflator::
output_data() const noexcept
  {
    if(!this->m_defl)
      return nullptr;

    return static_cast<Final_Deflator*>(this->m_defl.get())->m_out.data();
  }

size_t
Easy_Deflator::
output_size() const noexcept
  {
    if(!this->m_defl)
      return 0;

    return static_cast<Final_Deflator*>(this->m_defl.get())->m_out.size();
  }

void
Easy_Deflator::
output_clear() noexcept
  {
    if(!this->m_defl)
      return;

    static_cast<Final_Deflator*>(this->m_defl.get())->m_out.clear();
  }

void
Easy_Deflator::
deflate(const char* data, size_t size)
  {
    if(!this->m_defl)
      POSEIDON_THROW(("No output stream"));

    static_cast<Final_Deflator*>(this->m_defl.get())->deflate(data, size);
  }

void
Easy_Deflator::
sync_flush()
  {
    if(!this->m_defl)
      POSEIDON_THROW(("No output stream"));

    static_cast<Final_Deflator*>(this->m_defl.get())->sync_flush();
  }

void
Easy_Deflator::
finish()
  {
    if(!this->m_defl)
      POSEIDON_THROW(("No output stream"));

    static_cast<Final_Deflator*>(this->m_defl.get())->finish();
  }

}  // namespace poseidon
