// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "async_logger.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <time.h>
#include <sys/syscall.h>
namespace poseidon {
namespace {

struct Level_Config
  {
    char tag[16];
    cow_string color;
    cow_string file;
    bool p_stdout = false;
    bool p_stderr = false;
    bool trivial = false;
  };

struct Log_Message
  {
    uint32_t level : 8;
    uint32_t thrd_lwpid : 24;
    char thrd_name[16] = "unknown";
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
do_load_level_config(Level_Config& lconf, const Config_File& conf_file, const char* name)
  {
    // Set the tag. Three characters shall be reserved for the pair of brackets
    // and the null terminator.
    char* tag_wp = lconf.tag;
    xstrrpcpy(tag_wp, "[");
    xmemrpcpy(tag_wp, name, min(xstrlen(name), sizeof(lconf.tag) - 3));
    xstrrpcpy(tag_wp, "]");

    // Read settings.
    auto conf_value = conf_file.query("logger", name, "color");
    if(conf_value.is_string())
      lconf.color = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `logger.$1.color`: expecting a `string`, got `$2`",
          "[in configuration file '$3']"),
          name, conf_value, conf_file.path());

    conf_value = conf_file.query("logger", name, "file");
    if(conf_value.is_string())
      lconf.file = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `logger.$1.file`: expecting a `string`, got `$2`",
          "[in configuration file '$3']"),
          name, conf_value, conf_file.path());

    conf_value = conf_file.query("logger", name, "stdout");
    if(conf_value.is_boolean())
      lconf.p_stdout = conf_value.as_boolean();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `logger.$1.stdout`: expecting a `boolean`, got `$2`",
          "[in configuration file '$3']"),
          name, conf_value, conf_file.path());

    conf_value = conf_file.query("logger", name, "stderr");
    if(conf_value.is_boolean())
      lconf.p_stderr = conf_value.as_boolean();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `logger.$1.stderr`: expecting a `boolean`, got `$2`",
          "[in configuration file '$3']"),
          name, conf_value, conf_file.path());

    conf_value = conf_file.query("logger", name, "trivial");
    if(conf_value.is_boolean())
      lconf.trivial = conf_value.as_boolean();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `logger.$1.trivial`: expecting a `boolean`, got `$2`",
          "[in configuration file '$3']"),
          name, conf_value, conf_file.path());
  }

inline
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

void
do_write_nothrow(const Level_Config& lconf, const Log_Message& msg) noexcept
  try {
    linear_buffer mtext;
    ::rocket::ascii_numput nump;

    // Get the local system time.
    ::timespec tv;
    ::clock_gettime(CLOCK_REALTIME, &tv);
    ::tm tm;
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
    mtext.puts(lconf.tag);
    do_color(mtext, lconf, "0");  // reset
    mtext.putc(' ');

    // Write the message.
    do_color(mtext, lconf, "0");  // reset
    do_color(mtext, lconf, lconf.color.c_str());

    for(size_t k = 0;  k != msg.text.size();  ++k) {
      // Get the replacement sequence.
      uint32_t uch = static_cast<uint8_t>(msg.text[k]);
      const char* seq = s_escapes[uch];

      if(seq[0] == '\\') {
        // non-printable
        do_color(mtext, lconf, "7");
        mtext.puts(seq);
        do_color(mtext, lconf, "27");
      }
      else if(ROCKET_EXPECT(seq[1] == 0))
        mtext.putc(seq[0]);
      else
        mtext.puts(seq);
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
    mtext.puts(" '");
    mtext.puts(msg.thrd_name);
    mtext.puts("' ");

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

    // Write the message. Errors are ignored.
    if(lconf.p_stdout)
      (void)! ::write(STDOUT_FILENO, mtext.data(), mtext.size());

    if(lconf.p_stderr)
      (void)! ::write(STDERR_FILENO, mtext.data(), mtext.size());

    if(lconf.file != "")
      if(const auto fd = unique_posix_fd(::open(lconf.file.c_str(),
                                    O_WRONLY | O_APPEND | O_CREAT, 0644)))
        (void)! ::write(fd, mtext.data(), mtext.size());
  }
  catch(exception& stdex) {
    ::fprintf(stderr,
        "WARNING: Failed to write log message: %s\n"
        "[exception class `%s`]\n",
        stdex.what(), typeid(stdex).name());
  }

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Async_Logger, Level_Config);
POSEIDON_HIDDEN_X_STRUCT(Async_Logger, Log_Message);

Async_Logger::
Async_Logger() noexcept
  {
  }

Async_Logger::
~Async_Logger()
  {
  }

void
Async_Logger::
reload(const Config_File& conf_file)
  {
    // Hold the logging thread.
    recursive_mutex::unique_lock io_sync_lock(this->m_io_mutex);
    this->m_conf_level_bits.store(UINT32_MAX);

    // Parse new configuration.
    cow_vector<X_Level_Config> levels;
    levels.resize(6);
    do_load_level_config(levels.mut(0), conf_file, "fatal");
    do_load_level_config(levels.mut(1), conf_file, "error");
    do_load_level_config(levels.mut(2), conf_file, "warn");
    do_load_level_config(levels.mut(3), conf_file, "info");
    do_load_level_config(levels.mut(4), conf_file, "debug");
    do_load_level_config(levels.mut(5), conf_file, "trace");

    uint32_t level_bits = 0;
    for(size_t k = 0;  k != levels.size();  ++k)
      if((levels[k].file != "") || levels[k].p_stdout || levels[k].p_stderr)
        level_bits |= 1U << k;

    if(level_bits == 0)
      ::fputs("WARNING: Logger disabled\n", stderr);

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_levels.swap(levels);
    this->m_conf_level_bits.store(level_bits);
  }

void
Async_Logger::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    while(this->m_queue.empty())
      this->m_queue_avail.wait(lock);

    recursive_mutex::unique_lock io_sync_lock(this->m_io_mutex);
    this->m_io_queue.clear();
    this->m_io_queue.swap(this->m_queue);
    bool too_much = this->m_io_queue.size() <= 1024U;
    lock.unlock();

    // Get configuration of all levels.
    lock.lock(this->m_conf_mutex);
    const auto levels = this->m_conf_levels;
    lock.unlock();

    // Write all elements.
    for(const auto& msg : this->m_io_queue)
      if((msg.level < levels.size()) && !(too_much && levels[msg.level].trivial))
        do_write_nothrow(levels[msg.level], msg);

    this->m_io_queue.clear();
    io_sync_lock.unlock();
    ::sync();
  }

void
Async_Logger::
enqueue(uint8_t level, const char* func, const char* file, uint32_t line, cow_stringR text)
  {
    // Fill in the name and LWP ID of the calling thread.
    X_Log_Message msg;
    msg.level = level;
    msg.thrd_lwpid = (uint32_t) ::syscall(SYS_gettid) & 0xFFFFFFU;
    ::pthread_getname_np(::pthread_self(), msg.thrd_name, sizeof(msg.thrd_name));
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
Async_Logger::
synchronize() noexcept
  {
    // Get all pending elements.
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    recursive_mutex::unique_lock io_sync_lock(this->m_io_mutex);
    if(this->m_queue.empty())
      return;

    this->m_io_queue.clear();
    this->m_io_queue.swap(this->m_queue);
    lock.unlock();

    // Get configuration.
    lock.lock(this->m_conf_mutex);
    const auto levels = this->m_conf_levels;
    lock.unlock();

    // Write all elements.
    for(const auto& msg : this->m_io_queue)
      if(msg.level < levels.size())
        do_write_nothrow(levels[msg.level], msg);

    this->m_io_queue.clear();
    io_sync_lock.unlock();
    ::sync();
  }

}  // namespace poseidon
