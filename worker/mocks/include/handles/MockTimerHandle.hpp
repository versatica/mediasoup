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
		  std::function<int64_t()> getTimeMsInt64,
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

		void Start(int64_t timeoutMs, int64_t repeatMs = 0) override;

		void Stop() override
		{
			this->running     = false;
			this->expiresAtMs = std::numeric_limits<int64_t>::max();
		}

		void Restart() override
		{
			this->running     = true;
			this->expiresAtMs = this->getTimeMsInt64() + this->timeoutMs;
		}

		void Restart(int64_t timeoutMs, int64_t repeatMs = 0) override;

		int64_t GetTimeoutMs() const override
		{
			return this->timeoutMs;
		}

		int64_t GetRepeatMs() const override
		{
			return this->repeatMs;
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
		int64_t GetExpiresAtMs() const
		{
			return this->expiresAtMs;
		}

		bool EvaluateHasExpired()
		{
			if (this->getTimeMsInt64() >= this->expiresAtMs)
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
		std::function<int64_t()> getTimeMsInt64;
		const std::function<void()> onDelete;
		// Others.
		bool running{ false };
		int64_t timeoutMs{ 0 };
		int64_t repeatMs{ 0 };
		int64_t expiresAtMs{ std::numeric_limits<int64_t>::max() };
	};
} // namespace mocks

#endif
