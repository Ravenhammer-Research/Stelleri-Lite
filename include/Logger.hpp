/*
 * Logger.hpp
 * Minimal, generic logging utility. No protocol-specific code here.
 */

#pragma once

#include <chrono>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

#if defined(STELLERI_NETCONF) || STELLERI_NETCONF == 1

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif

#include <libnetconf2/log.h>
#include <libyang/libyang.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif

namespace logger {

#if defined(STELLERI_NETCONF) || STELLERI_NETCONF == 1
  static void nc_print_clb(const struct nc_session * /*session*/,
                           NC_VERB_LEVEL level, const char *msg);

  static void ly_print_clb(LY_LOG_LEVEL level, const char *msg,
                           const char * /*path*/, const char * /*app_tag*/,
                           uint64_t /*line*/);
#endif

  enum class Level { Debug = 0, Info, Warn, Error, Off };

  class Logger {
  public:
    static Logger &instance() {
      static Logger inst;
      return inst;
    }

    void setLevel(Level l) {
      std::lock_guard<std::mutex> g(mu_);
      level_ = l;
    }

    Level level() const { return level_; }

    void setOutput(std::ostream &os) {
      std::lock_guard<std::mutex> g(mu_);
      out_ = &os;
    }

    void log(Level l, const std::string &msg) {
      if (l < level_ || level_ == Level::Off)
        return;
      auto now = std::chrono::system_clock::now();
      std::time_t t = std::chrono::system_clock::to_time_t(now);

      std::stringstream ss;
      ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
      ss << " [" << levelToString(l) << "] " << msg << '\n';

      std::lock_guard<std::mutex> g(mu_);
      if (out_)
        (*out_) << ss.str();
    }

    void debug(const std::string &m) { log(Level::Debug, m); }
    void info(const std::string &m) { log(Level::Info, m); }
    void warn(const std::string &m) { log(Level::Warn, m); }
    void error(const std::string &m) { log(Level::Error, m); }

    // Log a caught std::exception with optional context message.
    void logException(const std::exception &ex,
                      const std::string &context = "") {
      std::string msg;
      if (!context.empty())
        msg = context + ": ";
      msg += "exception: ";
      msg += ex.what();
      log(Level::Error, msg);
    }

    // Log from an exception_ptr (rethrows to extract std::exception::what()).
    void logExceptionPtr(std::exception_ptr ep,
                         const std::string &context = "") {
      if (!ep) {
        log(Level::Error, context.empty() ? "exception: <null>"
                                          : context + ": exception: <null>");
        return;
      }

      try {
        std::rethrow_exception(ep);
      } catch (const std::exception &e) {
        logException(e, context);
      } catch (...) {
        log(Level::Error, context.empty() ? "exception: <non-std>"
                                          : context + ": exception: <non-std>");
      }
    }

  private:
    Logger() : level_(Level::Info), out_(&std::cerr) {
#if defined(STELLERI_NETCONF) || STELLERI_NETCONF == 1
      // Route libnetconf2 internal prints to our logger.
      nc_set_print_clb_session(nc_print_clb);
      // Route libyang internal prints to our logger.
      ly_set_log_clb(ly_print_clb);
#endif
    }

    ~Logger() = default;

    static const char *levelToString(Level l) {
      switch (l) {
      case Level::Debug:
        return "DEBUG";
      case Level::Info:
        return "INFO";
      case Level::Warn:
        return "WARN";
      case Level::Error:
        return "ERROR";
      default:
        return "UNKNOWN";
      }
    }

    mutable std::mutex mu_;
    Level level_;
    std::ostream *out_;
  };

  inline Logger &get() { return Logger::instance(); }


#if defined(STELLERI_NETCONF) || STELLERI_NETCONF == 1

  // Forward libnetconf2 print messages into our logger.
  static void nc_print_clb(const struct nc_session * /*session*/,
                          NC_VERB_LEVEL level, const char *msg) {
    auto &log = logger::get();
    std::string m = "libnetconf2: ";
    m += (msg ? msg : "");

    switch (level) {
      case NC_VERB_ERROR:
        log.error(m);
        break;
      case NC_VERB_WARNING:
        log.warn(m);
        break;
      case NC_VERB_VERBOSE:
        log.info(m);
        break;
      case NC_VERB_DEBUG:
      case NC_VERB_DEBUG_LOWLVL:
        log.debug(m);
        break;
      default:
        log.info(m);
        break;
    }
  }

  // Forward libyang print messages into our logger.
  static void ly_print_clb(LY_LOG_LEVEL level, const char *msg,
                          const char * /*path*/, const char * /*app_tag*/,
                          uint64_t /*line*/) {
    auto &log = logger::get();
    std::string m = "libyang: ";
    m += (msg ? msg : "");

    switch (level) {
      case LY_LLERR:
        log.error(m);
        break;
      case LY_LLWRN:
        log.warn(m);
        break;
      case LY_LLVRB:
        log.info(m);
        break;
      case LY_LLDBG:
        log.debug(m);
        break;
      default:
        log.info(m);
        break;
    }
  }

#endif
} // namespace logger
