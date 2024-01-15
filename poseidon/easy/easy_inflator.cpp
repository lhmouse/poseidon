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
    linear_buffer m_out;

    explicit
    Final_Inflator(zlib_Format format)
      :
        Inflator(format)
      { }

    virtual
    char*
    do_on_inflate_get_output_buffer(size_t& size) override
      {
        size = this->m_out.reserve_after_end(size);
        this->m_out.accept(size);
        return this->m_out.mut_end() - size;
      }

    virtual
    void
    do_on_inflate_truncate_output_buffer(size_t backup) override
      {
        this->m_out.unaccept(backup);
      }
  };

}  // namespace

Easy_Inflator::
~Easy_Inflator()
  {
  }

void
Easy_Inflator::
open(zlib_Format format)
  {
    auto infl = new_sh<Final_Inflator>(format);
    this->m_infl = infl;
    this->m_out = shptr<linear_buffer>(infl, &(infl->m_out));
  }

void
Easy_Inflator::
close() noexcept
  {
    this->m_infl = nullptr;
    this->m_out = nullptr;
  }

void
Easy_Inflator::
reset() noexcept
  {
    if(!this->m_infl)
      return;

    this->m_infl->reset();
    this->m_out->clear();
  }

const char*
Easy_Inflator::
output_data() const noexcept
  {
    if(!this->m_out)
      return nullptr;

    return this->m_out->data();
  }

size_t
Easy_Inflator::
output_size() const noexcept
  {
    if(!this->m_out)
      return 0;

    return this->m_out->size();
  }

void
Easy_Inflator::
output_clear() noexcept
  {
    if(!this->m_out)
      return;

    this->m_out->clear();
  }

size_t
Easy_Inflator::
inflate(chars_view data)
  {
    if(!this->m_infl)
      return 0;

    return this->m_infl->inflate(data);
  }

bool
Easy_Inflator::
finish()
  {
    if(!this->m_infl)
      return false;

    return this->m_infl->finish();
  }

}  // namespace poseidon
