#ifndef MS_MOCKS_MOCK_TIMER_HANDLE_HPP
#define MS_MOCKS_MOCK_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"

namespace mocks
{
	// Forward declaration.
	class MockShared;

	class MockTimerHandle : public TimerHandleInterface
	{
		// Only MockShared class can invoke the constructor.
		friend class mocks::MockShared;

	private:
		explicit MockTimerHandle() = default;

	public:
		MockTimerHandle& operator=(const MockTimerHandle&) = delete;

		MockTimerHandle(const MockTimerHandle&) = delete;

		~MockTimerHandle() override = default;

	public:
		void Start(uint64_t timeout, uint64_t repeat = 0) override
		{
			this->running = true;
			this->timeout = timeout;
			this->repeat  = repeat;
		}

		void Stop() override
		{
			this->running = false;
		}

		void Restart() override
		{
			this->running = true;
		}

		void Restart(uint64_t timeout, uint64_t repeat = 0) override
		{
			this->running = true;
			this->timeout = timeout;
			this->repeat  = repeat;
		}

		uint64_t GetTimeout() const override
		{
			return this->timeout;
		}

		uint64_t GetRepeat() const override
		{
			return this->repeat;
		}

		bool IsActive() const override
		{
			return this->running;
		}

	private:
		bool running{ false };
		uint64_t timeout{ 0u };
		uint64_t repeat{ 0u };
	};
} // namespace mocks

#endif
