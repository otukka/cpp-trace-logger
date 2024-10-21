#include "library.hpp"

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
