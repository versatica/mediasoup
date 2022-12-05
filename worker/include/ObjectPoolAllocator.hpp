#ifndef MS_OBJECT_POOL_ALLOCATOR_HPP
#define MS_OBJECT_POOL_ALLOCATOR_HPP

// #define MS_ALLOCATOR_FREE_ON_RETURN 1

#include "common.hpp"
#include <vector>

namespace Utils
{
	// Simple implementation of object pool only for single objects.
	// Arrays are allocated as usual.
	template<typename T>
	class ObjectPoolAllocator
	{
		std::shared_ptr<std::vector<T*>> pool_data;

	public:
		typedef T value_type;
		thread_local static Utils::ObjectPoolAllocator<T> Pool;

		ObjectPoolAllocator()
		{
			pool_data = std::shared_ptr<std::vector<T*>>(
			  new std::vector<T*>(),
			  [](std::vector<T*>* pool)
			  {
				  for (auto* ptr : *pool)
				  {
					  std::free(ptr);
				  }
				  delete pool;
			  });
		}

		template<typename U>
		ObjectPoolAllocator(const ObjectPoolAllocator<U>& other)
		  : pool_data(ObjectPoolAllocator<T>::Pool.pool_data)
		{
		}

		~ObjectPoolAllocator()
		{
		}

		T* allocate(size_t n)
		{
			if (n > 1)
			{
				return static_cast<T*>(std::malloc(sizeof(T) * n));
			}

			if (this->pool_data->empty())
			{
				return static_cast<T*>(std::malloc(sizeof(T)));
			}

			T* ptr = this->pool_data->back();
			this->pool_data->pop_back();

			return ptr;
		}

		void deallocate(T* ptr, size_t n)
		{
			if (!ptr)
			{
				return;
			}

			if (n > 1)
			{
				std::free(ptr);
				return;
			}

#ifdef MS_ALLOCATOR_FREE_ON_RETURN
			std::free(ptr);
#else
			this->pool_data->push_back(ptr);
#endif
		}
	};

	template<typename T>
	thread_local Utils::ObjectPoolAllocator<T> Utils::ObjectPoolAllocator<T>::Pool;
} // namespace Utils

#endif
