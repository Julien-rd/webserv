#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

class Logger {
public:
    enum Level {
        DEFAULT   = 0,
        DEBUG   = 1,
        INFO    = 2,
        WARNING = 3,
        ERROR   = 4
    };
    
    static Logger &getInstance();

    void setLevel(Level threshold);
    Level getLevel() const;

    void log(Level level, const std::string &message);

    void debug(const std::string &message);
    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);
    
    private:
    Logger();
    ~Logger();
    Logger(const Logger &other);
    Logger &operator=(const Logger &other);
    
    std::string levelToString(Level level) const;
    std::string timestamp() const;
    
    Level _threshold;
};

#endif