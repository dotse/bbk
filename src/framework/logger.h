// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// This is a simple logger. All classes that want to write to the global log
// file should inherit from this class.
//
// By default, logs will be written to cerr. To log elsewhere, you must
// call the static function setLogFile with a stream object (e.g. an ofstream
// or an ostringstream) which the logs will be written to. The stream will be
// used globally. You must make sure the setLogFile stream never is destroyed,
// at least not until setLogFile is called with another stream.
//
// This class has also a TimePoint typedef and some helper functions to measure
// relative time, based on std::chrono::steady_clock.
// The useful (static) time functions are timeNow(), timeAfter(double s),
// secondsSince(const TimePoint &t), secondsTo(const TimePoint &t).

#pragma once

#ifdef _WIN32
#define NOMINMAX
#else
#include <string.h>
#endif

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

class DummyStream {
public:
    template<class T>
    DummyStream &operator<<(T ) { return *this; }
    DummyStream& operator<<(std::ostream &(*)(std::ostream &) ) {
        return *this;
    }
    virtual ~DummyStream();
private:
};

typedef std::chrono::steady_clock::time_point TimePoint;

class Logger {
public:
    Logger(std::string label) :
        _label(label) {
        // TODO: single initialisation
        _blackHole.clear(std::istream::eofbit);
    }
    static void setLogFile(std::ostream &stream);

    // If current log is a file (ofstream), reopen it with new filename:
    static void reopenLogFile(const std::string &filename);

    // Max number of lines of log/warn/err:
    static void setLogLimit(unsigned int loglines = 100000,
                            unsigned int warnlines = 10000,
                            unsigned int errlines = 10000);

    static void sayTime(std::ostream &stream);
    static bool inError() {
        return in_error;
    }
    static std::ostream &err_log(const std::string &label) {
        if (err_count) {
            in_error = true;
            --err_count;
            *_logFile << "\n" << elapsed() << ' ' << label << " *** "
                      << (err_count ? "ERROR ***: " :  "LAST ERR ***: ");
            return *_logFile;
        } else {
            return _blackHole;
        }
    }
    static std::ostream &warn_log(const std::string &label) {
        if (warn_count) {
            --warn_count;
            *_logFile << "\n" << elapsed() << ' ' << label << " *** "
                      << (warn_count ? "WARNING ***: " : "LAST WARN ***: ");
            return *_logFile;
        } else {
            return _blackHole;
        }
    }
    static std::ostream &log(const std::string &label) {
        if (log_count) {
            --log_count;
            *_logFile << "\n" << elapsed() << ' ' << label << ": ";
            if (!log_count)
                *_logFile << "LAST LOG: ";
            return *_logFile;
        } else {
            return _blackHole;
        }
    }

    // Calling this often will be bad for performance:
    static void flushLogFile() {
        *_logFile << std::endl;
    }

    static void pauseLogging() {
        _logFile = &_blackHole;
    }
    static double secondsSince(const TimePoint &t);
    static double secondsTo(const TimePoint &t);
    static long msSince(const TimePoint &t);
    static long msTo(const TimePoint &t);
    static bool hasExpired(const TimePoint &t) {
        return secondsSince(t) >= 0;
    }
    // What time is it?
    static TimePoint timeNow() {
        return std::chrono::steady_clock::now();
    }
    // What time will it be after s seconds?
    static TimePoint timeAfter(double s) {
        return timeNow() + std::chrono::microseconds(toUs(s));
    }
    static std::chrono::microseconds::rep toUs(double t) {
        return static_cast<std::chrono::microseconds::rep>(1e6*t);
    }
    static std::string dateString(time_t t = 0);
    static std::string dateString2(time_t t = 0);

    // Create string of length random hex chars from system's random number
    // generator. The length should be a multiple of 4.
    static std::string createHashKey(unsigned int length = 20);

    std::string label() const {
        return _label;
    }

protected:

#ifdef TASKRUNNER_LOGERR
    std::ostream &errno_log() const;
    std::ostream &err_log() const {
        return err_log(_label);
    }
#else
    DummyStream &errno_log() const;
    static DummyStream &err_log() {
        return _dummyLog;
    }
#endif

#ifdef TASKRUNNER_LOGWARN
    std::ostream &warn_log() const {
        return warn_log(_label);
    }
#else
    static DummyStream &warn_log() {
        return _dummyLog;
    }
#endif
#ifdef TASKRUNNER_LOGINFO
    std::ostream &log() const {
        return log(_label);
    }
#else
    static DummyStream &log() {
        return _dummyLog;
    }
#endif
#ifdef TASKRUNNER_LOGDBG
    std::ostream &dbg_log() const {
        *_logFile << "\n" << elapsed() << ' ' << _label << ": ";
        return *_logFile;
    }
#else
    static DummyStream &dbg_log() {
        return _dummyLog;
    }
#endif
private:
    static long elapsed() {
        return msSince(start_time);
    }
    std::string _label;
    static thread_local bool in_error;
    static thread_local TimePoint start_time;
    static thread_local std::ostream *_logFile;
    static thread_local std::ostringstream _blackHole;
    static thread_local unsigned int log_count, warn_count, err_count;
    static DummyStream _dummyLog;
};
