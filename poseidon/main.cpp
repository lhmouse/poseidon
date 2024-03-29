// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "base/config_file.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/async_logger.hpp"
#include "static/timer_driver.hpp"
#include "static/async_task_executor.hpp"
#include "static/network_driver.hpp"
#include "utils.hpp"
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#ifdef POSEIDON_ENABLE_MYSQL
#include "static/mysql_connector.hpp"
#endif
#ifdef POSEIDON_ENABLE_MONGO
#include "static/mongo_connector.hpp"
#endif
#ifdef POSEIDON_ENABLE_REDIS
#include "static/redis_connector.hpp"
#endif
namespace {
using namespace poseidon;

#define PACKAGE_STRING      "poseidon master"
#define PACKAGE_URL         "https://github.com/lhmouse/poseidon"
#define PACKAGE_BUGREPORT   "lh_mouse@126.com"

[[noreturn]]
int
do_print_help_and_exit(const char* self)
  {
    ::printf(
//        1         2         3         4         5         6         7     |
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
Usage: %s [OPTIONS] [[--] DIRECTORY]

  -d      daemonize; detach from terminal and run in background
  -h      show help message then exit
  -V      show version information then exit
  -v      enable verbose mode

If DIRECTORY is specified, the working directory is switched there before
doing everything else.

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
//        1         2         3         4         5         6         7     |
      self,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    ::fflush(nullptr);
    ::quick_exit(0);
  }

[[noreturn]]
int
do_print_version_and_exit()
  {
    ::printf(
//        1         2         3         4         5         6         7     |
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
%s (internal %s)

Visit the homepage at <%s>.
Report bugs to <%s>.
)'''''''''''''''" """"""""""""""""""""""""""""""""""""""""""""""""""""""""+1,
// 3456789012345678901234567890123456789012345678901234567890123456789012345|
//        1         2         3         4         5         6         7     |
      PACKAGE_STRING, POSEIDON_ABI_VERSION_STRING,
      PACKAGE_URL,
      PACKAGE_BUGREPORT);

    ::fflush(nullptr);
    ::quick_exit(0);
  }

// Define command-line options here.
struct Command_Line_Options
  {
    // options
    bool daemonize = false;
    bool verbose = false;

    // non-options
    cow_string cd_here;
  };

// They are declared here for convenience.
Command_Line_Options cmdline;
unique_posix_fd daemon_pipe_wfd;
unique_posix_fd pid_file_fd;

// These are process exit status codes.
enum
  {
    exit_success            = 0,
    exit_system_error       = 1,
    exit_invalid_argument   = 2,
  };

[[noreturn]] ROCKET_NEVER_INLINE
int
do_exit_printf(int code, const char* fmt = nullptr, ...) noexcept
  {
    // Wait for pending logs to be flushed.
    ::fflush(nullptr);
    async_logger.synchronize();

    if(fmt) {
      // Output the string to standard error.
      ::va_list ap;
      va_start(ap, fmt);
      ::vfprintf(stderr, fmt, ap);
      va_end(ap);
    }

    // Perform fast exit.
    ::fputc('\n', stderr);
    ::quick_exit(code);
  }

ROCKET_NEVER_INLINE
void
do_parse_command_line(int argc, char** argv)
  {
    bool help = false;
    bool version = false;

    opt<bool> daemonize;
    opt<bool> verbose;
    opt<cow_string> cd_here;

    if(argc > 1) {
      // Check for common long options before calling `getopt()`.
      if(::strcmp(argv[1], "--help") == 0)
        do_print_help_and_exit(argv[0]);

      if(::strcmp(argv[1], "--version") == 0)
        do_print_version_and_exit();
    }

    // Parse command-line options.
    int ch;
    while((ch = ::getopt(argc, argv, "dhVv")) != -1)
      switch(ch)
        {
        case 'd':
          daemonize = true;
          break;

        case 'h':
          help = true;
          break;

        case 'V':
          version = true;
          break;

        case 'v':
          verbose = true;
          break;

        default:
          do_exit_printf(exit_invalid_argument,
              "%s: invalid argument -- '%c'\nTry `%s -h` for help.",
              argv[0], ::optopt, argv[0]);
        }

    // Check for early exit conditions.
    if(help)
      do_print_help_and_exit(argv[0]);

    if(version)
      do_print_version_and_exit();

    // If more arguments follow, they denote the working directory.
    if(argc - ::optind > 1)
      do_exit_printf(exit_invalid_argument,
          "%s: too many arguments -- '%s'\nTry `%s -h` for help.",
          argv[0], argv[::optind+1], argv[0]);

    if(argc - ::optind > 0)
      cd_here = cow_string(argv[::optind]);

    // Daemonization is off by default.
    if(daemonize)
      cmdline.daemonize = *daemonize;

    // Verbose mode is off by default.
    if(verbose)
      cmdline.verbose = *verbose;

    // The default working directory is empty which means 'do not switch'.
    if(cd_here)
      cmdline.cd_here = move(*cd_here);
  }

ROCKET_NEVER_INLINE
void
do_set_working_directory()
  {
    if(cmdline.cd_here.empty())
      return;

    if(::chdir(cmdline.cd_here.safe_c_str()) != 0)
      do_exit_printf(exit_system_error,
          "Could not set working directory to '%s': %m",
          cmdline.cd_here.c_str());
  }

[[noreturn]]
int
do_await_child_process_and_exit(::pid_t cpid)
  {
    ::fprintf(stderr, "Awaiting child process %d...\n", (int) cpid);
    int wstat = 0;

    ::pid_t r = POSEIDON_SYSCALL_LOOP(::waitpid(cpid, &wstat, 0));
    if(r < 0)
      do_exit_printf(exit_system_error, "Failed to await child process: %m");

    if(WIFEXITED(wstat))
      do_exit_printf(WEXITSTATUS(wstat));

    if(WIFSIGNALED(wstat))
      do_exit_printf(128 + WTERMSIG(wstat),
          "Child process %d terminated by signal %d: %s",
          (int) cpid, WTERMSIG(wstat), ::strsignal(WTERMSIG(wstat)));

    ::_Exit(1);
  }

ROCKET_NEVER_INLINE
int
do_daemonize_start()
  {
    if(!cmdline.daemonize)
      return 0;

    //  PARENT            CHILD             GRANDCHILD
    // --------------------------------------------------
    //  fork()
    //    |   \---------> setsid()
    //    |               pipe()
    //    |               fork()
    //    |                 |   \---------> initialize ...
    //    |                 |                 |
    //    |                 |                 v
    //    |                 v               write(pipe)
    //    |               read(pipe) <-----/  |
    //    v               _Exit()             |
    //  waitpid() <------/                    v
    //  _Exit()                             loop ...
    ::fprintf(stderr, "Daemonizing process %d...\n", (int) ::getpid());

    // Create the CHILD and wait.
    ::pid_t cpid = ::fork();
    if(cpid == -1)
      do_exit_printf(exit_system_error, "Could not spawn child process: %m");

    if(cpid != 0) {
      // The PARENT shall wait for CHILD and forward its exit status.
      do_await_child_process_and_exit(cpid);
    }

    // The CHILD shall create a new session and become its leader. This
    // ensures that a later GRANDCHILD will not be a session leader and
    // will not unintentially gain a controlling terminal.
    ::setsid();

    int pipefds[2];
    if(::pipe(pipefds) != 0)
      do_exit_printf(exit_system_error, "Could not create pipe: %m");

    unique_posix_fd rfd(pipefds[0]);
    unique_posix_fd wfd(pipefds[1]);

    // Create the GRANDCHILD.
    cpid = ::fork();
    if(cpid == -1)
      do_exit_printf(exit_system_error, "Could not spawn grandchild process: %m");

    if(cpid != 0) {
      // The CHILD shall wait for notification from the GRANDCHILD. If no
      // notification is received or an error occurs, the GRANDCHILD will
      // terminate and has to be reaped.
      char text[16];
      if(POSEIDON_SYSCALL_LOOP(::read(pipefds[0], text, sizeof(text))) > 0)
        ::_Exit(0);

      // The GRANDCHILD has not exited normally, so wait and forward its
      // exit status.
      do_await_child_process_and_exit(cpid);
    }

    // Close standard streams in the GRANDCHILD. Errors are ignored.
    rfd.reset(::socket(AF_UNIX, SOCK_STREAM, 0));
    if(rfd) {
      ::shutdown(rfd, SHUT_RDWR);

      (void)! ::dup2(rfd, STDIN_FILENO);
      (void)! ::dup2(rfd, STDOUT_FILENO);
      (void)! ::dup2(rfd, STDERR_FILENO);
    }

    // The GRANDCHILD shall continue execution.
    daemon_pipe_wfd = move(wfd);
    return 1;
  }

ROCKET_NEVER_INLINE
void
do_daemonize_finish()
  {
    if(!daemon_pipe_wfd)
      return;

    // Notify the parent process. Errors are ignored.
    POSEIDON_SYSCALL_LOOP(::write(daemon_pipe_wfd, "OK", 2));
    daemon_pipe_wfd.reset();
  }

template<class xObject>
void
do_create_resident_thread(xObject& obj, const char* name)
  {
    static ::sigset_t s_blocked_signals[1];

    // Initialize the list of signals to block. This is done only once.
    if(::sigismember(s_blocked_signals, SIGINT) == 0)
      for(int sig : { SIGINT, SIGTERM, SIGHUP, SIGALRM, SIGQUIT })
        ::sigaddset(s_blocked_signals, sig);

    // Create the thread now.
    ::pthread_t thrd;
    int err = ::pthread_create(
      &thrd,
      nullptr,
      +[](void* thread_param) -> void*
      {
        auto& xobj = *(xObject*) thread_param;

        ::pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
        ::pthread_sigmask(SIG_BLOCK, s_blocked_signals, nullptr);

        for(;;)
          try {
            xobj.thread_loop();
          }
          catch(exception& stdex) {
            ::fprintf(stderr,
                "WARNING: Caught an exception from thread loop: %s\n"
                "[static class `%s`]\n"
                "[exception class `%s`]\n",
                stdex.what(), typeid(xobj).name(), typeid(stdex).name());
          }
      },
      ::std::addressof(obj)
    );

    if(err != 0)
      do_exit_printf(exit_system_error, "Could not spawn thread '%s': %m", name);

    // Name the thread and detach it. Errors are ignored.
    ::pthread_setname_np(thrd, name);
    ::pthread_detach(thrd);
  }

ROCKET_NEVER_INLINE
void
do_create_threads()
  {
    do_create_resident_thread(async_logger, "logger");
    do_create_resident_thread(timer_driver, "timer");
    do_create_resident_thread(async_task_executor, "task_0");
    do_create_resident_thread(async_task_executor, "task_1");
    do_create_resident_thread(async_task_executor, "task_2");
    do_create_resident_thread(async_task_executor, "task_3");
    do_create_resident_thread(async_task_executor, "task_4");
    do_create_resident_thread(network_driver, "network");
  }

ROCKET_NEVER_INLINE
void
do_init_signal_handlers()
  {
    // Ignore certain signals for good.
    struct ::sigaction sigact = { };
    sigact.sa_handler = SIG_IGN;
    ::sigaction(SIGPIPE, &sigact, nullptr);
    ::sigaction(SIGCHLD, &sigact, nullptr);

#ifdef POSEIDON_USE_NON_CALL_EXCEPTIONS
    // Install fault handlers. Errors are ignored.
    sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = +[](int sig, ::siginfo_t* info, void* /*uctx*/)
      {
        const char* sig_name = "unknown";
        const char* sig_desc = "Unknown signal";

        switch(sig) {
#define do_signal_case_(name, desc)  \
          case (name):  sig_name = #name;  sig_desc = desc;  break  // no semicolon

          do_signal_case_(SIGTRAP, "Breakpoint");
          do_signal_case_(SIGBUS, "Alignment fault");
          do_signal_case_(SIGFPE, "Arithmetic exception");
          do_signal_case_(SIGILL, "Illegal instruction");
          do_signal_case_(SIGSEGV, "General protection fault");
        };

        POSEIDON_THROW((
            "Deadly signal $1 ($2) received: $3 at address $4"),
            sig, sig_name, sig_desc, info->si_addr);
      };

    ::sigaction(SIGTRAP, &sigact, nullptr);
    ::sigaction(SIGBUS, &sigact, nullptr);
    ::sigaction(SIGFPE, &sigact, nullptr);
    ::sigaction(SIGILL, &sigact, nullptr);
    ::sigaction(SIGSEGV, &sigact, nullptr);
#endif

    // Install termination handlers. Errors are ignored.
    sigact.sa_flags = 0;
    sigact.sa_handler = +[](int n) { exit_signal.store(n);  };

    ::sigaction(SIGINT, &sigact, nullptr);
    ::sigaction(SIGTERM, &sigact, nullptr);
    ::sigaction(SIGHUP, &sigact, nullptr);
    ::sigaction(SIGALRM, &sigact, nullptr);
  }

ROCKET_NEVER_INLINE
void
do_write_pid_file()
  {
    cow_string pid_file_path;
    const auto conf = main_config.copy();

    auto conf_value = conf.query("general", "pid_file_path");
    if(conf_value.is_string())
      pid_file_path = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `general.pid_file_path`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf.path());

    if(pid_file_path.empty())
      return;

    // Create the lock file and lock it in exclusive mode before overwriting.
    if(!pid_file_fd.reset(::creat(pid_file_path.safe_c_str(), 0644)))
      POSEIDON_THROW((
          "Could not create PID file '$1'",
          "[`creat()` failed: ${errno:full}]"),
          pid_file_path);

    if(::flock(pid_file_fd, LOCK_EX | LOCK_NB) != 0)
      POSEIDON_THROW((
          "Could not lock PID file '$1'",
          "[`flock()` failed: ${errno:full}]"),
          pid_file_path);

    // Write the PID of myself. Errors are ignored.
    POSEIDON_LOG_DEBUG(("Writing current process ID to '$1'"), pid_file_path.c_str());
    ::dprintf(pid_file_fd, "%d\n", (int) ::getpid());
    ::at_quick_exit([] { (void)! ::ftruncate(pid_file_fd, 0);  });

    // Downgrade the lock so the PID may be read by others.
    ::flock(pid_file_fd, LOCK_SH);
  }

ROCKET_NEVER_INLINE
void
do_seed_random()
  {
    ::srand(random_uint32());
    ::srand48(static_cast<long>(random_uint64()));
  }

ROCKET_NEVER_INLINE
void
do_check_ulimits()
  {
    ::rlimit rlim;
    if((::getrlimit(RLIMIT_CORE, &rlim) == 0) && (rlim.rlim_cur <= 0))
      POSEIDON_LOG_WARN((
          "Core dumps have been disabled. We highly suggest you enable them in case "
          "of crashes. See `/etc/security/limits.conf` for details."));

    if((::getrlimit(RLIMIT_NOFILE, &rlim) == 0) && (rlim.rlim_cur <= 10'000))
      POSEIDON_LOG_WARN((
          "The limit of number of open files (which is `$1`) is too low. This might "
          "result in denial of service when there are too many simultaneous network "
          "connections. We suggest you set it to least `10000` for production use. "
          "See `/etc/security/limits.conf` for details."),
          rlim.rlim_cur);
  }

ROCKET_NEVER_INLINE
void
do_load_addons()
  {
    ::asteria::V_array addons;
    const auto conf = main_config.copy();
    bool empty = true;

    auto conf_value = conf.query("addons");
    if(conf_value.is_array())
      addons = conf_value.as_array();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `addons`: expecting an `array`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf.path());

    for(const auto& addon : addons) {
      cow_string path;

      if(addon.is_string())
        path = addon.as_string();
      else if(!addon.is_null())
        POSEIDON_THROW((
            "Invalid path to add-on: expecting a `string`, got `$1`",
            "[in configuration file '$2']"),
            addon, conf.path());

      if(path.empty())
        continue;

      // Load the shared library.
      POSEIDON_LOG_INFO(("Loading add-on: $1"), path);
      void* so_handle = ::dlopen(path.safe_c_str(), RTLD_NOW | RTLD_NODELETE);
      if(!so_handle)
        POSEIDON_THROW((
            "Failed to load add-on: $1",
            "[`dlopen()` failed: $2]"),
            path, ::dlerror());

      for(const char* fn_name : { "_Z19poseidon_addon_mainv", "poseidon_addon_main" })
        if(void* fn = ::dlsym(so_handle, fn_name)) {
          // Check whether it is a function...?
          POSEIDON_LOG_INFO(("Invoking `poseidon_addon_main()` in add-on: $1"), path);
          reinterpret_cast<decltype(poseidon_addon_main)*>(fn) ();
          break;
        }

      POSEIDON_LOG_DEBUG(("Finished loading add-on: $1"), path);
      empty = false;
    }

    if(empty)
      POSEIDON_LOG_FATAL(("No add-on has been loaded. What's the job now?"));
  }

}  // namespace

int
main(int argc, char** argv)
  try {
    // Select the C locale.
    // UTF-8 is required for wide-oriented standard streams.
    ::setlocale(LC_ALL, "C.UTF-8");
    ::tzset();
    ::pthread_setname_np(::pthread_self(), "poseidon");
    ::pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

    // Note that this function shall not return in case of errors.
    do_parse_command_line(argc, argv);
    do_set_working_directory();
    main_config.reload();
    async_logger.reload(main_config.copy());
    do_daemonize_start();
    POSEIDON_LOG_INFO(("Starting up: $1"), PACKAGE_STRING);

    do_check_ulimits();
    do_init_signal_handlers();
    do_write_pid_file();
    do_seed_random();

#ifdef POSEIDON_ENABLE_MYSQL
    mysql_connector.reload(main_config.copy());
#endif
#ifdef POSEIDON_ENABLE_MONGO
    mongo_connector.reload(main_config.copy());
#endif
#ifdef POSEIDON_ENABLE_REDIS
    redis_connector.reload(main_config.copy());
#endif

    network_driver.reload(main_config.copy());
    fiber_scheduler.reload(main_config.copy());

    do_create_threads();
    do_load_addons();

    POSEIDON_LOG_INFO(("Startup complete: $1"), PACKAGE_STRING);
    do_daemonize_finish();
    async_logger.synchronize();

    // Schedule fibers if there is something to do, or no stop signal has
    // been received.
    while((fiber_scheduler.size() != 0) || (exit_signal.load() == 0))
      fiber_scheduler.thread_loop();

    int sig = exit_signal.load();
    POSEIDON_LOG_INFO(("Shutting down (signal $1: $2)"), sig, ::strsignal(sig));
    do_exit_printf(exit_success);
  }
  catch(exception& stdex) {
    // Print the message in `stdex`. There isn't much we can do.
    POSEIDON_LOG_FATAL(("$1"), stdex);
    do_exit_printf(exit_system_error, "%s", stdex.what());
  }
