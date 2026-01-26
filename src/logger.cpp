#include "tang/logger.h"

namespace tang {

// Initialize static mutex
std::mutex Logger::log_mutex_;

// Global logger instances
namespace logger {
    Logger runtime("RUNTIME", LogLevel::TRACE_LEVEL);
    Logger channel("CHANNEL", LogLevel::TRACE_LEVEL);
    Logger task("TASK", LogLevel::TRACE_LEVEL);
    Logger test("TEST", LogLevel::TRACE_LEVEL);
    Logger example("EXAMPLE", LogLevel::TRACE_LEVEL);
    
    /**
     * Initialize global loggers
     */
    void init() {
        // Default initialization - can be extended to read from config file
        runtime.set_level(LogLevel::INFO_LEVEL);
        channel.set_level(LogLevel::INFO_LEVEL);
        task.set_level(LogLevel::INFO_LEVEL);
        test.set_level(LogLevel::TRACE_LEVEL);
        example.set_level(LogLevel::TRACE_LEVEL);
    }
    
    /**
     * Set global log level
     */
    void set_level(LogLevel level) {
        runtime.set_level(level);
        channel.set_level(level);
        task.set_level(level);
        test.set_level(level);
        example.set_level(level);
    }
}

} // namespace tang