#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <numeric>
#include <functional>

// Task Queue and Thread Pool Implementation
class ThreadPool
{
public:
    ThreadPool(size_t poolSize);
    ~ThreadPool();

    // Add a new task to the queue
    void enqueueTask(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable condition;
    bool stop;

    void workerThread();
};

// ThreadPool Constructor: Launches 'poolSize' threads
ThreadPool::ThreadPool(size_t poolSize) : stop(false)
{
    for (size_t i = 0; i < poolSize; ++i)
    {
        workers.emplace_back([this] { this->workerThread(); });
    }
}

// ThreadPool Destructor: Waits for all threads to complete
ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop = true;
    }
    condition.notify_all();  // Wake up all threads
    for (auto& worker : workers)
    {
        worker.join();  // Wait for each thread to finish
    }
}

// Enqueue a new task to the pool's task queue
void ThreadPool::enqueueTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();  // Notify one thread that a task is available
}

// Function executed by each worker thread
void ThreadPool::workerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty())
                return;  // Exit if stopped and no tasks are left
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();  // Execute the task
    }
}

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
