#ifndef MS_MOCKS_MOCK_BACKOFF_TIMER_HANDLE_HPP
#define MS_MOCKS_MOCK_BACKOFF_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include <limits>

namespace mocks
{
	// Forward declaration.
	class MockShared;

	class MockBackoffTimerHandle : public BackoffTimerHandleInterface
	{
		// Only MockShared class can invoke the constructor.
		friend class mocks::MockShared;

	private:
		explicit MockBackoffTimerHandle(
		  BackoffTimerHandleOptions options,
		  std::function<uint64_t()> getTimeMs,
		  std::function<void()> onDelete);

	public:
		MockBackoffTimerHandle& operator=(const MockBackoffTimerHandle&) = delete;

		MockBackoffTimerHandle(const MockBackoffTimerHandle&) = delete;

		~MockBackoffTimerHandle() override
		{
			this->onDelete();
		}

	public:
		void Dump(int indentation = 0) const;

		void Start() override
		{
			this->running = true;
			// NOTE: Reset the expiration count, just like the real BackoffTimerHandle
			// does, so that the backoff starts over from the base timeout.
			this->expirationCount = 0;
			this->expiresAtMs     = this->getTimeMs() + ComputeNextTimeoutMs();
		}

		void Stop() override
		{
			this->running = false;
			// NOTE: Reset the expiration count, just like the real BackoffTimerHandle
			// does.
			this->expirationCount = 0;
			this->expiresAtMs     = std::numeric_limits<uint64_t>::max();
		}

		/**
		 * @remarks
		 * - This method must be defined in the .cpp since it's called by the
		 *   constructor. Otherwise clang-tidy complains:
		 *   "warning: Call to virtual method 'BackoffTimerHandle::SetBaseTimeoutMs'
		 *   during construction bypasses virtual dispatch
		 *   [clang-analyzer-optin.cplusplus.VirtualCall]"
		 */
		void SetBaseTimeoutMs(uint64_t baseTimeoutMs) override;

		bool IsRunning() const override
		{
			return this->running;
		}

		const std::string GetLabel() const override
		{
			return this->label;
		}

		std::optional<size_t> GetMaxRestarts() const override
		{
			return this->maxRestarts;
		}

		size_t GetExpirationCount() const override
		{
			return this->expirationCount;
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
		uint64_t ComputeNextTimeoutMs() const
		{
			auto expirationCount = this->expirationCount;

			switch (this->backoffAlgorithm)
			{
				case BackoffAlgorithm::FIXED:
				{
					return this->baseTimeoutMs;
				}

				case BackoffAlgorithm::EXPONENTIAL:
				{
					auto timeoutMs = this->baseTimeoutMs;

					while (expirationCount > 0 && timeoutMs < BackoffTimerHandleInterface::MaxTimeoutMs)
					{
						timeoutMs *= 2;
						--expirationCount;

						if (this->maxBackoffTimeoutMs.has_value() && timeoutMs > this->maxBackoffTimeoutMs.value())
						{
							return this->maxBackoffTimeoutMs.value();
						}
					}

					return std::min<uint64_t>(timeoutMs, BackoffTimerHandleInterface::MaxTimeoutMs);
				}

					NO_DEFAULT_GCC();
			}
		}

		void TriggerExpire();

	private:
		// Passed by argument.
		BackoffTimerHandleInterface::Listener* listener{ nullptr };
		const std::string label;
		uint64_t baseTimeoutMs;
		BackoffAlgorithm backoffAlgorithm;
		std::optional<uint64_t> maxBackoffTimeoutMs;
		std::optional<size_t> maxRestarts;
		std::function<uint64_t()> getTimeMs;
		const std::function<void()> onDelete;
		// Others.
		bool running{ false };
		size_t expirationCount{ 0 };
		uint64_t expiresAtMs{ std::numeric_limits<uint64_t>::max() };
	};
} // namespace mocks

#endif
