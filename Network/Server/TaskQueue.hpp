#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

class TaskQueue {
   public:
	void Shutdown() {
		isShuttingDown_.store(true, std::memory_order_release);
		cv_.notify_all();
	}

	void Push(std::function<void()> task) {
		std::lock_guard<std::mutex> lock(mutex_);
		tasks_.push(std::move(task));
		cv_.notify_one();
	}

	std::function<void()> Pop() {
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [this] {
			return !tasks_.empty() ||
				   isShuttingDown_.load(std::memory_order_acquire);
		});
		if (isShuttingDown_.load(std::memory_order_acquire)) {
			return nullptr;
		}
		auto task = std::move(tasks_.front());
		tasks_.pop();
		return task;
	}

   private:
	std::queue<std::function<void()>> tasks_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::atomic<bool> isShuttingDown_{false};
};
