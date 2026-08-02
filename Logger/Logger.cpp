#include "Logger.hpp"

#include <ctime>
#include <iostream>

Logger::Logger() : _threshold(Level::ERROR) {}
Logger::~Logger() {}
Logger::Logger(const Logger &) {}
Logger &Logger::operator=(const Logger &) { return *this; }

Logger &Logger::getInstance() {
    static Logger instance;
    return instance;
}

void         Logger::setLevel(Level::Value threshold) { _threshold = threshold; }
Level::Value Logger::getLevel() const { return _threshold; }

void Logger::log(Level::Value level, const std::string &message) {
    if (level < _threshold)
        return;
    std::string ts = timestamp();
    if (!ts.empty())
        std::cerr << ts << " ";
    std::cerr << "[" << levelToString(level) << "] " << message << "\n";
}

// void Logger::debug(const std::string &message) { log(Level::DEBUG, message); }
// void Logger::info(const std::string &message) { log(Level::INFO, message); }
// void Logger::warning(const std::string &message) { log(Level::WARNING, message); }
// void Logger::error(const std::string &message) { log(Level::ERROR, message); }

std::string Logger::levelToString(Level::Value level) const {
    switch (level) {
    case Level::DEBUG:
        return "DEBUG";
    case Level::INFO:
        return "INFO";
    case Level::WARNING:
        return "WARNING";
    case Level::DEFAULT:
        return "ERROR";
    case Level::ERROR:
        return "ERROR";
    }
    return " ";
}

std::string Logger::timestamp() const {
    char   buf[64];
    time_t now = time(0);
    if (now == (time_t) -1)
        return "";
    struct tm *timeinfo = gmtime(&now);
    if (timeinfo == NULL)
        return "";
    if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo) == 0)
        return "";
    return std::string(buf);
}

void log(Level::Value level, const std::string &msg) { Logger::getInstance().log(level, msg); }
