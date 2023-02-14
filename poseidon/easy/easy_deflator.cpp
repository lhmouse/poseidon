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
    linear_buffer m_out;

    explicit
    Final_Deflator(zlib_Format format, int level)
      : Deflator(format, level)  { }

    virtual
    pair<char*, size_t>
    do_on_deflate_get_output_buffer() override
      {
        size_t cap = this->m_out.reserve_after_end(1024);
        char* ptr = this->m_out.mut_end();
        this->m_out.accept(cap);
        return { ptr, cap };
      }

    virtual
    void
    do_on_deflate_truncate_output_buffer(size_t nbackup) override
      {
        this->m_out.unaccept(nbackup);
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
    auto defl = ::std::make_shared<Final_Deflator>(format, level);
    this->m_defl = defl;
    this->m_out = shared_ptr<linear_buffer>(defl, &(defl->m_out));
  }

void
Easy_Deflator::
clear() noexcept
  {
    if(!this->m_defl)
      return;

    this->m_defl->clear();
    this->m_out->clear();
  }

const char*
Easy_Deflator::
output_data() const noexcept
  {
    if(!this->m_out)
      return nullptr;

    return static_cast<Final_Deflator*>(this->m_defl.get())->m_out.data();
  }

size_t
Easy_Deflator::
output_size() const noexcept
  {
    if(!this->m_out)
      return 0;

    return static_cast<Final_Deflator*>(this->m_defl.get())->m_out.size();
  }

void
Easy_Deflator::
output_clear() noexcept
  {
    if(!this->m_out)
      return;

    static_cast<Final_Deflator*>(this->m_defl.get())->m_out.clear();
  }

size_t
Easy_Deflator::
deflate(const char* data, size_t size)
  {
    if(!this->m_defl)
      return 0;

    return this->m_defl->deflate(data, size);
  }

bool
Easy_Deflator::
sync_flush()
  {
    if(!this->m_defl)
      return false;

    return this->m_defl->sync_flush();
  }

bool
Easy_Deflator::
finish()
  {
    if(!this->m_defl)
      return false;

    return this->m_defl->finish();
  }

}  // namespace poseidon
