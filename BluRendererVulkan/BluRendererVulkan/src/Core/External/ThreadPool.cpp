#include "ThreadPool.h"

namespace blu::core::threads {
Thread::Thread() { worker = std::thread(&Thread::QueueLoop, this); }

Thread::~Thread() {
  if (worker.joinable()) {
    Wait();
    queue_mutex.lock();
    destroying = true;
    condition.notify_one();
    queue_mutex.unlock();
    worker.join();
  }
}

void Thread::AddJob(std::function<void()> function) {
  std::lock_guard<std::mutex> lock(queue_mutex);
  job_queue.push(std::move(function));
  condition.notify_one();
}

void Thread::Wait() {
  std::unique_lock<std::mutex> lock(queue_mutex);
  condition.wait(lock, [this]() { return job_queue.empty(); });
}

void Thread::QueueLoop() {
  while (true) {
    std::function<void()> job;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      condition.wait(lock, [this] { return !job_queue.empty() || destroying; });
      if (destroying) {
        break;
      }
      job = job_queue.front();
    }

    job();

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      job_queue.pop();
      condition.notify_one();
    }
  }
}
void ThreadPool::SetThreadCount(uint32_t count) {
  threads.clear();
  for (uint32_t i = 0; i < count; i++) {
    threads.push_back(std::make_unique<Thread>());
  }
}
void ThreadPool::Wait() {
  for (auto& thread : threads) {
    thread->Wait();
  }
}
}  // namespace blu::core::threads