#pragma once

#include <string>

class Level {
  public:
    enum Value { DEBUG = 1, INFO = 2, WARNING = 3, DEFAULT = 4, ERROR = 5 };
};

class Logger {
  public:
    static Logger &getInstance();

    void         setLevel(Level::Value threshold);
    Level::Value getLevel() const;

    void log(Level::Value level, const std::string &message);

    void debug(const std::string &message);
    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);

    Logger();
    Logger(const Logger &other);
    Logger &operator=(const Logger &other);
    ~Logger();

  private:
    std::string levelToString(Level::Value level) const;
    std::string timestamp() const;

    Level::Value _threshold;
};

void log(Level::Value level, const std::string &msg);
