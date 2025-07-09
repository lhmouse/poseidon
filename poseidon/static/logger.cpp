// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "logger.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <time.h>
#include <sys/syscall.h>
namespace poseidon {
namespace {

struct Level_Config
  {
    char tag[15];
    bool expendable = false;
    cow_string color;
    cow_vector<phcow_string> files;
  };

struct Message
  {
    uint32_t level : 8;
    uint32_t thrd_lwpid : 24;
    char thrd_name[16];
    const char* func;
    const char* file;
    uint32_t line;
    cow_string text;
  };

#define NEL_HT_  "\x1B\x45\t"

constexpr char s_escapes[][5] =
  {
    "\\0",   "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a",
    "\\b",   "\t",    NEL_HT_, "\\v",   "\\f",   "\\r",   "\\x0E", "\\x0F",
    "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17",
    "\\x18", "\\x19", "\\x1A", "\\x1B", "\\x1C", "\\x1D", "\\x1E", "\\x1F",
    " ",     "!",     "\"",    "#",     "$",     "%",     "&",     "\'",
    "(",     ")",     "*",     "+",     ",",     "-",     ".",     "/",
    "0",     "1",     "2",     "3",     "4",     "5",     "6",     "7",
    "8",     "9",     ":",     ";",     "<",     "=",     ">",     "?",
    "@",     "A",     "B",     "C",     "D",     "E",     "F",     "G",
    "H",     "I",     "J",     "K",     "L",     "M",     "N",     "O",
    "P",     "Q",     "R",     "S",     "T",     "U",     "V",     "W",
    "X",     "Y",     "Z",     "[",     "\\",    "]",     "^",     "_",
    "`",     "a",     "b",     "c",     "d",     "e",     "f",     "g",
    "h",     "i",     "j",     "k",     "l",     "m",     "n",     "o",
    "p",     "q",     "r",     "s",     "t",     "u",     "v",     "w",
    "x",     "y",     "z",     "{",     "|",     "}",     "~",     "\\x7F",
    "\x80",  "\x81",  "\x82",  "\x83",  "\x84",  "\x85",  "\x86",  "\x87",
    "\x88",  "\x89",  "\x8A",  "\x8B",  "\x8C",  "\x8D",  "\x8E",  "\x8F",
    "\x90",  "\x91",  "\x92",  "\x93",  "\x94",  "\x95",  "\x96",  "\x97",
    "\x98",  "\x99",  "\x9A",  "\x9B",  "\x9C",  "\x9D",  "\x9E",  "\x9F",
    "\xA0",  "\xA1",  "\xA2",  "\xA3",  "\xA4",  "\xA5",  "\xA6",  "\xA7",
    "\xA8",  "\xA9",  "\xAA",  "\xAB",  "\xAC",  "\xAD",  "\xAE",  "\xAF",
    "\xB0",  "\xB1",  "\xB2",  "\xB3",  "\xB4",  "\xB5",  "\xB6",  "\xB7",
    "\xB8",  "\xB9",  "\xBA",  "\xBB",  "\xBC",  "\xBD",  "\xBE",  "\xBF",
    "\\xC0", "\\xC1", "\xC2",  "\xC3",  "\xC4",  "\xC5",  "\xC6",  "\xC7",
    "\xC8",  "\xC9",  "\xCA",  "\xCB",  "\xCC",  "\xCD",  "\xCE",  "\xCF",
    "\xD0",  "\xD1",  "\xD2",  "\xD3",  "\xD4",  "\xD5",  "\xD6",  "\xD7",
    "\xD8",  "\xD9",  "\xDA",  "\xDB",  "\xDC",  "\xDD",  "\xDE",  "\xDF",
    "\xE0",  "\xE1",  "\xE2",  "\xE3",  "\xE4",  "\xE5",  "\xE6",  "\xE7",
    "\xE8",  "\xE9",  "\xEA",  "\xEB",  "\xEC",  "\xED",  "\xEE",  "\xEF",
    "\xF0",  "\xF1",  "\xF2",  "\xF3",  "\xF4",  "\\xF5", "\\xF6", "\\xF7",
    "\\xF8", "\\xF9", "\\xFA", "\\xFB", "\\xFC", "\\xFD", "\\xFE", "\\xFF",
  };

void
do_color(linear_buffer& mtext, const Level_Config& lconf, const char* code)
  {
    if(lconf.color.empty())
      return;

    // Emit the sequence only if colors are enabled.
    mtext.putc('\x1B');
    mtext.putc('[');
    mtext.puts(code);
    mtext.putc('m');
  }

template<typename xFiles>
void
do_write_nothrow(xFiles& io_files,  const Level_Config& lconf, const Message& msg) noexcept
  try {
    linear_buffer mtext;
    ::rocket::ascii_numput nump;

    // Get the local system time.
    struct timespec tv;
    ::clock_gettime(CLOCK_REALTIME, &tv);
    struct tm tm;
    ::localtime_r(&(tv.tv_sec), &tm);

    // Write the timestamp and tag.
    do_color(mtext, lconf, lconf.color.c_str());  // level color
    nump.put_DU(static_cast<uint32_t>(tm.tm_year + 1900), 4);
    mtext.putn(nump.data(), 4);
    mtext.putc('-');
    nump.put_DU(static_cast<uint32_t>(tm.tm_mon + 1), 2);
    mtext.putn(nump.data(), 2);
    mtext.putc('-');
    nump.put_DU(static_cast<uint32_t>(tm.tm_mday), 2);
    mtext.putn(nump.data(), 2);
    mtext.putc(' ');
    nump.put_DU(static_cast<uint32_t>(tm.tm_hour), 2);
    mtext.putn(nump.data(), 2);
    mtext.putc(':');
    nump.put_DU(static_cast<uint32_t>(tm.tm_min), 2);
    mtext.putn(nump.data(), 2);
    mtext.putc(':');
    nump.put_DU(static_cast<uint32_t>(tm.tm_sec), 2);
    mtext.putn(nump.data(), 2);
    mtext.putc('.');
    nump.put_DU(static_cast<uint32_t>(tv.tv_nsec), 9);
    mtext.putn(nump.data(), 9);
    mtext.putc(' ');
    do_color(mtext, lconf, "22;7");  // no bright; inverse
    mtext.putn(lconf.tag, ::strnlen(lconf.tag, sizeof(lconf.tag)));
    do_color(mtext, lconf, "0");  // reset
    mtext.putc(' ');

    // Write the message.
    do_color(mtext, lconf, "0");  // reset
    do_color(mtext, lconf, lconf.color.c_str());

    for(size_t k = 0;  k != msg.text.size();  ++k) {
      // Get the replacement sequence.
      uint32_t uch = static_cast<uint8_t>(msg.text[k]);
      const char* seq = s_escapes[uch];

      if(seq[0] != '\\')
        mtext.puts(seq);
      else if(seq[1] == 0)
        mtext.putc(seq[0]);
      else {
        // non-printable
        do_color(mtext, lconf, "7");
        mtext.puts(seq);
        do_color(mtext, lconf, "27");
      }
    }

    // Remove trailing space characters.
    while(is_any_of(mtext.end()[-1], {' ','\t','\r','\n'}))
      mtext.unaccept(1);

    // Terminate the message.
    do_color(mtext, lconf, "0");  // reset
    mtext.puts(NEL_HT_);

    // Write the thread name and ID.
    do_color(mtext, lconf, "90");  // grey
    mtext.puts("@@ thread ");
    nump.put_DU(msg.thrd_lwpid);
    mtext.putn(nump.data(), nump.size());
    mtext.puts(" [");
    mtext.putn(msg.thrd_name, ::strnlen(msg.thrd_name, sizeof(msg.thrd_name)));
    mtext.puts("] ");

    // Write the source location and enclosing function.
    do_color(mtext, lconf, "34");  // blue
    mtext.puts("inside function `");
    mtext.puts(msg.func);
    mtext.puts("` at '");
    mtext.puts(msg.file);
    mtext.putc(':');
    nump.put_DU(msg.line);
    mtext.putn(nump.data(), nump.size());
    mtext.puts("'" NEL_HT_);

    // Append a genuine new line for grep'ing.
    mtext.unaccept(1);
    do_color(mtext, lconf, "0");  // reset
    mtext.putc('\n');

    // Write the message to all output files. Errors are ignored.
    for(const auto& file : lconf.files)
      if(file != "") {
        auto r = io_files.try_emplace(file);
        if(r.second) {
          // A new element has just been inserted, so open the file.
          if(file == "/dev/stdout")
            r.first->second.reset(STDOUT_FILENO);  // no close
          else if(file == "/dev/stderr")
            r.first->second.reset(STDERR_FILENO);  // no close
          else
            r.first->second.reset(::open(file.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644));
        }

        if(r.first->second)
          (void)! ::write(r.first->second, mtext.data(), mtext.size());
      }
  }
  catch(exception& stdex) {
    ::fprintf(stderr,
        "WARNING: Failed to write log message: %s\n"
        "[exception class `%s`]\n",
        stdex.what(), typeid(stdex).name());
  }

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Logger,
  Level_Config);

POSEIDON_HIDDEN_X_STRUCT(Logger,
  Message);

Logger::
Logger() noexcept
  {
  }

Logger::
~Logger()
  {
  }

void
Logger::
reload(const Config_File& conf_file, bool verbose)
  {
    // Hold the logging thread.
    recursive_mutex::unique_lock io_sync_lock(this->m_sched_mutex);
    this->m_conf_level_bits.store(UINT32_MAX);

    // Parse new configuration.
    cow_vector<X_Level_Config> levels;
    static const char names[][8] = { "fatal", "error", "warn", "info", "debug", "trace" };
    levels.reserve(size(names));
    for(const char* name : names) {
      auto& lconf = levels.emplace_back();
      ::snprintf(lconf.tag, sizeof(lconf.tag), "[%s]", name);
      lconf.color = conf_file.get_string_opt(sformat("logger.$1.color", name)).value_or(&"");
      lconf.expendable = conf_file.get_boolean_opt(sformat("logger.$1.expendable", name)).value_or(false);

      bool has_stdout = false;
      size_t nfiles = conf_file.get_array_size_opt(sformat("logger.$1.files", name)).value_or(0);
      lconf.files.reserve(nfiles);
      for(uint32_t k = 0;  k != nfiles;  ++k) {
        lconf.files.emplace_back(conf_file.get_string(sformat("logger.$1.files[$2]", name, k)));
        has_stdout |= (lconf.files.back() == "/dev/stdout") || (lconf.files.back() == "/dev/stderr");
      }

      // In verbose mode, apply `/dev/stdout` to all levels, to make them
      // always visible.
      if(verbose && !has_stdout)
        lconf.files.emplace_back(&"/dev/stdout");
    }

    uint32_t level_bits = 0;
    for(size_t k = 0;  k != levels.size();  ++k)
      if(levels[k].files.size() != 0)
        level_bits |= 1U << k;

    if(level_bits == 0)
      ::fputs("WARNING: Logger is disabled.\n", stderr);

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_levels.swap(levels);
    this->m_conf_level_bits.store(level_bits);
  }

void
Logger::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    while(this->m_queue.empty())
      this->m_queue_avail.wait(lock);

    recursive_mutex::unique_lock io_sync_lock(this->m_sched_mutex);
    this->m_sched_queue.clear();
    this->m_sched_queue.swap(this->m_queue);
    size_t queue_size = this->m_sched_queue.size();
    lock.unlock();

    // Get configuration of all levels.
    lock.lock(this->m_conf_mutex);
    const auto levels = this->m_conf_levels;
    lock.unlock();

    // Write all elements.
    for(const auto& msg : this->m_sched_queue)
      if(msg.level >= levels.size())
        continue;
      else if((queue_size > 1000) && levels[msg.level].expendable)
        continue;
      else
        do_write_nothrow(this->m_sched_files, levels[msg.level], msg);

    this->m_sched_queue.clear();
    this->m_sched_files.clear();
    io_sync_lock.unlock();
    ::sync();
  }

void
Logger::
enqueue(uint8_t level, const char* func, const char* file, uint32_t line, const cow_string& text)
  {
    // Fill in the name and LWP ID of the calling thread.
    X_Message msg;
    msg.level = level;
    msg.thrd_lwpid = (uint32_t) ::syscall(SYS_gettid) & 0xFFFFFFU;

    if(::pthread_getname_np(::pthread_self(), msg.thrd_name, sizeof(msg.thrd_name)) != 0)
      ::strcpy(msg.thrd_name, "unknown");

    msg.func = func;
    msg.file = file;
    msg.line = line;
    msg.text = text;

    // Enqueue the element.
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    this->m_queue.emplace_back(move(msg));
    this->m_queue_avail.notify_one();
  }

void
Logger::
synchronize() noexcept
  {
    // Get all pending elements.
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    recursive_mutex::unique_lock io_sync_lock(this->m_sched_mutex);
    if(this->m_queue.empty())
      return;

    this->m_sched_queue.clear();
    this->m_sched_queue.swap(this->m_queue);
    lock.unlock();

    // Get configuration.
    lock.lock(this->m_conf_mutex);
    const auto levels = this->m_conf_levels;
    lock.unlock();

    // Write all elements.
    for(const auto& msg : this->m_sched_queue)
      if(msg.level < levels.size())
        do_write_nothrow(this->m_sched_files, levels[msg.level], msg);

    this->m_sched_queue.clear();
    this->m_sched_files.clear();
    io_sync_lock.unlock();
    ::sync();
  }

}  // namespace poseidon
