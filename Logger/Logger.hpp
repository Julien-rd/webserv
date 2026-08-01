#pragma once 

#include <string>

enum class Level{
    DEFAULT,
    DEBUG  ,
    INFO    ,
    WARNING ,
    ERROR   
};

class Logger {
public:
    
    static Logger &getInstance();

    void setLevel(Level threshold);
    Level getLevel() const;

    void log(Level level, const std::string &message);

    void debug(const std::string &message);
    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);
    
    Logger();
    Logger(const Logger &other);
    Logger &operator=(const Logger &other);
    ~Logger();
    private:
    
    std::string levelToString(Level level) const;
    std::string timestamp() const;
    
    Level _threshold;
};

void log(Level level, const std::string& msg);