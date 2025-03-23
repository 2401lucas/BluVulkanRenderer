#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace blu::core::threads {
class Thread {
 public:
  Thread();

  ~Thread();

  // Add a new job to the thread's queue
  void AddJob(std::function<void()> function);

  // Wait until all work items have been finished
  void Wait();

 private:
  // Loop through all remaining jobs
  void QueueLoop();

  bool destroying = false;
  std::thread worker;
  std::queue<std::function<void()>> job_queue;
  std::mutex queue_mutex;
  std::condition_variable condition;
};

class ThreadPool {
 public:
  std::vector<std::unique_ptr<Thread>> threads;

  // Sets the number of threads to be allocated in this pool
  void SetThreadCount(uint32_t count);

  // Wait until all threads have finished their work items
  void Wait();
};
}  // namespace blu::core::threads

#endif