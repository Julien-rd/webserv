#include "logger.hpp"

#include <ctime>
#include <iostream>

Logger::Logger() : _threshold(INFO) {}
Logger::~Logger() {}
Logger::Logger(const Logger &) {}
Logger &Logger::operator=(const Logger &) { return *this; }

Logger &Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(Level threshold) { _threshold = threshold; }
Logger::Level Logger::getLevel() const { return _threshold; }

void Logger::log(Level level, const std::string &message) {
    if (level < _threshold || level == DEFAULT)
        return;
    std::string ts = timestamp();
    if (!ts.empty())
        std::cerr << ts << " ";
    std::cerr << "[" << levelToString(level) << "] " << message << "\n";
}

void Logger::debug(const std::string &message) { log(DEBUG, message); }
void Logger::info(const std::string &message) { log(INFO, message); }
void Logger::warning(const std::string &message) { log(WARNING, message); }
void Logger::error(const std::string &message) { log(ERROR, message); }

std::string Logger::levelToString(Level level) const {
    switch (level) {
    case DEFAULT:
        return "DEFAULT";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
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