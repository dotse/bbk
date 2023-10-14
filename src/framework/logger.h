// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef max
#undef max
#endif
#else
#include <string.h>
#endif

#ifndef DEBUG
#define DEBUG 0
#endif
#ifndef TARGET_OS_IPHONE
#define TARGET_OS_IPHONE 0
#endif

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

#ifdef __ANDROID_API__
#include <android/log.h>
#endif

/// This class is used to optionally disable logging operations at compile time.
class DummyStream {
public:
    /// Do nothing, emulating the << stream operator.
    template<class T>
    DummyStream &operator<<(T ) { return *this; }
#ifdef __ANDROID_API__
    DummyStream &operator<<(const char *s) {
        __android_log_print(ANDROID_LOG_VERBOSE, "BBK", "%s", s);
        return *this; }
    DummyStream &operator<<(std::string s) {
        __android_log_print(ANDROID_LOG_VERBOSE, "BBK", "%s", s.c_str());
        return *this; }
    DummyStream &operator<<(int i) {
        __android_log_print(ANDROID_LOG_VERBOSE, "BBK", "%d", i);
        return *this; }
    DummyStream &operator<<(double x) {
        __android_log_print(ANDROID_LOG_VERBOSE, "BBK", "%f", x);
        return *this; }
#endif
    /// Do nothing, emulating the << stream operator.
    DummyStream& operator<<(std::ostream &(*)(std::ostream &) ) {
        return *this;
    }
    virtual ~DummyStream();
private:
};

/// \class TimePoint
/// The TimePoint class is used to measure elapsed time during execution,
/// for example by timer events.
///
/// It is simply a typedef to std::chrono::steady_clock::time_point.
///
/// Example:
///
///     TimePoint start = Logger::timeNow();
///     // Do stuff...
///     std::cout << Logger::secondsSince(start) << " seconds have elapsed.";
///     // 2.00042 seconds have elapsed.
typedef std::chrono::steady_clock::time_point TimePoint;

/// This is a simple logger. All classes that want to write to the global log
/// file should inherit from this class.
///
/// By default, logs will be written to cerr. To log elsewhere, you must
/// call the static function Logger::setLogFile with a stream object (e.g. an
/// std::ofstream or an std::ostringstream) which the logs will be written to.
/// The stream will be used globally. You must make sure the global stream
/// is never destroyed, at least not before Logger::setLogFile has been called
/// with another stream.
class Logger {
public:
    /// Each object of the Logger class (or its subclasses) have a log label,
    /// which will often be thought of as the name of the object.
    Logger(std::string label) :
        _label(label) {
        // TODO: single initialisation
        _blackHole.clear(std::istream::eofbit);
    }

    /// \brief Set global log destination.
    ///
    /// The given stream will be the destination of all subsequent log calls
    /// globally. You must make sure the global stream never is destroyed, at
    /// least not until Logger::setLogFile is called with another stream.
    static void setLogFile(std::ostream &stream);

    /// If current log is a file (ofstream), reopen it with new filename:
    static void reopenLogFile(const std::string &filename);

    /// \brief Set max number of lines of info/warn/err log.
    ///
    /// If 0, reset to previous (non-zero) max number of lines.
    /// After the limit has been reached, no more lines for that log level
    /// will be printed until the limit has been reset.
    static void setLogLimit(unsigned int loglines = 0,
                            unsigned int warnlines = 0,
                            unsigned int errlines = 0);

    /// Write current local time to the given stream
    static void sayTime(std::ostream &stream);

    /// Return true if any error has been logged (globally since start)
    static bool inError() {
        return in_error;
    }

    /// \brief Write a line of error log.
    ///
    /// Access the current global error log stream. A line feed and a preamble
    /// will be written to the stream. Then send whetever you want to the log
    /// stream using the standard std::ostream API.
    ///
    /// In non-static members of subclasses to Logger, the method
    /// Logger::err_log() should be used instead of this function.
    static std::ostream &err_log(const std::string &label) {
        if (err_count) {
            in_error = true;
            --err_count;
            *_logFile << "\n" << global_elapsed_ms() << ' ' << label << " *** "
                      << (err_count ? "ERROR ***: " :  "LAST ERR ***: ");
            return *_logFile;
        } else {
            return _blackHole;
        }
    }

    /// \brief Write a line of warning log.
    ///
    /// Access the current global warning log stream. A line feed and a preamble
    /// will be written to the stream. Then send whetever you want to the log
    /// stream using the standard std::ostream API.
    ///
    /// In non-static members of subclasses to Logger, the method
    /// Logger::warn_log() should be used instead of this function.
    static std::ostream &warn_log(const std::string &label) {
        if (warn_count) {
            --warn_count;
            *_logFile << "\n" << global_elapsed_ms() << ' ' << label << " *** "
                      << (warn_count ? "WARNING ***: " : "LAST WARN ***: ");
            return *_logFile;
        } else {
            return _blackHole;
        }
    }

    /// \brief Write a line of info log.
    ///
    /// Access the current global info log stream. A line feed and a preamble
    /// will be written to the stream. Then send whetever you want to the log
    /// stream using the standard std::ostream API.
    ///
    /// In non-static members of subclasses to Logger, the method
    /// Logger::warn_log() should be used instead of this function.
    static std::ostream &log(const std::string &label) {
        if (log_count) {
            --log_count;
            *_logFile << "\n" << global_elapsed_ms() << ' ' << label << ": ";
            if (!log_count)
                *_logFile << "LAST LOG: ";
            return *_logFile;
        } else {
            return _blackHole;
        }
    }

    /// Anything written to the global log may be buffered for quite some time,
    /// and thus not visible in the destination file. This method will flush
    /// the buffer and write an extra empty line.
    ///
    /// Calling this often may be bad for performance.
    static void flushLogFile() {
        *_logFile << std::endl;
    }

    /// Disable all log output until next call to Logger::setLogFile.
    static void pauseLogging() {
        _logFile = &_blackHole;
    }

    /// Return number of seconds since the given TimePoint.
    /// The returned value might be negative.
    static double secondsSince(const TimePoint &t);

    /// Return number of seconds until the given TimePoint.
    /// The returned value might be negative.
    static double secondsTo(const TimePoint &t);

    /// Return number of milliseconds since the given TimePoint.
    /// The returned value might be negative.
    static int64_t msSince(const TimePoint &t);

    /// Return number of milliseconds until the given TimePoint.
    /// The returned value might be negative.
    static int64_t msTo(const TimePoint &t);

    /// Return true if current time is after the given TimePoint.
    static bool hasExpired(const TimePoint &t) {
        return secondsSince(t) >= 0;
    }

    /// Return current time.
    static TimePoint timeNow() {
        return std::chrono::steady_clock::now();
    }

    /// Return current time plus s seconds.
    static TimePoint timeAfter(double s) {
        return timeNow() + std::chrono::microseconds(toUs(s));
    }

    /// Return a very distant time.
    static TimePoint timeMax() {
        return TimePoint::max();
    }

    /// Convert s (seconds) to std::chrono::microseconds
    static std::chrono::microseconds toUs(double t) {
        auto us = static_cast<std::chrono::microseconds::rep>(1e6*t);
	return std::chrono::microseconds(us);
    }

    /// Return local time, formatted as 2023-10-14T09:38:47+0200
    static std::string dateString(time_t t = 0);

    /// Return local time, formatted as Sat, 14 Oct 2023 09:38:47
    static std::string dateString2(time_t t = 0);

    /// \brief Return a random string.
    ///
    /// Create string of length random hex chars from system's random number
    /// generator. The length should be a multiple of 4.
    static std::string createHashKey(unsigned int length = 20);

    /// Return the object's log label.
    std::string label() const {
        return _label;
    }

    /// Modify the object's log label
    void resetLabel(const std::string &new_label) {
        _label = new_label;
    }

protected:

#if DEBUG
#define TASKRUNNER_LOGERR
#define TASKRUNNER_LOGWARN
#define TASKRUNNER_LOGINFO
#define TASKRUNNER_LOGBDG
#endif

#ifdef TASKRUNNER_LOGERR
    /// \brief Write a line of error log after a failed system call
    /// has set the global errno to a non-zero value.
    ///
    /// Access the current global error log stream. A line feed and a preamble,
    /// including the latest OS error, will be written to the stream.
    ///
    /// *Note!* The global error stream will be "disabled" (i.e. set to a dummy stream)
    /// unless compiler macro TASKRUNNER_LOGERR is defined.
    std::ostream &errno_log() const;

    /// \brief Write a line of error log.
    ///
    /// Access the current global error log stream. A line feed and a preamble
    /// will be written to the stream. Then send whetever you want to the log
    /// stream using the standard std::ostream API.
    ///
    /// *Note!* The global error log stream will be "disabled" (i.e. set to a
    /// dummy stream) unless compiler macro TASKRUNNER_LOGERR is defined.
    ///
    /// May be used in any non-static member of any subclass. Example:
    ///
    ///     err_log() << "Child task " << t->label() << " failed.";
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
    /// \brief Write a line of warning log.
    ///
    /// Access the current global warning log stream. A line feed and a
    /// preamble will be written to the stream. Then send whetever you want to
    /// the log stream using the standard std::ostream API.
    ///
    /// *Note!* The global warning log stream will be "disabled" (i.e. set to a
    /// dummy stream) unless compiler macro TASKRUNNER_LOGWARN is defined.
    std::ostream &warn_log() const {
        return warn_log(_label);
    }
#else
    static DummyStream &warn_log() {
        return _dummyLog;
    }
#endif
#ifdef TASKRUNNER_LOGINFO
    /// \brief Write a line of info log.
    ///
    /// Access the current global info log stream. A line feed and a preamble
    /// will be written to the stream. Then send whetever you want to the log
    /// stream using the standard std::ostream API.
    ///
    /// *Note!* The global info log stream will be "disabled" (i.e. set to a
    /// dummy stream) unless compiler macro TASKRUNNER_LOGINFO is defined.
    std::ostream &log() const {
        return log(_label);
    }
#else
    static DummyStream &log() {
        return _dummyLog;
    }
#endif
#ifdef TASKRUNNER_LOGDBG
    /// \brief Write a line of debug log.
    ///
    /// Access the current global debug log stream. A line feed and a preamble
    /// will be written to the stream. Then send whetever you want to the log
    /// stream using the standard std::ostream API.
    ///
    /// *Note!* The global debug log stream will be "disabled" (i.e. set to a
    /// dummy stream) unless compiler macro TASKRUNNER_LOGDBG is defined.
    std::ostream &dbg_log() const {
        *_logFile << "\n" << global_elapsed_ms() << ' ' << _label << ": ";
        return *_logFile;
    }
#else
    static DummyStream &dbg_log() {
        return _dummyLog;
    }
#endif
private:
    static int64_t global_elapsed_ms() {
        return msSince(global_start_time);
    }
    std::string _label;

#ifdef USE_THREADS
    thread_local
#endif
    static bool in_error;
#ifdef USE_THREADS
    thread_local
#endif
    static TimePoint global_start_time;
#ifdef USE_THREADS
    thread_local
#endif
    static std::ostream *_logFile;
#ifdef USE_THREADS
    thread_local
#endif
    static std::ostringstream _blackHole;
#ifdef USE_THREADS
    thread_local
#endif
    static unsigned int log_count, warn_count, err_count,
                        log_count_saved, warn_count_saved, err_count_saved;
    static DummyStream _dummyLog;
};
