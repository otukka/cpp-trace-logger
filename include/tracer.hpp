#ifndef TRACER_HPP_INCLUDED
#define TRACER_HPP_INCLUDED

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <cstring>
#include <memory>
#include <cassert>
#include <unistd.h>

class FileWriter
{
public:
    explicit FileWriter(const std::string& filename);

    ~FileWriter();

    std::ofstream& get();

private:
    std::ofstream file;
};

class TraceEvent
{
public:
    TraceEvent(const std::string& category, const std::string& name, char phase);

    void ts(std::chrono::steady_clock::time_point reference);

    void end();

    double duration() const;

    std::string toJson(bool first) const;

private:
    const std::string category_;
    const std::string name_;
    const char phase_;
    const size_t thread_id_;
    const pid_t pid_;
    uint64_t ts_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;
};

class Tracer
{
public:
    Tracer();

    void start(const std::string& filename);

    void stop();

    void flush();

    void logEvent(const std::string& category, const std::string& name, char phase);

    void logEvent(TraceEvent event);

private:
    void flush_();

    void handle_event(TraceEvent&& event);

private:
    std::unique_ptr<FileWriter> fileWriter_;
    std::ofstream* outputFile_;
    std::mutex mutex_;
    std::atomic<bool> isTracing;
    std::vector<TraceEvent> events_;
    std::atomic<bool> firstLine;
    std::atomic<size_t> event_count_;
    std::chrono::steady_clock::time_point start_time_;
};

class TraceLogger
{
public:
    static TraceLogger& instance();

    void start(const std::string& filename);

    void stop();

    void flush();

    void log_event(const std::string& category, const std::string& name, char phase);

    void log_event(TraceEvent event);

private:
    TraceLogger() = default;
    Tracer tracer_;
};

class TraceScoped
{
public:
    TraceScoped(const std::string& category, const std::string& name);

    ~TraceScoped();

private:
    TraceEvent event_;
};

#define TRACE_START(filename) TraceLogger::instance().start(filename)
#define TRACE_STOP()          TraceLogger::instance().stop()
#define TRACE_FLUSH()         TraceLogger::instance().flush()

#define TRACE_BEGIN(category, name) TraceLogger::instance().log_event(category, name, 'B')
#define TRACE_END(category, name)   TraceLogger::instance().log_event(category, name, 'E')

// Macro for easy function tracing.
#define TRACE_SCOPE_AUTO() TraceScoped UNIQUE_TRACE_SCOPE_OBJECT_NAME(__FILE__, __FUNCTION__)

#endif  // TRACER_HPP_INCLUDED
