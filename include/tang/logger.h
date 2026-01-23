#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace tang {

/**
 * Log level enumeration
 */
enum class LogLevel {
    DEBUG_LEVEL,
    INFO_LEVEL,
    WARN_LEVEL,
    ERROR_LEVEL
};

/**
 * Logger class for unified logging management
 */
class Logger {
private:
    std::string module_name_;
    LogLevel level_;
    static std::mutex log_mutex_;
    
    /**
     * Get current timestamp string
     */
    static std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        
        // Use safe version of localtime for Windows compatibility
        #ifdef _WIN32
            std::tm tm_info;
            localtime_s(&tm_info, &time_t);
            ss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S");
        #else
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        #endif
        
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    /**
     * Get log level string
     */
    static std::string get_level_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG_LEVEL: return "DEBUG";
            case LogLevel::INFO_LEVEL: return "INFO";
            case LogLevel::WARN_LEVEL: return "WARN";
            case LogLevel::ERROR_LEVEL: return "ERROR";
            default: return "UNKNOWN";
        }
    }
    
    /**
     * Check if log should be output based on level
     */
    bool should_log(LogLevel message_level) const {
        return static_cast<int>(message_level) >= static_cast<int>(level_);
    }
    
    /**
     * Format log message
     */
    std::string format_message(LogLevel message_level, const std::string& message, const std::string& file, int line) const {
        // Extract filename from path
        std::string filename = file;
        size_t last_slash = file.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            filename = file.substr(last_slash + 1);
        }
        
        std::stringstream ss;
        ss << "[" << get_timestamp() << "]"
           << "[" << get_level_string(message_level) << "]"
           << "[" << module_name_ << "]"
           << "[" << filename << ":" << line << "] "
           << message;
        return ss.str();
    }
    
    /**
     * Output log message to console
     */
    void output_message(LogLevel message_level, const std::string& formatted_message) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        
        if (message_level == LogLevel::ERROR_LEVEL) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }

public:
    /**
     * Constructor
     * @param module_name Name of the module using this logger
     * @param level Minimum log level to output
     */
    Logger(const std::string& module_name, LogLevel level = LogLevel::INFO_LEVEL)
        : module_name_(module_name), level_(level) {}
    
    /**
     * Log a debug message
     */
    void debug(const std::string& message, const std::string& file, int line) {
        if (should_log(LogLevel::DEBUG_LEVEL)) {
            output_message(LogLevel::DEBUG_LEVEL, format_message(LogLevel::DEBUG_LEVEL, message, file, line));
        }
    }
    
    /**
     * Log an info message
     */
    void info(const std::string& message, const std::string& file, int line) {
        if (should_log(LogLevel::INFO_LEVEL)) {
            output_message(LogLevel::INFO_LEVEL, format_message(LogLevel::INFO_LEVEL, message, file, line));
        }
    }
    
    /**
     * Log a warning message
     */
    void warn(const std::string& message, const std::string& file, int line) {
        if (should_log(LogLevel::WARN_LEVEL)) {
            output_message(LogLevel::WARN_LEVEL, format_message(LogLevel::WARN_LEVEL, message, file, line));
        }
    }
    
    /**
     * Log an error message
     */
    void error(const std::string& message, const std::string& file, int line) {
        if (should_log(LogLevel::ERROR_LEVEL)) {
            output_message(LogLevel::ERROR_LEVEL, format_message(LogLevel::ERROR_LEVEL, message, file, line));
        }
    }
    
    /**
     * Set log level for this logger
     */
    void set_level(LogLevel level) {
        level_ = level;
    }
    
    /**
     * Get current log level
     */
    LogLevel get_level() const {
        return level_;
    }
};

// Global logger instances for different modules
namespace logger {
    extern Logger runtime;
    extern Logger channel;
    extern Logger task;
    extern Logger test;
    extern Logger example;
    
    /**
     * Initialize global loggers
     */
    void init();
    
    /**
     * Set global log level
     */
    void set_level(LogLevel level);
}

// Convenience macros for logging
#define LOG_DEBUG(logger, msg) logger.debug(msg, __FILE__, __LINE__)
#define LOG_INFO(logger, msg) logger.info(msg, __FILE__, __LINE__)
#define LOG_WARN(logger, msg) logger.warn(msg, __FILE__, __LINE__)
#define LOG_ERROR(logger, msg) logger.error(msg, __FILE__, __LINE__)

#define LOG_DEBUG_FUNC(logger) logger.debug(std::string(__FUNCTION__) + " called", __FILE__, __LINE__)
#define LOG_DEBUG_FUNC_WITH_HANDLE(logger, handle) \
    do { \
        std::stringstream ss; \
        ss << __FUNCTION__ << " - handle: " << (handle ? "valid" : "null") \
           << ", done: " << (handle ? (handle.done() ? "true" : "false") : "N/A"); \
        logger.debug(ss.str(), __FILE__, __LINE__); \
    } while(0)

} // namespace tang