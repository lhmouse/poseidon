// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "base/config_file.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/logger.hpp"
#include "static/timer_driver.hpp"
#include "static/task_executor.hpp"
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
#include "static/mysql_connector.hpp"
#include "static/mongo_connector.hpp"
#include "static/redis_connector.hpp"
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
Usage: %s [OPTIONS] [[--] DIRECTORY [MODULE]...]

  -d      daemonize; detach from terminal and run in background
  -h      show help message then exit
  -V      show version information then exit
  -v      enable verbose mode

If DIRECTORY is specified, it specifies where 'main.conf' is to be located.
The working directory is switched there before configuration is loaded.
Therefore, relative paths in 'main.conf' are relative to 'main.conf' itself,
as one may expect.

If at least one MODULE is specified, the `modules` that are specified in
'main.conf' are ignored, and only those from the command line are loaded. In
this case, they are interpreted as file names, possibly relative before the
working directory is switched. If a module from '/usr/lib' is to be loaded,
it is necessary to specify an absolute path.

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
    cow_vector<cow_string> modules;
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
    logger.synchronize();

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
    ::rocket::unique_ptr<char, void (void*)> abs_path(nullptr, ::free);

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
          cmdline.daemonize = true;
          break;

        case 'h':
          help = true;
          break;

        case 'V':
          version = true;
          break;

        case 'v':
          cmdline.verbose = true;
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

    for(uint32_t k = (uint32_t) ::optind;  (int) k < argc;  ++k) {
      // If more arguments follow, they denote the working directory and
      // modules to load.
      if(!abs_path.reset(::realpath(argv[k], nullptr)))
        do_exit_printf(exit_system_error,
            "%s: invalid path -- '%s': %m",
            argv[0], argv[k]);

      if((int) k == ::optind)
        cmdline.cd_here.assign(abs_path.get());
      else
        cmdline.modules.emplace_back(abs_path.get());
    }
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
do_waitpid_and_exit(::pid_t cpid)
  {
    for(;;) {
      int wstat;
      if(POSEIDON_SYSCALL_LOOP(::waitpid(cpid, &wstat, 0)) == -1)
        do_exit_printf(exit_system_error,
            "Failed to await child process %d: %m", (int) cpid);

      if(WIFEXITED(wstat))
        ::_Exit(WEXITSTATUS(wstat));

      if(WIFSIGNALED(wstat))
        do_exit_printf(128 + WTERMSIG(wstat),
            "Child process %d terminated by signal %d: %s",
            (int) cpid, WTERMSIG(wstat), ::strsignal(WTERMSIG(wstat)));
    }
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
    ::pid_t cpid = ::fork();
    if(cpid == -1)
      do_exit_printf(exit_system_error, "Could not spawn child process: %m");
    else if(cpid != 0) {
      // The PARENT shall always wait for the CHILD and forward its exit
      // status.
      do_waitpid_and_exit(cpid);
    }

    // The CHILD shall create a new session and become its leader. This
    // ensures that a later GRANDCHILD will not be a session leader and
    // will not unintentially gain a controlling terminal.
    ::setsid();

    int pfds[2];
    if(::pipe(pfds) != 0)
      do_exit_printf(exit_system_error, "Could not create pipe: %m");

    cpid = ::fork();
    if(cpid == -1)
      do_exit_printf(exit_system_error, "Could not spawn grandchild process: %m");
    else if(cpid != 0) {
      ::close(pfds[1]);
      if(POSEIDON_SYSCALL_LOOP(::read(pfds[0], &(pfds[1]), 1)) <= 0) {
        // The GRANDCHILD has not exited normally, so wait, and forward
        // its exit status.
        do_waitpid_and_exit(cpid);
      }

      // Detach the GRANDCHILD.
      ::fprintf(stderr, "Process %d detached successfully\n", (int) cpid);
      ::_Exit(0);
    }

    ::close(pfds[0]);
    pfds[0] = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(pfds[0] != -1) {
      // Close standard streams in the GRANDCHILD, as it will be detached
      // from the CHILD. An attempt to write data to these streams shall
      // fail with `EPIPE`. Errors are ignored.
      (void)! ::dup2(pfds[0], STDIN_FILENO);
      (void)! ::dup2(pfds[0], STDOUT_FILENO);
      (void)! ::dup2(pfds[0], STDERR_FILENO);
      ::shutdown(pfds[0], SHUT_RDWR);
      ::close(pfds[0]);
    }

    // The GRANDCHILD shall continue execution.
    daemon_pipe_wfd.reset(pfds[1]);
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
    do_create_resident_thread(logger, "logger");
    do_create_resident_thread(timer_driver, "timer");
    do_create_resident_thread(task_executor, "task_0");
    do_create_resident_thread(task_executor, "task_1");
    do_create_resident_thread(task_executor, "task_2");
    do_create_resident_thread(task_executor, "task_3");
    do_create_resident_thread(task_executor, "task_4");
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
    if(!cmdline.modules.empty())
      return;

    // Get the path to the PID file.
    cow_string pid_file;
    const auto conf = main_config.copy();

    auto conf_value = conf.query("pid_file");
    if(conf_value.is_string())
      pid_file = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `pid_file`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf.path());

    if(pid_file.empty())
      return;

    // Create the lock file and lock it in exclusive mode before overwriting.
    if(!pid_file_fd.reset(::creat(pid_file.safe_c_str(), 0644)))
      POSEIDON_THROW((
          "Could not create PID file '$1'",
          "[`creat()` failed: ${errno:full}]"),
          pid_file);

    if(::flock(pid_file_fd, LOCK_EX | LOCK_NB) != 0)
      POSEIDON_THROW((
          "Could not lock PID file '$1'",
          "[`flock()` failed: ${errno:full}]"),
          pid_file);

    // Write the PID of myself. Errors are ignored.
    POSEIDON_LOG_DEBUG(("Writing current process ID to '$1'"), pid_file.c_str());
    ::dprintf(pid_file_fd, "%d\n", (int) ::getpid());
    ::at_quick_exit([] { (void)! ::ftruncate(pid_file_fd, 0);  });

    // Downgrade the lock so the PID may be read by others.
    ::flock(pid_file_fd, LOCK_SH);
  }

ROCKET_NEVER_INLINE
void
do_seed_random()
  {
    uint64_t seed[2];
    random_bytes(seed, sizeof(seed));

    ::srand(static_cast<uint32_t>(seed[0]));
    ::srand48(static_cast<long>(seed[1]));
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

    if((::getrlimit(RLIMIT_NOFILE, &rlim) == 0) && (rlim.rlim_cur <= 10000))
      POSEIDON_LOG_WARN((
          "The limit of number of open files (which is `$1`) is too low. This might "
          "result in denial of service when there are too many simultaneous network "
          "connections. We suggest you set it to least `10000` for production use. "
          "See `/etc/security/limits.conf` for details."),
          rlim.rlim_cur);
  }

ROCKET_NEVER_INLINE
void
do_load_modules()
  {
    cow_vector<cow_string> modules;

    if(!cmdline.modules.empty()) {
      // Load only modules from the command line.
      modules = cmdline.modules;
    }
    else {
      // Use `modules` from 'main.conf', which shall be an array of strings.
      ::asteria::V_array mods;
      const auto conf = main_config.copy();

      auto conf_value = conf.query("modules");
      if(conf_value.is_array())
        mods = conf_value.as_array();
      else if(!conf_value.is_null())
        POSEIDON_THROW((
            "Invalid `modules`: expecting an `array`, got `$1`",
            "[in configuration file '$2']"),
            conf_value, conf.path());

      for(const auto& r : mods)
        if(r.is_string())
          modules.emplace_back(r.as_string());
        else if(!r.is_null())
          POSEIDON_THROW((
              "Invalid module name: expecting a `string`, got `$1`",
              "[in configuration file '$2']"),
              r, conf.path());
    }

    for(const auto& name : modules) {
      POSEIDON_LOG_INFO(("Loading module: $1"), name);

      void* so_handle = ::dlopen(name.safe_c_str(), RTLD_NOW | RTLD_NODELETE);
      if(!so_handle)
        POSEIDON_THROW((
            "Failed to load module '$1': $2",
            "[`dlopen()` failed]"),
            name, ::dlerror());

      void* so_sym = ::dlsym(so_handle, "_Z20poseidon_module_mainv");
      if(so_sym)
        reinterpret_cast<decltype(poseidon_module_main)*>(so_sym) ();
    }

    if(modules.empty())
      POSEIDON_LOG_FATAL(("No module has been loaded. What's the job now?"));
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
    logger.reload(main_config.copy(), cmdline.verbose);
    do_daemonize_start();
    POSEIDON_LOG_INFO(("Starting up: $1"), PACKAGE_STRING);

    do_check_ulimits();
    do_init_signal_handlers();
    do_write_pid_file();
    do_seed_random();

    mysql_connector.reload(main_config.copy());
    mongo_connector.reload(main_config.copy());
    redis_connector.reload(main_config.copy());

    network_driver.reload(main_config.copy());
    fiber_scheduler.reload(main_config.copy());

    do_create_threads();
    do_load_modules();

    POSEIDON_LOG_INFO(("Startup complete: $1"), PACKAGE_STRING);
    do_daemonize_finish();
    logger.synchronize();

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
