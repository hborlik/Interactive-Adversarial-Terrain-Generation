/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-01-20
 * 
 * 
 * 
 */
#pragma once

#include <iostream>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <ios>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <type_traits>

#include <syslog.h>
#include <cxxabi.h>

namespace util::logging {


template<typename T>
inline std::string to_string(const T& v) noexcept {
    return std::to_string(v);
}

template<>
inline std::string to_string<std::string>(const std::string& v) noexcept {
    return v;
}

inline std::string getLogTime(std::chrono::time_point<std::chrono::system_clock> time) {
    auto epoch_seconds = std::chrono::system_clock::to_time_t(time);
    std::stringstream stream;
    stream << std::put_time(gmtime(&epoch_seconds), "[%F_%T");
    auto truncated = std::chrono::system_clock::from_time_t(epoch_seconds);
    auto delta_us = std::chrono::duration_cast<std::chrono::milliseconds>(time - truncated).count();
    stream << "." << std::fixed << std::setw(3) << std::setfill('0') << delta_us << "]";
    return stream.str();
}

enum class Verbosity : int {
    Info        = LOG_INFO,
    Notice      = LOG_NOTICE,
    Warning     = LOG_WARNING,
    Error       = LOG_ERR,
    Debug       = LOG_DEBUG,
    Trace       = 50
};

/**
 * @brief Base logger class
 * 
 */
class Logger {
public:
    virtual ~Logger() = default;

    virtual void log(Verbosity level, const std::string& message) const = 0;
    void info(const std::string& message) const;
    void notice(const std::string& message) const;
    void warning(const std::string& message) const;
    void error(const std::string& message) const;
    void debug(const std::string& message) const;
    void trace(const std::string& message) const;
};

/**
 * @brief Logger that prepends its name to all outputs
 * 
 */
class NamedLogger : public Logger {
    std::string name;
public:
    NamedLogger(std::string name);
    void log(Verbosity level, const std::string& message) const override;
};

class LogWriter {
public:
    LogWriter(Verbosity v) : LogVerbosity{v} {}
    virtual ~LogWriter() {}
    virtual void write(const std::string& message) {}

    const Verbosity LogVerbosity;
};

class FileLogWriter : public LogWriter {
    std::ofstream output_file;
public:
    FileLogWriter(std::filesystem::path filepath, Verbosity v) : LogWriter{v}, output_file{filepath} {}

    void write(const std::string_view message) override;
};

/**
 * @brief general purpose logger
 * 
 */
class LogManager {
public:
    template<typename... Args>
    void log(Verbosity level, const Args&&... args);

    template<typename... Args>
    void info(const Args&&... args) {
        log(Verbosity::Info, std::move(args)...);
    }

    template<typename... Args>
    void notice(const Args&&... args) {
        log(Verbosity::Notice, std::move(args)...);
    }

    template<typename... Args>
    void warning(const Args&&... args) {
        log(Verbosity::Warning, std::move(args)...);
    }

    void error(const std::string& message);

    /**
     * @brief utility function to get name of verbosity level
     * 
     * @param v 
     * @return std::string 
     */
    std::string verbosity_name(Verbosity v) const noexcept;

    /**
     * @brief Construct A logger
     * 
     * @tparam T 
     * @tparam Args 
     * @tparam std::enable_if<std::is_base_of<Logger, T>::value> 
     * @param args 
     * @return std::unique_ptr<Logger> 
     */
    template<typename T, typename = std::enable_if<std::is_class<T>::value>>
    static std::unique_ptr<Logger> make_logger() {
        int status;
        return std::make_unique<NamedLogger>(
            abi::__cxa_demangle(typeid(T).name(), 0, 0, &status)
            );
    }

    template<typename T, typename... Args>
    void add_writer(const Args&&... args) {
        log_writers.push_back(std::make_unique<T>(std::move(args)...));
    }

protected:
    std::mutex writerLock;

    std::vector<std::unique_ptr<LogWriter>> log_writers;
};

template<typename... Args>
void LogManager::log(Verbosity level, const Args&&... args) {
    std::lock_guard<std::mutex> _lock{writerLock};    
    std::string output_message = getLogTime(std::chrono::system_clock::now()) +
        " : " + verbosity_name(level) + " : " + (to_string(args) + ... + "") + "\n";
    std::clog << output_message;
    for(auto& lw : log_writers) {
        if ((int)(lw->LogVerbosity) >= (int)level)
            lw->write(output_message);
    }
    // syslog(LOG_INFO, message.c_str(), args...);
}

} // sprinkler::logging