// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <iostream>
#include <fstream>

#include "logger.h"
#include <iomanip>
#include <chrono>
#include <random>

#ifdef _WIN32
#include <winsock2.h>
#endif

double Logger::secondsSince(const TimePoint &t) {
    auto now = timeNow();
    std::chrono::duration<double> d = now - t;
    return d.count();
}

double Logger::secondsTo(const TimePoint &t) {
    auto now = timeNow();
    std::chrono::duration<double> d = t - now;
    return d.count();
}

int64_t Logger::msSince(const TimePoint &t) {
    auto now = timeNow();
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (now - t).count();
}

int64_t Logger::msTo(const TimePoint &t) {
    auto now = timeNow();
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (t - now).count();
}

std::string Logger::dateString(time_t t) {
    // put_time might not be available in older libs.
    char dstr[200];
    if (!t)
        t = time(nullptr);
    struct tm *dtm = localtime(&t);
    size_t len = strftime(dstr, sizeof(dstr), "%FT%T%z", dtm);
    return std::string(dstr, len);
}

std::string Logger::dateString2(time_t t) {
    // put_time might not be available in older libs.
    char dstr[200];
    if (!t)
        t = time(nullptr);
    struct tm *dtm = localtime(&t);
    size_t len = strftime(dstr, sizeof(dstr), "%a, %d %b %Y %H:%M:%S", dtm);
    return std::string(dstr, len);
}

std::string Logger::createHashKey(unsigned int length) {
    std::random_device rng;
    std::ostringstream s;
    std::uniform_int_distribution<unsigned short> dist;
    while (s.str().size() < length)
        s << std::setw(sizeof(unsigned short)*2) << std::uppercase
          << std::hex << std::setfill('0') << dist(rng);
    return s.str();
}

#ifdef USE_THREADS
thread_local
#endif
bool Logger::in_error = false;
#ifdef USE_THREADS
thread_local
#endif
TimePoint Logger::global_start_time(timeNow());
#ifdef USE_THREADS
thread_local
#endif
std::ostream *Logger::_logFile = &std::cerr;
#ifdef USE_THREADS
thread_local
#endif
unsigned int Logger::log_count = 100000;
#ifdef USE_THREADS
thread_local
#endif
unsigned int Logger::warn_count = 10000;
#ifdef USE_THREADS
thread_local
#endif
unsigned int Logger::err_count = 10000;
#ifdef USE_THREADS
thread_local
#endif
unsigned int Logger::log_count_saved = 100000;
#ifdef USE_THREADS
thread_local
#endif
unsigned int Logger::warn_count_saved = 10000;
#ifdef USE_THREADS
thread_local
#endif
unsigned int Logger::err_count_saved = 10000;

DummyStream Logger::_dummyLog;

#ifdef USE_THREADS
thread_local
#endif
std::ostringstream Logger::_blackHole;

DummyStream::~DummyStream() {
}

void Logger::setLogLimit(unsigned int loglines, unsigned int warnlines,
                         unsigned int errlines) {
    if (loglines)
        log_count_saved = loglines;
    if (warnlines)
        warn_count_saved = warnlines;
    if (errlines)
        err_count_saved = errlines;

    log_count = log_count_saved;
    warn_count = warn_count_saved;
    err_count = err_count_saved;
}

void Logger::sayTime(std::ostream &stream) {
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    stream << tm->tm_year+1900 << '-'
           << std::setfill('0') << std::setw(2) << tm->tm_mon+1 << '-'
           << std::setfill('0') << std::setw(2) << tm->tm_mday << ' '
           << std::setfill('0') << std::setw(2) << tm->tm_hour << ':'
           << std::setfill('0') << std::setw(2) << tm->tm_min << ':'
           << std::setfill('0') << std::setw(2) << tm->tm_sec;
}

#ifdef TASKRUNNER_LOGERR
std::ostream &Logger::errno_log() const {
    if (err_count) {
        in_error = true;
        --err_count;
        *_logFile << "\n" << global_elapsed_ms() << ' ' << _label << "*** "
                  << (err_count ? "ERROR ***: " :  "LAST ERR ***: ")
#ifdef _WIN32
                  << std::to_string(WSAGetLastError())
#else
                  << strerror(errno)
#endif
                  << ": ";
        return *_logFile;
    } else {
        return _blackHole;
    }
}
#else
DummyStream &Logger::errno_log() const {
    return _dummyLog;
}
#endif

void Logger::setLogFile(std::ostream &stream) {
    _logFile = &stream;
    in_error = false;
    global_start_time = timeNow();
    sayTime(stream);
}

void Logger::reopenLogFile(const std::string &filename) {
    if (std::ofstream *the_log = dynamic_cast<std::ofstream *>(_logFile)) {
        *the_log << "\n";
        the_log->close();
        the_log->open(filename, std::ios::app);
    }
}
