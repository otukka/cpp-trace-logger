#include "tracer.hpp"

FileWriter::FileWriter(const std::string& filename)
{
    file.open(filename, std::ios::out | std::ios::trunc);
    if (!file)
    {
        std::ostringstream errMsg;
        errMsg << "Error opening trace file: " << filename << " (" << std::strerror(errno) << ")";
        throw std::runtime_error(errMsg.str());
    }
}

FileWriter::~FileWriter()
{
    if (file.is_open())
    {
        file << "\n]\n}\n";
        file.close();
    }
}

std::ofstream& FileWriter::get()
{
    return file;
}

TraceEvent::TraceEvent(const std::string& category, const std::string& name, char phase) :
    category_(category), name_(name), phase_(phase),
    thread_id_(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFF), pid_(getpid()), ts_(0)
{
    start_time_ = std::chrono::steady_clock::now();
}

void TraceEvent::ts(std::chrono::steady_clock::time_point reference)
{
    ts_ = std::chrono::duration_cast<std::chrono::microseconds>(start_time_ - reference).count();
}

void TraceEvent::end()
{
    end_time_ = std::chrono::steady_clock::now();
}

double TraceEvent::duration() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(end_time_ - start_time_).count();
}

std::string TraceEvent::toJson(bool first) const
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

Tracer::Tracer() : isTracing(false), event_count_(0) {}

void Tracer::start(const std::string& filename)
{
    start_time_ = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    fileWriter_ = std::make_unique<FileWriter>(filename);
    outputFile_ = &fileWriter_->get();
    *outputFile_ << "{\n\"traceEvents\":\n[\n";
    isTracing = true;
    firstLine = true;
}

void Tracer::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    isTracing = false;
    flush_();
}

void Tracer::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    flush_();
    assert(event_count_ == 0);
}

void Tracer::logEvent(const std::string& category, const std::string& name, char phase)
{
    if (!isTracing)
    {
        return;
    }
    TraceEvent event{category, name, phase};
    logEvent(std::move(event));
}

void Tracer::logEvent(TraceEvent event)
{
    if (!isTracing)
    {
        return;
    }

    event.ts(start_time_);
    std::lock_guard<std::mutex> lock(mutex_);
    handle_event(std::move(event));
}

void Tracer::flush_()
{
    for (const auto& event : events_)
    {
        *outputFile_ << event.toJson(firstLine);
        firstLine = false;
        --event_count_;
    }
    events_.clear();
}

void Tracer::handle_event(TraceEvent&& event)
{
    events_.emplace_back(event);
    ++event_count_;
}

TraceLogger& TraceLogger::instance()
{
    static TraceLogger instance;
    return instance;
}

void TraceLogger::start(const std::string& filename)
{
    tracer_.start(filename);
}

void TraceLogger::stop()
{
    tracer_.stop();
}

void TraceLogger::flush()
{
    tracer_.flush();
}

void TraceLogger::log_event(const std::string& category, const std::string& name, char phase)
{
    tracer_.logEvent(category, name, phase);
}

void TraceLogger::log_event(TraceEvent event)
{
    tracer_.logEvent(std::move(event));
}

TraceScoped::TraceScoped(const std::string& category, const std::string& name) : event_(category, name, 'X') {}

TraceScoped::~TraceScoped()
{
    event_.end();
    TraceLogger::instance().log_event(std::move(event_));
}
