#include <iostream>
#include <vector>
#include <numeric>

#include "tracer.hpp"
#include "library.hpp"

// Task functions
namespace
{
std::mutex printMutex;

void computeSum(int start, int end)
{
    std::vector<int> numbers(end - start + 1);
    std::iota(numbers.begin(), numbers.end(), start);                // Fill with values from 'start' to 'end'
    long sum = std::accumulate(numbers.begin(), numbers.end(), 0L);  // Accumulate as long

    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << "Thread " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << " calculated sum from "
              << start << " to " << end << " as: " << sum << std::endl;
}

void printMessage(int threadId, const std::string& message)
{
    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << "Thread " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << " says: " << message
              << std::endl;
}
}

// Main function demonstrating thread pool usage
void func()
{
    const int numThreads = 3;  // Size of thread pool
    ThreadPool pool(numThreads);

    const int numTasks = 5;  // Number of tasks to create

    // Enqueue computeSum tasks
    for (int i = 0; i < numTasks; ++i)
    {
        pool.enqueueTask([i] { computeSum(i * 10, (i + 1) * 10 - 1); });
    }

    // Enqueue printMessage tasks
    for (int i = 0; i < numTasks; ++i)
    {
        pool.enqueueTask([i] { printMessage(i, "Hello from thread!"); });
    }
}

int main()
{
    TRACE_START("/tmp/trace.json");
    func();
    TRACE_STOP();
    return 0;
}
