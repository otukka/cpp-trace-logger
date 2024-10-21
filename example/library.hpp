#ifndef LIBRARY_HPP_INCLUDED
#define LIBRARY_HPP_INCLUDED

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool
{
public:
    // Constructor: Initializes a thread pool with the given size
    explicit ThreadPool(size_t poolSize);

    // Destructor: Cleans up the thread pool
    ~ThreadPool();

    // Add a new task to the queue
    void enqueueTask(std::function<void()> task);

private:
    std::vector<std::thread> workers;         // Vector of worker threads
    std::queue<std::function<void()>> tasks;  // Task queue

    std::mutex mutex;                   // Mutex for synchronizing access to the queue
    std::condition_variable condition;  // Condition variable for thread synchronization
    bool stop;                          // Flag to indicate whether the pool is stopping

    // Function executed by each worker thread
    void workerThread();
};

#endif  // LIBRARY_HPP_INCLUDED
