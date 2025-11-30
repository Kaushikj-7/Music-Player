#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <iostream>

enum class LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR
    CRITICAL
};

class Logger{
    public: 
     static Logger& instance();
     void setLogFile(const std::string& filename);
     void log(LogLevel level, const std::string& message);
     
    private:
     Logger() =default;
     Logger(const Logger&) = delete;
     Logger& operator = (const Logger&) = delete;
     std::string levelToString(LogLevel level);
     std::string currentTimestamp();

    private:
      std::ofstream logFile;  // File stream for log output.
      std::mutex logMutex;
