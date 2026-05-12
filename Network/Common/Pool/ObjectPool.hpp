#pragma once

#include <array>
#include <cstddef>
#include <functional>

template <typename T>
concept hasInit = requires(T obj) { obj.Init(); };

template <typename T>
concept hasClear = requires(T obj) { obj.Clear(); };

template <typename T, size_t N, bool isLazy = false>
class ObjectPool {
	using deleteFunc = std::function<void(T*)>;

   public:
	ObjectPool(deleteFunc deleter = nullptr) : deleter_(deleter) {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				new (&pool_[i * sizeof(T)]) T();
			}
		}
	}

	~ObjectPool() {
		if constexpr (!isLazy) {
			for (size_t i = 0; i < N; ++i) {
				auto* obj = reinterpret_cast<T*>(&pool_[i * sizeof(T)]);
				obj->~T();
			}
		}
	}

	template <typename... Args>
	T* Acquire(size_t idx, Args&&... args) {
		T* obj = reinterpret_cast<T*>(&pool_[idx * sizeof(T)]);

		if constexpr (isLazy) {
			new (obj) T(std::forward<Args>(args)...);
		} else if constexpr (hasInit<T>) {
			obj->Init(std::forward<Args>(args)...);
		}

		return obj;
	};

	bool Release(size_t idx) {
		T* obj = reinterpret_cast<T*>(&pool_[idx * sizeof(T)]);

		if (deleter_) {
			deleter_(obj);
		} else {
			if constexpr (isLazy) {
				obj->~T();
			} else if constexpr (hasClear<T>) {
				obj->Clear();
			}
		}

		return true;
	}

	T* Get(size_t idx) { return reinterpret_cast<T*>(&pool_[idx * sizeof(T)]); }

	deleteFunc deleter_;

   private:
	alignas(T) std::array<std::byte, sizeof(T) * N> pool_;
};
