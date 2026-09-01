#ifndef MS_MOCKS_MOCK_TIMER_HANDLE_HPP
#define MS_MOCKS_MOCK_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include <limits>

namespace mocks
{
	// Forward declaration.
	class MockShared;

	class MockTimerHandle : public TimerHandleInterface
	{
		// Only MockShared class can invoke the constructor.
		friend class mocks::MockShared;

	private:
		explicit MockTimerHandle(
		  TimerHandleInterface::Listener* listener,
		  std::string label,
		  std::function<uint64_t()> getTimeMs,
		  std::function<void()> onDelete);

	public:
		MockTimerHandle& operator=(const MockTimerHandle&) = delete;

		MockTimerHandle(const MockTimerHandle&) = delete;

		~MockTimerHandle() override
		{
			this->onDelete();
		}

	public:
		void Dump(int indentation = 0) const;

		void Start(uint64_t timeout, uint64_t repeat = 0) override
		{
			this->timeout = timeout;
			this->repeat  = repeat;

			this->running     = true;
			this->expiresAtMs = this->getTimeMs() + this->timeout;
		}

		void Stop() override
		{
			this->running     = false;
			this->expiresAtMs = std::numeric_limits<uint64_t>::max();
		}

		void Restart() override
		{
			this->running     = true;
			this->expiresAtMs = this->getTimeMs() + this->timeout;
		}

		void Restart(uint64_t timeout, uint64_t repeat = 0) override
		{
			Start(timeout, repeat);
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

		const std::string GetLabel() const override
		{
			return this->label;
		}

		// Methods for testing.
	public:
		uint64_t GetExpiresAtMs() const
		{
			return this->expiresAtMs;
		}

		bool EvaluateHasExpired()
		{
			if (this->getTimeMs() >= this->expiresAtMs)
			{
				TriggerExpire();

				return true;
			}
			else
			{
				return false;
			}
		}

	private:
		void TriggerExpire();

	private:
		// Passed by argument.
		TimerHandleInterface::Listener* listener{ nullptr };
		const std::string label;
		std::function<uint64_t()> getTimeMs;
		const std::function<void()> onDelete;
		// Others.
		bool running{ false };
		uint64_t timeout{ 0u };
		uint64_t repeat{ 0u };
		uint64_t expiresAtMs{ std::numeric_limits<uint64_t>::max() };
	};
} // namespace mocks

#endif
