#include "logger.h"

// LOGGER INSTANCE CREATION
// Meyer's Singleton: created on first use, thread-safe by language guarantees.
Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

// SET LOG FILE DESTINATION
void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);

    // Close existing log (if any)
    if (logFile.is_open()) {
        logFile.close();
    }

    // Open new log file
    logFile.open(filename, std::ios::out | std::ios::app);

    if (!logFile) {
        // If file cannot be opened, fallback to console logging
        std::cerr << "Failed to open log file: " << filename << "\n";
    }
}

// MAIN LOGGING FUNCTION
void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::string formattedMsg =
        currentTimestamp() + " [" + levelToString(level) + "] " + message + "\n";

    // File output if available
    if (logFile.is_open()) {
        logFile << formattedMsg;
        logFile.flush();
    }

    // Console fallback (ensures visibility during development/testing)
    else {
        std::cout << formattedMsg;
    }
}

// LOG LEVEL  STRING
std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

// TIMESTAMP GENERATOR
std::string Logger::currentTimestamp() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto time = system_clock::to_time_t(now);

    // Convert to human-readable string
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&time));

    return std::string(buffer);
}
