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
    explicit FileWriter(const std::string& filename)
    {
        file.open(filename, std::ios::out | std::ios::trunc);
        if (!file)
        {
            std::ostringstream errMsg;
            errMsg << "Error opening trace file: " << filename << " (" << std::strerror(errno) << ")";
            throw std::runtime_error(errMsg.str());
        }
    }

    ~FileWriter()
    {
        if (file.is_open())
        {
            file << "\n]\n}\n";
            file.close();
        }
    }

    std::ofstream& get()
    {
        return file;
    }

private:
    std::ofstream file;
};

class TraceEvent
{
public:
    TraceEvent(const std::string& category, const std::string& name, char phase) :
        category_(category), name_(name), phase_(phase),
        thread_id_(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFF), pid_(getpid()), ts_(0)
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void ts(std::chrono::steady_clock::time_point reference)
    {
        ts_ = std::chrono::duration_cast<std::chrono::microseconds>(start_time_ - reference).count();
    }

    void end()
    {
        end_time_ = std::chrono::steady_clock::now();
    }

    double duration() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time_ - start_time_).count();
    }

    std::string toJson(bool first) const
    {
        std::ostringstream oss;
        if (!first)
        {
            oss << ",\n";
        }

        oss << "{\"cat\":\"" << category_ << "\","
            << " \"name\":\"" << name_ << "\","
            << " \"pid\":" << pid_ << ","
            << " \"tid\":" << thread_id_ << ","
            << " \"ts\":" << ts_ << ","
            << " \"ph\":\"" << phase_ << "\"";

        if (phase_ == 'X')
        {
            oss << ", \"dur\":" << duration();
        }
        oss << "}";

        return oss.str();
    }

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
    Tracer() : isTracing(false), event_count_(0) {}

    ~Tracer() {}

    void start(const std::string& filename)
    {
        start_time_ = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        fileWriter_ = std::make_unique<FileWriter>(filename);
        outputFile_ = &fileWriter_->get();
        *outputFile_ << "{\n\"traceEvents\":\n[\n";
        isTracing = true;
        firstLine = true;
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isTracing = false;
        flush_();
    }

    void flush()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        flush_();
        assert(event_count_ == 0);
    }

    void logEvent(const std::string& category, const std::string& name, char phase)
    {
        if (!isTracing)
        {
            return;
        }
        TraceEvent event{category, name, phase};
        logEvent(std::move(event));
    }

    void logEvent(TraceEvent event)
    {
        if (!isTracing)
        {
            return;
        }

        event.ts(start_time_);
        std::lock_guard<std::mutex> lock(mutex_);
        handle_event(std::move(event));
    }

private:
    void flush_()
    {
        for (const auto& event : events_)
        {
            *outputFile_ << event.toJson(firstLine);
            firstLine = false;
            --event_count_;
        }
        events_.clear();
    }

    void handle_event(TraceEvent&& event)
    {
        events_.emplace_back(event);
        ++event_count_;
    }

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
    static TraceLogger& instance()
    {
        static TraceLogger instance;
        return instance;
    }

    void start(const std::string& filename)
    {
        tracer_.start(filename);
    }

    void stop()
    {
        tracer_.stop();
    }

    void flush()
    {
        tracer_.flush();
    }

    void log_event(const std::string& category, const std::string& name, char phase)
    {
        tracer_.logEvent(category, name, phase);
    }

    void log_event(TraceEvent event)
    {
        tracer_.logEvent(std::move(event));
    }

private:
    TraceLogger() = default;
    Tracer tracer_;
};

class TraceScoped
{
public:
    TraceScoped(const std::string& category, const std::string& name) : event_(category, name, 'X') {}

    ~TraceScoped()
    {
        event_.end();
        TraceLogger::instance().log_event(std::move(event_));
    }

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
