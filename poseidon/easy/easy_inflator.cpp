// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_inflator.hpp"
#include "../base/inflator.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Final_Inflator final : Inflator
  {
    ::std::string m_out;

    explicit
    Final_Inflator(zlib_Format format)
      : Inflator(format)
      { }

    virtual
    pair<char*, size_t>
    do_on_inflate_get_output_buffer() override
      {
        size_t old_size = this->m_out.size();
        this->m_out.append(1024, '*');
        return ::std::make_pair(&(this->m_out[0]) + old_size, this->m_out.size() - old_size);
      }

    virtual
    void
    do_on_inflate_truncate_output_buffer(size_t nbackup) override
      {
        this->m_out.erase(this->m_out.size() - nbackup);
      }
  };

}  // namespace

Easy_Inflator::
~Easy_Inflator()
  {
  }

void
Easy_Inflator::
start(zlib_Format format)
  {
    this->m_defl = ::std::make_shared<Final_Inflator>(format);
  }

void
Easy_Inflator::
reset() noexcept
  {
    this->m_defl = nullptr;
  }

const char*
Easy_Inflator::
output_data() const noexcept
  {
    if(!this->m_defl)
      return nullptr;

    return static_cast<Final_Inflator*>(this->m_defl.get())->m_out.data();
  }

size_t
Easy_Inflator::
output_size() const noexcept
  {
    if(!this->m_defl)
      return 0;

    return static_cast<Final_Inflator*>(this->m_defl.get())->m_out.size();
  }

void
Easy_Inflator::
output_clear() noexcept
  {
    if(!this->m_defl)
      return;

    static_cast<Final_Inflator*>(this->m_defl.get())->m_out.clear();
  }

void
Easy_Inflator::
inflate(const char* data, size_t size)
  {
    if(!this->m_defl)
      POSEIDON_THROW(("No output stream"));

    static_cast<Final_Inflator*>(this->m_defl.get())->inflate(data, size);
  }

void
Easy_Inflator::
finish()
  {
    if(!this->m_defl)
      POSEIDON_THROW(("No output stream"));

    static_cast<Final_Inflator*>(this->m_defl.get())->finish();
  }

}  // namespace poseidon
